#include "smdClient.h"
#include <thread>
#include <fstream>
#include <algorithm>
#include <curl/curl.h>
#include "nettoolbox.h"
#include "util/infra/debug/log.hpp"

namespace infra_network::http
{
    struct SMDClient::Impl
    {
        CURLSH* lShare {nullptr};
    };

    SMDClient::SMDClient()
    {
        impl = std::make_unique<Impl>();
        impl->lShare = curl_share_init();
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    }
    SMDClient::~SMDClient()
    {
        if(impl && impl->lShare){
            curl_share_cleanup(impl->lShare);
            impl->lShare = nullptr;
        }
    }
}

namespace infra_network::http
{
    // bool SMDClient::download(
    //     const std::string& url,
    //     const std::filesystem::path& savePath,
    //     int timeoutSec,
    //     std::function<void(float)> callback)
    // {

    //     return false;
    // }
}

struct RemoteFileInfo
{
    curl_off_t contentLength {-1};
    bool acceptRanges {false};
};

static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    const size_t total = size * nitems;
    auto* info = static_cast<RemoteFileInfo*>(userdata);
    if(info == nullptr)
        return total;

    std::string header(buffer, total);

    // 粗略判断 Accept-Ranges
    // 常见返回: "Accept-Ranges: bytes"
    auto pos = header.find("Accept-Ranges:");
    if(pos != std::string::npos)
    {
        if(header.find("bytes", pos) != std::string::npos)
            info->acceptRanges = true;
    }

    return total;
}


static bool queryRemoteFileInfo(
    CURLSH* share,
    const std::string& url,
    int timeoutSec,
    RemoteFileInfo& outInfo)
{
    CURL* curl = curl_easy_init();
    if(!curl)
        return false;

    curl_easy_setopt(curl, CURLOPT_SHARE, share);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &outInfo);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_off_t cl = -1;
    if(res == CURLE_OK)
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
    }

    curl_easy_cleanup(curl);

    if(res != CURLE_OK || httpCode < 200 || httpCode >= 400)
        return false;

    outInfo.contentLength = cl;
    return true;
}



struct PartWriteContext
{
    std::ofstream* ofs {nullptr};
    std::atomic<long long>* totalDownloaded {nullptr};
};

static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* ctx = static_cast<PartWriteContext*>(userdata);
    const size_t bytes = size * nmemb;

    if(ctx == nullptr || ctx->ofs == nullptr)
        return 0;

    ctx->ofs->write(static_cast<const char*>(ptr), static_cast<std::streamsize>(bytes));
    if(!(*ctx->ofs))
        return 0;

    if(ctx->totalDownloaded)
        ctx->totalDownloaded->fetch_add(static_cast<long long>(bytes), std::memory_order_relaxed);

    return bytes;
}

static bool downloadPart(
    CURLSH* share,
    const std::string& url,
    const std::filesystem::path& partPath,
    curl_off_t start,
    curl_off_t end,
    int timeoutSec,
    std::atomic<long long>& totalDownloaded)
{
    CURL* curl = curl_easy_init();
    if(!curl){
        LOG("f1");
        return false;
    }

    std::ofstream ofs(partPath, std::ios::binary);
    if(!ofs.is_open())
    {
        curl_easy_cleanup(curl);
        LOG("f2");
        return false;
    }

    static thread_local char fileBuf[1 << 20];
    ofs.rdbuf()->pubsetbuf(fileBuf, sizeof(fileBuf));

    PartWriteContext ctx;
    ctx.ofs = &ofs;
    ctx.totalDownloaded = &totalDownloaded;

    const std::string range = std::to_string(start) + "-" + std::to_string(end);

    curl_easy_setopt(curl, CURLOPT_SHARE, share);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 30L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 10L);
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024 * 1024);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    if(res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);
    ofs.close();

    // 分块下载时，通常希望返回 206 Partial Content
    // 但有些服务器可能仍返回 200，这里严格些，只接受 206
    LOG("ret");
    return (res == CURLE_OK && httpCode == 206);
}

