#include "recorderDS.h"
#include <thread>
#include <filesystem>

namespace temp_util
{
    std::string string_To_UTF8(const std::string& str)
    {
        int nwLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

        wchar_t* pwBuf = new wchar_t[nwLen + 1];//一定要加1
        ZeroMemory(pwBuf, nwLen * 2 + 2);

        MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

        int nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

        char* pBuf = new char[nLen + 1];
        ZeroMemory(pBuf, nLen + 1);

        WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

        std::string retStr(pBuf);

        delete[]pwBuf;
        delete[]pBuf;

        pwBuf = NULL;
        pBuf = NULL;

        return retStr;
    }
}

namespace audio_util::recorder
{
    RecorderDS::RecorderDS()
    : m_audio(nullptr)
    , m_isRecording(false)
    {}

    RecorderDS::~RecorderDS()
    {
        stopRecord(std::chrono::steady_clock::now());
        releaseRecord();
    }
}

namespace audio_util::recorder
{
    void RecorderDS::listDevice_supportRecord(std::vector<DeviceInfo>& deviceInfos)
    {
        deviceInfos.clear();
        RtAudio audio(RtAudio::Api::WINDOWS_DS);
        // std::cout << "API used: " << audio.getCurrentApi() << std::endl;

        std::vector<unsigned int> ids = audio.getDeviceIds();
        for(unsigned int id : ids)
        {
            RtAudio::DeviceInfo info = audio.getDeviceInfo(id);
            if(info.inputChannels > 0)
                // deviceInfos.emplace_back(DeviceInfo{id, temp_util::string_To_UTF8(info.name), info.inputChannels, info.outputChannels});
                deviceInfos.emplace_back(DeviceInfo{id, info.name, info.inputChannels, info.outputChannels});
        }
    }
}

namespace audio_util::recorder
{
    bool RecorderDS::setParam(const RecordParam& param)
    {
        if(m_isRecording) return false;
        m_recordParam = param;

        std::string extension = ".wav";
        std::filesystem::path savePath = std::filesystem::path(param.saveDir)/(param.name+extension);
        m_savePath = savePath.string();
        std::error_code ec;
        std::filesystem::create_directories(savePath.parent_path(), ec);
        if(ec) return false;
        return true;
    }

    std::string RecorderDS::getSavePath()
    {
        return m_savePath;
    }
}

namespace audio_util::recorder
{
    bool RecorderDS::startRecord(const std::chrono::time_point<std::chrono::steady_clock>& startTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_isRecording || m_isReleasing) return false;

        m_startTimeStamp = startTime;

        try
        {
            m_audio = new RtAudio(RtAudio::Api::WINDOWS_DS);

            RtAudio::StreamParameters iParams{};
            iParams.deviceId = m_recordParam.deviceID;
            iParams.nChannels = m_recordParam.nChannels;
            iParams.firstChannel = 0;

            RtAudio::StreamOptions options{};

            m_audio->setErrorCallback([](RtAudioErrorType, const std::string &errorText){
                std::cerr << "RtAudio Error: " << errorText << std::endl;
            });

            {
                // unsigned int bufferFrames = 1024;
                unsigned int bufferFrames = 128;
                m_outFile.open(m_savePath, std::ios::binary);
                // 写入文件首部
                m_writeWavHeader(m_outFile, m_recordParam.sampleRate, m_recordParam.nChannels, 0);
                m_totalSamplesWritten = 0;

                m_audio->openStream(nullptr, &iParams, RTAUDIO_SINT16,
                                    m_recordParam.sampleRate, &bufferFrames,
                                    &RecorderDS::m_recordCallback, this, &options);
                m_audio->startStream();
            }

            std::thread([this](){
                std::this_thread::sleep_until(m_startTimeStamp);
                m_isRecording = true;
            }).detach();
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
            m_cleanUp();
            return false;
        }
    }

    void RecorderDS::stopRecord(const std::chrono::time_point<std::chrono::steady_clock>& endTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_audio) return;
        m_isReleasing = true;
        m_releaseThread = std::thread(&RecorderDS::m_releaseThreadOn, this);
    }

    void RecorderDS::releaseRecord()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_isReleasing) return;

        if(m_releaseThread.joinable()) m_releaseThread.join();
        m_isReleasing = false;
    }
}

