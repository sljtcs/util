#pragma once
#include <filesystem>

namespace infra_hash
{
    std::string HashFile256(const std::filesystem::path& in);
}