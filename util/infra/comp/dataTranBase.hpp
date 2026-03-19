#pragma once
#include <mutex>

namespace comp_util
{
    template<typename T>
    class DataTranBase
    {
    public:
        void post(const T& in)
        {
            std::lock_guard<std::mutex> lock(mutex);
            buffer = in;
            isUpdate = true;
        }
        bool try_fetch(T& out)
        {
            if(!isUpdate) return false;
            std::lock_guard<std::mutex> lock(mutex);
            if(!isUpdate) return false;
            out = buffer;
            isUpdate = false;
            return true;
        }
    private:
        std::mutex mutex;
        bool isUpdate {false};
        T buffer;
    };
}