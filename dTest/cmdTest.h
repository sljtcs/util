#pragma once
#include "util/infra/debug/log.hpp"
#include "util/infra/system/command.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

namespace test
{
    void test()
    {
        LOG("=== System Utilities Test ===");
        
        // // Test 1: Basic command execution (hidden)
        // LOG("Test 1: Basic command execution (hidden)");
        // bool result1 = infra_sys::ExecuteCMDSyncHidden("echo Hello, World!");
        // LOG_VAR(result1);
        
        // // Test 2: Command with output capture
        // LOG("Test 2: Command with output capture");
        // auto output1 = infra_sys::ExecuteCMDWithOutput("echo This is a test");
        // if (output1) {
        //     LOG("Success: ");
        //     LOG_VAR(output1->success);
        //     LOG("Output: ");
        //     LOG(output1->output.c_str());
        //     LOG("Exit Code: ");
        //     LOG_VAR(output1->exit_code);
        // } else {
        //     LOG_ERR("Failed to execute command");
        // }
        
        // // Test 3: Check if command exists
        // LOG("Test 3: Check if command exists");
        // bool has_echo = infra_sys::IsCommandAvailable("echo");
        // bool has_ffmpeg = infra_sys::IsCommandAvailable("ffmpeg");
        // LOG_VAR(has_echo);
        // LOG_VAR(has_ffmpeg);
        
        // // Test 4: Get command output as lines
        // LOG("Test 4: Get command output as lines");
        // auto lines = infra_sys::GetCommandOutputLines("dir /b");
        // LOG("Number of lines: ");
        // LOG_VAR(lines.size());
        // for (size_t i = 0; i < std::min<size_t>(lines.size(), 5); ++i) {
        //     LOG_VAR(lines[i]);
        // }
        
        // Test 5: Pipe command (like ffmpeg example)
        LOG("Test 5: Pipe command execution");
        for(size_t idx=0 ; idx<10; ++idx){
            std::string ffmpeg_cmd = "ffmpeg.exe -i ./temp/test.h264 -c:v copy ./temp/test.mp4 -y";
            auto pipe_result = infra_sys::ExecuteCMDSync(ffmpeg_cmd);
            LOG("over");
            if(!pipe_result)
                LOG("execute failed");
        }

        // // Test 6: Error handling
        // LOG("Test 6: Error handling");
        // auto error_result = infra_sys::ExecuteCMDWithOutput("invalid_command_xyz");
        // if (error_result) {
        //     LOG("Error command success: ");
        //     LOG_VAR(error_result->success);
        //     LOG("Error command output: ");
        //     LOG(error_result->output.c_str());
        //     LOG("Error command exit code: ");
        //     LOG_VAR(error_result->exit_code);
        // } else {
        //     LOG_ERR("Error command execution failed");
        // }
        
        LOG("=== System Utilities Test Complete ===");
    }
}