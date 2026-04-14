#include "downloadClient.h"
#include "network_tool.hpp"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <curl/curl.h>

namespace infra_network::http
{
    DownloadClient::~DownloadClient()
    {
        stop();
    }

    static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* ofs = static_cast<std::ofstream*>(userdata);
        ofs->write(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
    {
        auto* item = static_cast<DownloadClient::Item*>(clientp);
        if(dltotal > 0) item->progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
        return 0;
    }
}

namespace infra_network::http
{
    void DownloadClient::downloadFile(const std::string& url, const std::filesystem::path& saveDir, DownloadCallback cb)
    {
        std::string fileName = infra_network::UrlConvert::extractFileName(url);
        std::filesystem::path savePath = saveDir / fileName;

        if(std::error_code ec; !std::filesystem::create_directories(savePath.parent_path(), ec) && ec) return;

        ItemPtr item = std::make_shared<Item>();
        item->name = savePath.filename().string();
        item->url = infra_network::UrlConvert::urlEncode(url);
        item->savePath = savePath;
        item->callback = std::move(cb);

        {
            std::lock_guard<std::mutex> lock(mtx);

            // 去重
            auto it = std::find_if( downloadList.begin(), downloadList.end(), [&](const ItemPtr& i) { return i->url == item->url; });
            if(it != downloadList.end()) return;

            downloadList.push_back(item);

            if(!active)
            {
                active = true;
                if(workerThread.joinable()) workerThread.join();
                workerThread = std::thread(&DownloadClient::threadOn, this);
            }
        }
    }

    void DownloadClient::getDownloadList(std::vector<ItemPtr>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.assign(downloadList.begin(), downloadList.end());
    }

    void DownloadClient::getCompleteList(std::vector<ItemPtr>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.assign(finishList.begin(), finishList.end());
    }

    void DownloadClient::clearCompleteList()
    {
        std::lock_guard<std::mutex> lock(mtx);
        finishList.clear();
    }

    void DownloadClient::stop()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            active = false;
        }
        if(workerThread.joinable())
            workerThread.join();
    }
}


namespace infra_network::http
{
    void DownloadClient::threadOn()
    {
        while(true)
        {
            ItemPtr task {nullptr};
            {
                std::lock_guard<std::mutex> lock(mtx);
                if(!active || downloadList.empty())
                {
                    active = false; break;
                }
                task = downloadList.front();
            }

            task->state = State::downloading;
            performDownloadFast(task);

            {
                std::lock_guard<std::mutex> lock(mtx);
                downloadList.pop_front();
                finishList.push_back(task);
            }

            if(task->callback) task->callback(task);
        }
    }

    void DownloadClient::performDownload(const ItemPtr& task)
    {
        CURL* curl = curl_easy_init();
        if(!curl)
        {
            task->state  = State::finish;
            task->result = Result::failed;
            return;
        }

        std::ofstream ofs(task->savePath, std::ios::binary);

        curl_easy_setopt(curl, CURLOPT_URL, task->url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task.get());

        CURLcode res = curl_easy_perform(curl);

        long httpCode = 0;
        if(res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        task->state = State::finish;
        task->result =
            (res == CURLE_OK && httpCode >= 200 && httpCode < 300)
                ? Result::success
                : Result::failed;

        curl_easy_cleanup(curl);
    }
        
    void DownloadClient::performDownloadFast(const ItemPtr& task)
    {
        CURL* curl = curl_easy_init();
        if(!curl)
        {
            task->state  = State::finish;
            task->result = Result::failed;
            return;
        }

        std::ofstream ofs(task->savePath, std::ios::binary);
        if(!ofs.is_open())
        {
            curl_easy_cleanup(curl);
            task->state  = State::finish;
            task->result = Result::failed;
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, task->url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

        // ========= 关键性能选项 =========

        // 跟随重定向（必开）
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // 不限制总时长
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);

        // 关闭低速中断
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 0L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 0L);

        // 关闭进度回调
        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task.get());

        // TCP keep-alive（长连接更稳）
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 60L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);

        // 更大的接收 buffer（可选）
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 512 * 1024L);

        // ========= 执行 =========

        CURLcode res = curl_easy_perform(curl);

        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        task->state = State::finish;
        task->result =
            (res == CURLE_OK && httpCode >= 200 && httpCode < 300)
                ? Result::success
                : Result::failed;

        curl_easy_cleanup(curl);
        ofs.close();

        if(task->result == Result::failed)
        {
            std::error_code ec;
            std::filesystem::remove(task->savePath, ec);
        }
    }
}