# RFLink Universal — Radio Engine V2 foundation (26 oktober 2026)

## Doel

Eén configureerbaar decodercontract voor alle drie CC1101-radio's. De manifesten beschrijven wat een protocol kan verwerken en hoe het zenden al door de bestaande engine is toegestaan; de centrale software beheert de uitvoering. Dit is een **broncode-/hosttestfase**, geen nieuwe, fysiek gevalideerde radiomodus.

## In deze versie toegevoegd

- `radio_v2` onder **elke** `capabilities[]` in alle negen decoderpack-manifesten (13 protocollen). Uniforme `rx.routes[]`- en `tx`-velden.
- Validatie en gegenereerde C++-routingcatalogus per ESPHome-build. Een ongeldige band, inputsoort, transport of een verzendclaim zonder bestaande goedgekeurde encoder stopt de build.
- `DecoderDispatchV2`: route-prefilter per radio en RAW/packet, controleert vervolgens **alle** kandidaten boven de ingestelde drempel; een foutief hoog scorende decoder blokkeert niet langer een geldige lagere match. Bij conflicterende, even sterke geldige resultaten wordt de ontvangst als ambigu behandeld, niet opgeslagen als apparaat.
- `RadioProfilePlanV2` groepeert compatibele capture-methoden zodat decoders dezelfde ontvangst kunnen delen. Verschillende methoden of radio-profielen krijgen **niet automatisch** tijdsloten. De engine meldt een conflict in de modelplanner; voor 868 MHz is één gecombineerde Itho/Orcon-ontvangst nog niet bewezen.
- `RadioScanScheduler` heeft geen vaste limiet van 16 entries meer. De scan blijft expliciet opt-in en exclusieve radio-eigendom blijft verplicht.
- Bestaande User Decoders gebruiken dezelfde channel/frame-selectie via een compatibiliteitsadapter; hun NVS-indeling en TX-voorwaarden zijn niet gewijzigd.
- Alle 433 MHz-receivers blijven RAW OOK/RMT, 868 MHz houdt zijn bestaande Itho packetconfiguratie en optionele RAMSES6-adapters. Geen nieuwe GPIO-bezetting of automatisch zenden.

## Configuratie per decoder

Voorbeeld (Somfy RTS, onder een capability in `manifest.json`):

```json
"radio_v2": {
  "rx": {"routes": [
    {"band": "433.42", "kind": "raw", "method": "raw_rmt", "profile": "ook_raw_43342"}
  ]},
  "tx": {"method": "legacy_tx_engine", "status": "host_tested", "band": "433.42"}
}
```

RX-route is een **vereiste**, geen opdracht om de CC1101 ongecontroleerd te herschrijven. De manifestgenerator controleert de beschikbare radio en methoden. TX wordt uitsluitend via de bestaande TX-engine geautoriseerd. User Decoder-interpreter en ingebouwde C++-decoders houden hun bestaande gescheiden uitvoeringsvormen; ze delen het ontvangstcontract, niet automatisch dezelfde algoritmische vrijheid.

## Welke radioprofielen worden gedeeld?

| CC1101 | Bestaande toestand | V2-plan |
|---|---|---|
| 433,42 MHz | RMT RAW OOK | Somfy en overige passende RAW-decoders kunnen dezelfde capture gebruiken |
| 433,92 MHz | RMT RAW OOK | Schakelaars, sensoren en eigen RAW-decoders gebruiken dezelfde capture |
| 868,30 MHz | Itho SPI packet, optionele RAMSES6-lab-adapters | Itho SPI en RAMSES UART/SPI blijven voorlopig **aparte routes**. Geen automatische profielwisseling of onbewezen gedeelde demodulatie |

De modulatie, bitrates, bandbreedtes en kanaalfilters moeten door de CC1101 fysiek passend zijn. Een CC1101 is geen SDR en kan niet tegelijk alle configuraties demoduleren. Exact dezelfde route-ID en capture-methode mogen worden gegroepeerd; andere routes vragen een apart hardwarevalidatie-/arbitertraject.

## Tests

- `python3 rflink_universal/tests/test_radio_engine_v2_manifests.py`
- `g++ -std=c++17 -O2 -Wall -Wextra -Werror -I rflink_universal/components/rflink_universal rflink_universal/tests/test_radio_engine_v2.cpp -o /tmp/radio_v2 && /tmp/radio_v2`
- `python3 rflink_universal/tests/test_decoder_catalog_v2.py`
- `python3 rflink_universal/tests/test_radio_transport_catalog.py`
- `bash rflink_universal/tests/run_tests.sh` voor volledige hostregressie (kan lang duren).

De hostbenchmark gebruikt 100 synthetische decoders en meet codepadkosten **op de testcomputer**, niet de werkelijke ESP32-S3. Er is nog geen claim van 100 gelijktijdig gebruikte radioprofielen, 100 NVS User Decoders of gegarandeerde ontvangst. De bestaande UserLibrary heeft `MAX_DECODERS = 8`. Een opslagverhoging vereist eerst geheugentests, persistentiereview en migratie.

## Nog te bouwen / op hardware te testen

1. RF-profielbundeling koppelen aan een gevalideerde fysieke CC1101-presetregistry (frequentie, modulatie, filter, bitrate, sync), met veilige rollback.
2. Werkelijke FSK-demodulatie voor een gecombineerde Itho/RAMSES-bitstream aantonen en beide decoders voeden met de juiste framevorm; zolang dit niet werkt huidige modes intact laten.
3. TX-vormen (RMT, GDO seriële bits, FIFO) uniform declareren mét echte hardware-adapters; geen ongeteste Orcon-pairing activeren.
4. Echte ESPHome/ESP-IDF firmwarebuild op de opgegeven versie en praktijktests: drie radio's, ruisbelasting, watchdog, TX/RX-coexistentie, overflows en herstel.
5. 100-decodercapaciteit meten op de echte ESP32 (CPU, heap, latency, event-loss), daarna eventueel NVS/User Library uitbreiden.

## Veiligheids-/compatibiliteitsgrenzen

Geen GPIO-, YAML-, NVS- of radio-schakelwijzigingen. Dit pakket kan via de bestaande updater worden verwerkt; controleer YAML, voer `Validate` en `Compile` uit vóór OTA. De v2-catalogus is een declaratief contract, **geen nieuwe fysieke adapter** en geen definitief bewijs dat alle protocollen tegelijkertijd kunnen luisteren.
