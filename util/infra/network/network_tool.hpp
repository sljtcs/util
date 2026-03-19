#pragma once
#include <string>
#include <regex>
#include <sstream>
#include <iomanip>

namespace network_util
{
    class UrlConvert
    {
    public:
        static std::pair<std::string, std::string> splitUrl(const std::string& url)
        {
            std::regex rgx(R"(^(https?://[^/]+)(/.*)$)");
            std::smatch match;
            if(std::regex_match(url, match, rgx))
                return {match[1].str(), match[2].str()};
            return {"",""};
        }

        static std::string urlEncode(const std::string& url)
        {
            auto [host, path] = splitUrl(url);
            if(host.empty() || path.empty()) return "";

            std::ostringstream oss;
            oss << host;
            for(char c : path)
            {
                if(isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~' || c=='/')
                    oss << c;
                else
                    oss << '%' << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)c;
            }
            return oss.str();
        }

        static std::string extractFileName(const std::string& url)
        {
            auto [host, path] = splitUrl(url);
            if(path.empty()) return "unknown";

            size_t qpos = path.find_first_of("?#");
            std::string cleanPath = (qpos == std::string::npos) ? path : path.substr(0, qpos);

            size_t pos = cleanPath.find_last_of('/');
            std::string filename = (pos == std::string::npos) ? cleanPath : cleanPath.substr(pos + 1);

            return filename;
        }
    };
}