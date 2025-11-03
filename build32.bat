@echo off
echo Building TC2 Navigator (32-bit)...

:: Compile icon resource
echo Compiling icon...
C:\msys64\mingw32\bin\windres.exe app_icon.rc -O coff -o app_icon.res

:: Build with icon
C:\msys64\mingw32\bin\gcc.exe -o TC2_navigator32.exe TC2_navigator.c lib/twincat_navigator.c lib/twincat_tree.c lib/twincat_cache.c lib/twincat_search.c app_icon.res -luser32 -lpsapi -lcomctl32
if %ERRORLEVEL% EQU 0 (
    echo [OK] Build completed: TC2_navigator32.exe
) else (
    echo [ERROR] Build failed!
)