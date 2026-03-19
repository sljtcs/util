#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>

/*
    双缓冲/模板参数定长
    后续相机数量增多需要重新编译
*/

template<typename T>
struct SlotData
{
    std::chrono::time_point<std::chrono::steady_clock> timeStamp;
    T data;
};

template <typename T>
class DoubleBufferSlot
{
public:
    DoubleBufferSlot()
	{
        m_buffers[0] = std::make_unique<SlotData<T>>();
        m_buffers[1] = std::make_unique<SlotData<T>>();
        m_frontBuffer.store(m_buffers[0].get(), std::memory_order_release);
    }
public:
    void write(const T& data)
	{
        auto now = std::chrono::steady_clock::now();
        SlotData<T>* backBuffer = getBackBuffer();
        backBuffer->timeStamp = now;
        backBuffer->data = data;
        m_frontBuffer.store(backBuffer, std::memory_order_release);
    }

    SlotData<T> read() const
	{
        SlotData<T>* frontBuffer = m_frontBuffer.load(std::memory_order_acquire);
        return *frontBuffer;
    }

    bool isLater(const std::chrono::time_point<std::chrono::steady_clock>& timeStamp) const
    {
        SlotData<T>* frontBuffer = m_frontBuffer.load(std::memory_order_acquire);
        return frontBuffer->timeStamp > timeStamp;
    }
private:
    SlotData<T>* getBackBuffer()
	{
        SlotData<T>* frontBuffer = m_frontBuffer.load(std::memory_order_acquire);
        return (frontBuffer == m_buffers[0].get()) ? m_buffers[1].get() : m_buffers[0].get();
    }
    std::unique_ptr<SlotData<T>> m_buffers[2];
    std::atomic<SlotData<T>*> m_frontBuffer;
};

template <typename T, size_t SlotCount>
class SlotBuffer
{
public:
    constexpr size_t size() const
    {
        return SlotCount;
    }

    void write(size_t idx, T&& data)
    {
        if(idx >= SlotCount) return;
        m_slots[idx].write(std::forward<T>(data));
    }

    SlotData<T> read(size_t idx) const
    {
        if(idx >= SlotCount) return m_empty;
        return m_slots[idx].read();
    }

    bool isLater(size_t idx, const std::chrono::time_point<std::chrono::steady_clock>& timeStamp) const
    {
        if(idx >= SlotCount) return false;
        return m_slots[idx].isLater(timeStamp);
    }
private:
    SlotData<T> m_empty{};
    std::array<DoubleBufferSlot<T>, SlotCount> m_slots;
};