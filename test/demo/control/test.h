#pragma once

#include "test/rely.h"
#include "util/component_util/safeQueue0.hpp"
#include "util/control_util/control_p.hpp"
#include "util/control_util/producer.hpp"
#include <queue>

namespace test
{
    SafeQueue0<size_t> sg_dataQueue;
    control_util::Producer sg_producer;
    control_util::Control_P sg_control;
}

namespace test
{
    void init()
    {
        sg_producer.init(control_util::Producer::Param{
            .rate       = {30},
            .margin     = 3,
            .callback   = [](size_t data){
                sg_dataQueue.push(data);
            }
        });
        sg_producer.start();
    }

    void consumer()
    {
        size_t data;
        size_t frameRate{60};
        FRAME_CONTROL_INIT(CONSUMER, frameRate);
        while(true)
        {
            FRAME_CONTROL_BEGIN(CONSUMER);
            if(sg_dataQueue.pop(data))
            {
                // LOG("data: ", data);
                sg_control.input(true);
            }
            else
            {
                // LOG("failed");
                sg_control.input(false);
            }
            FRAME_CONTROL_END(CONSUMER);

            DEBUG_PER_N_CALL_BEGIN(DEBUG_FPS, 50);
            frameRate = 60 * 1.0/sg_control.r;
            FRAME_CONTROL_CHANGE(CONSUMER, frameRate);
            LOG("frameRate: ", frameRate);
            DEBUG_PER_N_CALL_END;
        }
    }

    void test()
    {
        init();
        consumer();
    }
}