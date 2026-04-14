#pragma once
#include "util/infra/network/http/sync/smdClient.h"
#include "util/infra/debug/log.hpp"

namespace test
{
    void test()
    {
        infra_network::http::SMDClient dClient;
        dClient.download("http://8.134.60.74:8892/release/Release26.04/cublasLt64_11.dll", "./temp/cublasLt64_11.dll", 30000, [](float p){
            LOG(p);
        });
    }
}