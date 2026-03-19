#pragma once

#include "test/rely.h"
#include <functional>
#include "util/component_util/dataTranBase.hpp"
#include "util/component_util/latestValueCache.hpp"
#include "util/component_util/ringBuffer.hpp"

namespace test
{
    bool enable{false};
    std::thread produceThread;
    std::thread consumeThread;
    
    // using ContainerType = comp_util::LatestValueCache<int>;
    // using ContainerType = DataTran<int>;
    using ContainerType = comp_util::RingBuffer<int, 30>;
    ContainerType container;
}

namespace test
{
    void test()
    {
        enable = true;
        produceThread = std::thread([](){
            FRAME_CONTROL_INIT(PRODUCE, 30);
            int counter = 0;
            while(enable)
            {
                FRAME_CONTROL_BEGIN(PRODUCE);
                // container.post(counter);
                container.try_push(counter);
                ++counter;
                FRAME_CONTROL_END(PRODUCE);
            }
        });

        consumeThread = std::thread([](){
            int value {0};
            while(enable)
            {
                // if(container.try_fetch(value))
                if(container.pop(value))
                {
                    // LOG("value: ", value);
                    if(value > 1000) enable = false;
                    DEBUG_FRAME_RATE(FETCH, 3000);
                }
                else std::this_thread::yield();
            }
        });
        
        while(enable){
            SLEEP_MiLLISECOND(20);
        }

        if(produceThread.joinable()) produceThread.join();
        if(consumeThread.joinable()) consumeThread.join();
    }
}