@echo off
echo ========================================
echo Building TC2 Navigator...
echo ========================================

C:\msys64\mingw64\bin\gcc.exe -o TC2_navigator.exe TC2_navigator.c lib/twincat_navigator.c lib/twincat_tree.c lib/twincat_cache.c lib/twincat_search.c -luser32 -lpsapi -lcomctl32

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo [OK] Build completed: TC2_navigator.exe
    echo ========================================
) else (
    echo.
    echo ========================================
    echo [ERROR] Build failed!
    echo ========================================
    exit /b 1
)

pause
