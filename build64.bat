@echo off
C:\msys64\mingw64\bin\gcc.exe -o navigator.exe navigator.c lib/twincat_navigator.c twincat_project_parser.c -luser32 -lgdi32 -lcomctl32 -lpsapi
echo Build completed