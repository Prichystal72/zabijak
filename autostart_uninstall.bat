@echo off
echo ===============================================
echo  TwinCAT Navigator - AUTOSTART UNINSTALL
echo ===============================================
echo.

echo [1/2] Zastavuji program...
taskkill /F /IM twincat_navigator_autostart.exe >nul 2>&1

echo [2/2] Odstranuji Task Scheduler ulohu...
schtasks /Delete /TN "TwinCAT Navigator" /F

if %errorLevel% equ 0 (
    echo.
    echo [OK] Autostart odstranen uspesne!
) else (
    echo.
    echo [ERROR] Odstraneni selhalo!
)

echo.
pause
