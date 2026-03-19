#include "lowRecorderWASAPI.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <thread>
#include <filesystem>
#include <iostream>
#include <vector>
#include <comdef.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")

namespace audio_util::recorder
{
    std::string WideToUTF8(const std::wstring& wstr)
    {
        if(wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    // Helper to release COM objects safely
    template <class T> void SafeRelease(T **ppT)
    {
        if(*ppT)
        {
            (*ppT)->Release();
            *ppT = NULL;
        }
    }
}

namespace audio_util::recorder
{
    LowRecorderWASAPI::LowRecorderWASAPI()
    : m_isRecording(false)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    {}

    LowRecorderWASAPI::~LowRecorderWASAPI()
    {
        stopRecord(std::chrono::steady_clock::now());
        releaseRecord();
    }

    void LowRecorderWASAPI::listDevice_supportRecord(std::vector<DeviceInfo>& deviceInfos)
    {
        deviceInfos.clear();
        HRESULT hr = CoInitialize(NULL);
        if(FAILED(hr)) return;

        IMMDeviceEnumerator *pEnumerator = NULL;
        IMMDeviceCollection *pCollection = NULL;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if(FAILED(hr)) { CoUninitialize(); return; }

        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
        if(FAILED(hr)) { SafeRelease(&pEnumerator); CoUninitialize(); return; }

        UINT count = 0;
        pCollection->GetCount(&count);

        for(UINT i = 0; i < count; ++i)
        {
            IMMDevice *pDevice = NULL;
            if(FAILED(pCollection->Item(i, &pDevice))) continue;

            IPropertyStore *pProps = NULL;
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if(SUCCEEDED(hr))
            {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                if(SUCCEEDED(hr))
                {
                    std::string name = WideToUTF8(varName.pwszVal);
                    // Retrieve channel count (approximate via MixFormat)
                    // Note: Getting exact channel count requires activating AudioClient, which is expensive.
                    // RtAudio does similar probing. We can assume at least 1 or 2.
                    // For listing purposes, we'll try to get the MixFormat.
                    IAudioClient *pClient = NULL;
                    unsigned int inChannels = 0;
                    if(SUCCEEDED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pClient))) {
                        WAVEFORMATEX *pwfx = NULL;
                        if(SUCCEEDED(pClient->GetMixFormat(&pwfx)))
                        {
                            inChannels = pwfx->nChannels;
                            CoTaskMemFree(pwfx);
                        }
                        SafeRelease(&pClient);
                    }

                    deviceInfos.emplace_back(DeviceInfo{i, name, inChannels, 0});
                }
                PropVariantClear(&varName);
                SafeRelease(&pProps);
            }
            SafeRelease(&pDevice);
        }

        SafeRelease(&pCollection);
        SafeRelease(&pEnumerator);
        CoUninitialize();
    }

    bool LowRecorderWASAPI::setParam(const RecordParam& param)
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

    std::string LowRecorderWASAPI::getSavePath()
    {
        return m_savePath;
    }

    bool LowRecorderWASAPI::startRecord(const std::chrono::time_point<std::chrono::steady_clock>& startTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_isRecording || m_isReleasing) return false;

        m_startTimeStamp = startTime;
        m_isRecording = true;
        m_captureThread = std::thread(&LowRecorderWASAPI::m_captureThreadFunc, this);
        
        return true;
    }

    void LowRecorderWASAPI::stopRecord(const std::chrono::time_point<std::chrono::steady_clock>& endTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_isRecording) return;
        m_isReleasing = true;
        m_releaseThread = std::thread(&LowRecorderWASAPI::m_releaseThreadOn, this);
    }

    void LowRecorderWASAPI::releaseRecord()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_isReleasing && !m_isRecording) {
            if(m_captureThread.joinable()) m_captureThread.join();
            if(m_releaseThread.joinable()) m_releaseThread.join();
            return;
        }

        if(m_releaseThread.joinable()) m_releaseThread.join();
        if(m_captureThread.joinable()) m_captureThread.join();
        
        m_isReleasing = false;
        m_isRecording = false;
    }

    void LowRecorderWASAPI::m_releaseThreadOn()
    {
        std::this_thread::sleep_until(m_endTimeStamp);
        m_isRecording = false;
    }
}


namespace audio_util::recorder
{
    void LowRecorderWASAPI::m_captureThreadFunc()
    {
        // 设置线程低优先级
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        HRESULT hr = S_OK;
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDevice* pDevice = nullptr;
        WAVEFORMATEX* pwfx = nullptr;
        IAudioClient* pClient = nullptr;
        IAudioCaptureClient* pCapture = nullptr;
        HANDLE hEvent = nullptr;
        BYTE* pData = nullptr;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;

        CoInitialize(NULL);

        // 1. Get Device
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                            __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if(FAILED(hr)) goto Exit;

        IMMDeviceCollection* pCollection = nullptr;
        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
        if(SUCCEEDED(hr))
        {
            hr = pCollection->Item(m_recordParam.deviceID, &pDevice);
            pCollection->Release();
            pCollection = nullptr;
        }
        pEnumerator->Release();
        if(FAILED(hr) || !pDevice) goto Exit;

        // 2. Activate Audio Client
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pClient);
        pDevice->Release();
        if(FAILED(hr)) goto Exit;

        m_audioClient = pClient;

        // 3. Get Mix Format
        hr = m_audioClient->GetMixFormat(&pwfx);
        if(FAILED(hr)) goto Exit;

        m_wavChannels = pwfx->nChannels;
        m_wavSampleRate = pwfx->nSamplesPerSec;

