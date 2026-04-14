#include "syncDownloadClient.h"
#include <fstream>
#include <curl/curl.h>
#include "util/infra/debug/log.hpp"

namespace infra_network::http
{
    static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* ofs = static_cast<std::ofstream*>(userdata);
        ofs->write(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
    {
        auto* callback = static_cast<SyncDownloadClient::ProgressCallback*>(clientp);
        if(callback && *callback && dltotal > 0)
        {
            float progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
            (*callback)(progress);
        }
        return 0;
    }
}

namespace infra_network::http
{
    struct Impl{
        CURLSH* lShare = nullptr;
    };

    SyncDownloadClient::SyncDownloadClient()
    {
        impl = std::make_unique<Impl>();
        impl->lShare = curl_share_init();
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(impl->lShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    }

    SyncDownloadClient::~SyncDownloadClient()
    {
        if(impl && impl->lShare)
        {
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
        CURL* curl = curl_easy_init();
        if(!curl)
            return false;

        std::error_code ec;
        std::filesystem::create_directories(savePath.parent_path(), ec);

        std::ofstream ofs(savePath, std::ios::binary);
        if(!ofs.is_open())
        {
            curl_easy_cleanup(curl);
            return false;
        }

        // ⭐ 文件缓冲（关键优化）
        static thread_local char fileBuf[1 << 20]; // 1MB
        ofs.rdbuf()->pubsetbuf(fileBuf, sizeof(fileBuf));

        // ⭐ share（DNS + TLS 复用）
        curl_easy_setopt(curl, CURLOPT_SHARE, impl->lShare);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

        // TCP 优化
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 30L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 10L);

        // 吞吐量
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024 * 1024);

        // HTTP2
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

        // 低速保护
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);

        // 进度
        if(callback)
        {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback);
        }

        // 执行
        CURLcode res = curl_easy_perform(curl);

        long httpCode = 0;
        if(res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_easy_cleanup(curl);
        ofs.close();

        bool ok = (res == CURLE_OK && httpCode >= 200 && httpCode < 300);

        if(!ok)
            std::filesystem::remove(savePath, ec);

        return ok;
    }
}