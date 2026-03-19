#pragma once
#include "util/infra/comp/progress/progress.hpp"
#include "util/infra/debug/log.hpp"
#include "util/infra/debug/time.hpp"

float progress {0.0f};
namespace test
{
    enum class PHRASE : uint32_t
    {
        phraseA,
        phraseB,
        phraseC
    };
}

namespace test
{
    void test()
    {
        comp_util::ProgressAgent<PHRASE> progressAgent(progress);
        progressAgent.reset(
            {
                {PHRASE::phraseA, 100, 0.8},
                {PHRASE::phraseB, 1, 0.2}
            }
        );

        {
            size_t idx=0;
            while(idx++<100)
            {
                LOG("doA");
                progressAgent.step(PHRASE::phraseA);
                SLEEP_MiLLISECOND(100);
                LOG("progress: ", progress);
            }
            progressAgent.finish(PHRASE::phraseA);

            LOG("progress: ", progress);
            LOG("progress: ", progress);
            LOG("progress: ", progress);
            LOG("progress: ", progress);
            SLEEP_MiLLISECOND(3000);
            progressAgent.finish(PHRASE::phraseB);
            LOG("progress: ", progress);
        }
    }
}