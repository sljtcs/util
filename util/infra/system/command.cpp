#include "command.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <sstream>
#include <iostream>
#include "util/infra/debug/log.hpp"

namespace infra_sys
{
    // Helper function to convert string to wide string
    std::wstring StringToWide(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size);
        return wstr;
    }

    // Helper function to convert wide string to string
    std::string WideToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size, NULL, NULL);
        return str;
    }

    // Helper function to read pipe output
    std::string ReadPipe(HANDLE pipe) {
        std::string result;
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        return result;
    }

    bool ExecuteCMDSync(const std::string& command) {
        std::wstring cmd = StringToWide(command);
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        // Create console window
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        LOG(command);
        if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            return false;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }

    bool ExecuteCMDSyncHidden(const std::string& command) {
        std::wstring cmd = StringToWide(command);
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        // Hide console window
        DWORD flags = CREATE_NO_WINDOW | DETACHED_PROCESS;

        if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi)) {
            return false;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }

    std::optional<CommandResult> ExecuteCMDWithOutput(const std::string& command) {
        CommandResult result = {false, "", "", 0};

        SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE g_hChildStd_OUT_Rd = NULL;
        HANDLE g_hChildStd_OUT_Wr = NULL;

        // Create pipe for child process's STDOUT
        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
            return std::nullopt;
        }

        SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        si.cb = sizeof(STARTUPINFOW);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdError = g_hChildStd_OUT_Wr;
        si.hStdOutput = g_hChildStd_OUT_Wr;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        std::wstring cmd = StringToWide("cmd.exe /C " + command);

        if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(g_hChildStd_OUT_Rd);
            CloseHandle(g_hChildStd_OUT_Wr);
            return std::nullopt;
        }

        // Close write end of pipe before reading
        CloseHandle(g_hChildStd_OUT_Wr);

        // Read output
        result.output = ReadPipe(g_hChildStd_OUT_Rd);
        CloseHandle(g_hChildStd_OUT_Rd);

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            result.success = (exitCode == 0);
            result.exit_code = exitCode;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return result;
    }

    std::optional<CommandResult> ExecuteCMDWithOutputVisible(const std::string& command) {
        CommandResult result = {false, "", "", 0};

        SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE g_hChildStd_OUT_Rd = NULL;
        HANDLE g_hChildStd_OUT_Wr = NULL;

        // Create pipe for child process's STDOUT
        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
            return std::nullopt;
        }

        SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        si.cb = sizeof(STARTUPINFOW);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdError = g_hChildStd_OUT_Wr;
        si.hStdOutput = g_hChildStd_OUT_Wr;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        std::wstring cmd = StringToWide("cmd.exe /C " + command);

        if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(g_hChildStd_OUT_Rd);
            CloseHandle(g_hChildStd_OUT_Wr);
            return std::nullopt;
        }

        // Close write end of pipe before reading
        CloseHandle(g_hChildStd_OUT_Wr);

        // Read output
        result.output = ReadPipe(g_hChildStd_OUT_Rd);
        CloseHandle(g_hChildStd_OUT_Rd);

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            result.success = (exitCode == 0);
            result.exit_code = exitCode;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return result;
    }

    std::optional<CommandResult> ExecuteCMDWithPipe(const std::string& command) {
        // For Windows, we need to handle pipes differently
        // We'll use cmd.exe to handle the pipe syntax
        std::string full_command = "cmd.exe /C " + command;
        return ExecuteCMDWithOutputVisible(full_command);
    }

    std::optional<CommandResult> ExecuteCMDWithPipeHidden(const std::string& command) {
        // For Windows, we need to handle pipes differently
        // We'll use cmd.exe to handle the pipe syntax
        std::string full_command = "cmd.exe /C " + command;
        return ExecuteCMDWithOutput(full_command);
    }

    bool IsCommandAvailable(const std::string& command) {
        std::string test_command = "where " + command;
        auto result = ExecuteCMDWithOutput(test_command);
        return result->success;
    }

    std::vector<std::string> GetCommandOutputLines(const std::string& command) {
        auto result = ExecuteCMDWithOutput(command);
        if (!result) {
            return {};
        }

        std::vector<std::string> lines;
        std::istringstream iss(result->output);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        return lines;
    }
}