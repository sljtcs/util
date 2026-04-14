#include "file.h"
#include <openssl/evp.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>

namespace infra_hash
{
    std::string HashFile256(const std::filesystem::path& in)
    {
        std::ifstream file(in.string(), std::ios::binary);
        if(!file) return {};

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if(!ctx) return {};

        if(EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
        {
            EVP_MD_CTX_free(ctx);
            return {};
        }

        char buffer[4096];

        while(file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            EVP_DigestUpdate(ctx, buffer, file.gcount());
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;

        if(EVP_DigestFinal_ex(ctx, hash, &length) != 1)
        {
            EVP_MD_CTX_free(ctx);
            return {};
        }

        EVP_MD_CTX_free(ctx);

        std::ostringstream oss;
        for(unsigned int i = 0; i < length; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }

        return oss.str();
    }
}