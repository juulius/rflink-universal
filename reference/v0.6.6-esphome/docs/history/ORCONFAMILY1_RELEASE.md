# ORCONFAMILY1 — release-instructies (26 oktober 2026)

Deze release bevat 26 ingebouwde apparaatprofielen, waarvan 16 Orcon-15RF-familieprofielen. De 13 protocoldecoders en de fysieke TX-beveiliging zijn ongewijzigd.

## Voor bestaande gebruikers (bijv. 6 MHz SPI)

1. Gebruik de PATCH ZIP over jouw huidige **PROFILEFLOW1_COMPILEFIX1** broncode, behoud `rflink-universal.yaml` en jouw SPI-kloksnelheid van `6000000`.
2. Maak een backup van je huidige map en maak eerst een nieuwe release met `MAAK_RELEASE.bat`.
3. Compileer in ESPHome; controleer eerst de build, dan pas OTA flashen.
4. Selecteer bij Apparaten het exacte Orcon-profiel; onder 'Stap voor stap instellen' staat begeleidende uitleg. De bestaande MVS-15R-profile ID blijft behouden.
5. Nog geen echte Orcon-pairing of verzendfunctie: RX-only voor Orcon blijft de veilige standaard.

## Nieuwe installaties

Gebruik de FULL_SOURCE ZIP met bijbehorende YAML en `MAAK_RELEASE.bat` of de DEFAULT ESPHome-bundle. **De meegeleverde standaard-YAML bevat nog 200000 Hz**; zet opnieuw 6000000 als je dat wil behouden.

## Teststatus

Geslaagd: generator/cataloog, alle 4 locale-bundles, host syntax-check voor rflink_universal.cpp, profileselectie/veiligheidsregels, 100 decoder-manifest checks, UI JavaScript syntax en bestaande UI-handler fixture. Geen volledige ESPHome/ESP-IDF build, geen 868MHz TX op hardware en geen echte 15RF pairingtest uitgevoerd. Sommige oude host-tests vereisen ESP-IDF driver/uart.h, die niet beschikbaar is in de host-stubomgeving.

## Verwijzingen

Zie `rflink_universal/ORCONFAMILY1.md` voor modellen, beperkingen en officiële bronnen.
