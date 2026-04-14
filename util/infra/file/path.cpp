#include "path.h"

using namespace infra_file;


namespace fs = std::filesystem;

namespace infra_file
{
    static bool existDir_(const fs::path& in)
    {
        std::error_code ec;
        return (std::filesystem::is_directory(in, ec) && !ec);
    }
}

namespace infra_file
{
    bool isExistFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        return fs::exists(path, ec) && fs::is_regular_file(path, ec);
    }
    bool isExistFolder(const std::filesystem::path& path)
    {
        std::error_code ec;
        return fs::exists(path, ec) && fs::is_directory(path, ec);
    }
    bool ensureFolder(const std::filesystem::path& path)
    {
        std::error_code ec;
        if(fs::exists(path, ec))
            return fs::is_directory(path, ec);
        return fs::create_directories(path, ec);
    }
    bool ensureParent(const std::filesystem::path& path)
    {
        std::error_code ec;
        auto parent = path.parent_path();
        if(parent.empty())
            return true;
        if(fs::exists(parent, ec))
            return fs::is_directory(parent, ec);
        return fs::create_directories(parent, ec);
    }

    bool deleteFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        if(!fs::exists(path, ec) || ec) return false;
        if(!fs::is_regular_file(path, ec) || ec) return false;
        return fs::remove(path, ec) && !ec;
    }

    bool deleteFolder(const std::filesystem::path& path)
    {
        std::error_code ec;
        if(!fs::exists(path, ec) || ec) return false;
        if(!fs::is_directory(path, ec) || ec) return false;
        return fs::remove_all(path, ec) > 0 && !ec;
    }
}

namespace infra_file
{
    static bool getFileInDir_(const std::filesystem::path& root, std::vector<std::filesystem::path>& out)
    {
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(root,
            std::filesystem::directory_options::skip_permission_denied, ec);
        if(ec) return false;

        for(auto& entry : it)
        {
            if(entry.is_regular_file(ec) && !ec)
                out.push_back(entry.path());
            ec.clear();
        }
        return true;
    }

    bool getFileInDir(const std::filesystem::path& path, std::vector<std::filesystem::path>& out)
    {
        out.clear();
        if(!existDir_(path)) return false;
        if(!getFileInDir_(path, out)) return false;
        return true;
    }

    bool getFileInDir(const std::filesystem::path& path, const std::string& ext_, std::vector<std::filesystem::path>& out)
    {
        if(ext_.empty())
            return false;
        std::vector<fs::path> all;
        if(!getFileInDir(path, all))
            return false;
        std::copy_if(all.begin(), all.end(),
            std::back_inserter(out),[&](const auto& p){return p.extension() == ext_;});
        return true;
    }

    bool getDirInDir(const std::filesystem::path& path_, std::vector<std::filesystem::path>& out)
    {
        out.clear();
        if(!std::filesystem::exists(path_) || !std::filesystem::is_directory(path_))
            return false;

        std::error_code ec;
        for(const auto& entry : std::filesystem::directory_iterator(path_, ec))
        {
            if(ec)
                return false;
            if(entry.is_directory())
                out.push_back(entry.path());
        }
        return true;
    }
}

namespace infra_file
{
    bool copyTo(const std::filesystem::path& src, const std::filesystem::path& dst)
    {
        std::filesystem::create_directories(dst.parent_path());
        std::error_code ec;
        return std::filesystem::copy_file(src, dst,
            std::filesystem::copy_options::overwrite_existing,
            ec
        );
    }
}