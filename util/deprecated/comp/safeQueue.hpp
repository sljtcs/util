#pragma once
#include <thread>
#include <mutex>
#include <queue>

template <typename T>
class SafeQueue
{
private:
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    size_t size()
    {
        return q.size();
    }
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(value);
        cv.notify_one();
    }

    bool pop(T& result)
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return !q.empty(); });
        result = std::move(q.front());
        q.pop();
        return true;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }

    bool popLast(T& result)
    {
        std::lock_guard<std::mutex> lock(m);
        if(q.empty()) return false;

        // 逐个弹出直到剩下最后1个元素
        while(q.size() > 1)
            q.pop();

        result = std::move(q.front());
        q.pop();
        return true;
    }
};