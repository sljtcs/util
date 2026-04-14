#include "quickClient.h"
#include "network_tool.hpp"
#include <thread>
#include <fstream>
#include <functional>
#include <filesystem>
#include <curl/curl.h>
#include "util/infra/debug/log.hpp"

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
    bool QuickClient::post(
        const std::string& path,
        const std::string& body,
        const std::string& content_type,
        Respond& resp,
        long timeoutMs)
    {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = address.url(path);
        LOG(url);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    
        // POST body
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    
        // 设置 Content-Type
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Content-Type: " + content_type).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
        // 写回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    
        // 执行
        CURLcode code = curl_easy_perform(curl);
        if (code != CURLE_OK)
        {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return false;
        }
    
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status);
    
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return true;
    }
}