#pragma once

#include "test/rely.h"
#include <functional>
// #include "util/audio_util/recorderWASAPI.h"
#include "util/audio_util/lowRecorderWASAPI.h"

namespace test
{
    using Recorder = audio_util::recorder::LowRecorderWASAPI;
    Recorder recorder;
}

namespace test
{
    void test()
    {
        LOG("use WASAPI");
        using namespace audio_util::recorder;
        std::vector<Recorder::DeviceInfo> devInfo;
        recorder.listDevice_supportRecord(devInfo);
        for(const auto& dev : devInfo)
        {
            LOG("id: ", dev.id);
            LOG("name: ", dev.name);
        }
        if(devInfo.empty()) return;

        Recorder::RecordParam param;
        param.saveDir = "./archive";
        param.nChannels = 1;
        param.sampleRate = 44100;
        param.name = "test";
        param.deviceID = devInfo[1].id;

        auto start = std::chrono::steady_clock::now();
        // auto end = start + std::chrono::seconds(5);
        auto end = start + std::chrono::seconds(5);
        // auto end = start + std::chrono::seconds(60);

        recorder.setParam(param);
        LOG("save: ", recorder.getSavePath());

        recorder.startRecord(start);
        LOG("use WASAPI");
        std::this_thread::sleep_until(end);
        recorder.stopRecord(end);
        recorder.releaseRecord();
        LOG("record over");
    }
}