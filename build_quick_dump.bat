@echo off
echo === Building Quick Memory Dump ===

rem Clean old files
if exist quick_memory_dump.exe del quick_memory_dump.exe

rem Compile with proper linking
gcc -o quick_memory_dump.exe quick_memory_dump.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist quick_memory_dump.exe (
    echo ✅ Build successful: quick_memory_dump.exe
    echo.
    echo 🚀 Ready to run: ./quick_memory_dump.exe
) else (
    echo ❌ Build failed!
    pause
)