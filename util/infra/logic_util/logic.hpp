#pragma once
#include <thread>
#include <chrono>

/**
 * @brief N次调用触发
 */
#define LOGIC_PER_N_CALL_BEGIN(NAME, N)\
    do {\
        static size_t DPNT_count_##NAME = 0;\
        ++DPNT_count_##NAME;\
        if(DPNT_count_##NAME == N){\
            DPNT_count_##NAME = 0;
#define LOGIC_PER_N_CALL_END\
        }\
    } while(0);



/**
 * @brief 帧率控制
 * @details 周期定长
 * @param frameRate 期望帧率
 */
#define FRAME_CONTROL_CONST_INIT(NAME, frameRate)\
    auto frameInterval_##NAME = std::chrono::microseconds(size_t(1e6 / frameRate));\
    auto frameEndTime_##NAME = std::chrono::steady_clock::now()
#define FRAME_CONTROL_CONST_BEGIN(NAME)\
    frameEndTime_##NAME = std::chrono::steady_clock::now() + frameInterval_##NAME
#define FRAME_CONTROL_CONST_END(NAME)\
    std::this_thread::sleep_until(frameEndTime_##NAME)

/**
 * @brief 帧率控制
 * @details 周期变长
 * @param frameRate 期望帧率
 */
#define FRAME_CONTROL_INIT(NAME, frameRate)\
    size_t frameRate_##NAME = frameRate;\
    auto frameInterval_##NAME = std::chrono::microseconds(size_t(1e6 / frameRate));\
    auto frameStart_##NAME = std::chrono::steady_clock::now();\
    size_t frameCount_##NAME = 0
#define FRAME_CONTROL_BEGIN(NAME)\
    auto frameEndTime_##NAME = frameStart_##NAME + ++frameCount_##NAME * frameInterval_##NAME
#define FRAME_CONTROL_END(NAME) std::this_thread::sleep_until(frameEndTime_##NAME)
#define FRAME_CONTROL_CHANGE(NAME, frameRate)\
    if(frameRate_##NAME != frameRate){\
        frameInterval_##NAME = std::chrono::microseconds(size_t(1e6 / frameRate));\
        frameStart_##NAME = std::chrono::steady_clock::now();\
        frameCount_##NAME = 0;\
    }