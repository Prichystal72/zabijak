@echo off
echo === Building Direct Navigator ===

rem Clean old files
if exist direct_navigator.exe del direct_navigator.exe

rem Compile
gcc -o direct_navigator.exe direct_navigator.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist direct_navigator.exe (
    echo ✅ Build successful: direct_navigator.exe
    echo.
    echo 🚀 Ready to run: ./direct_navigator.exe
    echo 💡 Program pro přímé ovládání CoDeSys TreeView
) else (
    echo ❌ Build failed!
    pause
)