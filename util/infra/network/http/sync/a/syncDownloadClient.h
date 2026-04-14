#pragma once
#include <functional>
#include <filesystem>
#include <string>
#include <memory>

namespace infra_network::http
{
    class SyncDownloadClient
    {
    public:
        using ProgressCallback = std::function<void(float progress)>;

        SyncDownloadClient();
        ~SyncDownloadClient();

        // 原有接口保持不变
        bool download(
            const std::string& url,
            const std::filesystem::path& savePath,
            int timeoutSec,
            ProgressCallback callback = nullptr);
        bool downloadSingle(
            const std::string& url,
            const std::filesystem::path& savePath,
            int timeoutSec,
            ProgressCallback callback);
        bool supportsRange(const std::string& url);
        size_t getFileSize(const std::string& url);


        // ⭐ 新增：设置线程数（默认8）
        void setThreadCount(int threads) { threadCount_ = threads; }
        
        // ⭐ 新增：设置是否启用多线程（默认true）
        void setParallelMode(bool enable) { parallelMode_ = enable; }

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        int threadCount_ = 8;
        bool parallelMode_ = true;
    };
}