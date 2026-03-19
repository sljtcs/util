#pragma once
#include "network_tool.hpp"
#include <libcurl/curl/curl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <list>
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>
#include <functional>

#define DEBUG_HTTP_DOWNLOAD_CLIENT 1

namespace network_util::http
{

class DownloadClient
{
public:
    enum class State{pending, downloading, finish};
    enum class Result{none, success, failed};
    struct Item
    {
        std::string name;
        std::string url;
        State state {State::pending};
        Result result {Result::none};
        std::filesystem::path savePath;
        std::atomic<float> progress{0.0f};
    };
    using ItemPtr = std::shared_ptr<Item>;
    using DownloadCallback = std::function<void(const ItemPtr&)>;

public:
    DownloadClient()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~DownloadClient()
    {
        stop();
        curl_global_cleanup();
    }

    void setOnDownloadFinished(DownloadCallback cb)
    {
        std::lock_guard<std::mutex> lock(mtx);
        callback = cb;
    }

    void downloadFile(const std::string& url, const std::filesystem::path& saveDir)
    {
        const std::string& fileName = network_util::UrlConvert::extractFileName(url);
        std::filesystem::path savePath = saveDir/fileName;
        if(std::error_code ec; !std::filesystem::create_directories(savePath.parent_path(), ec) && ec) return;

        std::lock_guard<std::mutex> lock(mtx);

        // 检查列表中是否已有任务
        auto it = std::find_if(downloadList.begin(), downloadList.end(),
            [&url](const ItemPtr& item){ return item->url == url; });
        if(it != downloadList.end()) return;

        auto item = std::make_shared<Item>();
        item->name = savePath.filename().string();
        item->url = network_util::UrlConvert::urlEncode(url);
        item->savePath = savePath;

        downloadList.push_back(item);

        if(!workerThread.joinable())
        {
            active = true;
            workerThread = std::thread(&DownloadClient::threadOn, this);
        }
    }

    void getDownloadList(std::vector<ItemPtr>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.assign(downloadList.begin(), downloadList.end());
    }

    void stop()
    {
        active = false;
        if(workerThread.joinable())
            workerThread.join();
    }

private:
    void threadOn()
    {
        while(active)
        {
            ItemPtr task = nullptr;
            {
                std::lock_guard<std::mutex> lock(mtx);
                if(downloadList.front())
                    task = downloadList.front();
            }

            if(task)
            {
                task->state = State::downloading;
                performDownload(task);
                downloadList.pop_front();
                if(callback) callback(task);
            }
            else
            {
                active = false;
                break;
            }
        }
    }

    static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
    {
        std::ofstream* ofs = static_cast<std::ofstream*>(userdata);
        ofs->write(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
    {
        Item* item = static_cast<Item*>(clientp);
        if(dltotal > 0)
            item->progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
        return 0;
    }

    void performDownload(ItemPtr task)
    {
        CURL* curl = curl_easy_init();
        if(!curl) return;

        std::ofstream ofs(task->savePath, std::ios::binary);

        curl_easy_setopt(curl, CURLOPT_URL, task->url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &DownloadClient::writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

        // 限速 50KB/s
        // curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)50 * 1024);

        // 超时控制
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

        // 进度回调
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &DownloadClient::progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task.get());

        CURLcode res = curl_easy_perform(curl);
        task->state = State::finish;
        if(res != CURLE_OK)
            task->result = Result::success;
        else
            task->result = Result::failed;

        curl_easy_cleanup(curl);
    }

private:
    std::list<ItemPtr> downloadList;
    std::mutex mtx;
    std::thread workerThread;
    std::atomic<bool> active{false};
    DownloadCallback callback{nullptr};

#if DEBUG_HTTP_DOWNLOAD_CLIENT
public:
    static void debugDownloadList(const std::vector<ItemPtr>& list)
    {
#ifdef __linux__
        std::cout << "\033[2J\033[H";
#else
        system("cls");
#endif
        if(list.empty()){std::cout << "empty\n"; return;}
        for(const auto& item : list)
        {
            int percent = static_cast<int>(item->progress * 100.0f);

            const int barWidth = 30;
            int pos = percent * barWidth / 100;
            std::string bar(barWidth, ' ');
            std::fill(bar.begin(), bar.begin() + pos, '#');
            if(pos<barWidth) bar[pos] = '>';

            std::cout
                << "Name     : "    << item->name << "\n"
                << "URL      : "    << item->url << "\n"
                << "Path     : "    << item->savePath << "\n"
                << "Progress : "    << "[" << bar << "] " << percent << "%\n"
                << "-----------------------------------------------\n";
        }
    }
#endif
};

}