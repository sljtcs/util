#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <functional>

struct AMDSData
{
    size_t amds_id;
    float amds_data;
};

struct frameData
{
    size_t frame_id;
    long frame_data;
};

class TrackDataBase
{
public:
    virtual ~TrackDataBase() = default;
    virtual size_t idx() const = 0;
};

template<typename T>
class TrackData : public TrackDataBase
{
public:
    explicit TrackData(T&& data_): data(std::move(data_)){};
    static std::shared_ptr<TrackData> delege(T&& data)
    {
        return std::make_shared<TrackData>(data);
    }
public:
    virtual ~TrackData() = default;
    virtual size_t idx() const = 0;
protected:
    T data;
};

class TrackDataAMDS : public TrackData<AMDSData>
{
public:
    explicit TrackDataAMDS(AMDSData&& data) : TrackData(std::move(data)) {}
    size_t idx() const override
    {
        return data.amds_id;
    }
};


#define TrackCount 2
#define TrackLen 30
using TrackDataPtr = std::shared_ptr<TrackDataBase>;

#define IDX_COUNT(idx) (0x01<<idx)
#define IDX_FULL(idx) ((0x01<<(idx+1)) -1)

class MultTrackSync
{
public:
    using DataCallback = std::function<void(const std::array<TrackDataPtr, TrackCount>&)>;
public:
    void regist(DataCallback&& callback)
    {
        _callback = callback;
    }
public:
    void input(size_t track_idx, TrackDataPtr dataPtr)
    {
        size_t slot_idx = dataPtr->idx()%TrackLen;
        if(slot_idx == _syncWin.idx)
        {
            std::lock_guard<std::mutex> lock(_syncWin._mutex);
            if(slot_idx<_syncWin.max) return;
            store[track_idx][slot_idx] = dataPtr;
            count[slot_idx] |= IDX_COUNT(slot_idx);
            if(count[slot_idx] == IDX_FULL(TrackCount))
                syncCheck();
        }else
        {
            store[track_idx][slot_idx] = dataPtr;
            count[slot_idx] |= IDX_COUNT(slot_idx);
        }
    }
private:
    void syncCheck()
    {
        size_t check_idx = _syncWin.idx;
        size_t flagA {0x00}; flagA = ~flagA;
        size_t flagB {0x00};

        for(size_t idx=0; idx<TrackCount; ++idx)
        {
            flagA &= store[idx][check_idx]->idx();
            flagB |= store[idx][check_idx]->idx();
        }
        if(flagA == flagB)
        {
            for(size_t idx=0; idx<TrackCount; ++idx)
                _dataOut[idx] = store[idx][check_idx];
            _callback(_dataOut);
            (++_syncWin.idx) %= TrackLen;
            count[check_idx] &= 0x00;
        }
    }
private:
    std::array<uint32_t, TrackLen> count;
    std::array<std::array<TrackDataPtr, TrackLen>, TrackCount> store;
    DataCallback _callback {nullptr};
    std::array<TrackDataPtr, TrackCount> _dataOut;
private:
    struct SyncWin
    {
        size_t max {0};
        size_t idx {0};
        std::mutex _mutex;
    } _syncWin;
};