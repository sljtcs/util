#pragma once

#include "test/rely.h"
#include <functional>

#include "util/component_util/lockFreeQueue.hpp"

namespace test
{
    bool enable{false};
    std::thread produceThread;
    std::thread consumeThread;
    
    // Define Queue Type
    using QueueType = util::LockFreeQueue<int, 1024>;
    QueueType queue;
}

namespace test
{
    void test()
    {
        enable = true;
        
        produceThread = std::thread([](){
            int counter = 0;
            while(enable)
            {
                if(queue.push(counter))
                {
                    // LOG("Pushed: ", counter);
                    counter++;
                }
                else
                {
                    // Queue full, yield or busy wait
                    std::this_thread::yield();
                }
                
                if (counter > 100000) break; // Limit for test
            }
        });

        consumeThread = std::thread([](){
            int expected = 0;
            int value = 0;
            while(enable)
            {
                if(queue.pop(value))
                {
                    if (value != expected)
                    {
                         LOG_ERR("Sequence error! Expected: ", expected, " Got: ", value);
                         enable = false;
                         return;
                    }
                    expected++;
                    
                    if (expected > 100000) 
                    {
                        LOG("Test Passed: 100000 items processed correctly.");
                        enable = false;
                        break;
                    }
                }
                else
                {
                     // Queue empty
                     std::this_thread::yield();
                }
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