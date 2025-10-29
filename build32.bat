@echo off
echo Building TC2 Navigator (32-bit)...
C:\msys64\mingw32\bin\gcc.exe -o TC2_navigator32.exe TC2_navigator.c lib/twincat_navigator.c lib/twincat_tree.c lib/twincat_cache.c lib/twincat_search.c -luser32 -lpsapi -lcomctl32
if %ERRORLEVEL% EQU 0 (
    echo [OK] Build completed: TC2_navigator32.exe
) else (
    echo [ERROR] Build failed!
)