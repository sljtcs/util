#pragma once

#include <rtaudio/RtAudio.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>

namespace audio_util::recorder
{
    class RecorderDS
    {
    public:
        struct DeviceInfo
        {
            unsigned int id;
            std::string name;
            unsigned int inputChannel;
            unsigned int outputChannel;
        };
        struct RecordParam
        {
            std::string saveDir;
            std::string name;
            unsigned int deviceID   = 0;
            unsigned int sampleRate = 44100;
            unsigned int nChannels  = 1;
        };
    public:
        RecorderDS();
        ~RecorderDS();
    public:
        static void listDevice_supportRecord(std::vector<DeviceInfo>& deviceInfos);
        bool setParam(const RecordParam& recordParam);
        bool startRecord(const std::chrono::time_point<std::chrono::steady_clock>& startTime);
        void stopRecord(const std::chrono::time_point<std::chrono::steady_clock>& endTime);
        void releaseRecord();
        std::string getSavePath();
    private:
        static int m_recordCallback(void* outputBuffer, void* inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void* userData);
        void m_cleanUp();
        void m_writeWavHeader(std::ofstream &outFile, unsigned int sampleRate, unsigned int channels, unsigned int totalSamples);
        void m_finalizeWavHeader();
        void m_releaseThreadOn();
    private:
        std::mutex m_mutex;
        RtAudio* m_audio = nullptr;
        bool m_isRecording = false;
        bool m_isReleasing = false;

        size_t m_totalSamplesWritten = 0;
        RecordParam m_recordParam;
        std::string m_savePath;
        std::ofstream m_outFile;
        std::chrono::time_point<std::chrono::steady_clock> m_startTimeStamp;
        std::chrono::time_point<std::chrono::steady_clock> m_endTimeStamp;
        std::thread m_releaseThread;
    };
}