#pragma once
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>
#include <random>

namespace control_util
{
    class Producer
    {
    public:
        using dataCallback = std::function<void(size_t)>;
        struct Param
        {
            std::vector<uint8_t> rate;
            size_t margin;
            dataCallback callback;
        };
    public:
        Producer()
        {
            std::random_device rd;  
            m_gen = std::mt19937(rd());
        }
        ~Producer()
        {
            stop();
        }
    public:
        void init(const Param& param)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(m_isThreadOn) return;
            m_param = param;
        }

        void start()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(m_isThreadOn) return;

            m_isThreadOn = true;
            m_thread = std::thread(&Producer::m_threadOn, this);
        }
        void stop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_isThreadOn) return;
            if(m_thread.joinable()) m_thread.join();
        }
    private:
        void m_threadOn()
        {
            size_t intervalCount{0};
            auto start = std::chrono::steady_clock::now();
            auto intervalUnit = std::chrono::seconds(1);

            size_t rate_idx{0};
            std::uniform_int_distribution<> distrib(0, m_param.margin*2);

            while(m_isThreadOn)
            {
                auto intervalEnd = start+(++intervalCount)*intervalUnit;
                {
                    rate_idx = (rate_idx+1) % m_param.rate.size();
                    int margin = distrib(m_gen) - m_param.margin;
                    int rate = m_param.rate[rate_idx] + margin; 
                    for(size_t idx=0; idx<rate; ++idx)
                        m_param.callback(m_dataCount++);
                }
                std::this_thread::sleep_until(intervalEnd);
            }
        }
    private:
        std::mt19937 m_gen;
        Param m_param;
        size_t m_dataCount{0};
        bool m_isThreadOn{false};
        std::thread m_thread;
        std::mutex m_mutex;
    };
}