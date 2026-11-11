# RFLink Universal ORCONTXLAB2 – uitgevoerde tests, 28 oktober 2026

## Geslaagd in deze ontwikkelomgeving

- Native RAMSES-II-encoder: alle acht Orcon-commando's, UART-serialisatie, radio 868.30 MHz, verkeerde/ongepaarde virtuele zender geweigerd (C++ host).
- Koppelcodec: 1FC9-aanbod, 1FC9 ACK-check, 1FC9-bevestiging, 10E0-beschrijving, CRC/checksum en foutpaden (C++ host).
- Wizard: expliciete opt-in, wachten op ACK, daarna afzonderlijke Auto-test en gebruikersbevestiging (Chromium-simulatie op 1440 en 390 pixels).
- Profiel-/manifestschema: 9 decoderpacks, 13 protocollen, 26 profielen waarvan 16 Orcon, geen activering voor een onjuist profiel (Python).
- TX-engine register-regressie: Nexa, ByronSX, Dooya, RCSwitch, Somfy, Itho en RAMSES-II aanwezig; geen automatische RAMSES-TX vóór pairing (C++ host).
- Oudere UX3-, profiel-, logboek- en statusbrowserregressies: geslaagd.
- `test_profileflow1_compilefix1.py`: volledige `rflink_universal.cpp` syntaxcheck onder host-ESPHome-stubs geslaagd, niet de echte ESP-IDF-build.
- De grote `run_tests.sh`-reeks bereikte zonder fout o.a. monitor, opslag, RF/codec, MQTT-HA, TX1 en TX2. Er bleek vóór deze aanpassing een verwijzing naar een ontbrekend `test_tx_remaining.cpp` in het script te zitten; dit is vervangen door een echte regressietest, die afzonderlijk slaagt. De volledige suite is niet vanaf begin tot eind opnieuw voltooid.

## Nog niet getest of bewezen

- Volledige `esphome` validate/compile op de echte ESP32-S3 (toolchain niet beschikbaar).
- GPIO/GDO0-oscilloscoopmeting, over-the-air modulatie en fysiek RF-vermogen.
- 1FC9-ACK en succesvolle Orcon MVS-15R-pairing op de daadwerkelijke ventilatiebox.
- Langdurige stabiliteit (soak), NVS-powerloss/-reboot tijdens actieve koppelprocedure, en exacte RAMSES timing op echte hardware.
- Home Assistant-bediening op de echte MQTT-broker na het koppelen.

## Stopcriteria

- Geen fysiek ACK binnen 180 seconden, RX herstel loopt vast, radio fault, onverwachte ventilatorreactie, of herhaalde mislukte UART-preparation: stop en verzamel Logs → Alles export, relevante ESPHome-logs, exacte firmware-revisie. Zet desgewenst de labvlaggen terug op `false`.
- Logregel `UART completion` betekent **niet** `RF-uitzending gemeten` of `ventilatiebox accepteerde opdracht`.

De installatie-instructie en YAML-vlaggen staan in `ORCONTXLAB2_RELEASE_NL.md`.
