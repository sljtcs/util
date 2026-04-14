#pragma once
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <filesystem>
#include <functional>

namespace infra_network::http
{
    class DownloadClient
    {
    public:
        enum class State { pending, downloading, finish };
        enum class Result { none, success, failed };
        struct Item;
        using ItemPtr = std::shared_ptr<Item>;
        using DownloadCallback = std::function<void(ItemPtr)>;
        struct Item
        {
            std::string name;
            std::string url;
            std::filesystem::path savePath;
            std::atomic<float> progress{0.0f};
            State state {State::pending};
            Result result {Result::none};
            DownloadCallback callback;
        };

    public:
        ~DownloadClient();
        void downloadFile(const std::string& url, const std::filesystem::path& saveDir, DownloadCallback cb);
        void getDownloadList(std::vector<ItemPtr>& out);
        void getCompleteList(std::vector<ItemPtr>& out);
        void clearCompleteList();
        void stop();
    private:
        void threadOn();
        void performDownload(const ItemPtr& task);
        void performDownloadFast(const ItemPtr& task);
    private:
        std::list<ItemPtr> downloadList;
        std::list<ItemPtr> finishList;
        std::mutex mtx;
        std::thread workerThread;
        bool active {false};
    };
}