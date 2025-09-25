@echo off
echo === CLEANUP TESTOVACICH SOUBORU ===
echo.

echo Mazani testovacich .c souboru...
del /Q advanced_*.c 2>nul
del /Q analyze_*.c 2>nul  
del /Q complete_*.c 2>nul
del /Q comprehensive_*.c 2>nul
del /Q debug_*.c 2>nul
del /Q detailed_*.c 2>nul
del /Q detect_*.c 2>nul
del /Q disassembler.c 2>nul
del /Q docked_*.c 2>nul
del /Q doubleclick_*.c 2>nul
del /Q expand_*.c 2>nul
del /Q final_*.c 2>nul
del /Q find_*.c 2>nul
del /Q full_*.c 2>nul
del /Q hello.c 2>nul
del /Q improved_*.c 2>nul
del /Q memory_*.c 2>nul
del /Q mini.c 2>nul
del /Q minimal_*.c 2>nul
del /Q project_*.c 2>nul
del /Q quick_*.c 2>nul
del /Q simple_*.c 2>nul
del /Q smart_*.c 2>nul
del /Q syntax_*.c 2>nul
del /Q targeted_*.c 2>nul
del /Q test*.c 2>nul
del /Q time_*.c 2>nul
del /Q tree*.c 2>nul
del /Q ui_*.c 2>nul
del /Q ultra_*.c 2>nul
del /Q window_*.c 2>nul
del /Q closer.c 2>nul

echo Mazani nepotrebnych .exe souboru...
del /Q analyze_*.exe 2>nul
del /Q complete_*.exe 2>nul
del /Q debug_*.exe 2>nul
del /Q detailed_*.exe 2>nul
del /Q detect_*.exe 2>nul
del /Q disassembler.exe 2>nul
del /Q final_*.exe 2>nul
del /Q find_*.exe 2>nul
del /Q memory_*.exe 2>nul
del /Q zabijak32.exe 2>nul
del /Q zabijak64.exe 2>nul

echo Mazani textovych a ostatnich souboru...
del /Q *.txt 2>nul
del /Q *.o 2>nul

echo.
echo === CLEANUP DOKONCEN ===
echo.
echo PONECHANE SOUBORY:
dir /B *.c *.exe *.bat Makefile 2>nul
echo.
echo lib\ adresar:
dir /B lib\ 2>nul

pause