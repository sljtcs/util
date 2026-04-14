#pragma once
#include <windows.h>
#include <string>
#include <iostream>

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

class TestWin
{
public:
    TestWin(const std::string& winPrompt_)
    {
        winPrompt = winPrompt_;
        createWin();
    }
    ~TestWin()
    {
        destroy();
    }
public:
    HWND getHandle()
    {
        return hWnd;
    }
    void doMsgWork()
    {
        MSG msg = {};
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    void destroy()
    {
        if(hWnd)
        {
            DestroyWindow(hWnd);
            hWnd = nullptr;
        }
    }
    void setPos(int x, int y, int w, int h)
    {
        SetWindowPos(hWnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_SHOWWINDOW);
    }
private:
    void createWin()
    {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = TestWin::WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = winPrompt.c_str();
        RegisterClass(&wc);

        hWnd = CreateWindow(
            wc.lpszClassName, winPrompt.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            640, 480,
            nullptr, nullptr,
            wc.hInstance, this
        );

        if(!hWnd)
        {
            std::cerr << "Failed to create window!" << std::endl;
        }

        ShowWindow(hWnd, SW_SHOW);
    }
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        TestWin* pThis = nullptr;

        if(msg == WM_NCCREATE)
        {
            // 获取 this 指针并保存
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<TestWin*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else
        {
            // 获取已保存的 this 指针
            pThis = reinterpret_cast<TestWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        if(pThis)
        {
            switch(msg)
            {
                case WM_CLOSE:
                    pThis->onClose();           // 调用成员函数
                    DestroyWindow(hWnd);       // 销毁窗口
                    return 0;

                case WM_DESTROY:
                    PostQuitMessage(0);        // 通知退出消息循环
                    return 0;
            }
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    void onClose()
    {
        std::cout << "testWin: close" << std::endl; 
    }
private:
    HWND hWnd;
    std::string winPrompt;
};