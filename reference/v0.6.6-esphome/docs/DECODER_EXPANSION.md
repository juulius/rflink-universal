# Decoder API v2C – vier RAW-zendencoders met per-apparaat toestemming

**Huidige testrelease (21 september 2026).** RFLink is leidend voor apparaten en goedgekeurde functies. De generieke `tx_engine` kent geen hardgecodeerde Nexa-/Byron-/Dooya-/RC-Switch-takken: protocollen bieden in hun eigen `tx_encoder.cpp` een `prepare`-, `capture`- en `encode`-implementatie aan; `manifest.json` vermeldt alleen werkelijk gekoppelde, gecontroleerde encoders. Alle RF-zendbediening staat standaard uit; `ha_tx_controls: true`, MQTT en expliciete HA- en TX-goedkeuring per apparaat blijven vereist.

- **Nexa**: bestaande Aan/Uit-encoder ongewijzigd.
- **Byron SX**: bestaande encoder voor afzonderlijk goedgekeurde vaste deurbelcode ongewijzigd.
- **Dooya**: encoder voor *exact ontvangen en goedgekeurde `button_N`*, niet automatisch de semantische namen `open`, `close` of `stop`. Het radiokanaal, Dooya-kanaal en de controlecode van **iedere afzonderlijke knop** worden op het moment van zendtoestemming vastgezet. Een nieuwe knop of een latere ontvangen gewijzigde controlecode verandert bestaande zendcommando's niet; zet TX uit, controleer en keur opnieuw goed om de binding te vervangen. Bestaande Dooya-apparaten krijgen `tx_radio` en `check_button_N` eerst als voorstellen wanneer de knoppen opnieuw worden ontvangen.
- **RC-Switch**: uitsluitend een exact ingeleerd vast-code-telegram met expliciet vastgezette `code`, protocolnummer **1–8**, bitlengte **8–64** en 433,92MHz-radio. Geen automatische aan/uit-interpretatie of truc met verzonnen bitlengte. Bestaande RC-Switch-apparaten missen `bit_length`/`tx_radio` totdat opnieuw een passend signaal wordt ontvangen en de velden worden goedgekeurd.

**Decoderbibliotheek:** toont per protocol de werkelijk gekoppelde RX/TX-status uit het manifest. Itho RFT heeft nu native 868,30MHz packet-TX met een eigen virtuele RFLink-zender per ingeleerd apparaat; Somfy RTS gebruikt zijn native virtual-remote/rolling-codepad. KeeLoq blijft zonder TX zolang geen afzonderlijk gevalideerde sleutel-/tellerimplementatie bestaat. Weersensoren blijven bewust ontvangstsensoren.

**Nog te bouwen / fysiek te bevestigen:** (1) persistent functie-/kanaal-/relatiemodel v2 met gecontroleerde migratie; (2) MQTT-wijzigingsqueue bij brokeruitval; (3) webauthenticatie; (4) fysieke hardwaretest van iedere encoder, inclusief Itho RFT op een echte 868,30MHz-unit, en volledige ESPHome/Windows-test. Ontvangen functies worden niet automatisch goedgekeurd of naar Home Assistant geüpload. Een RF-zendaanvraag bewijst geen uitvoering door het doelapparaat.

**Voor bestaande gebruikers:** gebruik de updater voor je persoonlijke YAML en houd de huidige GPIO/OTA-secretwaarden. Bestaande NVS-records en Home Assistant-IDs blijven intact. Schakel zendbediening niet in voordat je ESPHome Validate en Compile hebt uitgevoerd én de relevante RF-modules op echte hardware hebt getest. Dit is géén productieclaim.

---

> **Actuele ontwikkelhandleiding:** `DECODER_API_V2.md`. De oudere informatie hieronder beschrijft de historie van de bestaande decoders.

# Decoderuitbreiding 1 — RFLink Universal

**Basis:** de werkende RADIOMON-release met BOOTFIX, GPIO48-LEDFIX, heartbeat UI en RMTFIX. Alle drie de bestaande CC1101-profielen en het RGB-statuslampje blijven ongewijzigd.

## Toegevoegd: drie 433,92MHz-decoderpakketten

