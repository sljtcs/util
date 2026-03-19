
#pragma once
#include <new>
#include <mutex>

namespace comp_util
{
    struct alignas(std::hardware_destructive_interference_size) CacheLineAtomicInt
    {
        std::atomic<int> value;
        char pad[std::hardware_destructive_interference_size - sizeof(std::atomic<int>)];
    };

    template<typename T>
    class LatestValueCache
    {
    public:
        void post(const T& v)
        {
            auto g = _read.load(std::memory_order_relaxed);
            _data = v;
            _read.store(g + 1, std::memory_order_release);
        }

        bool try_fetch(T& out)
        {
            auto g = _read.load(std::memory_order_acquire);
            if(g == _last) return false;
            out = _data;
            _last = g;
            return true;
        }
    private:
        std::atomic<uint64_t> _read{0};
        T _data;
        uint64_t _last{0};
    };

    template<typename T>
    class LatestValueCacheDouble
    {
    public:
        void post(const T& v)
        {
            int w = _write.load(std::memory_order_relaxed);
            _buffer[w] = v;
            _read.value.store(w, std::memory_order_release);
            _write.store(1 - w, std::memory_order_relaxed);
        }

        bool try_fetch(T& out)
        {
            int r = _read.value.load(std::memory_order_acquire);
            if(r == _last) return false;
            out = _buffer[r];
            _last = r;
            return true;
        }

    private:
        CacheLineAtomicInt _read{0};
        std::atomic<int> _write{1};
        T _buffer[2];
        int _last{-1};
    };
}