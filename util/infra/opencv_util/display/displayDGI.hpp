#pragma once

#include <Windows.h>
#include <opencv2/opencv.hpp>

class ImgViewer
{
public:
    static void showImg_GDI(HWND hwnd, const cv::Mat& image)
    {
        if(image.empty() || image.channels() != 3 || image.depth() != CV_8U) return;

        RECT rect;
        GetClientRect(hwnd, &rect);
        int winW = rect.right - rect.left;
        int winH = rect.bottom - rect.top;

        HDC hdc = GetDC(hwnd);

        cv::Mat resized;
        cv::resize(image, resized, cv::Size(winW, winH), 0, 0, cv::INTER_LINEAR);

        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = resized.cols;
        bmi.bmiHeader.biHeight = -resized.rows;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        StretchDIBits(
            hdc,
            0, 0, resized.cols, resized.rows,
            0, 0, resized.cols, resized.rows,
            resized.data,
            &bmi,
            DIB_RGB_COLORS,
            SRCCOPY
        );

        ReleaseDC(hwnd, hdc);
    }
};

