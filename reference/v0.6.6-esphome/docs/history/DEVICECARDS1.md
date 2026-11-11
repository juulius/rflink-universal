# DEVICECARDS1 — Apparaten: frequentiekaarten

Basis: RFLink Universal NAV4 + QA1 (2026-10-25).

## Wat is veranderd?

- De radiokeuze op **Apparaten** gebruikt nu dezelfde grotere selecteerbare kaarten als **Activiteit**, in plaats van een selectievak.
- Vier vaste keuzes: **Alle radio's**, **433.42 MHz**, **433.92 MHz**, **868.30 MHz**.
- **Onbekende radio** verschijnt als vijfde kaart wanneer zulke apparaten in de actuele zoek-/pairingselectie voorkomen, of wanneer deze keuze al actief is.
- Elke kaart toont het aantal passende apparaten. De tellingen houden rekening met de zoektekst en de optie voor koppelbare apparaten.
- Een klik past direct het radiofilter toe, zonder extra bevestiging. De actieve kaart blijft gemarkeerd (`aria-pressed`) na automatisch vernieuwen.
- Het bestaande zoekveld en **Wissen** blijven behouden. Wissen zet zowel zoekterm als radiofilter terug.
- Responsief: de kaarten gebruiken de beschikbare breedte; op een smal telefoonscherm staan twee kaarten naast elkaar. Geen horizontale scroll.
- Alle vier de talen (EN/NL/FR/DE) hebben de nieuwe apparaattelling.

## Bewust niet veranderd

- Geen wijzigingen aan CC1101, radioontvangst, decoders, logging, MQTT, Wi-Fi/OTA, NVS of hardware-YAML.
- De optie om koppelbare apparaten te tonen is behouden.
- Het apparaatbeheer, signalen, namen, HA-functies en bestaande actieknoppen blijven werken.

## Validatie

- `python tests/test_nav4_browser.py`: PASS op desktop (1680), laptop (1280) en mobiel (390). Bevat checks voor zoeken, aantallen, selectie, behoud tijdens auto-refresh, onbekende radio, Wissen, browserterug/vooruit en MQTT POST-body.
- `python tests/test_settings_compact1.py`: PASS.
- `python tests/test_rx_log_browser.py`: PASS.
- `python tests/test_simple_ui_browser.py`: PASS.
- `python tests/test_localization_browser.py`: PASS, vier talen.
- `python tests/test_nav3_browser.py`: PASS.
- `bash tests/run_web_smoke.sh`: PASS (C++ host-stubcompile inclusief web-UI).

Een volledige ESPHome/ESP-IDF-firmwarebuild en fysieke ESP32-S3/CC1101-hardwaretest zijn **nog niet** uitgevoerd voor DEVICECARDS1.

## Installeren

Pak deze ZIP uit over een **kopie** van je bestaande bronbestanden. Werk de lokale `rflink_universal/` bij, maar behoud je eigen werkende YAML en secrets. Compileer vervolgens via ESPHome en test de website voordat je een nieuwe stabiele release benoemt.
