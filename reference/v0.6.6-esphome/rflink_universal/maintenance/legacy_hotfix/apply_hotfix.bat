@echo off
setlocal

rem Run from the folder containing this .bat file.
cd /d "%~dp0"
if errorlevel 1 (
    echo ERROR: Could not open the hotfix folder.
    pause
    exit /b 1
)

if not exist "apply_hotfix.py" (
    echo ERROR: apply_hotfix.py was not found in this folder.
    pause
    exit /b 1
)

rem Prefer the Windows Python launcher; fall back to python on PATH.
where py >nul 2>nul
if not errorlevel 1 (
    py -3 "apply_hotfix.py" %*
) else (
    where python >nul 2>nul
    if errorlevel 1 (
        echo ERROR: Python 3 was not found. Install Python or add it to PATH.
        pause
        exit /b 1
    )
    python "apply_hotfix.py" %*
)

set "result=%errorlevel%"
echo.
if "%result%"=="0" (
    echo Hotfix script finished successfully.
) else (
    echo Hotfix script failed with error code %result%.
)
pause
exit /b %result%
