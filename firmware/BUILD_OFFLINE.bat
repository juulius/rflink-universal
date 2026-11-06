@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set IDF_COMPONENT_MANAGER=0
where idf.py >nul 2>nul
if errorlevel 1 (
  echo [ERROR] ESP-IDF shell ontbreekt. Start de lokale ESP-IDF 5.5 CMD / PowerShell.
  exit /b 1
)
where python >nul 2>nul
if errorlevel 1 (echo [ERROR] Python niet beschikbaar & exit /b 1)
python tools\generate_board_header.py
if errorlevel 1 exit /b 1
python tools\preflight.py
if errorlevel 1 exit /b 1
python tools\source_manifest.py
if errorlevel 1 exit /b 1
if exist sdkconfig (
  echo [INFO] Bestaande sdkconfig wordt gebruikt. Vergelijk met sdkconfig.defaults.
)
echo [INFO] Alleen lokale componenten; de build doet geen componentdownloads.
call idf.py build
if errorlevel 1 exit /b 1
python tools\package_release.py
if errorlevel 1 exit /b 1
echo [OK] Releasepakket in map release\
endlocal
