#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <functional>

namespace infra_network::http
{
    struct Impl;
    class SyncDownloadClient
    {
    public:
        SyncDownloadClient();
        ~SyncDownloadClient();
    public:
        using ProgressCallback = std::function<void(float)>;
        bool download(const std::string& url, const std::filesystem::path& savePath, int timeoutSec, ProgressCallback callback);
    private:
        std::unique_ptr<Impl> impl{nullptr};
    };
}