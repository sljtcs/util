#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <array>
#include <mutex>
#include <semaphore>
#include <iostream>

template <size_t Capa, size_t Per> requires (Capa>0 && Per>0)
class SliceThreadPool
{
public:
    template <typename Context>
    using SliceThreadCallback = std::function<bool(size_t, Context&)>;

    SliceThreadPool()
    {
        m_stopping.store(false);
        m_activeThreads.store(0);
        for(size_t idx=0; idx<Capa; ++idx)
        {
            m_semaphores[idx] = std::make_unique<std::binary_semaphore>(0);
            m_threadState[idx].store(false);
        }
    }

    ~SliceThreadPool()
    {
        shutdownAll();
    }

    std::pair<size_t, size_t> sliceRange(size_t thread_idx)
    {
        return {thread_idx * Per, Per};
    }

    // 确保线程数满足slice的需求
    template <typename Context>
    void ensureThread(size_t slice, const SliceThreadCallback<Context>& callback)
    {
        size_t amount = _threadNeed(slice);
        std::lock_guard<std::mutex> lock(m_mutex);
        if(amount<=m_activeThreads) return;
        for(size_t idx=0; idx<amount; ++idx)
        {
            if(!m_threadState[idx].load())
            {
                m_threadState[idx].store(true);
                ++m_activeThreads;

                m_threadSlot[idx] = std::thread([this, idx, callback](){
                    Context context;
                    while(!m_stopping.load()){
                        m_semaphores[idx]->acquire();
                        if(m_stopping.load()) break;
                        callback(idx, context);
                    }
                    m_threadState[idx].store(false);
                });
            }
        }
    }

    template <typename Context, typename T>
    void ensureThread(size_t slice, bool (T::*memFunc)(size_t, Context&), T* obj)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t amount = _threadNeed(slice);
        std::cout << "need: " << amount << std::endl;
        for(size_t idx=0; idx<amount; ++idx)
        {
            if(!m_threadState[idx].load())
            {
                m_threadState[idx].store(true);
                ++m_activeThreads;

                if(m_threadSlot[idx].joinable()) m_threadSlot[idx].join();
                m_threadSlot[idx] = std::thread([this, idx, memFunc, obj](){
                    std::cout << "slice thread: " << idx << " listen" << std::endl;
                    Context context;
                    while(!m_stopping.load()){
                        m_semaphores[idx]->acquire();
                        if(m_stopping.load()) break;
                        if(!(obj->*memFunc)(idx, context)) break;
                    }
                    m_threadState[idx].store(false);
                    --m_activeThreads;
                    std::cout << "slice thread: " << idx << " quit" << std::endl;
                });
            }
        }
    }

    // 触发所有活跃线程
    void triggerAll(size_t count = 1)
    {
        size_t active = m_activeThreads.load();
        for(size_t idx=0; idx<active; ++idx)
            m_semaphores[idx]->release(count);
    }

    // 触发前N活跃线程
    void triggerN(size_t n, size_t count = 1)
    {
        size_t active = m_activeThreads.load();
        if(n > active) n = active;
        for(size_t idx=0; idx<n; ++idx)
            m_semaphores[idx]->release(count);
    }

    void shutdownAll()
    {
        m_stopping.store(true);
        for(size_t idx=0; idx<Capa; ++idx)
            m_semaphores[idx]->release();

        std::lock_guard<std::mutex> lock(m_mutex);
        for(size_t idx=0; idx<Capa; ++idx) {
            if(m_threadSlot[idx].joinable()) m_threadSlot[idx].join();
            m_threadState[idx].store(false);
        }
        m_activeThreads.store(0);
        m_stopping.store(false);
    }

private:
    size_t _threadNeed(size_t slice)
    {
        size_t need = (slice + Per - 1) / Per;
        return need<Capa ? need : Capa;
    }

private:
    std::atomic<bool> m_stopping;
    std::atomic<size_t> m_activeThreads;
    std::array<std::atomic<bool>, Capa> m_threadState;
    std::array<std::thread, Capa> m_threadSlot;
    std::array<std::unique_ptr<std::binary_semaphore>, Capa> m_semaphores;
    std::mutex m_mutex;
};