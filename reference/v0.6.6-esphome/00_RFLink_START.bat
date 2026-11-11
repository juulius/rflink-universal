@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0" || goto failed
if /I "%~1"=="release" goto release_cli
if /I "%~1"=="update" goto update_cli
if /I "%~1"=="test" goto test
if /I "%~1"=="help" goto help
if not "%~1"=="" goto usage
:menu
cls
echo =======================================================
echo   RFLink Universal - 1 startpunt
 echo =======================================================
echo.
echo   1. Mijn bestaande RFLink veilig bijwerken [aanbevolen]
echo   2. Schone standaardrelease maken [geen eigen YAML]
echo   3. Release en updater testen [ontwikkelaar]
echo   4. Handleiding openen
 echo   0. Afsluiten
 echo.
set "KEUZE="
set /p "KEUZE=Kies 0, 1, 2, 3 of 4: "
if "%KEUZE%"=="1" goto update_interactive
if "%KEUZE%"=="2" goto release
if "%KEUZE%"=="3" goto test_confirm
if "%KEUZE%"=="4" goto help
if "%KEUZE%"=="0" exit /b 0
goto menu
:update_interactive
echo.
echo UPDATE: uw originele YAML wordt nooit overschreven.
echo Er wordt een afzonderlijk CUSTOM-bundle gebouwd.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\update_rflink.ps1" -Mode Normal
goto result
:update_cli
shift
set "MODE=Normal"
if /I "%~1"=="automatic" (set "MODE=Automatic"&shift)
if /I "%~1"=="advanced" (set "MODE=Advanced"&shift)
if /I "%~1"=="normal" (set "MODE=Normal"&shift)
if "%~1"=="" goto update_noargs
if "%~2"=="" goto update_one
if "%~3"=="" goto update_two
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\update_rflink.ps1" -Mode %MODE% -ExistingYaml "%~1" -NewRelease "%~2" -BaseYaml "%~3"
goto result
:update_two
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\update_rflink.ps1" -Mode %MODE% -ExistingYaml "%~1" -NewRelease "%~2"
goto result
:update_one
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\update_rflink.ps1" -Mode %MODE% -ExistingYaml "%~1"
goto result
:update_noargs
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\update_rflink.ps1" -Mode %MODE%
goto result
:release_cli
if not "%~2"=="" goto usage
:release
echo.
echo DEFAULT: bouwt uitsluitend de meegeleverde standaard-YAML.
echo Dit is NIET uw persoonlijke 6 MHz / experimentele TX-configuratie.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\maak_release.ps1"
goto result
:test_confirm
if defined RFLINK_TEST_NO_PAUSE goto test
echo.
echo Tests maken tijdelijke kopieen en draaien verschillende controles.
set "BEVESTIG="
set /p "BEVESTIG=Start ontwikkeltests? [j/N]: "
if /I not "%BEVESTIG%"=="j" goto menu
:test
echo.
echo Developer tests - geen firmware wordt geflasht.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0rflink_universal\tools\test_update_release.ps1"
goto result
:help
if not exist "%~dp0docs\README.md" goto failed
notepad.exe "%~dp0docs\README.md"
if not defined RFLINK_TEST_NO_PAUSE pause
exit /b 0
:usage
echo Gebruik: 00_RFLink_START.bat [release ^| update [normal^|automatic^|advanced] [bestaande.yaml] [nieuwe_release] [basis.yaml] ^| test ^| help]
exit /b 1
:result
set "RESULT=%errorlevel%"
echo.
if "%RESULT%"=="0" (echo Actie geslaagd. Controleer de gemaakte bestanden in dist of update_output.) else (echo ACTIE MISLUKT. Lees de foutmelding en gebruik geen oudere bestanden uit dist.)
if not defined RFLINK_TEST_NO_PAUSE pause
exit /b %RESULT%
:failed
echo Ontbrekend script of documentatie. Gebruik de volledige SOURCE-ZIP.
if not defined RFLINK_TEST_NO_PAUSE pause
exit /b 1
