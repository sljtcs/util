#pragma once
#include <string>

namespace infra_network::http
{
    class Address
    {
    public:
        void set(const std::string& ip, size_t port);
        std::string base() const;
        template<typename... Args>
        std::string url(Args&&... paths) const
        {
            std::string result = base();
            auto append = [&](std::string_view p)
            {
                if(p.empty()) return;

                if(!result.empty() && result.back() != '/')
                    result += '/';

                while(!p.empty() && p.front() == '/')
                    p.remove_prefix(1);
                result.append(p);
            };

            (append(std::forward<Args>(paths)), ...);
            return result;
        }
    private:
        std::string ip_;
        size_t port_;
    };
}