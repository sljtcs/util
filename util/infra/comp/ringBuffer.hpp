#pragma once
#include <atomic>
#include <array>

namespace comp_util
{
    template <typename T, size_t Capacity>
    class RingBuffer
    {
    public:
        RingBuffer()
        : head_(0)
        , tail_(0)
        {}
        ~RingBuffer()
        {}

        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        bool try_push(const T& value)
        {
            const size_t curr_tail = tail_.load(std::memory_order_relaxed);
            const size_t next_tail = (curr_tail + 1) % Capacity;

            // full
            if(next_tail == head_.load(std::memory_order_acquire))
                return false;

            buffer_[curr_tail] = value;
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }

        void push(const T& value)
        {
            const size_t curr_tail = tail_.load(std::memory_order_relaxed);
            const size_t next_tail = (curr_tail + 1) % Capacity;

            // full
            if(next_tail == head_.load(std::memory_order_acquire))
            {
                const size_t curr_head = head_.load(std::memory_order_acquire);
                head_.store((curr_head + 1) % Capacity, std::memory_order_release);
            }

            buffer_[curr_tail] = value;
            tail_.store(next_tail, std::memory_order_release);
        }

        bool pop(T& value)
        {
            const size_t curr_head = head_.load(std::memory_order_relaxed);
            if(curr_head == tail_.load(std::memory_order_acquire))
                return false;

            value = std::move(buffer_[curr_head]);
            head_.store((curr_head + 1) % Capacity, std::memory_order_release);
            return true;
        }

        size_t size() const
        {
            size_t t = tail_.load(std::memory_order_relaxed);
            size_t h = head_.load(std::memory_order_relaxed);
            if(t >= h) return t - h;
            return Capacity + t - h;
        }

        bool empty() const
        {
            return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
        }
    private:
        alignas(64) std::atomic<size_t> head_;
        alignas(64) std::atomic<size_t> tail_;
        std::array<T, Capacity> buffer_;
    };
}