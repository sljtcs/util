#include <windows.h>
#include <string>

static bool runCommandHidden(const std::string& command)
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // CreateProcess 会修改 command buffer，必须可写
    std::string cmd = command;

    BOOL ok = CreateProcessA(
        nullptr,
        cmd.data(),          // 注意：必须是可写 buffer
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,    // 关键：不创建控制台窗口
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok)
        return false;

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}