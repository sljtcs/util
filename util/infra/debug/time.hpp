#pragma once

#ifndef UNABLE_DEBUG_TIME
    #include <thread>
    #include <chrono>
    #include <iostream>
#endif

#ifndef UBABLE_DEBUG_TIME
    #define TIME_BLOCK_BEGIN(NAME) \
        auto __start_time_##NAME = std::chrono::high_resolution_clock::now();
    #define TIME_BLOCK_END(NAME) \
        do { \
            auto __end_time_##NAME = std::chrono::high_resolution_clock::now(); \
            auto __duration_##NAME = std::chrono::duration_cast<std::chrono::milliseconds>(__end_time_##NAME - __start_time_##NAME).count(); \
            std::cout << "[TIME] " << #NAME << " took " << __duration_##NAME << " ms\n"; \
        } while(0);

    #define TIME_PER_N_CALL(NAME, N) \
        do { \
            static int DNC_count_##NAME = 0; \
            static auto DNC_lastTime_##NAME = std::chrono::high_resolution_clock::now(); \
            ++DNC_count_##NAME; \
            if(DNC_count_##NAME >= N){ \
                auto __now_##NAME = std::chrono::high_resolution_clock::now(); \
                auto __duration_##NAME = std::chrono::duration_cast<std::chrono::milliseconds>(__now_##NAME - DNC_lastTime_##NAME).count(); \
                std::cout << "[N_CALL] " << #NAME << " every " << N << " calls took " << __duration_##NAME << " ms\n"; \
                DNC_count_##NAME = 0; \
                DNC_lastTime_##NAME = __now_##NAME; \
            } \
        } while(0)

    /**
     * @brief 调试帧率
     * @param mills 帧率检查周期
     */
    #define DEBUG_FRAME_RATE(NAME, mills)\
        do { \
            static auto DFR_intervalStart_##NAME = std::chrono::steady_clock::now(); \
            static auto DFR_intervalEnd_##NAME = DFR_intervalStart_##NAME + std::chrono::milliseconds(mills); \
            static size_t DFR_timeCount_##NAME = 0; \
            ++DFR_timeCount_##NAME; \
            auto DFR_now_##NAME = std::chrono::steady_clock::now(); \
            if(DFR_now_##NAME > DFR_intervalEnd_##NAME) { \
                auto DFR_elapsed_##NAME = std::chrono::duration_cast<std::chrono::milliseconds>(DFR_now_##NAME - DFR_intervalStart_##NAME).count(); \
                float DFR_frameRate_##NAME = (static_cast<float>(DFR_timeCount_##NAME) / DFR_elapsed_##NAME) * 1000.0f; \
                std::cout << "[FRAME_RATE] " << #NAME << " : " << DFR_frameRate_##NAME << " fps\n"; \
                DFR_timeCount_##NAME = 0; \
                DFR_intervalStart_##NAME = DFR_now_##NAME; \
                DFR_intervalEnd_##NAME = DFR_now_##NAME + std::chrono::milliseconds(mills); \
            } \
        } while(0)

    #define DEBUG_FRAME_RATE_OBJ(OBJ_PTR, NAME, mills)\
        do { \
            static auto DFR_intervalStart_##NAME = std::chrono::steady_clock::now(); \
            static auto DFR_intervalEnd_##NAME = DFR_intervalStart_##NAME + std::chrono::milliseconds(mills); \
            static std::unordered_map<const void*, size_t> DFR_timeCount_##NAME; \
            ++DFR_timeCount_##NAME[OBJ_PTR]; \
            auto DFR_now_##NAME = std::chrono::steady_clock::now(); \
            if(DFR_now_##NAME > DFR_intervalEnd_##NAME) { \
                auto DFR_elapsed_##NAME = std::chrono::duration_cast<std::chrono::milliseconds>(DFR_now_##NAME - DFR_intervalStart_##NAME).count(); \
                for(auto& kv : DFR_timeCount_##NAME) { \
                    float fps = static_cast<float>(kv.second) / DFR_elapsed_##NAME * 1000.0f; \
                    std::cout << "[FRAME_RATE] " << #NAME << " (key=" << kv.first << ") : " << fps << " fps\n"; \
                    kv.second = 0; \
                } \
                DFR_intervalStart_##NAME = DFR_now_##NAME; \
                DFR_intervalEnd_##NAME = DFR_now_##NAME + std::chrono::milliseconds(mills); \
            } \
        } while(0)

    #define DEBUG_PER_N_CALL_BEGIN(NAME, N)\
        do {\
            static size_t DPNT_count_##NAME = 0;\
            ++DPNT_count_##NAME;\
            if(DPNT_count_##NAME == N){\
                DPNT_count_##NAME = 0;
    #define DEBUG_PER_N_CALL_END\
            }\
        } while(0);

#else
    #define TIME_BLOCK_BEGIN(NAME)
    #define TIME_BLOCK_END(NAME)
    #define TIME_EVERY_N_CALLS(NAME, N)
    #define DEBUG_FRAME_RATE(NAME, mills)
#endif

#define SLEEP_MiLLISECOND(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

