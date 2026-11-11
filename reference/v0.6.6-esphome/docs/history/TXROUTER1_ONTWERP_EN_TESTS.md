# RFLink Universal – TXROUTER1 (prototype)

## Architectuurbesluit

Eén protocolonafhankelijke TX-engine voor CC1101 433,42 / 433,92 / 868,30 MHz. Voor ontvangst en verzending gelden dezelfde scheidingen:

1. Decoder manifest beschrijft het TX-radioprofiel, preferred/fallback transport, protocolacties en mogelijkheden.
2. De protocolencoder maakt RAW-pulsen, SPI-FIFO-pakketten of seriële bits; protocolspecifieke handshakes blijven in het decoderpack.
3. De centrale TX-engine valideert decoderpermissies, apparaatidentiteit, de gekozen actie, frequentie, status van de radio, gereserveerde rolling-code/sequence-state en rate limits.
4. De transportlaag kiest beschikbare adapters. Geen protocolnamen in de fysieke transportkeuze.
5. De fysieke adapters voeren een gevalideerde TX uit; zij configureren de radio, beschermen toegang tot de bus, herstellen RX en markeren eventuele RF-fouten.

## Implementatie in deze patch

- `radio_v2.tx.transport` is voor **alle 13 ingebouwde protocollen** gedeclareerd in `manifest.json`. Elk protocol heeft dezelfde velden: `preferred`, `fallback`, `profile`.
- Generator valideert dat deze gegevens overeenkomen met de **bestaande toegestane decoderencoder**; een aangepast manifest kan geen TX-rechten toevoegen.
- De gegenereerde C++-catalogus bevat RX- en TX-profielen en kent de optie `RMT_RAW`, `SPI_FIFO_PACKET`, `GDO_SERIAL` en `NONE`.
- Generieke transportselector controleert ontvangende frametype en beschikbare adapters. De route kan alleen gekozen worden als de bijbehorende driver fysiek aanwezig en toegestaan is.
- De native TX-planner gebruikt de gegenereerde TX-transportcatalogus. De bestaande RMT- en SPI-FIFO-implementatie lopen via de generieke routekeuze.
- Persistente oude **User Decoder**-recepten worden op dezelfde routekeuze aangesloten via een read-only compatibiliteitsadapter; het NVS-opslagformaat is niet gewijzigd.
- Encoderinterface is voorbereid met `encode_serial()`, maar standaard weigert die methode totdat een passende encoder hem daadwerkelijk implementeert.
- Voor `RamsesII` is `gdo_serial` als **beoogde transportmethode** vastgelegd, maar er is geen toegestane TX-encoder en `enabled` is expliciet false. Geen RF-transmissie vrijgeschakeld.

## Tests uitgevoerd

Geslaagd:
- 9 ingeschakelde decoderpacks / 13 protocollen – schema- en capability-validatie
- gegenereerde RX/TX-header in de werkelijke ESPHome-mappenstructuur gecompileerd
- 100 arbitraire TX-profielen / 13 catalogus-routes / fallback- en afwijzingsregels
- C++-host-syntaxis van TX-engine en RFLink-runtime
- Somfy RTS TX hostregressie (virtuele zender, rolling-code-regels)
- 433 MHz TX-decoderregistratie / opt-in / MQTT-status / handmatige driverstatus
- bestaande Radio Engine V2 100-decoder-stresstest

Nog niet bewezen:
- complete ESPHome/ESP-IDF-build en UART/RMT/GDO-hardwaregedrag op ESP32-S3
- fysieke SPI-TX voor alle kanalen, dynamisch wijzigen van modem/presets
- fysieke GDO-seriële TX voor RAMSES-II en echte Orcon `1FC9`-koppelhandshake
- eigen decoders zelf TX-transport in de webinterface laten kiezen: oude RAW/PACKET User Decoders gebruiken de compatibiliteitsadapter; nieuwe expliciete GDO-profielen vereisen opslaguitbreiding
- de apparaatkaart-layoutfix uit de andere, nog niet uitgegeven werkversie zit **niet** in dit zelfstandig afgeleide PAIRING1-pakket

## Installatieadvies

Dit is een **broncodeprototype op basis van PAIRING1**, geen flashklare, hardwaregeverifieerde release. Bouw het eerst in ESPHome; laat de huidige werkende firmware draaien zolang de volledige build en regressies niet geslaagd zijn. De PATCH ZIP kan op PAIRING1 worden toegepast; hij bevat geen YAML-bestand en verandert dus jouw `cc1101_spi_rate_hz: "6000000"` niet.

## Volgende technische werkstap

Implementeer één per-radio TX-adapterinterface en sluit de fysieke GDO-seriële driver daarop aan. Bouw de Orcon-encoder in het `ramses_ii` decoderpack als concrete implementatie van `encode_serial()`. Koppel koppeling, telemetrie, RMT, SPI FIFO en GDO aan dezelfde centrale scheduler/owner, zonder afzonderlijke Orcon-TX-route in de RFLink-core.
