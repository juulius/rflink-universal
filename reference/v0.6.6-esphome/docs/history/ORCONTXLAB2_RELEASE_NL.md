# RFLink Universal – ORCONTXLAB2 experimentele Orcon-bediening

**Gebouwd op:** GDOUARTLAB1-UX3-SYSDEBUG1-LOGBOOK1. **Revisie:** `...-LOGBOOK1-ORCONTXLAB2`.

## Wat is nu echt aangesloten?

- De ingebouwde `RamsesII`-decoder bevat een geregistreerde `ramses_ii`-TX-encoder. De acht profielacties **Afwezig, Auto, Laag, Midden, Hoog, 15/30/60 minuten** worden via de bestaande RAMSES-II-codec opgebouwd.
- De TX-router blijft **protocolonafhankelijk**. Alleen het decoder-/profielpakket kent Orcon/RAMSES-telegrammen; de centrale engine kiest `GDO_SERIAL`, radio 3 (868,30 MHz), en de fysieke adapter herstelt na verzending weer RX.
- Voor **Orcon MVS-15R / 15RF** is een opt-in **1FC9-koppelprocedure**: nieuwe virtuele class-37-zender-ID opslaan vóór TX; 1FC9-aanbod uitzenden; wachten op een passend 1FC9-antwoord (ACK) van een ontvanger; 1FC9-bevestiging en 10E0-beschrijving sturen; vervolgens een afzonderlijke Auto-test en *jouw* bevestiging dat de ventilator reageerde. Pas daarna komen de gewone knoppen vrij.
- De bestaande radiolog, systeemlog en weblog blijven bestaan. De systeemlog registreert encoderselectie, verzendroute, radio-eigenaarschap, koppelstappen, ACK, TX-fouten en herstel. De logviewer combineert die zonder dubbele opgeslagen vierde log.
- **Geen automatische adoptie van bestaande afstandsbediening-ID's; geen automatische oude koppelgegevens overschrijven; geen succes claimen na alleen UART-completion.**

## Vooraf: instellingen in jouw BESTAANDE YAML

**Belangrijk:** behoud de YAML die jij op jouw print gebruikt, inclusief de aangepaste **6 MHz SPI**, drie radio-CS/GDO-pinnen, Wi-Fi, secrets, MQTT en bestaande devicegegevens. De meegeleverde `rflink-universal.yaml` is een **voorbeeld met conservatieve SPI-snelheid**, geen vervanging voor jouw PCB-YAML.

Voor een bewuste test moeten in jouw `rflink_universal:`-blok deze opties staan:

```yaml
  tx_controls_enabled: true
  packet_transmitter: cc1101_868
  gdo_serial_tx_gpio: ${radio3_gdo0}
  gdo_serial_tx_lab: true
  ramses_tx_probe_lab: true
  ramses_radio_lab: true
  decoder_auto_scan_lab: false
```

Als `packet_transmitter` en `gdo_serial_tx_gpio` al staan, pas ze niet dubbel toe. `ramses_spi_lab` kan als afzonderlijke RX-labroute gebruikt worden, maar **begin met de UART-RX-route**; laat `ramses_spi_lab: false` zolang je die niet doelbewust test. De eerste test moet met TX-labvlaggen `false` zijn om te controleren dat alle drie radio's en ontvangst nog goed werken. Er mag nooit een tweede functie aan dezelfde GDO0-pin toegewezen zijn.

## Test op de ESP32-S3

