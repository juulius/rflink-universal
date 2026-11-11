# Radio Engine V2 – testresultaten, 26 oktober 2026

## Geslaagd

- Manifestvalidatie: 9 ingebouwde decoderpacks, 13 protocollen, alle RX/TX routes voldoen aan één schema.
- C++ dispatch: na een hoge maar foutieve match alsnog de lagere geldige decoder vinden; dubbele conflicterende geldige decodes blijven ambigu; derde decoder krijgt geen onjuiste RAW/packet-capture.
- Syntactische C++-hostcompilatie van `rflink_universal.cpp` met ESPHome-hoststubs, `-Wall -Wextra -Werror`.
- 100 synthetische decoders: prefilter sloeg 97/100 irrelevante decoders over; 10.000 captures verwerkt in circa 55–70 ms op deze **desktop-host**, niet gemeten op de ESP32-S3.
- Scheduler verwerkt 100 regels zonder de oude 16-entry-grens; géén automatische time-sharing buiten de bestaande opt-in.
- Capture-profielplanner: compatibele 433,42/433,92-profielen delen dezelfde capture; meerdere 868-modusvereisten blijven gescheiden.
- RAMSES-II echte opname (#653 en #661): Away en Auto correct, ongeldig pakket geweigerd, checksum gevalideerd.
- Bestaande Python-regressies: decoder catalog, transport catalog, profielen, user library promotie, verpakking, allemaal geslaagd.
- Brede C++ host-suite is voorbij radio, UART, SPI, RAMSES RX/TX-codecs, radiomodeherstel, weerstations, logging, HA, MQTT, persistence en TX1/TX2 gekomen. Alle afgeronde testblokken hadden PASS.

## Niet afgemaakt / niet uitgevoerd

- De volledige host-suite haalde de aangehouden uitvoeringstijdslimiet en is daarom **niet volledig afgerond**. Zie `RADIOV2_HOST_SUITE.log` in de werkmap; dit logbestand zit bewust niet in de distributie.
- Geen volledige ESPHome/ESP-IDF-firmwarebuild met de daadwerkelijke SDK, geen Windows release/updater-run.
- Geen fysieke 433- of 868-MHz-hardwaretest, geen gelijktijdige Itho/RAMSES-demodulatie aangetoond, geen Orcon-TX- of pairingsucces aangetoond.
- Nog geen meting van CPU/RAM, packet loss of RSSI-ruis bij 100 decoders op de fysieke ESP32-S3.
- Bestaande NVS User Decoder-limiet blijft 8. Het aantal 100 is alleen de synthetische verwerkingsengine-stresstest, niet het aantal persistent opslaan decoders.
