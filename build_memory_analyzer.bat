@echo off
echo === Building Memory Analyzer ===

rem Clean old files
if exist memory_analyzer.exe del memory_analyzer.exe

rem Compile with proper linking
gcc -o memory_analyzer.exe memory_analyzer.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist memory_analyzer.exe (
    echo ✅ Build successful: memory_analyzer.exe
    echo.
    echo 🚀 Ready to run: ./memory_analyzer.exe
) else (
    echo ❌ Build failed!
    pause
)