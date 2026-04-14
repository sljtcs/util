#pragma once
#include <chrono>

class TimeCount
{
public:
    TimeCount() = delete;
    ~TimeCount() = delete;
public:
    struct TimeCountCtx
    {
        size_t totalCount = 0;
        size_t intervalCount = 0;
        std::chrono::time_point<std::chrono::steady_clock> prevInterval;
        float rate = 0.0f;
    };
public:
    static void count(TimeCountCtx& ctx)
    {
        ++ctx.totalCount;
        ++ctx.intervalCount;
    }
    static bool calRate(TimeCountCtx& ctx, size_t intervalMs)
    {
        auto now = std::chrono::steady_clock::now();
        if(ctx.prevInterval.time_since_epoch().count() == 0) 
        {
            ctx.prevInterval = now;
            return false;
        }

        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - ctx.prevInterval).count();
        if(elapsed_ms >= intervalMs)
        {
            ctx.rate = (static_cast<float>(ctx.intervalCount)/elapsed_ms)*1e3;
            ctx.intervalCount = 0;
            ctx.prevInterval = now;
            return true;
        }
        return false;
    }
};