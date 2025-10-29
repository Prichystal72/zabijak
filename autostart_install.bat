@echo off
echo ===============================================
echo  TwinCAT Navigator - AUTOSTART SETUP
echo ===============================================
echo.

REM Zjisti plnou cestu k exe
set "EXE_PATH=%~dp0twincat_navigator_autostart.exe"

echo [INFO] Cesta k programu: %EXE_PATH%
echo.

REM Vytvor Task Scheduler task pro autostart pri prihlaseni
echo [1/2] Vytvarim Task Scheduler ulohu...

schtasks /Create /TN "TwinCAT Navigator" /TR "\"%EXE_PATH%\"" /SC ONLOGON /RL HIGHEST /F

if %errorLevel% equ 0 (
    echo [OK] Uloha vytvorena uspesne!
    echo.
    echo [2/2] Chces spustit program hned? (A/N)
    choice /c AN /n
    if errorlevel 2 goto :skip_start
    if errorlevel 1 goto :start_now
    
    :start_now
    echo.
    echo Spoustim TwinCAT Navigator...
    start "" "%EXE_PATH%"
    echo.
    echo [OK] Program bezi na pozadi!
    echo Test: Stiskni Left Ctrl + Left Alt + Mezernik
    goto :end
    
    :skip_start
    echo.
    echo Program se spusti automaticky po prihlas
eni.
    echo Pro rucni spusteni pouzij: twincat_navigator_autostart.exe
) else (
    echo.
    echo [ERROR] Vytvoreni ulohy selhalo!
    echo         Spust jako Administrator.
)

:end
echo.
pause
