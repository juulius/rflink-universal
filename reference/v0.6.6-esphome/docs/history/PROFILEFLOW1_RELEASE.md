# PROFILEFLOW1 — uniforme apparaatprofielen en beheer (2026-10-26)

## Wat is nieuw

1. Dezelfde apparaatstroom voor alle protocollen: een RF-signaal drie keer waarnemen, een voorstel voor een apparaatprofiel tonen, profiel kiezen/bevestigen, het apparaat opslaan.
2. Het gekozen profiel staat bovenaan de opgeslagen apparaatkaart. De knoppen voor dat profiel worden centraal weergegeven; de oude afzonderlijke Somfy/Itho-paneelrendering is vervangen om dubbele bediening te voorkomen.
3. Ingebouwde apparaatprofielen declareren `controls` afzonderlijk van `expected_actions`. Dit zijn functievoorstellen, geen bewijs van een veilige zendmogelijkheid.
4. Actieve verzendknoppen gebruiken expliciete per-actie `tx_ready_actions` die de C++-encoder en actuele radiotoestand bepalen. Onveilige of nog niet geïmplementeerde TX blijft uitgeschakeld. Orcon/RAMSES-II blijft RX-only; koppelen en fysieke TX zijn nog niet gevalideerd.
5. De profielkeuze kan via de standaard device-API worden opgeslagen (met protocolcontrole en NVS-rollback), zonder dat de User Library eerst geopend hoeft te worden.
6. Onopgeslagen RF-kandidaten krijgen een verwijderknop (tijdelijk; nieuwe ontvangst kan hetzelfde voorstel opnieuw tonen). Opgeslagen technische veldregels krijgen een verwijderknop onder Advanced. Bij HA/MQTT-offline of actieve TX gelden de bestaande veiligheidsbeperkingen.
7. Profile suggestion: alleen door protocol en ontvangen actie(s) onderbouwd, met handmatige modelcontrole; Orcon wordt niet stilzwijgend als zeker merk herkend.
8. Bestaande device-IDs, data in NVS, decoderpacks, 433/868 pinout en radioregisters blijven ongewijzigd.

## Tests

- `test_profileflow1.cpp`: PASS (profielknoppen, voorzichtig voorstel, verwijderen onopgeslagen signaal, geen impact op opgeslagen apparaat).
- C++ syntax-only: PASS (`rflink_universal.cpp`, `device_store.cpp`, `tx_engine.cpp`, Somfy/Itho TX encoders) met host ESPHome-stubs.
- `test_official_profiles.py`: PASS.
- `test_esphome_generated_include_paths.py`: PASS.
- `test_packaging.py`: PASS.
- `test_localization_browser.py`: PASS (vier talen).
- `test_qa1_accessibility.py`: PASS.
- Brede hosttests: alle stappen t/m Somfy HA: PASS; eerdere suite op dit systeem stopt bij legacy `-DUSE_ESP32` hosttest doordat het echte ESP-IDF-bestand `driver/uart.h` niet in de hoststubs staat. Geen echte ESP-IDF-build uitgevoerd.

## Veiligheid en resterende grenzen

- Niet verifiëren als 'volledig ESPHome-gecompileerd' vóór echte ESPHome-build.
- RX-inleren van één knop leert niet noodzakelijk de fysieke zenderidentiteit voor terugzenden; een deviceprofiel biedt wel mogelijke acties.
- Itho/Somfy vereisen pairing en een valide zendstaat; het ontbreken van een vooraf waargenomen andere functie blokkeert bij die twee concrete onafhankelijke protocollencoders niet meer automatisch alle profielacties.
- Voor user-defined decoders blijven TX-autorisatie en controles afzonderlijk vereist.
- Verwijderde technische velden kunnen bij herhaalde ontvangst weer als **nieuw voorstel** terugkomen. Voor permanent negeren van specifieke velden is latere NVS-uitbreiding nodig.