        // 4. Initialize Audio Client
        const REFERENCE_TIME bufferDuration   = 30 * 10000;
        const REFERENCE_TIME periodicity      = 30 * 10000;
        hr = m_audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            bufferDuration,
            periodicity,
            pwfx,
            NULL
        );
        // hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
        //                             AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        //                             10000000, // 1 sec buffer
        //                             0,
        //                             pwfx,
        //                             NULL);
        if(FAILED(hr)) goto Exit;

        REFERENCE_TIME defaultPeriod = 0, minPeriod = 0;
        m_audioClient->GetDevicePeriod(&defaultPeriod, &minPeriod);
        std::cout << "Device default period: " << defaultPeriod / 10000.0 << " ms\n";

        // 5. Get Capture Client
        hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapture);
        if(FAILED(hr)) goto Exit;
        m_captureClient = pCapture;

        // 6. Setup Event
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!hEvent) goto Exit;
        hr = m_audioClient->SetEventHandle(hEvent);
        if(FAILED(hr)) goto Exit;

        // 7. Open File and Write Header (PCM16)
        m_outFile.open(m_savePath, std::ios::binary);
        m_writeWavHeader(m_outFile, m_wavSampleRate, m_wavChannels, 0);

        // 8. Start recording
        std::this_thread::sleep_until(m_startTimeStamp);
        hr = m_audioClient->Start();

        // quit MMCSS
        DWORD taskIndex = 0;
        HANDLE hTask = AvSetMmThreadCharacteristics((LPCTSTR)"Audio", &taskIndex);
        if(hTask) AvRevertMmThreadCharacteristics(hTask);

        // 9. Capture loop
        while(m_isRecording && !m_isReleasing)
        {
            DWORD retval = WaitForSingleObject(hEvent, 2000);
            if(retval != WAIT_OBJECT_0) continue;

            hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
            if(SUCCEEDED(hr) && numFramesAvailable > 0)
            {
                std::vector<short> samples(numFramesAvailable * pwfx->nChannels, 0);

                bool isFloat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
                            (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                                ((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

                if(!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
                {
                    if(isFloat)
                    {
                        float* pFloatData = (float*)pData;
                        for(UINT i = 0; i < numFramesAvailable * pwfx->nChannels; ++i)
                        {
                            float sample = pFloatData[i];
                            if(sample > 1.0f) sample = 1.0f;
                            if(sample < -1.0f) sample = -1.0f;
                            samples[i] = static_cast<short>(sample * 32767.0f);
                        }
                    }
                    else if(pwfx->wBitsPerSample == 16)
                    {
                        memcpy(samples.data(), pData, numFramesAvailable * pwfx->nBlockAlign);
                    }
                    else
                    {
                        // 其他格式暂不支持
                        m_captureClient->ReleaseBuffer(numFramesAvailable);
                        continue;
                    }
                }

                m_outFile.write(reinterpret_cast<char*>(samples.data()), samples.size() * sizeof(short));
                m_totalSamplesWritten += numFramesAvailable;
            }

            m_captureClient->ReleaseBuffer(numFramesAvailable);
        }

        m_audioClient->Stop();
        m_finalizeWavHeader();
        CloseHandle(hEvent);

    Exit:
        if(pwfx) CoTaskMemFree(pwfx);
        if(pCapture) pCapture->Release();
        if(m_audioClient) m_audioClient->Release();
        if(m_outFile.is_open()) m_outFile.close();
        CoUninitialize();
    }

    void LowRecorderWASAPI::m_writeWavHeader(std::ofstream &outFile, unsigned int sampleRate, unsigned int channels, unsigned int totalSamples)
    {
        uint16_t bitsPerSample = 16;
        uint16_t blockAlign = channels * bitsPerSample / 8;
        uint32_t byteRate = sampleRate * blockAlign;
        uint32_t dataSize = 0; // start with 0
        uint32_t chunkSize = 36 + dataSize;

        outFile.write("RIFF", 4);
        outFile.write(reinterpret_cast<char*>(&chunkSize), 4);
        outFile.write("WAVE", 4);

        outFile.write("fmt ", 4);
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1; // PCM
        outFile.write(reinterpret_cast<char*>(&fmtSize), 4);
        outFile.write(reinterpret_cast<char*>(&audioFormat), 2);
        outFile.write(reinterpret_cast<char*>(&channels), 2);
        outFile.write(reinterpret_cast<char*>(&sampleRate), 4);
        outFile.write(reinterpret_cast<char*>(&byteRate), 4);
        outFile.write(reinterpret_cast<char*>(&blockAlign), 2);
        outFile.write(reinterpret_cast<char*>(&bitsPerSample), 2);

        outFile.write("data", 4);
        outFile.write(reinterpret_cast<char*>(&dataSize), 4);
    }

    void LowRecorderWASAPI::m_finalizeWavHeader()
    {
        std::fstream fs(m_savePath, std::ios::binary | std::ios::in | std::ios::out);
        if(!fs.is_open()) return;

        uint16_t bitsPerSample = 16;
        uint16_t blockAlign = m_wavChannels * bitsPerSample / 8;
        uint32_t dataSize = m_totalSamplesWritten * blockAlign;
        uint32_t chunkSize = 36 + dataSize;

        fs.seekp(4, std::ios::beg);
        fs.write(reinterpret_cast<char*>(&chunkSize), 4);

        fs.seekp(40, std::ios::beg);
        fs.write(reinterpret_cast<char*>(&dataSize), 4);

        fs.close();
    }
}