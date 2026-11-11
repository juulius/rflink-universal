# v0.6.2 Radio Gaga — controles bij projectopruiming

## Geslaagd (in deze omgeving)

- Eén BAT-startbestand in hoofdmap, documentatie onder `docs/`.
- Bestandspaden in releasegenerator en updater aangepast; documentatie is vereiste SOURCE-ZIP-inhoud.
- Firmware-fingerprint consistent met ingesloten webinterface en locale-bundel.
- Chromium-versietest op 390 px, EN/NL/FR/DE.
- Decoderpackcontrole (9 packs, 13 protocollen), catalogus/C++-hosttest en pinout-/YAML-regressies.
- Gerichte statische PS-release/updater-contracttests, inclusief bronversie en fallbackbewaking.
- ZIP-structuur en volledigheid, na opnieuw openen (apart gecontroleerd).

## Openstaande beperkingen

- Windows PowerShell 5.1 en `cmd.exe` kunnen niet rechtstreeks in deze Linux-omgeving worden uitgevoerd; de daadwerkelijke BAT-menuactie, updater en PowerShell-verpakker moeten op Windows worden getest.
- ESPHome/ESP-IDF-compile en CC1101/RF-hardwaretest zijn niet uitgevoerd.
- De bredere host-suite breekt af op `test_radio_monitor_core.cpp:17` (`radio.activity().empty()`), in ongewijzigde runtimecode. Dit is **niet** als geslaagd aangemerkt.
- Er is geen recente v0.6.1-koppellog in deze beurt beschikbaar; de laatst zichtbare diagnose is van v0.6 boot #129.

## Wat is bewust niet aangepast?

- `rflink-universal.yaml` (de standaard-YAML is byte-identiek aan v0.6.1).
- De RF/TX-code, apparatuur-opslag, ESP32-pinout en gebruikersprofielen.

## Beveiliging

De root-BAT voert geen flash of OTA uit. Bijwerken gebruikt de bestaande drie-weg-merger met hardwarecontrole en schrijft naar een nieuwe outputlocatie. Het origineel blijft onaangeroerd.
