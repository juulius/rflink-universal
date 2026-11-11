# PROFILESAFE1 — gecontroleerde hersteltest (1 november 2026)

## Wat lost deze versie op?

De gedecodeerde crashlocatie is `RFLinkWebHandler::user_library_json_()` rond de oude regel 3498. Een groot JSON-antwoord met alle ingebouwde profielinformatie veroorzaakte een C++-geheugenallocatiefout (`Abort`). Deze versie vraagt alleen compacte lijsten op en laadt gedetailleerde profielen en opgenomen RF-samples afzonderlijk. Daarmee is de vastgestelde risicovolle coderegel uit het standaardpad gehaald.

## Updaten zonder gegevensverlies

1. Bewaar je huidige werkende persoonlijke YAML en ESPHome-secrets, en eventueel een NVS-/apparaatbackup.
2. Gebruik de volledige SOURCE-zip als nieuwe projectbron. Bij jouw bestaande ESP32 gebruik je de **custom updater `UPDATE_RFLINK.bat`** om je eigen 6 MHz SPI- en PCB-instellingen te behouden; `MAAK_RELEASE.bat` gebruikt de DEFAULT-YAML.
3. ESPHome: eerst Validate, dan Compile, dan pas Install/OTA. Er is geen vooraf gebouwde `.bin` in de SOURCE-zip.
4. Na opstarten: `/rflink/api/status` moet eindigen met `PROFILESAFE1`. Controleer de nieuwe waarden `internal_free_bytes` en `largest_internal_block_bytes`.
5. Open `/rflink/api/profile-catalog`. Er moeten 26 profielen zijn, waarvan 16 met protocol `RamsesII`.
6. Open Apparaten en kies voor `ventilator box` → `Apparaat instellen`; de profielkeuze moet 16 Orcon-opties aanbieden. `Verwijderen` vraagt eerst bevestiging. **Probeer verwijderen alleen als je dat echt wilt; annuleren is veilig.**
7. Controleer de bootdiagnose en seriële logs minimaal enkele minuten, inclusief meerdere keren naar Apparaten/Bibliotheek navigeren. Noteer bij nieuwe crash firmware-revisie, `.elf` en de volledige nieuwe backtrace.

## Veiligheid

De werkelijke Orcon-TX/koppeling blijft experimenteel; deze reparatie activeert geen RF-functies automatisch. Omdat op hardware herhaalde panic-resets zijn gezien, is de nieuwe versie **niet als stabiel bewezen** totdat compilatie en hardware-reboot/duurtest slagen. Geen apparaten of NVS wissen om dit probleem te herstellen.

## API-verandering

`/rflink/api/user-library` retourneert voor decoders `sample_count` en `samples: []`. Voor het bewerken/exporteren vraagt de vernieuwde UI afzonderlijk `GET /rflink/api/user-decoder-detail?id=<id>` en `?id=<id>&sample=0` (enzovoort) op. Externe clients die voorheen automatisch alle samples via één GET lazen, moeten deze nieuwe on-demand route gebruiken.
