@echo off
echo === Building Structure Analyzer ===

rem Clean old files
if exist structure_analyzer.exe del structure_analyzer.exe
if exist listbox_structure_overview.txt del listbox_structure_overview.txt

rem Compile
gcc -o structure_analyzer.exe structure_analyzer.c lib/twincat_navigator.c -luser32 -lpsapi -ladvapi32 -Wall -O2

if exist structure_analyzer.exe (
    echo ✅ Build successful: structure_analyzer.exe
    echo.
    echo 🚀 Ready to run: ./structure_analyzer.exe
    echo 💡 Vytvoří: listbox_structure_overview.txt
) else (
    echo ❌ Build failed!
    pause
)