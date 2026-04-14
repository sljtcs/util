// syncDownloadClient.cpp
#include "syncDownloadClient.h"
#include <fstream>
#include <curl/curl.h>
#include <thread>
#include <vector>
#include <future>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sstream>
#include <algorithm>
#include "util/infra/debug/log.hpp"

namespace infra_network::http
{
    // 分片信息
    struct Chunk {
        size_t start;
        size_t end;
        size_t id;
        size_t downloaded{0};
    };

    struct GlobalProgress {
        std::atomic<size_t> totalDownloaded{0};
        std::atomic<size_t> totalSize{0};
        std::atomic<bool> cancel{false};
        SyncDownloadClient::ProgressCallback userCallback;
        std::mutex callbackMutex;
        std::chrono::steady_clock::time_point startTime;
    };

    // 单分片下载（独立CURL实例）
    static bool downloadChunk(
        const std::string& url,
        const std::filesystem::path& tempPath,
        Chunk& chunk,
        CURLSH* share,
        int timeoutSec,
        GlobalProgress& global)
    {
        if(global.cancel.load()) return false;

        CURL* curl = curl_easy_init();
        if (!curl) return false;

        // 打开临时文件
        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, tempPath.string().c_str(), "rb+");
        if (err != 0 || !fp) {
            curl_easy_cleanup(curl);
            return false;
        }
        
        if (fseek(fp, static_cast<long>(chunk.start), SEEK_SET) != 0) {
            fclose(fp);
            curl_easy_cleanup(curl);
            return false;
        }

        // 构建 Range
        std::ostringstream range;
        range << chunk.start << "-" << chunk.end;

