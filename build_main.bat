@echo off
echo Building TwinCAT Smart Navigator...

:: Build main navigator
C:\msys64\mingw64\bin\gcc.exe -o twincat_navigator_main.exe twincat_navigator_main.c twincat_project_parser.c twincat_path_finder.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -lcomctl32

if %ERRORLEVEL% EQU 0 (
    echo ✅ Build completed successfully: twincat_navigator_main.exe
) else (
    echo ❌ Build failed!
    exit /b 1
)

:: Optional: Build legacy navigator
C:\msys64\mingw64\bin\gcc.exe -o navigator.exe navigator.c lib/twincat_navigator.c twincat_project_parser.c -luser32 -lgdi32 -lcomctl32 -lpsapi

if %ERRORLEVEL% EQU 0 (
    echo ✅ Legacy navigator built: navigator.exe
) else (
    echo ⚠️ Legacy navigator build failed (not critical)
)

pause