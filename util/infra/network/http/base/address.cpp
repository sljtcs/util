#include "address.h"
#include <sstream>

namespace infra_network::http
{
    void Address::set(const std::string& ip, size_t port)
    {
        ip_ = ip;
        port_ = port;
    }

    std::string Address::base() const
    {
        std::ostringstream oss;
        oss << "http" << "://" << ip_ << ":" << port_;
        return oss.str();
    }
}