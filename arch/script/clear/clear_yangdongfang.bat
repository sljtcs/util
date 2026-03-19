@echo off
REM ========================
REM 清理 Release 目录脚本
REM ========================

setlocal enabledelayedexpansion

REM 要删除的文件夹列表（用空格分隔）
set "folders=Download Settings logs AppData UpdateData"

REM 要删除的文件列表（用空格分隔）
set "files=debug.log ydmocap_log.txt imgui.ini dll_test_exe.exe main0.exe main_exe.exe tool_icons2h.exe"

REM 基础路径
set "base=.\yangdongfang"

REM 删除文件夹
for %%F in (%folders%) do (
    if exist "%base%\%%F" (
        echo unlink folder: %base%\%%F
        rd /s /q "%base%\%%F"
    )
)

REM 删除文件
for %%F in (%files%) do (
    if exist "%base%\%%F" (
        echo unlink file: %base%\%%F
        del /f /q "%base%\%%F"
    )
)

echo.
echo clear over
endlocal
pause