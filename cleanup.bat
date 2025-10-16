@echo off
echo === CISTENI WORKSPACE ===
echo.

echo Mazani starych .exe souboru...
del /Q expand_and_find.exe 2>nul
del /Q full_memory_analyzer.exe 2>nul
del /Q hidden_item_navigator.exe 2>nul
del /Q memory_analyzer.exe 2>nul
del /Q navigator.exe 2>nul
del /Q quick_memory_dump.exe 2>nul
del /Q smart_navigator.exe 2>nul
del /Q standard_treeview_analyzer.exe 2>nul
del /Q structure_analyzer.exe 2>nul
del /Q target_navigator.exe 2>nul
del /Q twincat_navigator_main.exe 2>nul

echo Mazani starych .c souboru...
del /Q expand_and_find.c 2>nul
del /Q full_memory_analyzer.c 2>nul
del /Q hidden_item_navigator.c 2>nul
del /Q memory_analyzer.c 2>nul
del /Q memory_analyzer.h 2>nul
del /Q navigator.c 2>nul
del /Q quick_memory_dump.c 2>nul
del /Q smart_navigator.c 2>nul
del /Q standard_treeview_analyzer.c 2>nul
del /Q structure_analyzer.c 2>nul
del /Q target_navigator.c 2>nul
del /Q twincat_navigator_main.c 2>nul
del /Q twincat_path_finder.c 2>nul
del /Q twincat_path_finder.h 2>nul
del /Q twincat_project_parser.c 2>nul
del /Q twincat_project_parser.h 2>nul

echo Mazani starych build skriptu...
del /Q build_full_analyzer.bat 2>nul
del /Q build_memory_analyzer.bat 2>nul
del /Q build_quick_dump.bat 2>nul
del /Q build_smart.bat 2>nul
del /Q build_structure_analyzer.bat 2>nul
del /Q build_target_navigator.bat 2>nul
del /Q build_test_branch.bat 2>nul
del /Q build_test_find.bat 2>nul
del /Q build_test_simple.bat 2>nul
del /Q extract_hierarchy.bat 2>nul

echo Mazani starych vystupu (.hex, .txt, .xml)...
del /Q BA17xx.hex 2>nul
del /Q Palettierer.hex 2>nul
del /Q complete_memory_dump.hex 2>nul
del /Q quick_memory_dump.hex 2>nul
del /Q BA17xx.pro_listbox_state.txt 2>nul
del /Q complete_project_structure.txt 2>nul
del /Q CODESYS_DISCOVERY.txt 2>nul
del /Q full_expanded_structure.txt 2>nul
del /Q full_memory_dump.txt 2>nul
del /Q full_memory_dump2.txt 2>nul
del /Q hierarchy.txt 2>nul
del /Q listbox_structure_overview.txt 2>nul
del /Q output.txt 2>nul
del /Q project_explorer_structure.xml 2>nul

echo Mazani stare dokumentace...
del /Q README_MEMORY_ANALYZER.md 2>nul

echo.
echo === HOTOVO ===
echo Workspace vycisten!
pause