| Decoder | Signaal | Inhoud | Integriteitscheck |
| --- | --- | --- | --- |
| Acurite 609TXC / 609A1TX | OOK 40 bits, PWM | Sensor-ID, temperatuur, vochtigheid, batterijalarm | Additieve byte-checksum |
| Rubicson / TFA 30.3197 (36-bit variant) | OOK 36 bits, PPM | ID + kanaal, temperatuur, batterijalarm | CRC-8 poly 0x31, init 0x6c |
| Fine Offset WH2 (niet WH2A/WH5) | OOK 48 bits, PWM | Sensor-ID, temperatuur, vochtigheid | Preamble 0xFF, CRC-8 poly 0x31 |

De decoderpakketten hebben `enabled: true` en `host_test: true` en worden door de bestaande `decoder_directory` automatisch aangetroffen. Deze uitgave bevat bovendien een linkerfix: automatisch gegenereerde, expliciete decoderankers voorkomen dat de ESP-IDF static linker de geregistreerde decoderobjecten stilzwijgend weglaat (`0 decoder(s) registered`). Het bestaande `rflink-universal.yaml` en secrets worden bij een normale update behouden. Dit historische decoderuitbreidingshoofdstuk wijzigde destijds geen pinouts, RGB-driver, TX/RX-RMT-toewijzing, webserver-startvolgorde of de toenmalige 868MHz-Itho-pakketmodus. In de huidige ITHO1-branch is die packet-TX centraal aan ingeleerde apparaten gekoppeld. Wel zijn de componentcode, decoder-registry en het ESPHome-codegeneratiebestand bijgewerkt om decoderobjecten expliciet te linken.

### Betrouwbaarheid en beperkingen

- De frame-indelingen zijn onderzocht aan de hand van de openbare rtl_433-implementaties. Dit project bevat nieuw geschreven C++ en heeft geen rtl_433 runtime-afhankelijkheid.
- Een aparte linkerregressietest bootst een echte statische `.a`-decoderbibliotheek na, bewijst dat alle drie nieuwe decoders via expliciete ankers worden gelinkt, en controleert dat dezelfde bibliotheek zonder ankers inderdaad nul registraties oplevert.
- Hostregressietests gebruiken synthetische volledige RF-pulstreinen (correcte én ongeldige berichten): registratie, datavelden, CRC/checksum, verkeerd frequentiegebied, pakket versus raw, korte berichten en corruptie.
- Dit is **geen hardware-gevalideerde ondersteuning**: 433,92MHz-CC1101 en fysieke sensorsignalen zijn nog niet getest. Pulsbreedtes, framefragmentatie en detectiekans moeten met echte radio worden gecontroleerd.
- InFactory PT-310 heeft soms 37/38 bits en wordt niet als volledig ondersteund geclaimd: alleen de bevestigde 36-bit Rubicson-indeling is geïmplementeerd.
- De sensor-ID kan veranderen wanneer de batterij wordt vervangen. Herkenning van sensor-ID is niet hetzelfde als een gegarandeerde permanente identiteit.
- **RX-only**: er zijn geen zendsleutels, spoof-functies of koppelmogelijkheden toegevoegd.

### Bronnen

- https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c
- https://github.com/merbanan/rtl_433/blob/master/src/devices/rubicson.c
- https://github.com/merbanan/rtl_433/blob/master/src/devices/fineoffset.c

### Installeren/updaten

1. Maak een back-up van jouw `rflink-universal.yaml` en `secrets.yaml`.
2. Importeer de nieuwe `.esphomebundle.tar.gz` in ESPHome Device Builder via **Create device → Advanced → Import from file**. Bestaande apparaat niet verwijderen.
3. **Behoud jouw bestaande hoofd-YAML en secrets**; voeg de drie nieuwe decoderfolders toe. Bij conflicten in bestaande broncode de correcte nieuwere releasebestanden kiezen. **Neem alle gewijzigde bestanden in `rflink_universal/components/rflink_universal/` over**: de decoderlinker-fix zit daar, niet alleen in de drie nieuwe decoderfolders.
4. Validate → Install → OTA. Bij radio-loos testen blijven drie CC1101-FAIL-meldingen verwacht; de webinterface en OTA behoren bereikbaar te blijven.
5. Controleer bij het opstarten hoeveel decoderpakketten werkelijk geregistreerd worden. Een geslaagde broncodetest is geen bewijs van werkende ESP-IDF-linking op hardware.

### Windows-release

`MAAK_RELEASE.bat` gebruikt uitsluitend native Windows PowerShell en `tar.exe`; **geen geïnstalleerde Python nodig**. De source-ZIP bevat beheer en testen, de ESPHome-bundle bevat uitsluitend de benodigde runtimebestanden, geen BAT, backups of secrets.
