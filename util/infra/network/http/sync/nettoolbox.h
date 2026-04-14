#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>
#include <algorithm>

struct BandwidthTool
{
public:
    void push(long long bytes)
    {
        if(bytes <= 0)
            return;

        const auto now = Clock::now();
        const auto nowSec = toSeconds(now);

        std::lock_guard<std::mutex> lock(mMutex);

        if(mBuckets.empty())
        {
            initBuckets(nowSec);
            mLastCalcSec = nowSec;
        }
        else
        {
            advanceBuckets(nowSec);
        }

        mBuckets[mHead].bytes += bytes;
    }

    // 每隔 intervalSec 秒才重新计算一次
    double getSpeed() const
    {
        const auto now = Clock::now();
        const auto nowSec = toSeconds(now);

        std::lock_guard<std::mutex> lock(mMutex);

        if(mBuckets.empty() || intervalSec == 0)
            return 0.0;

        // 未到刷新周期，直接返回缓存
        if(mLastCalcSec != 0 && (nowSec - mLastCalcSec) < intervalSec)
            return mCachedSpeed;

        const_cast<BandwidthTool*>(this)->advanceBuckets(nowSec);

        long long totalBytes = 0;

        for(const auto& bucket : mBuckets)
        {
            if(bucket.timestampSec == 0)
                continue;

            if(nowSec >= bucket.timestampSec &&
               nowSec - bucket.timestampSec < intervalSec)
            {
                totalBytes += bucket.bytes;
            }
        }

        mCachedSpeed = static_cast<double>(totalBytes) / static_cast<double>(intervalSec);
        mLastCalcSec = nowSec;

        return mCachedSpeed;
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mBuckets.clear();
        mHead = 0;
        mCachedSpeed = 0.0;
        mLastCalcSec = 0;
    }

public:
    unsigned int intervalSec {5};

private:
    using Clock = std::chrono::steady_clock;

    struct Bucket
    {
        long long timestampSec = 0;
        long long bytes = 0;
    };

    static long long toSeconds(const Clock::time_point& tp)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   tp.time_since_epoch()).count();
    }

    void initBuckets(long long nowSec)
    {
        const unsigned int bucketCount = std::max<unsigned int>(1u, intervalSec);
        mBuckets.assign(bucketCount, Bucket{});
        mHead = 0;
        mBuckets[mHead].timestampSec = nowSec;
        mBuckets[mHead].bytes = 0;
    }

    void advanceBuckets(long long nowSec)
    {
        if(mBuckets.empty())
        {
            initBuckets(nowSec);
            return;
        }

        long long headSec = mBuckets[mHead].timestampSec;
        if(headSec == 0)
        {
            mBuckets[mHead].timestampSec = nowSec;
            mBuckets[mHead].bytes = 0;
            return;
        }

        if(nowSec <= headSec)
            return;

        long long diff = nowSec - headSec;
        const long long bucketCount = static_cast<long long>(mBuckets.size());

        if(diff >= bucketCount)
        {
            for(auto& bucket : mBuckets)
            {
                bucket.timestampSec = 0;
                bucket.bytes = 0;
            }

            mHead = 0;
            mBuckets[mHead].timestampSec = nowSec;
            mBuckets[mHead].bytes = 0;
            return;
        }

        for(long long i = 0; i < diff; ++i)
        {
            mHead = (mHead + 1) % mBuckets.size();
            mBuckets[mHead].timestampSec = headSec + i + 1;
            mBuckets[mHead].bytes = 0;
        }
    }

private:
    mutable std::mutex mMutex;
    std::vector<Bucket> mBuckets;
    size_t mHead {0};

    mutable double mCachedSpeed {0.0};
    mutable long long mLastCalcSec {0};
};

// struct BandwidthTool
// {
// public:
//     BandwidthTool() = default;

//     explicit BandwidthTool(unsigned int intervalSeconds)
//         : intervalSec(intervalSeconds)
//     {
//     }

