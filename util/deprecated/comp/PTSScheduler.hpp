#pragma once
#include <deque>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <functional>
#include <condition_variable>

/**
 * @brief 相对时间戳调度器
 * @details callback 耗时过长将造成队列堆积
 */
template <typename Payload>
class PTSScheduler
{
public:
    struct Frame
    {
        uint64_t pts_ns;
        Payload  data;
    };
    using FramePtr = std::shared_ptr<Frame>;
    using SyncCallback = std::function<void(FramePtr)>;
public:
    void setSyncCallback(const SyncCallback& callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex_control);
        if(m_enable) return;
        m_callback = callback;
    }
    void push(FramePtr framePtr)
    {
        if(!framePtr) return;
        {
            std::lock_guard<std::mutex> lock(m_mutex_data);
            // while(m_queue.size() > 60) m_queue.pop();
            m_queue.push(framePtr);
        }
        m_cv.notify_one();
    }
public:
    bool start()
    {
        std::lock_guard<std::mutex> lock(m_mutex_control);
        if(m_enable) return true;
        if(!m_callback) return false;

        m_enable = true;
        m_thread = std::thread(&PTSScheduler::m_onSync, this);
        return true;
    }
    void stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex_control);
        if(!m_enable) return;
        m_enable = false;
        if(m_thread.joinable())
            m_thread.join();
        std::queue<FramePtr>().swap(m_queue);
    }
private:
    void m_onSync()
    {
        bool timeBaseInit {false};
        uint64_t lastPTS {0};
        std::chrono::steady_clock::time_point targetTimePoint;

        FramePtr frame;
        while(m_enable)
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex_data);
                m_cv.wait(lock, [this]{ return !m_queue.empty() || !m_enable; });
                if(!m_enable) break;
                frame = m_queue.front();
                m_queue.pop();
            }

            if(!timeBaseInit)
            {
                lastPTS = frame->pts_ns;
                targetTimePoint = std::chrono::steady_clock::now();
                timeBaseInit = true;
            }

            int64_t deltaTime = static_cast<int64_t>(frame->pts_ns - lastPTS);
            lastPTS = frame->pts_ns;
            targetTimePoint += std::chrono::nanoseconds(deltaTime);

            // if((targetTimePoint+2*deltaTime) < std::chrono::steady_clock::now()) continue;
            std::this_thread::sleep_until(targetTimePoint);
            m_callback(frame);
        }
    }
private:
    SyncCallback m_callback;
    std::mutex m_mutex_control;
    std::mutex m_mutex_data;
    std::queue<FramePtr> m_queue;
    std::thread m_thread;
    std::condition_variable m_cv;
    bool m_enable {false};
};