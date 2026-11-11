# RFLink Universal — v0.5.2 Radiolove

Datum: 3 november 2026  
Type: releasegenerator-correctie en patchrelease, geen verandering aan de fysieke radio- of TX-driver.

## Aanleiding

`MAAK_RELEASE.bat` stopte op:

`Firmware source changed since v0.5.1 was versioned (expected ab8207406f7b, actual 9fc21a3463f3)`

De volledige v0.5.1 SOURCE-ZIP is opnieuw gecontroleerd. De 130 firmwarebronbestanden leverden met ordinale bestandsnaamvolgorde juist wél `ab8207406f7b` op. `Sort-Object -CaseSensitive` in de Windows PowerShell 5.1-generator gebruikt een cultuurafhankelijke rangschikking van namen. Daardoor verschilde de input van SHA-256 op sommige systemen, zonder inhoudelijke codewijziging.

## Correctie

- In `rflink_universal/tools/maak_release.ps1` wordt de lijst bestandsnaam + SHA256 nu via `[Array]::Sort(..., [StringComparer]::Ordinal)` gesorteerd.
- De controle blijft **fail-closed**. Bronbestanden gewijzigd zonder nieuwe hash/versie? Dan stopt de release nog altijd.
- Release en UI naar `v0.5.2 Radiolove`; de SHA-256-bronhash onder technische details is de hash van de in deze ZIP opgenomen firmwarebroncode, exclusief `firmware_identity.h`.
- Alle Windows testverwachtingen en UI-versietests zijn bijgewerkt.
- Voortaan kan men met `python rflink_universal/tools/verify_release_fingerprint.py` onafhankelijk de hash nakijken; Python is NIET nodig om `MAAK_RELEASE.bat` te gebruiken.

## Installeren

1. Pak de **hele** v0.5.2 FULL SOURCE-ZIP uit in een nieuwe, korte map, bijvoorbeeld `C:\RFLink_v052`.
2. Gebruik `MAAK_RELEASE.bat` voor een standaardrelease en `UPDATE_RFLINK.bat` voor jouw persoonlijke YAML (6 MHz SPI en eigen PCB-pinout).
3. Gebruik de nieuw gemaakte bestanden uit de `dist`-map; gebruik geen oude bundle.
4. Controleer na installatie in Instellingen de naam `v0.5.2 Radiolove` en de SHA-256-bronhash.

## Teststatus

- Firmwarebronhash onafhankelijk opnieuw berekend en gecontroleerd.
- Negatieve test: wijzig één bronbestand -> afwijkende hash -> release moet weigeren.
- Bestaande browsertests en relevante pakketcontroles worden op de v0.5.2-broncode uitgevoerd.
- Windows PowerShell 5.1 en `MAAK_RELEASE.bat` zelf konden in deze Linux-testomgeving niet daadwerkelijk worden uitgevoerd. De ordinale API is compatibel met PowerShell 5.1, maar de eerste echte Windows-uitvoering is nog de eindcontrole.
- Geen volledige ESPHome-compile of fysieke RF-test uitgevoerd voor deze packagingpatch.