1. Maak een backup van jouw YAML en exporteer eigen decoders, profielen en apparaten. Compileer en flash **eerst zonder ingeschakelde TX-labvlaggen**. Controleer drie radio's online, bestaande 433 MHz RX, 868 MHz RX en `firmware_revision` met suffix `ORCONTXLAB2`.
2. Schakel daarna alleen bewust de bovenstaande TX-labvlaggen in en compileer/install opnieuw. Controleer onder **Logboeken → Systeem** of `lab_enabled` en de radioadapter gereed zijn. Zoek bij fouten op radio-reservering, UART-preparatie en RX-herstel.
3. Test eerst de losse **10E0-radiozelftest** via de bestaande experimentele systeemdiagnose: dit is geen Orcon-commando of koppeling. Zonder tweede RF-ontvanger of scope is 'UART completed' **geen** bewijs van correcte RF-uitzending.
4. Open **Apparaten**, selecteer jouw opgeslagen MVS-15R/15RF-apparaat en bevestig het exacte **Orcon MVS-15R / 15RF**-profiel. Open **Stap voor stap instellen**. De koppelactie verschijnt pas als adapter en labflags gereed zijn.
5. Zorg dat alleen de eigen ventilatiebox in leerstand staat, volg de officiële Orcon-handleiding en kies bewust **Koppelen starten (1FC9)**. RFLink zendt één aanbod en wacht maximaal 180 s op een passend antwoord. Andere bekende RX-gegevens blijven behouden. Blijft het bij `waiting_ack`, exporteer dan alle logboeken en probeer niet blind onbeperkt opnieuw.
6. Bij een geldig ACK volgen bevestiging en 10E0-beschrijving. De wizard toont dan **Test Auto**. Test één keer (cooldown 15 s). Controleer daadwerkelijk de reactie van jouw ventilatiebox. **Alleen als hij reageert** klik je op bevestigen; anders niet.
7. Daarna worden de acht bedienfuncties voor dit profiel beschikbaar via de gewone apparaatkaart, voor zover het echte model deze functies ondersteunt. Controleer de eerste commando's één voor één. In de systeemlog blijft `TX verzonden` onderscheiden van `apparaat reageerde`.

**Als er geen ACK is:** mogelijk wordt GDO/UART fysiek onjuist uitgezonden, klopt RF-frequentie/modulatie/bitvolgorde/timing niet, staat de box niet in leerstand of werkt het RX-venster op deze PCB niet. Voor verdere diagnose zijn idealiter een tweede ontvanger of oscilloscoop aan GDO0, de gecombineerde diagnose-export en ESPHome-logs nodig. De software mag dan niet automatisch `paired=true` zetten.

## Beperkingen – serieus nemen

- **Software-/hosttests geslaagd, fysieke hardware niet gevalideerd.** Op deze ontwikkelomgeving is geen complete ESPHome/ESP-IDF-compile, RF-meetopstelling of echte Orcon-koppeling uitgevoerd. De eerste fysieke build kan aanpassingen vereisen.
- Orcon/RAMSES-over-the-air timings en daadwerkelijke 1FC9-compatibiliteit zijn experimenteel. Het is mogelijk dat de ventilatiebox deze implementatie nog niet accepteert.
- De pairingstatus tussen reboot en geslaagde verificatie is bewust niet 'gepaard'; bij een incomplete poging blijft de virtuele zender-ID bewaard en kunnen gebruikers dezelfde ID gecontroleerd opnieuw proberen.
- **Geen WAN-publicatie** van RFLink-webbeheer: bestaande HTTP-toegangscontrole is nog niet afdoende. Gebruik een vertrouwd lokaal netwerk.
- TX is fysiek actief zodra jij de labvlaggen expliciet inschakelt en de wizard start. Test alleen op je eigen apparaat, onder toezicht, met mogelijkheid de firmware terug te zetten. Een succesvolle UART-API-call is niet hetzelfde als een werkend Orcon-commando.

## Technische controles

- `tests/test_orcon_experimental_encoder.cpp`: alle acht commands, framing en afwijzen van ongepaarde/verkeerde profielen.
- `tests/test_orcon_experimental_browser.py`: wizard, ACK-fase, Test Auto en verificatie op 1440 en 390 px (gesimuleerde API).
- `tests/test_ramses_ii_tx.cpp`: encoding, checksums en bind-ACK-guards.
- `tests/test_orcon_family1.py`, `tests/test_radio_engine_v2_manifests.py`: 26 profielen, veilige decoder-/TX-manifestcontracten.
- `tests/test_profileflow1_compilefix1.py`: host-C++ syntaxcontrole (niet ESPHome/ESP-IDF).
- `tests/test_logbook_combined_browser.py`: afzonderlijke en gecombineerde logs.
- `tests/run_tests.sh`: uitgebreide suite; zie testverslag. Een host-test is geen hardwarevalidatie.

**Architectuurregel:** geen protocolspecifieke zendlussen in CC1101-driver; decoder beschrijft RF-codering, universele engine claimt de radio, fysiek transport zendt en herstelt de RX, systeemlog verklaart alles.
