#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <functional>
#include <algorithm>

#define IDX_COUNT(idx) (0x01<<idx)
#define IDX_FULL(idx) ((0x01<<(idx)) -1)

/**
 * @brief 多路同步器
 * @details 数据源相位差应 >TrackLen
 * @param TrackCount 数据源轨道数
 * @param TrackLen 循环缓冲区长度
 */

class TrackDataBase
{
public:
    virtual ~TrackDataBase() = default;
    virtual size_t idx() const = 0;
};
using TrackDataPtr = std::shared_ptr<TrackDataBase>;

template<typename T, typename Derived>
class TrackData : public TrackDataBase
{
public:
    explicit TrackData(T&& data_): data(std::move(data_)){};
    static std::shared_ptr<Derived> delege(T&& data)
    {
        return std::make_shared<Derived>(std::move(data));
    }
    static T Release(std::shared_ptr<TrackDataBase> basePtr)
    {
        auto derivedPtr = std::dynamic_pointer_cast<Derived>(basePtr);
        return std::move(derivedPtr->data);
    }
public:
    virtual ~TrackData() = default;
    virtual size_t idx() const = 0;
protected:
    T data;
};

template<size_t N>
concept ValidTrackCount = (N > 0) && (N <= 32);
template<size_t N>
concept ValidTrackLen = (N > 0);

template<size_t TrackCount, size_t TrackLen>
requires ValidTrackCount<TrackCount> && ValidTrackLen<TrackLen>
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
        if(track_idx >= TrackCount) return;
        size_t slot_idx = dataPtr->idx()%TrackLen;
        
        {
            std::lock_guard<std::mutex> lock(_mutex[slot_idx]);
            size_t data_idx = dataPtr->idx();
            if(_max_idx[slot_idx] > data_idx) return;
            if(_max_idx[slot_idx] < data_idx){
                count[slot_idx] = 0x00;
                _max_idx[slot_idx] = data_idx;
            }
            
            count[slot_idx] |= IDX_COUNT(track_idx);
            store[track_idx][slot_idx] = dataPtr;
            if(count[slot_idx] == IDX_FULL(TrackCount))
                syncCheck(slot_idx);
        }
    }
private:
    void syncCheck(size_t check_idx)
    {
        for(size_t idx=0; idx<TrackCount; ++idx)
            _dataOut[idx] = store[idx][check_idx];
        count[check_idx] = 0x00;
        _callback(_dataOut);
    }
private:
    std::array<size_t, TrackLen> _max_idx {0};
    std::array<std::mutex, TrackLen> _mutex;
    std::array<uint32_t, TrackLen> count {0};
    std::array<std::array<TrackDataPtr, TrackLen>, TrackCount> store;
    std::array<TrackDataPtr, TrackCount> _dataOut;
    DataCallback _callback {nullptr};
};