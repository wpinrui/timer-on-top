@echo off
cl /nologo /O2 /W3 /EHsc /Fe:TimerOnTop.exe main.cpp /link /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup
if %errorlevel% == 0 (
    echo Build succeeded: TimerOnTop.exe
) else (
    echo Build failed.
)
