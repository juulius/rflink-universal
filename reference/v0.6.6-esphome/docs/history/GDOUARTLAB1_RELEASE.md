# RFLink Universal – GDOUARTLAB1 (27 oktober 2026)

## Wat is gebouwd
- Generieke fysieke CC1101 **asynchrone GDO0/UART1-TX-adapter** (`cc1101_gdo_uart_tx.h`) met fysieke 868,30-MHz-preset. Hardwarecode bevat geen Orcon- of andere protocolspecifieke commandoregels.
- Decoder/encoder levert een gereed seriële 8N1-bytesequentie; radio-eigenaar configureert GDO0, modulatie, datasnelheid en TX/RX-herstel. Bestaande SPI-bus en GPIO16 blijven behouden.
- Afzonderlijke, host-testbare RX/TX-transactiebewaker (`gdo_serial_tx_session.h`) met begrensde herhalingen, herstel na gedeeltelijke TX-fout, RX-verificatie en sticky foutmelding bij herstelproblemen.
- Optionele YAML-instellingen: `gdo_serial_tx_lab: false` (standaard) en `gdo_serial_tx_gpio: ${radio3_gdo0}`. Labvalidatie controleert dat het om het bestaande GDO0 van radio 3 gaat.
- Regie via bestaande `SerialTxAdapter` en universele `TxTransport::GDO_SERIAL`-dispatcher; geen aparte Orcon-driver in de TX-engine.
- De eerder aangepaste apparaatkaart (openblijvende technische details) blijft aanwezig.

## Veiligheidsstatus
**Orcon-bedieningsknoppen blijven uitgeschakeld.** In `decoders/ramses_ii/manifest.json` staat de TX-capability nog op `encoder: none`, `status: disabled`; de firmware kan daardoor geen RAMSES-II-bediening vanuit de gewone UI starten. Alleen de fysieke **labadapter** is nu technisch uitgewerkt, nog niet geactiveerd of op werkelijke CC1101-hardware gevalideerd. `gdo_serial_tx_lab` aanzetten is op zichzelf geen Orcon-zendfunctie.

Waarom: zonder meting kunnen we niet bevestigen dat een UART1/GDO0-uitzending daadwerkelijk overeenkomt met Orcon RF op de antenne, dat de normale GDO0-interrupt betrouwbaar herstelt of dat een Orcon-ventilatiebox de koppelhandshake accepteert. Geen serienummer van bestaande afstandsbediening hergebruiken.

## Teststatus
- GDO TX-beveiliging: doorgaan, pauze/herhaling, acquire-fout, prepare-fout, UART-fout, failed restore, sticky fault: hosttests geslaagd.
- C++ core-syntax en UART/GDO-hardwareheader tegen **mock-ESP-IDF-headers**: geslaagd. **Dit is geen echte ESPHome/ESP-IDF-build.**
- Manifesten: 9 decoderpacks, 13 protocollen gevalideerd.
- Universele TX-route, 100 records, decoderregistratie en 433 MHz TX/MQTT-regressie: geslaagd.
- Alle acht RAMSES-II-commando's: `encodeWire()` geeft exact dezelfde 8N1-bitstroom als `encodeOnAir()`; codec koppeltest geslaagd.
- Apparaatkaart UI-status: host/browserfixture geslaagd.
- Nog niet uitgevoerd: volledige ESPHome/ESP-IDF-compile, fysieke GDO0-meting, succesvolle RX-restore op PCB, Orcon-koppeling en werkende bediening.

## Eerste hardwarestap
1. Bouw eerst met `gdo_serial_tx_lab: false` en controleer dat alle drie radio's online komen en RX werkt.
2. Controleer vervolgens afzonderlijk in een gecontroleerde opstelling de GDO0-timing en RX-herstelprocedure, zonder bestaande koppelgegevens te overschrijven.
3. Pas na hardwarebewijs een echte decoder-encoderregistratie, uitsluitend voor gevalideerde acties en met de normale koppelwizard. De manifest-TX-poort gaat dan pas open.

## Bestanden
- SOURCE ZIP is compleet; voor bestaande projecten de PATCH over TXROUTER2+DEVICEUI2 toepassen. **De PATCH bevat geen `rflink-universal.yaml`**, zodat persoonlijke SPI-instelling (6 MHz), API-sleutels en GPIO-keuzes intact blijven.
- De standaard FULL SOURCE en bundle bevatten de conservatieve 200-kHz SPI-YAML. Neem bij installatie altijd je eigen YAML mee.
