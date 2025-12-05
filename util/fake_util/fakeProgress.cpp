#include "fakeProgress.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fake_util
{
    void FakeProgress::printProgressBar(double progress, int width)
    {
        int pos = static_cast<int>(progress * width);
        std::ostringstream oss;

        oss << "[";
        for(int i = 0; i < width; ++i)
        {
            if(i<pos) oss << "=";
            else if(i == pos) oss << ">";
            else oss << " ";
        }
        oss << "] "
            << std::fixed << std::setprecision(2) << (progress * 100.0) << " %";

        // 覆盖当前行
        std::cout << "\r" << oss.str() << std::flush;
    }

    void FakeProgress::init(Duration estimate)
    {
        m_estimate = std::max(estimate, Duration{0.1});
        m_start = TimePoint::now();
        m_progress = 0.0f;
        m_done = false;
    }

    double FakeProgress::getProgress()
    {
        if(m_done)
        {
            if(m_step > 0)
            {
                m_progress += m_stepSize;
                if(m_progress > 1.0) m_progress=1.0;
                --m_step;
            }
            return m_progress;
        }
        
        double t = std::chrono::duration_cast<Duration>(TimePoint::now() - m_start).count() / m_estimate.count();
        double p = (t < LINEAR_PHASE? t : LINEAR_PHASE + CURVE_PHASE * (1.0 - 1.0 / sqrt(1.0 + (t - LINEAR_PHASE) / CURVE_PHASE)));
        m_progress = std::max(m_progress, std::min(p,1.0));
        return m_progress;
    }

    void FakeProgress::setFinish(int smoothSteps)
    {
        if(!m_done)
        {
            m_done = true;
            m_step = smoothSteps;
            m_stepSize = (1.0 - m_progress) / std::max(1, smoothSteps);
        }
    }

    bool FakeProgress::isFinished()
    {
        return m_done && (m_step == 0);
    }
}