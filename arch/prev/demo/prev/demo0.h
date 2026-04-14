#pragma once

#include "test/rely.h"
#include <functional>
// #include "util/component_util/lockFreeQueue.hpp"
#include "latestValueCache.hpp"

namespace test
{
    bool enable{false};
    std::thread produceThread;
    std::thread consumeThread;
    
    // using ContainerType = util::LockFreeQueue<int, 1024>;
    using ContainerType = comp_util::LatestValueCache<int>;
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
                container.post(counter);
                if(container.post(counter))
                {
                    LOG("Pushed: ", counter);
                    counter++;
                }
                else
                {
                    // Queue full, yield or busy wait
                    std::this_thread::yield();
                }
                
                if(counter > 100000) break;
                FRAME_CONTROL_END(PRODUCE);
            }
        });

        consumeThread = std::thread([](){
            int expected = 0;
            int value = 0;
            FRAME_CONTROL_INIT(CONSUME, 30);
            while(enable)
            {
                FRAME_CONTROL_BEGIN(CONSUME);
                if(container.(value))
                {
                    LOG("fetch: ", value);
                    if(value != expected)
                    {
                        LOG_ERR("Sequence error! Expected: ", expected, " Got: ", value);
                        enable = false;
                        return;
                    }
                    expected++;
                    
                    if(expected > 100000) 
                    {
                        LOG("Test Passed: 100000 items processed correctly.");
                        enable = false;
                        break;
                    }
                }
                else
                {
                    std::this_thread::yield();
                }
                FRAME_CONTROL_END(CONSUME);
            }
        });
        
        while(enable)
        {
            SLEEP_MiLLISECOND(20);
        }

        if (produceThread.joinable()) produceThread.join();
        if (consumeThread.joinable()) consumeThread.join();
    }
}