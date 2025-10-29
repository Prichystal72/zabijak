@echo off
echo ===============================================
echo  Building TwinCAT Navigator Service
echo ===============================================
echo.

gcc -o twincat_service.exe twincat_service.c -luser32 -ladvapi32 -Wall

if %errorLevel% equ 0 (
    echo.
    echo [OK] Kompilace uspesna!
    echo.
    echo Dalsi kroky:
    echo   1. service_debug.bat    - Test v debug rezimu
    echo   2. service_install.bat  - Instalace jako sluzba (AS ADMIN)
    echo   3. service_uninstall.bat - Odinstalace sluzby (AS ADMIN)
) else (
    echo.
    echo [ERROR] Kompilace selhala!
)

echo.
pause
