#pragma once

#include "test/rely.h"
#include <functional>
#include "util/audio_util/recorderDS.h"
#include <windows.h>

namespace test
{
    audio_util::recorder::RecorderDS recorder;
}

namespace test
{
    void test()
    {
        using namespace audio_util::recorder;
        std::vector<RecorderDS::DeviceInfo> devInfo;
        recorder.listDevice_supportRecord(devInfo);
        for(const auto& dev : devInfo)
        {
            LOG("id: ", dev.id);
            LOG("name: ", dev.name);
        }
        if(devInfo.empty()) return;

        RecorderDS::RecordParam param;
        param.saveDir = "./archive";
        param.nChannels = 1;
        param.sampleRate = 44100;
        param.name = "test";
        param.deviceID = devInfo[0].id;

        auto start = std::chrono::steady_clock::now();
        // auto end = start + std::chrono::seconds(20);
        auto end = start + std::chrono::seconds(60);

        recorder.setParam(param);
        LOG("save: ", recorder.getSavePath());

        recorder.startRecord(start);
        std::this_thread::sleep_until(end);
        recorder.stopRecord(end);
        recorder.releaseRecord();
        LOG("record over");
    }
}