namespace audio_util::recorder
{
    void RecorderDS::m_cleanUp()
    {
        if(m_audio)
        {
            if(m_audio->isStreamOpen()) m_audio->closeStream();
            delete m_audio;
            m_audio = nullptr;
        }
        if(m_outFile.is_open()) m_outFile.close();
        m_isRecording = false;
    }

    void RecorderDS::m_releaseThreadOn()
    {
        std::this_thread::sleep_until(m_endTimeStamp);

        // 先关闭流
        if(m_audio->isStreamOpen()) m_audio->stopStream();
        m_audio->closeStream();
        delete m_audio;
        m_audio = nullptr;
        m_isRecording = false;
        m_finalizeWavHeader();
    }

    void RecorderDS::m_writeWavHeader(std::ofstream &outFile, unsigned int sampleRate, unsigned int channels, unsigned int totalSamples)
    {
        unsigned int byteRate = sampleRate * channels * 2;
        unsigned int dataSize = totalSamples * channels * 2;

        outFile.write("RIFF", 4);
        unsigned int chunkSize = 36 + dataSize;
        outFile.write(reinterpret_cast<const char *>(&chunkSize), 4);
        outFile.write("WAVE", 4);

        outFile.write("fmt ", 4);
        unsigned int subchunk1Size = 16;
        outFile.write(reinterpret_cast<const char *>(&subchunk1Size), 4);
        unsigned short audioFormat = 1;
        outFile.write(reinterpret_cast<const char *>(&audioFormat), 2);
        outFile.write(reinterpret_cast<const char *>(&channels), 2);
        outFile.write(reinterpret_cast<const char *>(&sampleRate), 4);
        outFile.write(reinterpret_cast<const char *>(&byteRate), 4);
        unsigned short blockAlign = channels * 2;
        outFile.write(reinterpret_cast<const char *>(&blockAlign), 2);
        unsigned short bitsPerSample = 16;
        outFile.write(reinterpret_cast<const char *>(&bitsPerSample), 2);

        outFile.write("data", 4);
        outFile.write(reinterpret_cast<const char *>(&dataSize), 4);
    }

    void RecorderDS::m_finalizeWavHeader()
    {
        if(!m_outFile.is_open()) return;

        unsigned int dataSize = m_totalSamplesWritten * m_recordParam.nChannels * 2;
        unsigned int chunkSize = 36 + dataSize;

        m_outFile.seekp(4, std::ios::beg);
        m_outFile.write(reinterpret_cast<const char *>(&chunkSize), 4);
        m_outFile.seekp(40, std::ios::beg);
        m_outFile.write(reinterpret_cast<const char *>(&dataSize), 4);

        m_outFile.close();
    }

    int RecorderDS::m_recordCallback(
        void* outputBuffer, void* inputBuffer,
        unsigned int nFrames,
        double streamTime, RtAudioStreamStatus status,
        void* userData)
    {
        // static std::once_flag f;
        // std::call_once(f, []{
        //     SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        // });
        if(!inputBuffer || nFrames == 0) return 0;

        auto* recorder = static_cast<RecorderDS*>(userData);
        if(!recorder->m_isRecording) return 0;

        short* in = static_cast<short*>(inputBuffer);
        size_t frameCount = nFrames * recorder->m_recordParam.nChannels;

        // 写入文件
        recorder->m_outFile.write(reinterpret_cast<const char*>(in), frameCount * sizeof(short));

        // 采样总数
        recorder->m_totalSamplesWritten += nFrames;
        return 0;
    }
}