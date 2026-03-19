#pragma once
#include <chrono>

namespace fake_util
{
    class FakeProgress
    {
    public:
        /**
         * @param LINEAR_PHASE 线性进度阶段
         * @param STOP_EDGE 完成前能达到的最大进度
         * @param CURVE_PHASE 曲线进度阶段 无需设置
         */
        static constexpr double LINEAR_PHASE {0.5};
        static constexpr double STOP_EDGE {0.95};
        static constexpr double CURVE_PHASE {STOP_EDGE - LINEAR_PHASE};
        using TimePoint = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double>;
    public:
        /**
         * @brief 终端显示进度条
         * @param progress 进度
         * @param width 进度条长度
         */
        static void printProgressBar(double progress, int width = 50);
    public:
        /**
         * @brief 初始化进度条
         * @param estimate 估计真实事务完成所需时间
         */
        void init(Duration estimate);
        /**
         * @brief 真实事务完成
         * @param smoothSteps 平均分割剩余进度步数
         */
        void setFinish(int smoothSteps = 50);
        /**
         * @brief 获取进度
         * @return 进度值
         */
        double getProgress();
        /**
         * @brief 虚假进度条是否完成
         * @return 是否完成
         */
        bool isFinished();
    private:
        Duration m_estimate {1.0};
        TimePoint::time_point m_start {TimePoint::now()};
        double m_progress {0.0};
        double m_stepSize {0.0};
        bool m_done {false};
        int m_step {0};
    };
}