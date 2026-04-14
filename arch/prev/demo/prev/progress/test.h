#pragma once

#include "test/rely.h"

namespace test
{
    void testFakeProgress()
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point doneTime = start + std::chrono::seconds(15);
        std::chrono::steady_clock::time_point end = start + std::chrono::seconds(7);
        std::chrono::duration<double> duration = end-start;
        fake_util::FakeProgress fakeProgrss;
        fakeProgrss.init(duration);

        bool isOver = false;
        bool done = false;
        while(!isOver)
        {
            if(!done)
            {
                if(std::chrono::steady_clock::now() > doneTime)
                {
                    fakeProgrss.setFinish(20);
                    done = true;
                }
            }
            double ret = fakeProgrss.getProgress();
            isOver = fakeProgrss.isFinished();
            fake_util::FakeProgress::printProgressBar(ret, 50);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    void test()
    {
        testFakeProgress();
        std::cout << "test" << std::endl;
    }
}