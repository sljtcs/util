#include <opencv2/opencv.hpp>
#include <vector>

namespace cam_util
{
    template<uint8_t N>
    class FrameComposer_C
    {
    public:
        FrameComposer_C() = default;
        void compose(const std::vector<cv::Mat> frames, cv::Mat& out)
        {
            if(frames.empty()) return;

            cv::Mat refData = frames[0];
            int cellWidth  = refData.cols;
            int cellHeight = refData.rows;

            if(_canvas.empty())
            {
                _canvas = cv::Mat::zeros(cellHeight * N, cellWidth * N, CV_8UC3);
            }

            size_t frameNum = N*N < frames.size()? N*N : frames.size();
            for(size_t idx=0; idx<frames.size(); ++idx)
            {
                size_t row_idx = idx / N;
                size_t col_idx = idx % N;
                frames[idx].copyTo(_canvas(cv::Rect(col_idx * cellWidth, row_idx * cellHeight, cellWidth, cellHeight)));
            }

            out = _canvas.clone();
        }

        void parse(const cv::Mat& in, std::vector<cv::Mat>& out)
        {
            out.clear();
            int cellWidth  = in.cols / N;
            int cellHeight = in.rows / N;

            for(size_t row_idx=0; row_idx<N; ++row_idx)
                for(size_t col_idx=0; col_idx<N; ++col_idx)
                {
                    cv::Mat cell = in(cv::Rect(col_idx * cellWidth, row_idx * cellHeight, cellWidth, cellHeight)).clone();
                    out.push_back(cell);
                }
        }
    private:
        cv::Mat _canvas;
    };
}


namespace cam_util
{
    template<uint8_t N>
    class FrameComposer_G
    {
    public:
        static void compose(const std::vector<cv::cuda::GpuMat>& frames, std::vector<cv::cuda::GpuMat>& outs)
        {
            if(frames.empty()) return;
            size_t numFrames = frames.size();
            size_t numOut = (numFrames + N*N - 1) / (N*N);
            outs.clear(); outs.reserve(numOut);

            size_t cellWidth  = frames[0].cols;
            size_t cellHeight = frames[0].rows;

            for(size_t idx=0; idx<numFrames; idx+=(N*N))
            {
                cv::cuda::GpuMat out = cv::cuda::GpuMat(cellHeight * N, cellWidth * N, frames[0].type());
                for(size_t sub_idx=0; sub_idx<(N*N); ++sub_idx)
                {
                    size_t row_idx = sub_idx / N;
                    size_t col_idx = sub_idx % N;
                    cv::cuda::GpuMat roi = out(cv::Rect(col_idx * cellWidth, row_idx * cellHeight, cellWidth, cellHeight));
                    frames[idx+sub_idx].copyTo(roi);
                }
                outs.push_back(out);
            }
        }

        static void compose(const std::vector<cv::cuda::GpuMat>& frames, cv::cuda::GpuMat& out)
        {
            if(frames.empty()) return;
            int cellWidth  = frames[0].cols;
            int cellHeight = frames[0].rows;

            if(out.empty() || out.cols != cellWidth || out.rows != cellHeight)
            {
                out = cv::cuda::GpuMat(cellHeight * N, cellWidth * N, frames[0].type());
            }
            out.setTo(cv::Scalar::all(0));

            size_t frameNum = std::min<size_t>(N*N, frames.size());
            for(size_t idx=0; idx<frameNum; ++idx)
            {
                size_t row_idx = idx / N;
                size_t col_idx = idx % N;

                cv::cuda::GpuMat roi = out(cv::Rect(col_idx * cellWidth, row_idx * cellHeight, cellWidth, cellHeight));
                frames[idx].copyTo(roi);
            }
        }

        static void parse(const cv::Mat& in, std::vector<cv::Mat>& out)
        {
            out.clear();
            int cellWidth  = in.cols / N;
            int cellHeight = in.rows / N;

            for(size_t row_idx=0; row_idx<N; ++row_idx)
                for(size_t col_idx=0; col_idx<N; ++col_idx)
                {
                    cv::Mat cell = in(cv::Rect(col_idx * cellWidth, row_idx * cellHeight, cellWidth, cellHeight)).clone();
                    out.push_back(cell);
                }
        }
    };
}