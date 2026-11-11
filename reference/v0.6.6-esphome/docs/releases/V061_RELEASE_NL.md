# RFLink Universal v0.6.1 — Radio Gaga (UARTTRACE61 + DEVICEDETAIL61)

**Releasetype:** gerichte experimentele TX-correctie + de eerder goedgekeurde vereenvoudiging van de apparaatdetails op basis van v0.6.  
**Status:** software-/hosttests; fysieke RF-uitzending en een volledige ESPHome/ESP-IDF-build nog niet bewezen.  
**Bronhash:** `c1eacd6d85b9` (SHA-256-fingerprint van 131 runtimebronnen; geen Git-commit).

## Diagnose die deze patch motiveerde

In `rflink-diagnose-129.json` werden twee 1FC9-koppelopdrachten in de hoofdloop verwerkt. Bij beide meldde de CC1101-driver: `configure CC1101 modem / UART 8N1 FAILED`. De radio werd wel gereserveerd en de oorspronkelijke RX-configuratie werd na elke poging hersteld. Dat bewijst geen RF-uitzending: beide pogingen stopten vóór de eigenlijke TX-stap. Het stack-watermerk was bij beide pogingen 4134 bytes; een nieuwe panic is in dat tijdvenster niet waargenomen.

## Firmwarewijzigingen

1. **UART-volgorde:** `uart_driver_install(UART_NUM_1, ...)` wordt uitgevoerd vóór `uart_param_config(UART_NUM_1, ...)`. Dit volgt de volgorde uit ESPHome/ESP-IDF 5.5: installatie kan UART-registers terugzetten naar defaults.
2. **Per-stap foutdiagnostiek:** aparte stappen voor het radiolease, CC1101 SIDLE/SFTX, UART-driverinstallatie, 8N1/baud, GPIO16-uitgang en idle-high, TX-pinroute, modemregisters, register-readback, STX/MARCSTATE, verzonden UART-bytes, TX-drain, en RX-herstel/verificatie.
3. **Duidelijke afwijzing:** ESP-IDF-foutcodes via `esp_err_to_name` en hexwaarden; registerafwijkingen met exact adres, verwachte en gelezen waarde. Het eerste TX-probleem blijft behouden tenzij RX-herstel zelf ook faalt; bij mislukte RX-verificatie blijft de zendroute fail-closed.
4. **Radiolog én Systeemlog:** de daadwerkelijke hardwarestappen worden in beide logs geregistreerd. De Radiolog bevat geen sleutels, raw payloads of onjuiste claims over ontvangst door de ventilatiebox.
5. **Correcte status:** 'TX aan driver aangeboden' is geen bewezen uitzending. 'CC1101 entered TX', 'UART1 transmit enqueue' en 'UART1 TX drain' worden afzonderlijk zichtbaar.
6. **Universele architectuur behouden:** het protocolonafhankelijke `SerialTxAdapter`-contract en de gedeelde TX-router zijn niet omzeild; geen verandering aan SPI-snelheid, radio-pinout of decoderopslag.

## UI-wijziging

De eerder goedgekeurde pagina **Signalen en technische details** is afgevlakt: zichtbare secties voor Ontvangen functies, Beheer en Home Assistant (met directe knoppen), RF-signalen, beschikbare instelhulp en technische apparaat-ID. Geen extra overkoepelende accordion. De inhoud en bedienhandlers blijven behouden. Vier talen (EN/NL/FR/DE) bijgewerkt.

## Testen

- C++-hosttest met fake ESP-IDF/CC1101 controleert UART-initialisatievolgorde, normaal TX-pad, register-readback, foutcodepropagatie en RX-rollback na een fout.
- GDO-serial-sessiontests controleren fail-closed gedrag, herhalingen en permanent blokkeren na falende RX-herstelcontrole.
- Chromium-browsertests voor Status → Apparaten → instellingen op desktop 1440 px en mobiel 390 px, Orcon-wizard, Itho-bediening en Radio-logboek.
- JavaScript-syntax, lokale vertalingen, ingebedde webinterface en vaste bronhash worden opnieuw gecontroleerd.
- De tests sturen geen werkelijk RF-signaal uit. Hardwarewerking is niet bewezen.

## Geteste onderdelen en bekende beperkingen

**Geslaagd in deze omgeving:**
- C++-hosttest `test_cc1101_gdo_uart_host.py` met gesimuleerde UART/CC1101: UART-driver vóór configuratie, succesvolle mock-TX, gesimuleerde ESP-IDF-parameterfout, registerreadback-mismatch, herstel naar RX.
- `test_gdo_serial_tx_session.cpp`: toegestane herhalingen, foutdetail, RX-herstel en sticky-fault-beveiliging.
- Chromium: Radio Gaga-apparatenroute (1440/390 px), `DeviceUX4`-routes, Orcon-koppelwizard, Itho-bediening en PAIRRADIO9-radiofouten.
- Vier taalversies en firmwareversie in mobiele versiekaart, 26 officiële profielen (waaronder 16 Orcon), manifest/radio-transportcontracten, website-inbedding, JS-syntax.
- Pakketintegriteit, ordinal releasehash en PowerShell-interpolatiechecks.

**Niet als geslaagd opgegeven:**
- Een oudere uitgebreide logboek-browsertest liep in deze omgeving vast; de gerichte radiologtest slaagde wel.
- Een historische updater-test faalde op een ITHO1-decoderbaselinevergelijking, die niet door deze TX/UI-wijziging is aangepast.
- Geen Windows PowerShell 5.1-uitvoering van de BAT-release, geen volledige ESPHome-/ESP-IDF-compile, geen fysieke TX- of CC1101-meting.

## Eerste hardwarecontrole (na firmware-update)

1. Gebruik `UPDATE_RFLINK.bat` met je persoonlijke YAML (6 MHz SPI en TX-labopties); overschrijf het origineel niet.
2. Controleer de nieuwe `firmware_version` en `firmware_build_hash` via `/rflink/api/status`.
3. Open **Logboeken → Radio → 868,30 MHz** en laat ruis verborgen. Houd ook ESPHome-seriële logs beschikbaar.
4. Doe **één** 1FC9-koppelpoging en exporteer het diagnosepakket. Verwacht exacte fase en eventueel foutcode of afwijkend register; niet meer alleen 'preparation failed'.
5. Alleen wanneer de TX-drain en RX-verificatie slagen, kun je controleren of een tweede ontvanger een werkelijk telegram oppikt. De box hoeft voor deze eerste hardwaretest niet in koppelstand te staan, maar zonder juiste stand wordt geen ACK verwacht.
6. Bij RX-herstelproblemen geen nieuwe TX-pogingen doen voordat de radio veilig opnieuw is geverifieerd.

**Openstaande hardwarepunten:** het fysieke 868,30 MHz TX-signaal, het 1FC9-antwoord en het daadwerkelijk koppelen van de Orcon-box. De patch verlaagt het vermoede risico van een foutieve UART-configuratie, maar is geen bewijs dat RF-zenden nu werkt.
