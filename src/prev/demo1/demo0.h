#pragma once

#include "test/rely.h"
#include "util/ffmpeg_util/ffmpegUtil.hpp"

namespace test
{
    std::string input = "./video/test.mp4";
    std::string name = "trim.mp4";
}

namespace test
{
    void test()
    {
        ffmpeg_util::TrimHandle::Param param{
            .base = {
                .inPath = input,
                .outName = name
            },
            .frameRange = {120, 360}
        };

        ffmpeg_util::TrimHandle::trimVideo(param);
        LOG("test");
    }
}

// void test0()
// {
//     ffmpeg_util::ffmpegUtil::ConvertParam param{
//         .inPath = "./video/test.avi",
//         .outDir = "./video",
//         .outName = "trim",
//         .vType = ffmpeg_util::ffmpegUtil::VideoType::MP4,
//         .aType = ffmpeg_util::ffmpegUtil::AudioType::AAC
//     };

//     ffmpeg_util::ffmpegUtil::convertVideo(param);
//     LOG("test");
// }