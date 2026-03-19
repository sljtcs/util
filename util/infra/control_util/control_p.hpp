#pragma once
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <iostream>

namespace control_util
{
    class Control_P
    {
    public:
        void input(bool result)
        {
            attempt++;
            if(result) success++;
            if(attempt >= window_size)
                update();
        }
    public:
        void update()
        {
            double p = double(success) / double(attempt);

            // EMA 平滑
            // ema_success_rate = ema_alpha * p + (1.0 - ema_alpha) * ema_success_rate;
            // double e = p_target - ema_success_rate;
            // std::cout << p_target << " : " << ema_success_rate << " : " << e << std::endl;

            double e = p_target - p;
            if(e<0) e *= 5;


            if(std::abs(e) >= deadzone)
            {
                // 乘法更新
                double factor = 1.0 + Kp * e;

                // 限制单步变化
                double max_step = 0.12;
                factor = std::clamp(factor, 1.0 - max_step, 1.0 + max_step);

                r = r * factor;
                r = std::clamp(r, r_min, r_max);
            }

            // 重置计数
            attempt = 0;
            success = 0;
        }
    public:
        double nominal_fps          {30.0};
        double p_target             {0.98};
        double Kp                   {0.25};
        double r_min                {0.50};
        double r_max                {5.00};
        double ema_alpha            {0.5};
        int window_size             {60};
        double deadzone             {0.003};

        // 同级参数
        int attempt                 {0};
        int success                 {0};
        double ema_success_rate     {0};
        double r                    {1.0};
    };
}