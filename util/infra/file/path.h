#pragma once
#include <vector>
#include <filesystem>

namespace infra_file
{
    bool isExistFile(const std::filesystem::path& path);
    bool isExistFolder(const std::filesystem::path& path);
    bool ensureFolder(const std::filesystem::path& path);
    bool ensureParent(const std::filesystem::path& path);

    bool deleteFile(const std::filesystem::path& path);
    bool deleteFolder(const std::filesystem::path& path);

    bool getFileInDir(const std::filesystem::path& path, std::vector<std::filesystem::path>& out);
    bool getFileInDir(const std::filesystem::path& path, const std::string& extension, std::vector<std::filesystem::path>& out);
    bool getDirInDir(const std::filesystem::path& path, std::vector<std::filesystem::path>& out);

    bool copyTo(const std::filesystem::path& src, const std::filesystem::path& dst);
}