# RFLink Universal PAIRING1: uitgevoerde controles

Status: gedeeltelijk geïmplementeerd. Niet vrijgeven als bewezen Orcon TX-release.

## Geslaagd
- 26 ingebouwde profielen; expliciete pairing-methoden en veilige manifestgeneratie.
- Decoderpacks (9 packs, 13 protocolidentiteiten) en radio-profielcatalogus.
- C++-syntaxcontrole rflink_universal.cpp met host ESPHome-stubs.
- ESPHome-achtige gegenereerde headers en includes.
- UserLibrary: v2 metadata (controls, stappen, methode), oude NVS v1-snapshot migratie getest, v2 round-trip, afwijzen niet-bestaande knoppen.
- Chromium: 1300px + 390px, profiel- en decoderbrowser, Orcon-stappen zichtbaar, geen TX-actie in Orcon-wizard, Itho-koppelflow beschikbaar.
- Bestaande radioengine: 100 decoders, 100 scheduler-entries, 10.000 hostcaptures.
- RAMSES-II-codec: acht commando's, 1FC9 offer/ACK/confirm, 10E0, checksums, timeouts en software round-trip.
- Profiel-UI, releasepakketinhoud en GPIO-pinout.

## Niet uitgevoerd / niet bewezen
- Volledige ESPHome/ESP-IDF toolchainbuild op echte target.
- Fysieke Orcon CC1101 TX/UART en 1FC9 handshake met MVS-15R.
- HA-bediening van Orcon: blijft uitgezet omdat TX niet gereed is.
- Multi-day soak, reboot- en power-loss tests op het echte board.
- Volledige oudere end-to-end testsuite (gericht getest, niet alles met builds doorlopen).

## Veiligheid
De profielwizard toont geen verzendsucces als toestelbevestiging. Een eigen profiel geeft geen ongecontroleerde toegang tot SPI, UART of CC1101-TX.
