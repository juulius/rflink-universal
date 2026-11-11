# RFLink Universal — v0.5.3 Radiolove

## Waarom deze patch?

In v0.5.2 stopte `UPDATE_RFLINK.bat` meteen met:

`Variable reference is not valid. ':' was not followed by a valid variable name character.`

Het probleem zat in drie interpolaties in `rflink_universal/tools/update_rflink.ps1` (oorspronkelijke regels 381, 387 en 391): `$label:` in PowerShell-strings.

## Herstel

- De drie voorkomens zijn gewijzigd in `${label}:`, zonder de besluit- of conflictlogica te wijzigen.
- De bestaande Windows-regressies bevatten nu een echte PowerShell AST-parsercontrole (`[System.Management.Automation.Language.Parser]::ParseFile`) voor updater, releasegenerator en testscript.
- De ontwikkeltests controleren bovendien het specifieke `$variabele:`-probleem onafhankelijk van Windows.
- Firmwareversie v0.5.3 Radiolove, bijbehorende bronhash in de firmware-identiteit, en patchnotes voor de webinterface bijgewerkt.

## Opgeslagen crashes in Logboeken

- De logviewer haalt `/rflink/api/status` nu zelf op, zodat NVS-bootgeschiedenis direct zichtbaar is onder Systeem / Crashes en herstarts.
- Maximaal één statusaanvraag per 30 seconden in de relevante logtab; ontbrekende data of HTTP-fouten blijven herkenbaar.
- De NVS-geschiedenis bewaart maximaal acht bootrecords met reden en bootnummer, zonder historische tijd of backtrace. De afzonderlijke ESPHome crashdump komt niet automatisch in de logviewer. Overschreven oude records kunnen niet achteraf worden teruggehaald.
- Controleer bij een lege log eventueel `boot_history` en `boot_history_persisted` onder `/rflink/api/status`.

## Veiligheid tijdens bijwerken

`UPDATE_RFLINK` mag een onzekere PCB-variant of een onbekende firmwarewijziging **niet automatisch goedkeuren**. Je originele `rflink-universal.yaml` moet ongewijzigd blijven. Controleer je 6 MHz SPI, GPIO-pinout en experimentele TX-labopties in de gegenereerde nieuwe YAML voordat je flasht.

## Uitvoeren

1. Pak deze volledige v0.5.3 SOURCE-ZIP uit in een nieuwe korte map, bijv. `C:\RFLink_v053`.
2. Voer `UPDATE_RFLINK.bat` uit (Normal). Selecteer je bestaande persoonlijke YAML.
3. Bij een conflict: controleer de melding. Stop liever dan onbekende instellingen te overschrijven.
4. Controleer de nieuw gegenereerde YAML en maak dan pas de bijbehorende CUSTOM-release.

## Teststatus

Broncode-/regexcontrole, nieuwe embedded UI, bronhashcontrole en gerichte bestaande browsertests zijn uitgevoerd. De echte Windows PowerShell 5.1-updater kan in deze Linux-testomgeving niet worden gestart. Deze release is daarom nog niet bevestigd met een complete Windows-integratietest of ESPHome-hardwareflash.
