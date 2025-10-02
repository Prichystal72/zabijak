@echo off
echo === Building Target Navigator ===

rem Clean old files
if exist target_navigator.exe del target_navigator.exe

rem Compile
gcc -o target_navigator.exe target_navigator.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist target_navigator.exe (
    echo ✅ Build successful: target_navigator.exe
    echo.
    echo 🚀 Ready to run: ./target_navigator.exe
    echo 🎯 Program automaticky vyhledá a přepne na ST_Markiere_WT_NIO
) else (
    echo ❌ Build failed!
    pause
)