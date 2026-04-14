#include "messageClient.h"
#include "network_tool.hpp"
#include <thread>
#include <fstream>
#include <functional>
#include <filesystem>
#include <curl/curl.h>

namespace infra_network::http
{
    static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
    {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }
}

namespace infra_network::http
{
    void MessageClient::get(const std::string& url, const Parameters& params, TextCallback cb, long timeoutMs)
    {
        std::thread([url, params, cb, timeoutMs]()
        {
            CURL* curl = curl_easy_init();
            if (!curl)
            {
                cb(false, "");
                return;
            }

            std::string fullUrl = url;
            if(!params.empty())
            {
                fullUrl += "?";
                bool first = true;
                for(const auto& [k, v] : params)
                {
                    if (!first) fullUrl += "&";
                    first = false;

                    char* escKey = curl_easy_escape(curl, k.c_str(), 0);
                    char* escVal = curl_easy_escape(curl, v.c_str(), 0);
                    fullUrl += std::string(escKey) + "=" + std::string(escVal);
                    curl_free(escKey);
                    curl_free(escVal);
                }
            }
            std::string respBody;

            curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respBody);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                curl_easy_cleanup(curl);
                cb(false, "");
                return;
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            curl_easy_cleanup(curl);
            cb(http_code == 200, respBody);

        }).detach();
    }
}