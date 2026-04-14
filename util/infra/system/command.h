#pragma once
#include <string>
#include <vector>
#include <optional>

namespace infra_sys
{
    // 同步执行指令等待直到返回
    // 不隐藏弹出的终端窗口
    bool ExecuteCMDSync(const std::string& command);
    // 隐藏弹出的终端窗口
    bool ExecuteCMDSyncHidden(const std::string& command);

    // Enhanced command execution with output capture
    struct CommandResult {
        bool success;
        std::string output;
        std::string error;
        int exit_code;
    };

    // Execute command and capture output (hidden console)
    std::optional<CommandResult> ExecuteCMDWithOutput(const std::string& command);
    // Execute command with visible console
    std::optional<CommandResult> ExecuteCMDWithOutputVisible(const std::string& command);

    // Execute command with pipe support (like ffmpeg example)
    std::optional<CommandResult> ExecuteCMDWithPipe(const std::string& command);
    // Execute command with pipe support (hidden console)
    std::optional<CommandResult> ExecuteCMDWithPipeHidden(const std::string& command);

    // Utility function to check if command exists
    bool IsCommandAvailable(const std::string& command);
    // Get command output as vector of lines
    std::vector<std::string> GetCommandOutputLines(const std::string& command);
}