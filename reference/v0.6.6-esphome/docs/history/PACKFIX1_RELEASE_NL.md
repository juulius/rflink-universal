# RFLink Universal — PACKFIX1 (28 oktober 2026)

## Probleem
`MAAK_RELEASE.bat` stopte met `Radio monitor runtime missing; do not package partial UI/API versions.` bij de ORCONTXLAB2/LOGBOOK1-broncode. Dit was een verouderde releasecontrole, geen aantoonbaar ontbrekende zenddriver of radiomonitor.

## Oorzaak
`maak_release.ps1` eiste letterlijk `data-i18n="static.038"`. Sinds LOGBOOK1 gebruikt de logboekpagina `data-i18n="logbook.title"` en toont hij de weergaven Alles/Radio/Systeem/Web op de bestaande `data-view="monitor"` route. De oude tekstmarker is terecht verdwenen.

## Oplossing
- Verouderde check vervangen door een fail-closed controle op de huidige logboekpagina, de afzonderlijke logbuffers, de radiomonitor en beide systeemtrace-API-routes. Er is **niet** simpelweg een controle verwijderd.
- De bestaande native DEFAULT/CUSTOM-bestandsnamen blijven ongewijzigd om updater- en Windows-integratietests niet te breken. Verwijder een oude `dist` na een mislukte poging en gebruik uitsluitend de nieuwe `dist` na een geslaagde run.
- Deze PACKFIX1-uitleg is opgenomen in de volledige broncode-ZIP; de releasegeschiedenis staat tevens in `RELEASE_NOTES.md`.
- Nieuw `test_packfix1_release.py` bewaakt dat de releasecontrole en de actuele webinterface blijven overeenkomen.

## Gebruik op Windows
Pak de nieuwe `FULL_SOURCE.zip` volledig uit naar bijvoorbeeld `C:\RFLink`, inclusief de mappenstructuur. Voer in de hoofdmap `MAAK_RELEASE.bat` uit zonder argumenten. De resulterende SOURCE-zip en `.esphomebundle.tar.gz` verschijnen in `dist`. Gebruik geen oude bestanden uit `dist` als het script faalt. Laat je bestaande persoonlijke YAML met 6 MHz SPI en de juiste PCB-pinout intact bij het updaten via de updater.

## Testgrenzen
De aangepaste broncode en releaseguard zijn statisch op Linux getest. Een echte `powershell.exe` 5.1-uitvoering van `MAAK_RELEASE.bat`, ESPHome-compile, flash en RF-test moeten op de Windows/ESPHome-hardware worden geverifieerd. Het PACKFIX1-pakket verandert niets aan de fysieke radio- of Orcon-zendalgoritmen.
