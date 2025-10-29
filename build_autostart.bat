@echo off
echo ===============================================
echo  Building TwinCAT Navigator Autostart
echo ===============================================
echo.

gcc -o twincat_navigator_autostart.exe twincat_navigator_autostart.c -luser32 -Wall

if %errorLevel% equ 0 (
    echo.
    echo [OK] Kompilace uspesna!
    echo.
    echo Dalsi kroky:
    echo   1. autostart_install.bat  - Instalace autostartu (AS ADMIN)
    echo   2. Nebo spust rucne: twincat_navigator_autostart.exe
    echo   3. autostart_uninstall.bat - Odstraneni autostartu
) else (
    echo.
    echo [ERROR] Kompilace selhala!
)

echo.
pause
