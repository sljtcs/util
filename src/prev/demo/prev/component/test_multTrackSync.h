#pragma once

#include "test/rely.h"
#include <functional>
#include "util/component_util/multTrackSync.hpp"
#include "util/logic_util/logic.hpp"

struct AMDSData
{
    size_t amds_id;
    float amds_data;
};
class TrackDataAMDS : public TrackData<AMDSData, TrackDataAMDS>
{
public:
    explicit TrackDataAMDS(AMDSData&& data) : TrackData(std::move(data)) {}
    size_t idx() const override
    {
        return data.amds_id;
    }
};

struct FrameData
{
    size_t frame_id;
    long frame_data;
};
class TrackDataFrame : public TrackData<FrameData, TrackDataFrame>
{
public:
    explicit TrackDataFrame(FrameData&& data) : TrackData(std::move(data)) {}
    size_t idx() const override
    {
        return data.frame_id;
    }
};

namespace test
{
    bool enable {true};
    std::thread AMDSThread;
    std::thread FrameThread;
    MultTrackSync<2, 30> sg_multTrackSync;
    std::array<TrackDataPtr, 2> outData;
    static void onArrive(const std::array<TrackDataPtr, 2>& data)
    {
        outData = data;
        AMDSData amdsData = TrackDataAMDS::Release(outData[0]);
        FrameData frameData = TrackDataFrame::Release(outData[1]);

        LOG("");
        LOG("amdsData: ", amdsData.amds_id);
        LOG("frameData: ", frameData.frame_id);
        if(amdsData.amds_id == 100) enable = false;
    }
}

namespace test
{
    void test()
    {
        sg_multTrackSync.regist(onArrive);
        AMDSThread = std::thread([](){
            size_t idx{15};
            FRAME_CONTROL_CONST_INIT(AMDS_THREAD, 30);
            while(enable)
            {
                FRAME_CONTROL_CONST_BEGIN(AMDS_THREAD);
                AMDSData data{.amds_id = idx++};
                sg_multTrackSync.input(0, TrackDataAMDS::delege(std::move(data)));
                FRAME_CONTROL_CONST_END(AMDS_THREAD);
            }
        });
        FrameThread = std::thread([](){
            size_t idx{0};
            FRAME_CONTROL_CONST_INIT(FRAME_THREAD, 30);
            while(enable)
            {
                FRAME_CONTROL_CONST_BEGIN(FRAME_THREAD);
                FrameData data{.frame_id = idx++};
                sg_multTrackSync.input(1, TrackDataFrame::delege(std::move(data)));
                FRAME_CONTROL_CONST_END(FRAME_THREAD);
            }
        });

        while(enable)
        {
            SLEEP_MiLLISECOND(20);
        }
        FrameThread.join();
        AMDSThread.join();
    }
}