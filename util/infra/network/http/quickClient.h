#pragma once
#include <map>
#include <string>
#include <functional>
#include "base/address.h"

namespace infra_network::http
{

    class QuickClient
    {
    public:
        struct Respond
        {
            size_t status;
            std::string body;
        };
        bool post(const std::string& path, const std::string& body, const std::string& content_type, Respond& resp, long timeoutMs);
    public:
        Address address;
    };
}