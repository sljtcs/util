#pragma once
#include <chrono>
#include <opencv2/opencv.hpp>

namespace cam_util
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    struct FrameData
    {
        size_t idx;
        TimeStamp timeStamp;
        cv::Mat data;
    };
    using CameraSyncFrameCallback = std::function<void(std::vector<FrameData>&)>;
}