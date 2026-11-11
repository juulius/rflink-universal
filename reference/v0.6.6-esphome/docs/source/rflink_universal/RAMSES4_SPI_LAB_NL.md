# RAMSES4 — SPI-only RAMSES-II, experimentele ontvangst en FIFO-zendcodec

## Wat werkt al in software?

- CC1101-FIFO via **alleen SPI**: vaste hardware-sync `B3 2A`, bekende UART-headerbits voor reconstructie, softwarematige Manchester/RAMSES-II-frame- en checksumvalidatie.
- Streaming RX zonder de bestaande Itho-63-bytegrens, FIFO-diagnose voor overflow, vervuilde frames en time-outs.
- Streaming TX via variable-length FIFO, met het eerste byte als lengte, bijvullen bij dalend TXBYTES en afbreken bij underflow/time-out. De RAMSES-II-commandocode kan echte Orcon-frames opbouwen.
- ESPHome CC1101-hardwareadapter: pauzeert de bestaande pakket-IRQ, wisselt registerprofiel exclusief tijdens de RAMSES3-radiolease, en vraagt volledige `reset()`/`configure()` + register-readback vóór Itho weer beschikbaar komt.
- Geldige RAMSES-ontvangsten gaan door de **bestaande decoderregistry, signaallog en apparatenpipeline**, zonder speciale Orcon-uitzonderingen.

## Wat is uitdrukkelijk nog niet bewezen?

De on-air CC1101-preambles en Manchester-gecodeerde RAMSES-II-UART-framing passen niet standaard op elkaar. `B32A` is een reproduceerbare 16-bit subreeks uit de RAMSES-header, maar de CC1101 kan vereisen dat er een geschikte preamble vóór staat. Of de CC1101 in de praktijk de complete Orcon-transmissies betrouwbaar detecteert, moet met de echte antenne en drukknoppen getest worden. Hetzelfde geldt voor de effectieve over-the-air TX-framing bij FIFO-transmissie. **Een succesvolle encode-decode-hosttest bewijst geen succesvolle zending naar de ventilatiebox.**

Er is **geen** Orcon-pairingknop of werkende fysieke TX vrijgegeven. Het softwarematige TX-FIFO-pad zit al in de testmodule, maar is opzettelijk niet aangesloten op de gebruikersinterface of MQTT. De officiële, bewezen alternatieve werkwijze gebruikt SPI voor configuratie en GDO/UART voor continue bits; SPI-only is een laboratoriumexperiment.

## Veilig testen op jouw ESP32-S3

1. Maak vooraf een back-up van de bestaande firmware/configuratie en de RFLink-bibliotheek. Bewaar de oude stabiele SOURCE en YAML.
2. Maak een aparte **CUSTOM-labconfiguratie**. Zet onder de bestaande `rflink_universal:`-sectie `ramses_spi_lab: true` (standaard `false`). Alle GPIO's, secrets en Itho-instellingen blijven hetzelfde.
3. Voeg desgewenst *als tweede item binnen het bestaande `button:`-blok* deze handmatige testknop toe (dus geen nieuw tweede `button:`-blok):

```yaml
  - platform: template
    name: "RFLink RAMSES SPI RX (3s lab)"
    entity_category: diagnostic
    on_press:
      - lambda: |-
          const bool started = id(rflink_core).request_ramses_spi_lab_rx(3000);
          ESP_LOGI("rflink.ramses_spi", "RX lab window: %s", started ? "started" : "blocked");
```

4. ESPHome **Validate + Compile**. Alleen wanneer beide slagen, zelf op de print proberen, liefst via USB met seriële logging. Doet de firmware onverwacht, flash dan de vorige bekende goede firmware.
5. Druk op de labknop, bedien in de volgende drie seconden de Orcon 15RF (`Auto`, `1`, `2`, `3`) en kijk bij **Activiteit** of de seriële logs naar RAMSES-II herkenningen. Herhaal meerdere keren; verwacht korte Itho-ontvangstonderbrekingen tijdens het 3-secondenvenster.
6. Controleer of Itho erna blijft functioneren. Bij herstel- of SPI-fout blijft de 868-radio **fail-closed** in de software, totdat een bewuste recovery/reboot heeft plaatsgevonden.
7. Als er géén RAMSES-ontvangsten zijn ondanks aantoonbaar drukknopsignaal, betekent dat niet dat er niets uitgezonden wordt: waarschijnlijk voldoet het gekozen SPI-syncprofiel niet. De veilige volgende stap is een alternatieve sync- en preambletest of een transparante GDO-bitstream.

## Technische beperkingen

- De RX-FIFO is slechts 64 bytes. De 38,4kbit/s radiostroom is ca. 4,8kB/s; bij een wachttijd groter dan ca. 13 ms kan een volle FIFO ontstaan. De labpoller moet dus met hoge regelmaat lopen. Dit wordt **niet** gegarandeerd door de gewone ESPHome-hoofdloop tijdens gelijktijdige drukte.
- RX-labmodus is handmatig, begrensd tot maximaal 30s, en niet cyclisch geactiveerd. Geen geruisloze terugkerende Itho/Orcon-scans zonder eigen betrouwbaarheidstests.
- De radiohardware is niet fysiek gecontroleerd in deze ontwikkelomgeving. De ESPHome-versie en de exacte `cc1101`-component moeten de gebruikte API's ondersteunen.
- De TX-engine gebruikt een CC1101 FIFO-frame met hardware-preamble/sync en gecodeerde data; het **exacte** Orcon-frame op antenneniveau blijft onbewezen. Daarom is fysieke bediening/pairing standaard geblokkeerd.
- De SPI-klok staat in de huidige YAML op 200kHz; voldoende bandbreedte is aannemelijk, maar timing moet op echte hardware worden bevestigd.
- Er is geen aanpassing van het PCB-ontwerp nodig: GDO0 en GDO2 mogen aangesloten blijven, maar deze test gebruikt geen UART.

## Bronnen

- TI CC1101 datasheet: https://www.ti.com/lit/ds/symlink/cc1101.pdf
- ESPHome CC1101 driver: https://github.com/esphome/esphome/tree/dev/esphome/components/cc1101
- Orcon 15RF RAMSES-II decoder/controller: https://github.com/me-processware/orcon-15rf-controller

De originele Orcon/RAMSES-II-bronverwijzingen en licentie-informatie blijven in `decoders/ramses_ii/ATTRIBUTION.md` staan.
