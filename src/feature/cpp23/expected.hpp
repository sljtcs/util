#pragma once
#include <expected>
#include <string>
#include <system_error>
#include <iostream>
#include "util/infra/debug/log.hpp"

enum class ParseErr
{
    Empty,
    InvalidChar,
    Overflow
};

inline const char* to_string(ParseErr e) {
    switch (e) {
        case ParseErr::Empty: return "Empty";
        case ParseErr::InvalidChar: return "InvalidChar";
        case ParseErr::Overflow: return "Overflow";
    }
    return "Unknown";
}


namespace test
{
    std::expected<int, ParseErr> parse_int(std::string_view s)
    {
        if(s.empty())
            return std::unexpected(ParseErr::Empty);

        long long x = 0;
        for(char c : s)
        {
            if(c < '0' || c > '9') return std::unexpected(ParseErr::InvalidChar);
            x = x * 10 + (c - '0');
            if(x > std::numeric_limits<int>::max())
                return std::unexpected(ParseErr::Overflow);
        }
        return static_cast<int>(x);
    }

    void test()
    {
        LOG("test");
        for(auto s : {"", "12a3", "2147483647", "2147483648"})
        {
            auto r = parse_int(s);
            if(r)
            {
                std::cout << "parse_int(\"" << s << "\") = " << *r << "\n";
            } else {
                std::cout << "parse_int(\"" << s << "\") error = " << to_string(r.error()) << "\n";
            }
        }
    }
}