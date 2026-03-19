#pragma once

#include <barrier>
#include <latch>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <mutex>
#include <iostream>

/**
 * @brief 同步回合控制器
 * @param N: 参与线程数量
 * @param T: 同步回合数量
 */
template <size_t N, size_t T>
class SyncTurn
{
public:
    SyncTurn()
    : TICK_TOTAL(T)
    , _barrier1(N + 1)
    , _barrier2(N + 1)
    {}
    ~SyncTurn(){}
    SyncTurn(const SyncTurn&) = delete;
    SyncTurn& operator=(const SyncTurn&) = delete;
public:
    size_t waitNextTick()
    {
        size_t t = _tickCount.load(std::memory_order_acquire);
        if(t == TICK_TOTAL) return t;
        static int i {0};
        _barrier1.arrive_and_wait();
        _barrier2.arrive_and_wait();
        return t;
    }
    void sleepUtil() const
    {
        std::this_thread::sleep_until(_startTime + std::chrono::milliseconds(_tickCount.load() * _tickInterval));
    }
    bool start(size_t tickInterVal=200)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_enable) return false;

        {
            _enable = true;
            _tickInterval = tickInterVal;
            _tickCount.store(0, std::memory_order_relaxed);
            _startTime = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        }

        std::thread tickLoop(
            [this](){
                while(_enable)
                {
                    if(_tickCount.load(std::memory_order_acquire) == TICK_TOTAL) break;

                    _barrier1.arrive_and_wait();
                    _tickCount.fetch_add(1, std::memory_order_release);
                    _barrier2.arrive_and_wait();
                }
            }
        );
        tickLoop.detach();

        return true;
    }
public:
    void onPhaseComplete()
    {
        _tickCount.fetch_add(1, std::memory_order_release);
    }
private:
    const size_t TICK_TOTAL;
    std::atomic<size_t> _tickCount{0};
    bool _enable = false;
    std::chrono::steady_clock::time_point _startTime;
    size_t _tickInterval;
    std::barrier<> _barrier1;
    std::barrier<> _barrier2;
    std::mutex _mutex;
};