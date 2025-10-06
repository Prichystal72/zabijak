@echo off
echo === CISTENI TESTS SLOZKY ===
echo.

cd tests

echo Mazani starych testu...
del /Q final_extractor.c 2>nul
del /Q final_extractor.exe 2>nul
del /Q navigator_clean_debug.exe 2>nul
del /Q reference_structure_parser.c 2>nul
del /Q test_branch_operations.c 2>nul
del /Q test_branch_operations.exe 2>nul
del /Q test_find_target.c 2>nul
del /Q test_find_target.exe 2>nul
del /Q test_path_finder.c 2>nul
del /Q test_path_finder.exe 2>nul

echo.
echo Ponechano: test_simple_toggle.c/.exe (uzitecny priklad)
echo.
echo === HOTOVO ===
echo Tests vycisteny!
cd ..
pause
