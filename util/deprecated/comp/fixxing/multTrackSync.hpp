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
 * @bug 存在缺陷 窗口滑落
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
        store[track_idx][slot_idx] = dataPtr;
        uint32_t old = count[slot_idx].fetch_or(IDX_COUNT(track_idx), std::memory_order_acq_rel);
        uint32_t now = old | IDX_COUNT(track_idx);
        count[slot_idx].fetch_or(IDX_COUNT(track_idx), std::memory_order_acq_rel);
        if((slot_idx == _syncWin.idx) && (now == IDX_FULL(TrackCount)))
            syncCheck();
    }
private:
    void syncCheck()
    {
        std::lock_guard<std::mutex> lock(_syncWin.mutex);
        size_t check_idx = _syncWin.idx;
        size_t flagA {store[0][check_idx]->idx()};
        size_t flagB {flagA};
        for(size_t idx=1; idx<TrackCount; ++idx)
        {
            flagA &= store[idx][check_idx]->idx();
            flagB |= store[idx][check_idx]->idx();
            if(flagA ^ flagB) return;
        }
        for(size_t idx=0; idx<TrackCount; ++idx)
            _dataOut[idx] = store[idx][check_idx];
        _callback(_dataOut);
        (++_syncWin.idx) %= TrackLen;
        count[check_idx] = 0x00;
    }
private:
    std::array<std::atomic<uint32_t>, TrackLen> count;
    std::array<std::array<TrackDataPtr, TrackLen>, TrackCount> store;
    DataCallback _callback {nullptr};
    std::array<TrackDataPtr, TrackCount> _dataOut;
private:
    struct SyncWin{
        size_t idx {0};
        std::mutex mutex;
    } _syncWin;
};

// void syncCheck()
// {
//     std::lock_guard<std::mutex> lock(_syncWin.mutex);
//     size_t check_idx = _syncWin.idx;
//     size_t flagA {0x00}; flagA = ~flagA;
//     size_t flagB {0x00};

//     // 如果全部值相等则相与相或的值相同
//     for(size_t idx=0; idx<TrackCount; ++idx)
//     {
//         flagA &= store[idx][check_idx]->idx();
//         flagB |= store[idx][check_idx]->idx();
//     }
//     // 同步成功触发回调
//     if(flagA == flagB)
//     {
//         for(size_t idx=0; idx<TrackCount; ++idx)
//             _dataOut[idx] = store[idx][check_idx];
//         _callback(_dataOut);
//         (++_syncWin.idx) %= TrackLen;
//         count[check_idx] = 0x00;
//     }
// }