        // 配置（复用share优化DNS/TLS）
        curl_easy_setopt(curl, CURLOPT_SHARE, share);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_RANGE, range.str().c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 256 * 1024); // 256KB per thread

        // 写回调 - 直接写入文件 + 统计进度
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
            [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* ctx = static_cast<std::pair<FILE*, Chunk*>*>(userdata);
                size_t written = fwrite(ptr, 1, size * nmemb, ctx->first);
                ctx->second->downloaded += written;
                return written;
            });
        
        std::pair<FILE*, Chunk*> writeCtx{fp, &chunk};
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeCtx);

        // 进度回调（仅用于检测cancel）
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
            [](void* clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
                return static_cast<GlobalProgress*>(clientp)->cancel.load() ? 1 : 0;
            });
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &global);

        CURLcode res = curl_easy_perform(curl);
        
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }

        curl_easy_cleanup(curl);
        fclose(fp);

        return (res == CURLE_OK && httpCode >= 200 && httpCode < 300);
    }

    // 进度监控线程
    static void progressMonitor(GlobalProgress& global, int intervalMs = 500) {
        if (!global.userCallback) return;

        size_t lastDownloaded = 0;
        
        while (!global.cancel.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
            
            size_t current = global.totalDownloaded.load();
            size_t total = global.totalSize.load();
            
            if (total > 0) {
                float progress = static_cast<float>(current) / static_cast<float>(total);
                {
                    std::lock_guard<std::mutex> lock(global.callbackMutex);
                    if (global.userCallback) {
                        LOG(progress);
                        global.userCallback(progress);
                    }
                }
            }
            
            // 检测完成
            if (current >= total) break;
            lastDownloaded = current;
        }
    }

    struct SyncDownloadClient::Impl {
        CURLSH* lShare = nullptr;
    };

    SyncDownloadClient::SyncDownloadClient() {
        impl = std::make_unique<Impl>();
        impl->lShare = curl_share_init();
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    }

    SyncDownloadClient::~SyncDownloadClient() {
        if (impl && impl->lShare) {
            curl_share_cleanup(impl->lShare);
            impl->lShare = nullptr;
        }
    }

    bool SyncDownloadClient::download(
        const std::string& url,
        const std::filesystem::path& savePath,
        int timeoutSec,
        ProgressCallback callback)
    {
        LOG("on download");
        // ⭐ 非多线程模式：回退到原始单线程
        if (!parallelMode_ || threadCount_ <= 1) {
            return downloadSingle(url, savePath, timeoutSec, callback);
        }

        LOG("1");
        std::error_code ec;
        std::filesystem::create_directories(savePath.parent_path(), ec);

        // 1. 获取文件大小（HEAD请求）
        size_t fileSize = getFileSize(url);
        LOG(fileSize);
        if (fileSize == 0) {
            // 无法获取大小，回退单线程
            return downloadSingle(url, savePath, timeoutSec, callback);
        }

        // 2. 检查服务器是否支持Range
        if (!supportsRange(url)) {
            return downloadSingle(url, savePath, timeoutSec, callback);
        }

        // 3. 创建临时文件
        auto tempPath = savePath;
        tempPath += ".tmp";
        LOG(tempPath);
        
        {
            std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
            ofs.seekp(static_cast<std::streamoff>(fileSize - 1));
            ofs.put(0);
            if (!ofs.good()) return false;
        }

        LOG("ok");
        // 4. 计算分片
        int threads = (std::min)(threadCount_, static_cast<int>(fileSize / (1024 * 1024))); // 至少1MB/线程
        threads = (std::max)(threads, 1);
        LOG(threads);
        
        std::vector<Chunk> chunks;
        size_t chunkSize = fileSize / threads;
        
        for (int i = 0; i < threads; i++) {
            Chunk chunk;
            chunk.id = i;
            chunk.start = i * chunkSize;
            chunk.end = (i == threads - 1) ? (fileSize - 1) : ((i + 1) * chunkSize - 1);
            chunks.push_back(std::move(chunk));
        }

        LOG(chunkSize);

        // 5. 全局进度
        GlobalProgress global;
        global.totalSize = fileSize;
        global.userCallback = callback;
        global.startTime = std::chrono::steady_clock::now();

        LOG("global");

        // 6. 启动进度监控线程
        std::thread monitorThread([&]() {
            progressMonitor(global);
        });

        LOG("monitor");

        // 7. 并行下载
        std::vector<std::future<bool>> futures;
        
        for (auto& chunk : chunks) {
            futures.push_back(std::async(std::launch::async, [&]() -> bool {
                bool ok = downloadChunk(url, tempPath, chunk, impl->lShare, timeoutSec, global);
                
                // 累加进度
                global.totalDownloaded.fetch_add(chunk.downloaded);
                
                if (!ok) global.cancel.store(true);
                return ok;
            }));
        }

        LOG("futures");

        // 8. 等待完成
        bool allSuccess = true;
        for (auto& f : futures) {
            if (!f.get()) allSuccess = false;
        }

        global.cancel.store(true);
        if (monitorThread.joinable()) {
            monitorThread.join();
        }

        LOG("allSuccess");

        // 9. 收尾
        if (allSuccess) {
            std::filesystem::rename(tempPath, savePath, ec);
            if (callback) callback(1.0f);
            return !ec;
        } else {
            std::filesystem::remove(tempPath, ec);
            return false;
        }

        LOG("over");
    }

    // 单线程回退（保持原有逻辑）
    bool SyncDownloadClient::downloadSingle(
        const std::string& url,
        const std::filesystem::path& savePath,
        int timeoutSec,
        ProgressCallback callback)
    {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::error_code ec;
        std::filesystem::create_directories(savePath.parent_path(), ec);

        std::ofstream ofs(savePath, std::ios::binary);
        if (!ofs.is_open()) {
            curl_easy_cleanup(curl);
            return false;
        }

        static thread_local char fileBuf[1 << 20];
        ofs.rdbuf()->pubsetbuf(fileBuf, sizeof(fileBuf));

        curl_easy_setopt(curl, CURLOPT_SHARE, impl->lShare);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024 * 1024);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
            [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* ofs = static_cast<std::ofstream*>(userdata);
                ofs->write(static_cast<char*>(ptr), size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

        if (callback) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) -> int {
                    auto* cb = static_cast<ProgressCallback*>(clientp);
                    if (*cb && dltotal > 0) {
                        (*cb)(static_cast<float>(dlnow) / static_cast<float>(dltotal));
                    }
                    return 0;
                });
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback);
        }

        CURLcode res = curl_easy_perform(curl);
        
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }

        curl_easy_cleanup(curl);
        ofs.close();

        bool ok = (res == CURLE_OK && httpCode >= 200 && httpCode < 300);
        if (!ok) {
            std::filesystem::remove(savePath, ec);
        } else if (callback) {
            callback(1.0f);
        }

        return ok;
    }

    // 辅助：获取文件大小
    size_t SyncDownloadClient::getFileSize(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return 0;

        double size = 0;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SHARE, impl->lShare);

        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
        }

        curl_easy_cleanup(curl);
        return (size > 0) ? static_cast<size_t>(size) : 0;
    }

    // 辅助：检查Range支持
    bool SyncDownloadClient::supportsRange(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        bool supports = false;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        if (curl_easy_perform(curl) == CURLE_OK) {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            supports = (httpCode == 206); // Partial Content
        }

        curl_easy_cleanup(curl);
        return supports;
    }
}