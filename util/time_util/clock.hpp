#include <chrono>

namespace time_util
{
    class Clock
    {
    public:
        static double current_timeSecond()
        {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> duration_sec = now.time_since_epoch();
            return duration_sec.count();
        }
    };
}