#pragma once

#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>
#include <random>

#pragma comment(lib, "ws2_32.lib")

/*
NTP(Network Time Protocol)
时间格式UTC 自 1900-01-01 至今秒数

请求报文 48Bytes
    byte1:
        LI(Leap Indicator)  闰秒信息
        VN(Version Number)  协议版本
        Mode                客户端/服务器/广播模式
    byte2-byte48:
        0x00                填充位

返回报文 48Bytes
    byte1-byte39:
        Reference Timestamp 服务器上次校准的时间
        Originate Timestamp 客户端发送时间
        Receive Timestamp   服务器收到请求时间
    byte40-byte47:
        Transmit Timestamp  服务器发送响应时间
*/

// 测试失效

/*
"ntp1.aliyun.com", \
"ntp2.aliyun.com", \
"ntp3.aliyun.com", \
"ntp4.aliyun.com", \
"ntp5.aliyun.com", \
"ntp6.aliyun.com", \
"ntp7.aliyun.com", \
"time.cloudflare.com", \
"time.google.com" \
"ntp4.aliyun.com", \
"pool.ntp.org", \
"pool.ntp.org", \
*/

// AnyCast/Pool
#define NTP_ANYCAST_POOL \
"ntp1.aliyun.com", \
"ntp2.aliyun.com", \
"ntp.aliyun.com"

// NIST
#define NTP_NIST \
"129.6.15.28", \
"129.6.15.30", \
"129.6.15.27", \
"129.6.15.29"

#define BASE_DELAY_MS 1000
#define MAX_DELAY_MS 5000

namespace ntp
{
    constexpr uint64_t NTP_TIMESTAMP_DELTA = 2208988800ull;

    struct UtcResult
    {
        bool ok = false;
        std::time_t utc_time = 0;
        std::string server_used;
    };

    // 域名解析
    static std::vector<sockaddr_in> resolve_ipv4(const std::string& host, uint16_t port)
    {
        std::vector<sockaddr_in> out;
        addrinfo hints{};
        hints.ai_family = AF_INET; // IPV4
        hints.ai_socktype = SOCK_DGRAM; // UDP
        hints.ai_protocol = IPPROTO_UDP;

        addrinfo* res = nullptr;
        if(getaddrinfo(host.c_str(), nullptr, &hints, &res) == 0 && res)
        {
            for(auto* p = res; p; p = p->ai_next)
            {
                sockaddr_in sin = *reinterpret_cast<sockaddr_in*>(p->ai_addr);
                sin.sin_port = htons(port);
                out.push_back(sin);
            }
            freeaddrinfo(res);
        }
        else
        {
            // 转换点分十进制
            sockaddr_in sin{};
            sin.sin_family = AF_INET;
            sin.sin_port = htons(port);
            if(InetPtonA(AF_INET, host.c_str(), &sin.sin_addr) == 1)
                out.push_back(sin);
        }
        return out;
    }

    // 随机种子
    std::mt19937& global_rng()
    {
        static std::random_device rd;
        static std::mt19937 rng(rd());
        return rng;
    }
}

namespace ntp
{

// NTP请求 48bytes
static void make_ntp_request(unsigned char (&buf)[48])
{
    std::memset(buf, 0, sizeof(buf));
    buf[0] = 0x1B; // LI=0 VN=3 Mode=3
}

// 读取 transmit timestamp 的秒部（network byte order -> host）
static bool parse_transmit_seconds(const unsigned char* pkt, size_t len, uint32_t& seconds_be)
{
    if(len < 48) return false;
    std::memcpy(&seconds_be, pkt + 40, sizeof(seconds_be));
    return true;
}

// 多目标并发发送
UtcResult get_utc_from_ntp(int per_attempt_timeout_ms, int max_attempts)
{
    UtcResult result{};

    const std::vector<std::string> servers = {
        NTP_ANYCAST_POOL,
        // NTP_NIST
    };

    // Winsock 初始化
    WSADATA wsa{};
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return result;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == INVALID_SOCKET)
    {
        WSACleanup();
        return result;
    }

