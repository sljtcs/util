#pragma once

#include <future>
template<typename T>
class ThenFuture
{
public:
    using Callback = std::function<void(const T&)>;
    ThenFuture(std::future<T>&& fut)
    : future_(std::move(fut))
    {}

    void then(Callback cb)
    {
        std::thread([f = std::move(future_), cb]() mutable {
            try {
                T value = f.get();
                cb(value);
            } catch (...) {
                // 简化：忽略异常
            }
        }).detach();
    }

private:
    std::future<T> future_;
};