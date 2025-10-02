@echo off
echo === Hierarchy Extractor ===
echo.

findstr /R "^\[" listbox_structure_overview.txt > hierarchy.txt

echo Hierarchie extrahována do: hierarchy.txt
echo.
type hierarchy.txt