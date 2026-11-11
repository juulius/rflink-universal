# RAMSES6 — decoder-declared SPI/UART transports (test release)

## Opzet

De decoder kiest **niet** zelf een GPIO of roept een CC1101-driver aan. Zijn
`manifest.json` beschrijft alleen het gewenste radio-transport. De centrale
`RadioTransportPolicy` bepaalt het voorkeurs- en fallback-pad op basis van
beschikbare hardware, toegestane opties, time-outs en exclusief radiogebruik.
De `RadioModeManager` reserveert radio 2, onderbreekt de bestaande Itho-
packet-driver, activeert de gekozen hardwareadapter en verklaart Itho pas weer
beschikbaar na een gecontroleerde reset en registerverificatie.

De gedeelde SPI-bus blijft actief voor alle drie de CC1101-chips. UART1 is
uitsluitend een **extra RX-ingang** voor de demodulatie van de derde CC1101 via
zijn **GDO2**-lijn. CSN, SCLK, MOSI en MISO blijven ongewijzigd.

### Decoder-manifest

`rflink_universal/decoders/ramses_ii/manifest.json`:

```json
"radio_transport": {
  "radio": "868.30",
  "preferred_rx": "uart_stream",
  "fallback_rx": "spi_fifo_sync",
  "max_window_ms": 30000,
  "scan_every_ms": 5000,
  "scan_window_ms": 250
}
```

`fallback_rx` mag `none`, `spi_fifo_sync`, `spi_fifo_raw`, `spi_packet` of
`uart_stream` zijn, mits hardware en protocol dat ondersteunen. Hetzelfde geldt
voor `preferred_rx`, behalve `none`. Voor IthoRFT is het `spi_packet` zonder
fallback of automatische scan. De generator valideert de configuratie tijdens
de ESPHome-build en maakt er een onveranderlijke C++-catalogus van.

Bij een gewone broncode-release vereist een verandering van het decoder-manifest
een nieuwe build. Dat is geen vrije registereditor op de webinterface. In
**Instellingen > Decoderbibliotheek > Advanced** worden voorkeur en fallback
wel zichtbaar. Een toekomstige UI-editor kan dezelfde gevalideerde instellingen
gebruiken, maar moet geen onbekende pinnen of CC1101-registers toestaan.

### Firmware opt-in (standaard UIT)

```yaml
rflink_universal:
  ramses_radio_lab: false
  decoder_auto_scan_lab: false
  ramses_uart_gdo2_pin: ${radio3_gdo2}
```

Stel alleen voor een gecontroleerde RX-praktijktest `ramses_radio_lab: true` in.
De bestaande `request_ramses_spi_lab_rx(3000)` testknop kan blijven bestaan,
maar gebruikt voortaan de *decoder policy* en kiest automatisch UART wanneer
GDO2 beschikbaar is; bij ontbrekende UART valt hij terug op de gedeclareerde
SPI-FIFO-methode. De nieuwe generieke functie heet
`request_decoder_rx_window("RamsesII", 3000)`.

Een echte UART-startfout leidt **pas na bewezen volledig Itho-herstel** tot
eenmalige SPI-fallback voor een volgende test. Geen ontvangen pakketten is
nooit op zichzelf bewijs van een UART-fout. Bij mislukte Itho-herstelcontrole
blijft de radiomodus in FAULT en stopt het systeem verdere acties op die radio.

Voor een expliciete time-sharingproef mag ook
`decoder_auto_scan_lab: true` worden gezet. De door de decoder gedeclareerde
RX-delen mogen maximaal 20% van een scaninterval innemen en worden uitsluitend
gepland wanneer Itho beschikbaar is. De RAMSES-testconfiguratie reserveert
250 ms van elke 5 seconden (5%). Bij omschakelen bestaat een reële kans op het
missen van korte Orcon- of Itho-berichten: beide protocollen kunnen niet
continu met één CC1101 in verschillende modems worden ontvangen.

`ramses_spi_lab` en `ramses_spi_unsynced` bestaan nog voor de historische
SPI-only-lab. Gebruik voor nieuwe UART-voorbeelden bij voorkeur
`ramses_radio_lab` en de decoderinstellingen.

## Wat werkt en wat niet?

**Softwaregetest:** transportpolicy en -manifest, non-blocking tijdslots,
UART-byteframing met controlesom (inclusief fragmenten, twee pakketten per
UART-read, beschadigde berichten, timeout), bestaande RAMSES RX/TX-codecs,
RadioModeManager herstel- en faalscenario's. De ESP32/UART-header is met
teststubs op C++-syntax gecontroleerd.

**Nog hardwaretest noodzakelijk:** UART1/ESP-IDF driverinitialisatie op jouw
ESP32-S3, echte GDO2 UART-framing en timing, veilige SPI-registerwijzigingen,
Itho-herstel bij stroomfouten, afwisselend luisteren en gedeelde SPI-bus onder
belasting. Een echte ESPHome/ESP-IDF-compilatie was in deze omgeving niet
beschikbaar. Deze update is geen hardwaregevalideerde release.

**Uitgeschakeld:** RAMSES-II fysieke TX, Orcon-koppelen, timerknoppen en
Home Assistant-zendbediening. De bestaande RX/TX-codec is alleen een
softwarevoorbereiding; de fysieke UART/GDO0 TX-waveform is nog niet gekoppeld.
Voor Orcon TX en pairing moeten GDO0-signalen nauwkeurig getimed worden en de
pairing-responsen op de echte hardware getest worden.

## Aanpak voor de eerste fysieke test

1. Maak een back-up van de werkende YAML en user decoder/device library.
2. Gebruik de updater om bestaande GPIO-, Wi-Fi-, API/OTA- en secretinstellingen
   te bewaren. Vervang deze niet door de DEFAULT-YAML.
3. Zet `ramses_radio_lab: false` en doe **ESPHome Validate en Compile**. Test
   na OTA dat alle drie CC1101-radio's en Itho als voorheen werken.
4. Zet dan `ramses_radio_lab: true`, maar laat `decoder_auto_scan_lab: false`.
   Gebruik een **kort handmatig RX-venster** en druk een paar keer op Auto.
5. Controleer ontvangst, afgewezen frames, radio-foutstatus en `Itho restored`.
   Test daarna een Itho-ontvangst zodat herstel niet alleen registermatig maar
   ook functioneel bewezen is.
6. Activeer **geen Orcon-zenden of pairing** op basis van alleen deze softwaretests.

## Externe technische referenties

- Texas Instruments, CC1101 datasheet: SPI-configuratie en asynchrone seriële GDO-data
- https://github.com/wlcrs/esphome-ramses (referentie voor GDO2/UART-RX)
- RFLink Universal: `RADIO_PROFILES.md`, `DECODER_API_V2.md`, `RAMSES3_RADIO_SAFETY_NL.md`
