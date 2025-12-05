#pragma once
#include <thread>
#include <mutex>
#include <queue>

template <typename T>
class SafeQueueBase
{
private:
    std::queue<T> q;
    std::mutex m;
public:
    size_t size()
    {
        return q.size();
    }
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(value);
    }

    bool pop(T& result)
    {
        std::unique_lock<std::mutex> lock(m);
        if(q.empty()) return false;
        result = std::move(q.front());
        q.pop();
        return true;
    }
};