> **RXLOG2 HOTFIX (2026-10-25):** Fixes reboot/abort in streamed RX history and malformed noise event JSON. Read `RXLOG2_CRASHFIX.md` before flashing. Hardware stability and full ESPHome compilation remain unverified.

> Decoder API v2B: zie `../DECODER_API_V2.md` (SOURCE-zip) voor capability-manifests, werkende functies en beperkingen.

# RFLink Universal — componenten / code-overzicht

Dit is de RC3-gebaseerde bron uit de aangeleverde `v0.3.3-RC4_WebHandler_Fix`
met inbegrepen WebHandler-linkerfix. Zie eerst `../START_HERE.md` en
`../RELEASE_NOTES.md`; deze documentatie claimt geen volledige originele RC4.

- `components/rflink_universal/`: ESPHome external component, C++ RF-core,
  decoderregister, device store, Itho/Somfy codecs en embedded HTML-webhandler.
- `decoders/`: zelfstandige decoderpacks; elk met manifest en decoder.cpp.
  Een nieuwe decoder toevoegen vereist geen aanpassing aan de centrale core.
- `ui/index.html`: webinterfacebron; `tools/embed_web_ui.py` maakt `web_ui.h`.
- `tests/`: hosttests / web smoke / hotfix-/release-/pakketchecks.
- `options/`: WS2812 RGB-status, op het gebruikte ESP32-S3-board standaard ingeschakeld via GPIO48.
- `tools/maak_upload.py`: veilige ZIP/tar.gz-verpakking (start via root BAT).
- `maintenance/legacy_hotfix/`: oud script alleen als historisch hulpmiddel.

**Huidige radio-indeling:** SPI MOSI11/SCK12/MISO13; RF1 433.42
CS5/GDO0(TX)6/GDO2(RX)4; RF2 433.92 CS9/GDO0(TX)8/GDO2(RX)10;
RF3 868.30 CS15/GDO0(packet)16, GDO2/IRQ17 gereserveerd en niet gebruikt in YAML.
Dit is een softwaremapping, geen verificatie van fysieke PCB/footprints.

**Profiel:** 433.42 en 433.92 ASK/OOK raw; 868.30 Itho 2-FSK packet.
De 868-radio kan niet gelijktijdig raw OOK én Itho 2-FSK luisteren.
Het oude RC3-README beschreef een tweede YAML die in deze ZIP ontbreekt;
alleen `rflink-universal.yaml` wordt geleverd.

**Webinterface:** `/rflink` op lokaal netwerk; Status, Apparaten, Leren,
Instellingen/Hulp. Geen ingebouwde HTTP-authenticatie in de kerncode.
De verdere protocolrichtlijnen staan in `DECODER_GUIDE.md`,
`PROTOCOL_SOURCES.md` en `WEB_UI.md`.

**Tests:** vanuit de hoofdmap `bash rflink_universal/tests/run_release_checks.sh`.
Hosttests zijn geen ESPHome firmwarecompile of hardwaretest.


## HA2 – actueel beheer (optionele MQTT Discovery)

MQTT is één keer in te stellen via de centrale YAML-labels in `INSTELLINGEN.md`. Na inleren kun je in het opslaan-scherm direct de standaard aangevinkte optie **Toevoegen aan Home Assistant** gebruiken. Bestaande apparaatfuncties staan centraal in RFLink: nieuwe velden/knopacties zijn voorstellen die pas na Goedkeuren via MQTT Discovery zichtbaar worden; bestaande readings/events worden live gepubliceerd. Verwijderen via Functies beheren ruimt uitsluitend de betreffende HA-functie op na bevestiging. Zie `START_HERE.md` en `RELEASE_NOTES.md` voor grenzen en tests.

### Historisch: HA1, vervangen door HA2

## Home Assistant (optionele MQTT Discovery, HA1)



1. Zorg voor een bereikbare MQTT-broker en de MQTT-integratie in Home Assistant. Gebruik je eigen brokeradres en apart aangemaakte gebruikersgegevens.
2. Open `rflink-universal.yaml`, zoek de `packages:` sectie en wijzig `# ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml` in `ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml` (laat de indentatie staan).
3. Voeg aan je **eigen** ESPHome `secrets.yaml` deze drie sleutels toe: `rflink_mqtt_broker`, `rflink_mqtt_user`, `rflink_mqtt_password`. Waarden staan niet in de release; zie `rflink_universal/secrets.example.yaml`.
4. Voer in ESPHome eerst Validate en Compile uit en installeer daarna pas. De vorige OTA-configuratie blijft voor deze update behouden.
5. Open de RFLink-webinterface, ga naar **Apparaten** en kies bij een herkend apparaat `Toevoegen aan HA`. Het apparaat krijgt op basis van beschikbare en ondersteunde gegevens bijvoorbeeld temperatuur-, vocht- en batterij-entiteiten, of een knop-event. Laat het apparaat nogmaals zenden voor de eerste actuele meetwaarde.
6. Met `Hernoemen` verandert de zichtbare naam bij de volgende succesvolle MQTT-publicatie. `Verwijder uit HA` verwijdert alleen de MQTT Discovery-configuraties; het apparaat blijft in RFLink. `Verwijderen` verwijdert het record uit RFLink én bij actieve HA-koppeling de Discovery-registraties.

**Belangrijke grenzen:** alle HA-wijzigingen vereisen een verbonden broker. Bij een brokerstoring blokkeert uitschakelen/verwijderen van een HA-gepubliceerd apparaat, om achterblijvende registraties te voorkomen. MQTT-publicatie bevestigt niet dat Home Assistant de gegevens heeft verwerkt: controleer de MQTT-integratie bij problemen. RFLink exporteert alleen expliciet ondersteunde sensormetingen en knopacties; geen RF-identiteiten of rolling codes. Een afstandsbediening die alleen ontvangst levert, verschijnt als event en niet als fictieve schakelaar. Native ESPHome API en standaard GPIO-pinout worden niet omgezet. Het dashboard zelf is alleen bedoeld voor een vertrouwd lokaal netwerk.

**Tests:** C++-hosttests dekken Discovery JSON, toevoegen, waarden/acties, hernoemen, herverbinden, broker-uitval, verwijderen en niet-ondersteunde gegevens. De volledige ESPHome 2026.9 firmwarecompile, Windows PowerShell en broker/HA-hardwaretests moeten op de eigen installatie worden uitgevoerd.

