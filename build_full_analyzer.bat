@echo off
echo === Building Full Memory Analyzer ===

rem Clean old files
if exist full_memory_analyzer.exe del full_memory_analyzer.exe
if exist complete_memory_dump.hex del complete_memory_dump.hex

rem Compile with proper linking
gcc -o full_memory_analyzer.exe full_memory_analyzer.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist full_memory_analyzer.exe (
    echo ✅ Build successful: full_memory_analyzer.exe
    echo.
    echo 🚀 Ready to run: ./full_memory_analyzer.exe
    echo 💡 Program vytvoří úplný hex dump do: complete_memory_dump.hex
) else (
    echo ❌ Build failed!
    pause
)