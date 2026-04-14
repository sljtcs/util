#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <functional>

namespace infra_network::http
{

    class SMDClient
    {
    public:
        SMDClient();
        ~SMDClient();
    public:
        bool download(
            const std::string& url,
            const std::filesystem::path& savePath,
            int timeoutSec,
            std::function<void(float)> callback
        );
    public:
        struct Impl;
        size_t threadCount {4};
        std::unique_ptr<Impl> impl{nullptr};
    };
}