//     // 累加新下载的字节数
//     void push(long long bytes)
//     {
//         if(bytes <= 0)
//             return;

//         const auto now = Clock::now();
//         const auto nowSec = toSeconds(now);

//         std::lock_guard<std::mutex> lock(mMutex);

//         if(mBuckets.empty())
//         {
//             initBuckets(nowSec);
//         }
//         else
//         {
//             advanceBuckets(nowSec);
//         }

//         mBuckets[mHead].bytes += bytes;
//     }

//     // 返回最近 intervalSec 秒的平均速度，单位：bytes/s
//     double getSpeed() const
//     {
//         const auto now = Clock::now();
//         const auto nowSec = toSeconds(now);

//         std::lock_guard<std::mutex> lock(mMutex);

//         if(mBuckets.empty())
//             return 0.0;

//         const_cast<BandwidthTool*>(this)->advanceBuckets(nowSec);

//         long long totalBytes = 0;
//         unsigned int validBucketCount = 0;

//         for(const auto& bucket : mBuckets)
//         {
//             if(bucket.timestampSec == 0)
//                 continue;

//             if(nowSec >= bucket.timestampSec &&
//                nowSec - bucket.timestampSec < intervalSec)
//             {
//                 totalBytes += bucket.bytes;
//                 ++validBucketCount;
//             }
//         }

//         if(validBucketCount == 0 || intervalSec == 0)
//             return 0.0;

//         return static_cast<double>(totalBytes) / static_cast<double>(intervalSec);
//     }

//     void reset()
//     {
//         std::lock_guard<std::mutex> lock(mMutex);
//         mBuckets.clear();
//         mHead = 0;
//     }

// public:
//     unsigned int intervalSec {5}; // 统计周期，单位秒
//     mutable double mCachedSpeed {0.0};
// mutable long long mLastCalcSec {0};

// private:
//     using Clock = std::chrono::steady_clock;

//     struct Bucket
//     {
//         long long timestampSec = 0; // 该桶对应的秒
//         long long bytes = 0;
//     };

//     static long long toSeconds(const Clock::time_point& tp)
//     {
//         return std::chrono::duration_cast<std::chrono::seconds>(
//                    tp.time_since_epoch())
//             .count();
//     }

//     void initBuckets(long long nowSec)
//     {
//         const unsigned int bucketCount = std::max<unsigned int>(1u, intervalSec);
//         mBuckets.assign(bucketCount, Bucket{});
//         mHead = 0;
//         mBuckets[mHead].timestampSec = nowSec;
//         mBuckets[mHead].bytes = 0;
//     }

//     void advanceBuckets(long long nowSec)
//     {
//         if(mBuckets.empty())
//         {
//             initBuckets(nowSec);
//             return;
//         }

//         long long headSec = mBuckets[mHead].timestampSec;
//         if(headSec == 0)
//         {
//             mBuckets[mHead].timestampSec = nowSec;
//             mBuckets[mHead].bytes = 0;
//             return;
//         }

//         if(nowSec <= headSec)
//             return;

//         long long diff = nowSec - headSec;
//         const long long bucketCount = static_cast<long long>(mBuckets.size());

//         if(diff >= bucketCount)
//         {
//             for(auto& bucket : mBuckets)
//             {
//                 bucket.timestampSec = 0;
//                 bucket.bytes = 0;
//             }

//             mHead = 0;
//             mBuckets[mHead].timestampSec = nowSec;
//             mBuckets[mHead].bytes = 0;
//             return;
//         }

//         for(long long i = 0; i < diff; ++i)
//         {
//             mHead = (mHead + 1) % mBuckets.size();
//             mBuckets[mHead].timestampSec = headSec + i + 1;
//             mBuckets[mHead].bytes = 0;
//         }
//     }

// private:
//     mutable std::mutex mMutex;
//     std::vector<Bucket> mBuckets;
//     size_t mHead {0};
// };