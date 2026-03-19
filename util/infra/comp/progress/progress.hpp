#pragma once
#include <vector>
#include "util/infra/debug/log.hpp"

namespace comp_util
{
    // ProgressAgent does NOT own the progress value.
    // Caller must ensure the referenced float outlives this handle.
    template<typename LabelT>
    class ProgressAgent
    {
    public:
        ProgressAgent() = delete;
        ProgressAgent(const ProgressAgent&) = delete;
        ProgressAgent& operator=(const ProgressAgent&) = delete;
        explicit ProgressAgent(float& data)
        : out(data){}
    private:
        float& out;

    public:
        struct Stage
        {
            LabelT label;
            uint32_t dest;
            uint32_t curr;
            float weight;

            Stage(LabelT l, uint32_t d, float w)
            : label(l), dest(d), curr(0), weight(w)
            {}
        };
    public:
        void reset(const std::vector<Stage>& stage)
        {
            _stage = normalize(stage);
            out = 0.0f;
        }
        void step(LabelT label)
        {
            for(auto& stage : _stage)
                if(stage.label == label){
                    ++stage.curr;
                    update();
                    return;
                }
        }
        void finish(LabelT label)
        {
            for(auto& stage : _stage)
                if(stage.label == label){
                    stage.curr = stage.dest;
                    update();
                    return;
                }
        }
    private:
        std::vector<Stage> _stage;
        void update()
        {
            float progress {0.0f};
            for(auto& stage : _stage)
                progress += stage.curr * stage.weight/stage.dest;
            out = progress;
        }
        std::vector<Stage> normalize(const std::vector<Stage>& in)
        {
            std::vector<Stage> out;

            for(const auto& s : in)
            {
                auto it = std::find_if(out.begin(), out.end(), [&](const Stage& t) { return t.label == s.label; });
                if(it != out.end())
                    continue;

                Stage stage = s;
                if(stage.dest == 0)
                {
                    stage.dest = 1;
                    stage.curr = 1;
                }
                if(stage.weight < 0)
                    stage.weight *= -1.0f;

                out.push_back(stage);
            }

            float sum {0.0f};
            for(const auto& stage : out)
                sum += stage.weight;

            if(sum > std::numeric_limits<float>::epsilon())
            {
                const float inv = 1.0f / sum;
                for(auto& s : out)
                    s.weight *= inv;
            }
            else
            {
                const float equal = 1.0f / out.size();
                for(auto& s : out)
                    s.weight = equal;
            }
            return out;
        }
    };
}