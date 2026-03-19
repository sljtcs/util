#pragma once
#include <queue>
#include <mutex>
#include <cstddef>
#include <functional>
#include <vector>

class IDPool
{
public:
    static IDPool& instance()
    {
        static IDPool inst;
        return inst;
    }
    size_t acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_freeIds.empty())
        {
            size_t id = m_freeIds.top();
            m_freeIds.pop();
            return id;
        }
        return m_nextId++;
    }
    void release(size_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_freeIds.push(id);
    }
private:
    IDPool():m_nextId(1){}
    ~IDPool() = default;
    IDPool(const IDPool&) = delete;
    IDPool& operator=(const IDPool&) = delete;

    size_t m_nextId;
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_freeIds; 
    std::mutex m_mutex;
};