    // 接收超时设置
    timeval tv{};
    tv.tv_sec  = per_attempt_timeout_ms / 1000;
    tv.tv_usec = (per_attempt_timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    // 解析所有目标地址
    std::vector<std::pair<std::string, sockaddr_in>> rawTargets;
    rawTargets.reserve(32);
    for(const auto& host : servers)
    {
        auto addrs = resolve_ipv4(host, 123);
        for(auto& a : addrs) rawTargets.emplace_back(host, a);
    }
    if(rawTargets.empty())
    {
        closesocket(sock);
        WSACleanup();
        return result;
    }

    // 构造NTP请求
    unsigned char pkt[48];
    make_ntp_request(pkt);

    // 发送请求至所有目标
    for(int attempt=0; attempt<max_attempts; ++attempt)
    {
        // 随机化序列 (避免频繁请求黑名单)
        std::vector<std::pair<std::string, sockaddr_in>> targets = rawTargets;
        std::shuffle(targets.begin(), targets.end(), global_rng());
        // 随机取3 (避免全量发送黑名单)
        targets.resize(std::min(3, (int)targets.size()));

        std::vector<std::string> used_names;
        used_names.reserve(targets.size());

        for(const auto& [name, addr] : targets)
        {
            sendto(sock,
                   reinterpret_cast<const char*>(pkt),
                   sizeof(pkt),
                   0,
                   reinterpret_cast<const sockaddr*>(&addr),
                   sizeof(addr));
            // std::cout << "sendto: " << name << std::endl;

            used_names.push_back(name);
        }

        // 等待首个请求
        while(true)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            timeval sel_tv{};
            sel_tv.tv_sec  = tv.tv_sec;
            sel_tv.tv_usec = tv.tv_usec;

            int ready = select(0, &readfds, nullptr, nullptr, &sel_tv);
            if(ready <= 0)
            {
                // std::cout << "ntp timeout" << std::endl;
                break;
            }

            if(FD_ISSET(sock, &readfds))
            {
                unsigned char resp[512];
                sockaddr_in from{};
                int fromlen = sizeof(from);
                int n = recvfrom(
                    sock,
                    reinterpret_cast<char*>(resp),
                    sizeof(resp),
                    0,
                    reinterpret_cast<sockaddr*>(&from),
                    &fromlen
                );

                // 返回报文长度不符
                if(n < 48) continue;

                uint32_t tx_seconds_be{};
                // 解析失败
                if(!parse_transmit_seconds(resp, static_cast<size_t>(n), tx_seconds_be)) continue;

                uint32_t tx_seconds = ntohl(tx_seconds_be);
                // 异常时间戳
                if(tx_seconds < NTP_TIMESTAMP_DELTA) continue;

                result.ok = true;
                result.utc_time = static_cast<std::time_t>(tx_seconds - NTP_TIMESTAMP_DELTA);

                char ipbuf[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));
                result.server_used = ipbuf;

                closesocket(sock);
                WSACleanup();
                return result;
            }
        }

        // 退让
        int delay_ms = std::min(BASE_DELAY_MS * (1 << attempt), MAX_DELAY_MS);
        std::uniform_int_distribution<int> dist(BASE_DELAY_MS, delay_ms);
        // 随机抖动
        int jitter = dist(global_rng());
        // std::cout << "sleep for: " << jitter << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(jitter));
    }

    closesocket(sock);
    WSACleanup();
    return result;
}

// 外部接口
inline std::pair<bool, std::chrono::system_clock::time_point>
get_utc_timepoint(int timeout_ms = 3000, int attempts = 2, size_t timeZone = 0)
{
    auto r = get_utc_from_ntp(timeout_ms, attempts);
    if(!r.ok) return {false, {}};
    return {true, std::chrono::system_clock::from_time_t(r.utc_time) + std::chrono::hours(timeZone)};
}

}