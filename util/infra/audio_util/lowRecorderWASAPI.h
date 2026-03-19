#pragma once
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>

struct IAudioClient;
struct IAudioCaptureClient;

namespace audio_util::recorder
{
    class LowRecorderWASAPI
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
        LowRecorderWASAPI();
        ~LowRecorderWASAPI();
    public:
        static void listDevice_supportRecord(std::vector<DeviceInfo>& deviceInfos);
        bool setParam(const RecordParam& recordParam);
        bool startRecord(const std::chrono::time_point<std::chrono::steady_clock>& startTime);
        void stopRecord(const std::chrono::time_point<std::chrono::steady_clock>& endTime);
        void releaseRecord();
        std::string getSavePath();
    private:
        void m_writeWavHeader(std::ofstream &outFile, unsigned int sampleRate, unsigned int channels, unsigned int totalSamples);
        void m_finalizeWavHeader();
        void m_releaseThreadOn();
        void m_captureThreadFunc(); // New capture loop

    private:
        std::mutex m_mutex;
        
        // WASAPI Interfaces
        IAudioClient* m_audioClient = nullptr;
        IAudioCaptureClient* m_captureClient = nullptr;
        uint16_t m_wavChannels;
        uint32_t m_wavSampleRate;
        
        bool m_isRecording = false;
        bool m_isReleasing = false;

        size_t m_totalSamplesWritten = 0;
        RecordParam m_recordParam;
        std::string m_savePath;
        std::ofstream m_outFile;
        std::chrono::time_point<std::chrono::steady_clock> m_startTimeStamp;
        std::chrono::time_point<std::chrono::steady_clock> m_endTimeStamp;
        std::thread m_releaseThread;
        std::thread m_captureThread;
    };
}