static bool mergeParts(
    const std::vector<std::filesystem::path>& partFiles,
    const std::filesystem::path& savePath)
{
    std::error_code ec;
    std::filesystem::create_directories(savePath.parent_path(), ec);

    std::ofstream out(savePath, std::ios::binary);
    if(!out.is_open())
        return false;

    static thread_local char mergeBuf[1 << 20];

    for(const auto& part : partFiles)
    {
        std::ifstream in(part, std::ios::binary);
        if(!in.is_open())
            return false;

        while(in)
        {
            in.read(mergeBuf, sizeof(mergeBuf));
            const std::streamsize got = in.gcount();
            if(got > 0)
                out.write(mergeBuf, got);
        }
    }

    out.close();
    return true;
}



namespace infra_network::http
{
    bool SMDClient::download(
        const std::string& url,
        const std::filesystem::path& savePath,
        int timeoutSec,
        std::function<void(float)> callback)
    {
        RemoteFileInfo info;
        if(!queryRemoteFileInfo(impl->lShare, url, timeoutSec, info))
            return false;

        // 单线程
        if(!info.acceptRanges || info.contentLength <= 0){
            LOG("d with single");
            return false;
        }

        const curl_off_t totalSize = info.contentLength;
        threadCount = std::max<int>(1, threadCount);
        threadCount = std::min<int>(threadCount, 32);

        // 文件太小就别分太多线程
        const curl_off_t minChunkSize = 2 * 1024 * 1024; // 2MB
        int maxReasonableThreads = static_cast<int>((totalSize + minChunkSize - 1) / minChunkSize);
        maxReasonableThreads = std::max<int>(1, maxReasonableThreads);
        threadCount = std::min<int>(threadCount, maxReasonableThreads);

        // 文件分块
        const curl_off_t partSize = (totalSize + threadCount - 1) / threadCount;
        std::vector<std::filesystem::path> partFiles;
        partFiles.reserve(threadCount);
        for(int i = 0; i < threadCount; ++i){
            partFiles.push_back(savePath.string() + ".part" + std::to_string(i));
        }

        std::atomic<long long> totalDownloaded{0};
        std::atomic<bool> failed{false};
        std::thread progressThread;
        std::atomic<bool> progressStop{false};

        // 进度回调线程
        if(callback)
        {
            progressThread = std::thread([&]()
            {
                BandwidthTool bwTool; // 5秒平均速度
                unsigned long long prev {0};
                while(!progressStop.load(std::memory_order_relaxed))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    long long done = totalDownloaded.load(std::memory_order_relaxed);
                    float p = static_cast<float>(done) / static_cast<float>(totalSize);
                    bwTool.push(done-prev);
                    LOG(prev, " : ", done, " / ", done-prev);
                    prev = done;
                    LOG("average speed: ", bwTool.getSpeed()/1024, " KB/s");
                    callback(p);
                }

                long long done = totalDownloaded.load(std::memory_order_relaxed);
                float p = static_cast<float>(done) / static_cast<float>(totalSize);
                if(p > 1.0f)
                    p = 1.0f;
                callback(p);
            });
        }


        // 多线程下载
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        for(int i = 0; i < threadCount; ++i)
        {
            const curl_off_t start = static_cast<curl_off_t>(i) * partSize;
            const curl_off_t end = std::min<curl_off_t>(start + partSize - 1, totalSize - 1);

            LOG(start, " -> " , end);
            workers.emplace_back([&, i, start, end]()
            {
                if(failed.load(std::memory_order_relaxed))
                    return;
                if(!downloadPart(
                        impl->lShare,
                        url,
                        partFiles[i],
                        start,
                        end,
                        timeoutSec,
                        totalDownloaded))
                {
                    LOG("i failed: ", i);
                    failed.store(true, std::memory_order_relaxed);
                }
            });
        }


        // 等待下载线程结束
        LOG("waiting for workers...");
        for(auto& t : workers)
        {
            if(t.joinable())
                t.join();
        }

        // 停止进度线程
        progressStop.store(true, std::memory_order_relaxed);
        if(progressThread.joinable())
            progressThread.join();

        std::error_code ec;
        if(failed.load(std::memory_order_relaxed))
        {
            for(const auto& f : partFiles)
                std::filesystem::remove(f, ec);
            return false;
        }

        if(!mergeParts(partFiles, savePath))
        {
            for(const auto& f : partFiles)
                std::filesystem::remove(f, ec);
            std::filesystem::remove(savePath, ec);
            return false;
        }

        for(const auto& f : partFiles)
            std::filesystem::remove(f, ec);

        if(callback)
            callback(1.0f);

        return true;
    }
}