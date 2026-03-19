#pragma once
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "data_type.h"

namespace ffmpeg_util
{
    class ffmpegUtil
    {
    public:
        enum class AudioType { AAC };
        enum class VideoType { MP4, MKV };
        struct ConvertParam
        {
            std::string inPath;
            std::string outDir;
            std::string outName;
            VideoType vType;
            AudioType aType;
        };
        struct ConbineParam
        {
            std::string videoPath;
            std::string audioPath;
            std::string outDir;
            std::string outName;
            VideoType outType;
        };
    public:
        static bool WAVtoAAC(const std::string& inWAV, const std::string& outAAC)
        {
            std::ostringstream oss;
            oss << "ffmpeg -y -i " << inWAV << " -c:a aac " << outAAC << std::endl;
            std::string command = oss.str();
            int ret = system(command.c_str());
            return (ret == 0);
        }

        static bool convertVideo(const ConvertParam& param)
        {
            std::string outPath = _conbinePath(param.outDir, param.outName, param.vType);
            std::ostringstream oss;
            oss << "ffmpeg -y -i " << param.inPath
                << " -c:v copy " << outPath
                << std::endl;
            std::string command = oss.str();

            int ret = system(command.c_str());
            return (ret == 0);
        }

        static bool conbineVideoAudio(const ConbineParam& param)
        {
            std::string outPath = _conbinePath(param.outDir, param.outName, param.outType);
            if(outPath.empty()) return false;

            std::ostringstream oss;
            oss << "ffmpeg -y -i " << param.videoPath
                << " -i " << param.audioPath
                << " -c:v copy -c:a aac -shortest " << outPath
                << std::endl;
            std::string command = oss.str();

            int ret = system(command.c_str());
            return (ret == 0);
        }
    private:
        static std::string _toExtension(VideoType type)
        {
            switch(type)
            {
                case VideoType::MP4: return ".mp4";
                case VideoType::MKV: return ".mkv";
                default: return ".mp4";
            }
        }
        static std::string _conbinePath(const std::string& dir, const std::string& name, VideoType type)
        {
            std::filesystem::path path = std::filesystem::path(dir) / (name + _toExtension(type));
            return path.string();
        }
    };
}



namespace ffmpeg_util
{
    /**
     * @brief 裁剪器
     */
    class TrimHandle
    {
    public:
        struct Param
        {
            BaseParam base;
            std::array<size_t,2> frameRange;
        };
    public:
        static bool trimVideo(const Param& param)
        {
            BaseParam base = param.base;
            if(!BaseHandle::validBaseParam(base))
                return false;

            std::filesystem::path outPath = std::filesystem::path(base.outDir) / base.outName;

            char filter[128]{0};
            std::snprintf(
                filter, sizeof(filter),
                // "select='between(n,%zu,%zu)',setpts=N/FRAME_RATE/TB",
                "trim=start_frame=%zu:end_frame=%zu,setpts=PTS-STARTPTS",
                param.frameRange[0],
                param.frameRange[1]
            );

            std::ostringstream oss;
            oss << "ffmpeg -y "
                << "-hwaccel cuda -hwaccel_output_format cuda "
                << "-i \"" << base.inPath << "\" "
                << "-vf \"" << filter << "\" "
                << "-c:v h264_nvenc -preset p7 -tune hq -rc vbr -cq 18 "
                << "\"" << outPath.string() << "\"";
            std::string command = oss.str();

            int ret = std::system(command.c_str());
            return (ret == 0);
        }
    };
}