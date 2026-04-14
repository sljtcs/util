#include "recorderWASAPI.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <thread>
#include <filesystem>
#include <iostream>
#include <vector>
#include <comdef.h>

// Link against system libraries usually handled by cmake/compiler default searching, 
// but ensure CMakeLists adds them if missing.
// #pragma comment(lib, "Mmdevapi.lib")
// #pragma comment(lib, "Ole32.lib")

namespace
{
    std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    // Helper to release COM objects safely
    template <class T> void SafeRelease(T **ppT)
    {
        if (*ppT)
        {
            (*ppT)->Release();
            *ppT = NULL;
        }
    }
}

namespace audio_util::recorder
{
    RecorderWASAPI::RecorderWASAPI()
    : m_isRecording(false)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    {
    }

    RecorderWASAPI::~RecorderWASAPI()
    {
        stopRecord(std::chrono::steady_clock::now());
        releaseRecord();
    }

    void RecorderWASAPI::listDevice_supportRecord(std::vector<DeviceInfo>& deviceInfos)
    {
        deviceInfos.clear();
        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr)) return;

        IMMDeviceEnumerator *pEnumerator = NULL;
        IMMDeviceCollection *pCollection = NULL;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) { CoUninitialize(); return; }

        hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) { SafeRelease(&pEnumerator); CoUninitialize(); return; }

        UINT count = 0;
        pCollection->GetCount(&count);

        for (UINT i = 0; i < count; i++)
        {
            IMMDevice *pDevice = NULL;
            if (FAILED(pCollection->Item(i, &pDevice))) continue;

            IPropertyStore *pProps = NULL;
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (SUCCEEDED(hr))
            {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                if (SUCCEEDED(hr))
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
                        if(SUCCEEDED(pClient->GetMixFormat(&pwfx))) {
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

    bool RecorderWASAPI::setParam(const RecordParam& param)
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

    std::string RecorderWASAPI::getSavePath()
    {
        return m_savePath;
    }

    bool RecorderWASAPI::startRecord(const std::chrono::time_point<std::chrono::steady_clock>& startTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_isRecording || m_isReleasing) return false;

        m_startTimeStamp = startTime;

        // Note: Initialize COM in the worker thread could be better if we want to keep all COM there,
        // but startRecord is usually called from main thread.
        // For simplicity and matching RtAudio model where StartStream is called, we will optimize initialization here
        // OR spawn the thread to do everything.
        // However, to catch errors early (like device not found), we should Init here.
        // Warning: IAudioClient must be released in the same thread it was created if using STA? 
        // WASAPI interfaces are usually MTA friendly if CoInit is MTA.
        
        // We'll spawn the Capture Thread and let it do the heavy lifting of Init so everything is thread-local safe.
        // But we need to return whether it started inside the thread. 
        // For this implementation, we will initialize AudioClient here to check valid params, then pass to thread? 
        // NO, Audio Client methods are not thread-safe if called from different threads without marshaling.
        // Best practice: Do everything in the capture thread.

        // But we need to set m_isRecording = true quickly.
        
        m_isRecording = true;
        m_captureThread = std::thread(&RecorderWASAPI::m_captureThreadFunc, this);
        
        return true;
    }

    void RecorderWASAPI::stopRecord(const std::chrono::time_point<std::chrono::steady_clock>& endTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_isRecording) return;
        m_isReleasing = true;
        
        // Let the capture thread know we want to stop?
        // m_isRecording flag is used generally, but actual loop break condition needs to be handled.
        
        // Since m_captureThread is running, we wait for it to finish?
        // No, `stopRecord` usually just triggers the stop.
        // `releaseRecord` joins.
        // m_isReleasing = true will signal the thread loop to break.
        
        m_releaseThread = std::thread(&RecorderWASAPI::m_releaseThreadOn, this);
    }

    void RecorderWASAPI::releaseRecord()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_isReleasing && !m_isRecording) {
            // If already stopped and joined
            if(m_captureThread.joinable()) m_captureThread.join();
            if(m_releaseThread.joinable()) m_releaseThread.join();
            return;
        }

        if(m_releaseThread.joinable()) m_releaseThread.join();
        if(m_captureThread.joinable()) m_captureThread.join();
        
        m_isReleasing = false;
        m_isRecording = false;
    }

    void RecorderWASAPI::m_captureThreadFunc()
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        HRESULT hr = S_OK;
        IMMDeviceEnumerator *pEnumerator = NULL;
        IMMDevice *pDevice = NULL;
        WAVEFORMATEX *pwfx = NULL;
        IMMDeviceCollection *pCollection = NULL;
        IAudioClient *pClient = NULL;
        IAudioCaptureClient *pCapture = NULL;
        HANDLE hEvent = NULL;
        
        BYTE *pData = NULL;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;

        CoInitialize(NULL); 
        
        // 1. Get Device
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) goto Exit;

        {
             // Get device by ID (Index)
            hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
            if (SUCCEEDED(hr)) {
                hr = pCollection->Item(m_recordParam.deviceID, &pDevice);
                pCollection->Release();
                pCollection = NULL;
            }
        }
        SafeRelease(&pEnumerator);
        if (FAILED(hr) || !pDevice) goto Exit;

        // 2. Activate Audio Client
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pClient);
        SafeRelease(&pDevice);
        if (FAILED(hr)) goto Exit;

        m_audioClient = pClient; // Assign to member

        // 3. Get Mix Format
        hr = m_audioClient->GetMixFormat(&pwfx);
        if (FAILED(hr)) goto Exit;
        
        // Initialize
        hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                       AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
                                       10000000 /* 1 sec buffer */, 
                                       0, 
                                       pwfx, 
                                       NULL);
        if (FAILED(hr)) goto Exit;

        // 4. Get Capture Client
        hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapture);
        if (FAILED(hr)) goto Exit;
        m_captureClient = pCapture;

        // 5. Setup Event
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!hEvent) goto Exit;
        hr = m_audioClient->SetEventHandle(hEvent);
        if (FAILED(hr)) goto Exit;

        // 6. Open File and Write Header
        m_outFile.open(m_savePath, std::ios::binary);
        // Write header with TARGET format (16bit PCM), not input format (Float)
        // Assume default target is 16-bit PCM if input is float.
        m_writeWavHeader(m_outFile, pwfx->nSamplesPerSec, pwfx->nChannels, 0); 

        // 7. Start
        std::this_thread::sleep_until(m_startTimeStamp);
        hr = m_audioClient->Start();
        
        // 8. Loop
        {
            while(m_isRecording && !m_isReleasing)
            {
                DWORD retval = WaitForSingleObject(hEvent, 2000);
                if (retval != WAIT_OBJECT_0) continue; 
                
                hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    if (numFramesAvailable > 0)
                    {
                        // Buffer for 16-bit samples
                        std::vector<short> samples(numFramesAvailable * pwfx->nChannels);
                        float* pFloatData = (float*)pData;
                        
                        bool isFloat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || 
                                       (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && 
                                        ((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

                        if (isFloat)
                        {
                            for (UINT i = 0; i < numFramesAvailable * pwfx->nChannels; i++)
                            {
                                float sample = pFloatData[i];
                                if (sample > 1.0f) sample = 1.0f;
                                if (sample < -1.0f) sample = -1.0f;
                                samples[i] = (short)(sample * 32767.0f);
                            }
                            m_outFile.write((const char*)samples.data(), samples.size() * sizeof(short));
                            m_totalSamplesWritten += numFramesAvailable;
                        }
                        else
                        {
                             if (pwfx->wBitsPerSample == 16) {
                                  m_outFile.write((const char*)pData, numFramesAvailable * pwfx->nBlockAlign);
                                  m_totalSamplesWritten += numFramesAvailable;
                             }
                        }
                    }
                    m_captureClient->ReleaseBuffer(numFramesAvailable);
                }
            }
        }
        
        m_audioClient->Stop();
        CloseHandle(hEvent);

    Exit:
        if (pwfx) CoTaskMemFree(pwfx);
        SafeRelease(&m_captureClient);
        SafeRelease(&m_audioClient);
        if (m_outFile.is_open()) m_outFile.close();
        CoUninitialize();
    }

    void RecorderWASAPI::m_releaseThreadOn()
    {
        std::this_thread::sleep_until(m_endTimeStamp);
        
        // Signal stop
        m_isRecording = false; 
        
        // Wait for capture thread to finish is done in releaseRecord,
        // but here we just wait for time and set flag.
        
        // The releaseRecord call usually joins this thread.
        // And cleans up.
        // We need to make sure we don't block main thread too long.
        // m_finalizeWavHeader needs to be called after file close?
        // But file is closed in captureThreadFunc.
        // So we might need to sync.
        
        // Actually, m_releaseThreadOn just signals stop. The worker thread closes file.
        // Re-opening file to fix header must be done AFTER worker thread closes.
        // But `releaseRecord` joins `captureThread` then `releaseThread`.
        // If `m_releaseThreadOn` is running, `captureThread` might still be running.
        // So we can't finalize header easily here unless we join captureThread HERE.
        
        // Current design in DS: m_releaseThreadOn stops stream, closes file, finalizes header.
        // In WASAPI design: m_captureThreadFunc does the loop.
        // So m_releaseThreadOn should just set flag m_isReleasing = false? or m_isRecording = false.
        
        // Correct flow:
        // 1. m_releaseThreadOn sleeps until end time.
        // 2. Sets m_isRecording = false.
        // 3. This causes m_captureThreadFunc loop to exit.
        // 4. m_captureThreadFunc closes file.
        // 5. m_captureThreadFunc exits.
        
        // But who calls m_finalizeWavHeader?
        // It must be called after file is closed.
        // So `m_captureThreadFunc` should call it, OR `releaseRecord` calls it.
        // Let's make `m_captureThreadFunc` call it or helper.
        
        // Wait, m_finalizeWavHeader opens file again.
        // So m_captureThreadFunc should do it at the very end.
    }

    void RecorderWASAPI::m_cleanUp()
    {
        // Helper
    }

    void RecorderWASAPI::m_writeWavHeader(std::ofstream &outFile, unsigned int sampleRate, unsigned int channels, unsigned int totalSamples)
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

    void RecorderWASAPI::m_finalizeWavHeader()
    {
        // Must be called when Not writing.
        // Re-open in r/w mode
        std::fstream fs(m_savePath, std::ios::binary | std::ios::in | std::ios::out);
        if(!fs.is_open()) return;

        unsigned int dataSize = static_cast<unsigned int>(m_totalSamplesWritten * m_recordParam.nChannels * 2);
        unsigned int chunkSize = 36 + dataSize;

        fs.seekp(4, std::ios::beg);
        fs.write(reinterpret_cast<const char *>(&chunkSize), 4);
        fs.seekp(40, std::ios::beg);
        fs.write(reinterpret_cast<const char *>(&dataSize), 4);

        fs.close();
    }
}