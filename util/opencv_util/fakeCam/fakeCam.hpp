#pragma once
#include "data_type.h"
#include <thread>
#include <atomic>
#include <string>

namespace cam_util
{
    class FakeCam
    {
    public:
        struct Param
        {
            std::string source;
            size_t camCount = 0;
            size_t frameRate = 30;
        };
        struct CamParam
        {
            size_t width, height;
            size_t frameRate;
        };
    public:
        ~FakeCam()
        {
            stop();
        }
    public:
        bool setParam(const Param& param)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_isCamOn) return false;
            if(param.camCount == 0 || param.frameRate == 0 || param.source.empty()) return false;
            _param = param;
            return true;
        }
        bool setCallback(CameraSyncFrameCallback cb)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_isCamOn) return false;
            _callback = cb;
            return true;
        }
        bool start()
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if(_isCamOn) return false;
            if(!_callback)
            {
                std::cout << "set callback frist" << std::endl;
                return false;                
            }
            if(!_capture.open(_param.source))
            {
                std::cout << "failed to open source: " << _param.source << std::endl;
                return false;
            }

            _isCamOn = true;
            _syncThread = std::thread(&FakeCam::_onSync, this);
            return true;
        }
        bool stop()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!_isCamOn) return true;
            _isCamOn = false;
            if(_syncThread.joinable())
                _syncThread.join();
            _capture.release();
            return true;
        }
        bool getCamParam(CamParam& camParam)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!_capture.isOpened()) return false;

            camParam.width     = static_cast<size_t>(_capture.get(cv::CAP_PROP_FRAME_WIDTH));
            camParam.height    = static_cast<size_t>(_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
            camParam.frameRate = static_cast<size_t>(_capture.get(cv::CAP_PROP_FPS));
            return true;
        }
    private:
        void _onSync()
        {
            auto periodMs = std::chrono::duration<double>(1e-3*(int)(1e3 / _param.frameRate));
            std::vector<FrameData> frameList;
            cv::Mat frame;
            while(_isCamOn)
            {
                auto start_time = std::chrono::steady_clock::now();
                auto next_tick = start_time + periodMs;

                // 获取图像填充帧序号
                {
                    if(!_capture.read(frame))
                    {
                        _capture.set(cv::CAP_PROP_POS_FRAMES, 0);
                        continue;
                    }
                    
                    frameList.clear();
                    for(size_t idx=0; idx<_param.camCount; ++idx)
                    {
                        cv::Mat data = frame.clone();
                        _printText(data, idx);
                        FrameData frameData{idx, std::chrono::steady_clock::now(), data};
                        frameList.emplace_back(frameData);
                    }

                    _callback(frameList);
                }

                auto now = std::chrono::steady_clock::now();
                if(now < next_tick)
                    std::this_thread::sleep_until(next_tick);
            }
        }
        void _printText(cv::Mat& data, size_t id)
        {
            cv::putText(
                data,
                std::to_string(id),
                cv::Point(50, 100),
                cv::FONT_HERSHEY_SIMPLEX,
                1.5,
                cv::Scalar(0, 255, 0),
                3,
                cv::LINE_AA
            );
        }
    private:
        Param _param;
        std::atomic<bool> _isCamOn = false;
        CameraSyncFrameCallback _callback = nullptr;
        cv::VideoCapture _capture;
        std::thread _syncThread;
        std::mutex _mutex;
    };
}