@echo off
setlocal EnableExtensions DisableDelayedExpansion
title RFLink Universal - Portable release maken
cd /d "%~dp0"
if errorlevel 1 goto :folder_error

echo.
echo ==============================================================
echo RFLink Universal v0.7.0 Radioactive - PORTABLE RELEASE BUILDER
echo ==============================================================
echo Geen Docker. Geen installatie. Geen downloads. Geen flashen.
echo.

REM The SDK is a separate fully provisioned, local, offline Windows bundle.
REM All environment changes are scoped to this cmd.exe process.
set "RF_PORTABLE_SDK=%~dp0portable_sdk"
set "IDF_PATH=%RF_PORTABLE_SDK%\esp-idf"
set "IDF_TOOLS_PATH=%RF_PORTABLE_SDK%\idf-tools"
set "IDF_COMPONENT_MANAGER=0"
set "PIP_NO_INDEX=1"
set "PYTHONNOUSERSITE=1"

if not exist "%IDF_PATH%\tools\idf.py" goto :missing_sdk
if not exist "%IDF_PATH%\export.bat" goto :missing_sdk
if not exist "%RF_PORTABLE_SDK%\python\python.exe" goto :missing_python
if not exist "%RF_PORTABLE_SDK%\git\cmd\git.exe" goto :missing_git

REM Force the portable Python and Git, never silently use system installations.
set "PATH=%RF_PORTABLE_SDK%\python;%RF_PORTABLE_SDK%\git\cmd;%PATH%"
"%RF_PORTABLE_SDK%\python\python.exe" --version
if errorlevel 1 goto :bad_tools
"%RF_PORTABLE_SDK%\git\cmd\git.exe" --version
if errorlevel 1 goto :bad_tools

echo [1/3] Portable ESP-IDF omgeving activeren...
call "%IDF_PATH%\export.bat"
if errorlevel 1 goto :bad_tools

REM Check both tool presence and major/minor framework identity.
where idf.py >nul 2>nul
if errorlevel 1 goto :bad_tools
where cmake >nul 2>nul
if errorlevel 1 goto :bad_tools
where ninja >nul 2>nul
if errorlevel 1 goto :bad_tools
where xtensa-esp32s3-elf-gcc >nul 2>nul
if errorlevel 1 goto :bad_tools
for /f "delims=" %%V in ('idf.py --version 2^>nul') do set "RF_IDF_VERSION=%%V"
if not defined RF_IDF_VERSION goto :bad_tools
echo %RF_IDF_VERSION% | findstr /C:"v5.5" /C:"5.5." >nul
if errorlevel 1 goto :wrong_version

REM Prevent any component-manager network fetches; requirements are prebundled.
echo [2/3] Firmware lokaal compileren...
call "%~dp0BUILD_OFFLINE.bat"
if errorlevel 1 goto :build_failed

echo [3/3] Releasebestanden controleren...
if not exist "%~dp0release\release_manifest.json" goto :build_failed

echo.
echo ==============================================================
echo [OK] Release gemaakt in release\
echo [LET OP] Ontwikkelversie: flash NIET over de werkende v0.6.6.
echo ==============================================================
goto :ok

:missing_sdk
echo.
echo [FOUT] Het volledige ESP-IDF portable pakket ontbreekt.
echo Verwacht: portable_sdk\esp-idf\tools\idf.py
echo Geen installatie of download gestart.
echo Lees: PORTABLE_BUILDER_LEESMIJ.md
goto :failed
:missing_python
echo [FOUT] Portable Python ontbreekt: portable_sdk\python\python.exe
goto :failed
:missing_git
echo [FOUT] Portable Git ontbreekt: portable_sdk\git\cmd\git.exe
goto :failed
:wrong_version
echo [FOUT] Onjuiste ESP-IDF versie: %RF_IDF_VERSION%
echo Vereist: 5.5.x, gebundeld in portable_sdk.
goto :failed
:bad_tools
echo.
echo [FOUT] De portable SDK is onvolledig of niet verplaatsbaar.
echo Controleer ESP-IDF, Python packages, CMake, Ninja en de ESP32-S3 compiler.
echo Raadpleeg PORTABLE_BUILDER_LEESMIJ.md. Er is niets geinstalleerd.
goto :failed
:build_failed
echo.
echo [FOUT] Compilatie of releasecontrole mislukt. Geen firmware geflasht.
goto :failed
:folder_error
echo [FOUT] Projectmap kan niet worden geopend.
goto :failed
:ok
echo.
pause
exit /b 0
:failed
echo.
echo [NIET GESLAAGD] Firmware NIET geflasht.
echo.
pause
exit /b 1
