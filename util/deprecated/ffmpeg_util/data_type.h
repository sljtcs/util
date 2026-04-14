#pragma once
#include <string>
#include <filesystem>

namespace ffmpeg_util
{
    struct BaseParam
    {
        std::string inPath;
        std::string outDir;
        std::string outName;
    };

    enum class AudioType { AAC };
    enum class VideoType { MP4, MKV };

    class BaseHandle
    {
    public:
        static bool validBaseParam(BaseParam& param)
        {
            if(param.inPath.empty()) return false;
            std::filesystem::path input(param.inPath);
            if(param.outDir.empty())
                param.outDir = input.parent_path().string();
            if(param.outName.empty())
                param.outName = input.filename().string();
            return true;
        }
    public:
        static std::string toExtension(VideoType type)
        {
            switch(type)
            {
                case VideoType::MP4: return ".mp4";
                case VideoType::MKV: return ".mkv";
                default: return ".mp4";
            }
        }
        static std::string conbinePath(const std::string& dir, const std::string& name, VideoType type)
        {
            std::filesystem::path path = std::filesystem::path(dir) / (name + toExtension(type));
            return path.string();
        }

        static std::string toExtension(AudioType type)
        {
            switch(type)
            {
                case AudioType::AAC: return ".aac";
                default: return ".aac";
            }
        }
        static std::string conbinePath(const std::string& dir, const std::string& name, AudioType type)
        {
            std::filesystem::path path = std::filesystem::path(dir) / (name + toExtension(type));
            return path.string();
        }
    };
}