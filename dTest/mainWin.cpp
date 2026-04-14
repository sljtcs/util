#include "windows.h"
#include "cmdTest.h"
#include "util/infra/debug/log.hpp"
#include "util/infra/debug/time.hpp"

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>      // 用于 printf
#include <iostream>     // 用于 std::cout（可选）

// 你的测试函数声明
namespace test {
    void test();
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ====================== 控制台输出部分 ======================
    // 确保控制台存在（Console 子系统下通常已自动创建）
    AllocConsole();                    // 如果需要强制分配控制台
    freopen("CONOUT$", "w", stdout);   // 重定向 stdout 到控制台
    freopen("CONOUT$", "w", stderr);   // 重定向 stderr
    setvbuf(stdout, NULL, _IONBF, 0);  // 关闭缓冲，立即看到输出

    std::cout << "程序启动成功！这是控制台输出。" << std::endl;
    printf("这里也可以用 printf 输出。\n");
    std::cout << "test::test() 将在下面执行...\n" << std::endl;

    // ====================== 图形窗口部分 ======================
    // 1. 注册窗口类
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MySimpleWindow";

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "窗口类注册失败！", "错误", MB_ICONERROR);
        return 0;
    }

    // 2. 创建窗口
    HWND hWnd = CreateWindowEx(
        0,
        "MySimpleWindow",
        "带控制台的简单窗口",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
        return 0;
    }

    // 3. 显示窗口
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 4. 调用你的测试函数（可以在控制台看到它的输出）
    test::test();

    std::cout << "test::test() 执行完毕。\n" << std::endl;

    // 5. 消息循环（让窗口保持运行）
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    std::cout << "程序即将退出。\n";
    FreeConsole();   // 可选：退出前释放控制台

    return (int)msg.wParam;
}