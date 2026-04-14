#pragma once
#include <map>
#include <string>
#include <functional>

namespace infra_network::http
{
    class MessageClient
    {
    public:
        using TextCallback = std::function<void(bool success, const std::string& text)>;
        using Parameters = std::map<std::string, std::string>;
        void get( const std::string& url, const Parameters& params, TextCallback cb, long timeoutMs = 5000);
    };
}