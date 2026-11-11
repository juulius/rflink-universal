# RAMSES5 — SPI-only Orcon receive laboratory (Nederlands)

**Status: softwarematig getest; niet op echte CC1101 gevalideerd.** De normale Itho-ontvangst en het bestaande RFLink-platform blijven de standaard. Dit is een ontvangstdianostiek en **geen werkende Orcon-afstandsbediening**.

## Nieuw ten opzichte van RAMSES4

- Twee RX-testmodi op dezelfde bestaande SPI-aansluiting:
  - `ramses_spi_unsynced: false` — CC1101 detecteert experimenteel `0xB32A` en ontvangt de rest via zijn RX FIFO.
  - `ramses_spi_unsynced: true` — CC1101 wacht niet op een hardwaresync; de decoder zoekt softwarematig op iedere bitpositie naar RAMSES-II UART-startbytes, Manchester-inhoud en checksum. Kan continu achtergrondruis genereren en is alleen voor korte gecontroleerde tests.
- Een **expliciet 26MHz/38,4kbaud 2-FSK** modemprofiel, in plaats van de datarate van Itho over te nemen. Modeminstellingen: `MDMCFG4=5A`, `MDMCFG3=83`, `MDMCFG2=02` (sync) of `00` (zonder sync), `DEVIATN=48` (ongeveer ±50,8 kHz). Ga na of jouw CC1101-oscillator werkelijk 26 MHz is.
- De RX FIFO wordt niet meer structureel volledig leeggelezen terwijl een pakket nog binnenkomt: één byte blijft staan tot een korte rustige periode. Dit voorkomt de door Texas Instruments beschreven dubbelgelezen laatste byte. Een standalone singleton wordt pas na twee waarnemingen uitgelezen.
- Controleren of de modeminstellingen echt zijn teruggelezen; aanvullende registerchecks bij het opnieuw activeren van Itho (waaronder `MDMCFG3` en `DEVIATN`).
- Aan het einde van elk labvenster staat in de ESPHome-logger een samenvatting met ontvangen bytes, geslaagde decodes, FIFO-overflows, pollingvertraging en Itho-herstel.
- Hosttests voor beide RX-modi, lange TX-FIFO in simulatie, gemanipuleerde registerreadback, TX-underflow en herstel.

## Zo test je op je eigen print

1. Bewaar een backup van de huidige werkende firmware, YAML, secrets en User Library. Installeer alleen vanaf een afzonderlijke CUSTOM-labvariant; houd de oude SOURCE-release paraat.
2. Pas **in het bestaande `rflink_universal:` blok** aan:

```yaml
  ramses_spi_lab: true
  ramses_spi_unsynced: false  # Test 1: hardware-sync B32A
```

3. Voeg deze testknop als *item in je al bestaande `button:` lijst* toe:

```yaml
  - platform: template
    name: "RFLink RAMSES SPI ontvangsttest 3s"
    entity_category: diagnostic
    on_press:
      - lambda: |-
          ESP_LOGI("rflink.spi", "RAMSES RX requested: %s",
                   id(rflink_core).request_ramses_spi_lab_rx(3000) ? "accepted" : "blocked");
```

4. Voer **ESPHome Validate én Compile** uit. De hosttests die bij deze release horen vervangen dat niet. Installeer pas als de normale CC1101 en beide 433 MHz-radios correct op jouw PCB zijn gemapt.
5. Start de 3-seconden-test, druk direct meerdere keren op de Auto-knop van de originele Orcon 15RF en noteer of je nieuwe RAMSES-II-signalen krijgt. Doe dit enkele keren. Itho kan tijdens deze drie seconden geen signalen ontvangen.
6. Controleer in de logger de regel `RAMSES SPI lab:` en kijk of daar **`Itho restored`** staat. De Itho-ontvangst moet na het testvenster opnieuw werken.
7. Wil je vergelijken, zet dan `ramses_spi_unsynced: true`, bouw opnieuw, voer **dezelfde test** uit en vergelijk de diagnose. Deze tweede modus kan ook een stroom ruis ontvangen en moet niet permanent aan staan.
8. Zet na de vergelijking `ramses_spi_lab: false` en maak opnieuw een productiebuild, tenzij we samen verdere hardwaretests doen.

## Wat betekenen de logwaarden?

Bijvoorbeeld:

```text
RAMSES SPI lab: Itho restored, RX valid=1 bytes=116 polls=424 fifo_peak=8 overflow=0 timeouts=0 slow_polls=0 max_gap_ms=2 io_errors=0
```

- `Itho restored`: terugzetten én registercontrole geslaagd. Bij `RADIO FAULT` stop je de proef en herstart je met de bekende goede firmware; de software blokkeert terecht de 868 MHz-radio.
- `RX valid`: *volledig* gedecodeerd RAMSES-II-bericht, met correcte structuur en checksum (geen zekerheid over de identiteit van de zender zonder apparaatprofiel).
- `bytes`: via de SPI-RX-FIFO gelezen gegevens. >0 met valid=0 betekent dat er wat is gelezen, niet dat het Orcon is.
- `fifo_peak`: hoogste geobserveerde FIFO-bezetting. Tegen 64 aan betekent een groot risico op overflow.
- `slow_polls`: aantal polls met >8 ms ertussen. Bij 38,4 kbit/s kan een volle 64-byte FIFO al binnen circa 13 ms ontstaan. Normale drukte in de ESPHome-loop kan dus RF-data laten verloren gaan.
- `overflow` / `timeouts`: gedropte, onvolledige ontvangsten.

De waarden zijn **cumulatief sinds het opstarten**, zodat meerdere tests met elkaar vergeleken kunnen worden. Gebruik desgewenst een verse herstart voor een schone nulmeting.

## Grenzen / beveiliging

- **Geen fysieke Orcon-TX of pairing geactiveerd** in webinterface, Home Assistant of MQTT. Alleen de softwarematige RAMSES/TX-codec en de FIFO-TX-hostsimulatie bestaan.
- Synchronisatie in SPI-pakketmodus is nog niet op de antenne bewezen. De CC1101 zoekt doorgaans een preamble om betrouwbaar bits te synchroniseren; het UART-achtige RAMSES-II-patroon wijkt daarvan af. De officiële ESPHome RAMSES-projecten gebruiken daarom GDO/UART als bewezen route. SPI-only kan ook ongeschikt blijken voor continue betrouwbare ontvangst.
- Er is maar één 868,3 MHz CC1101, dus Itho en Orcon zijn **niet gelijktijdig** te ontvangen wanneer zij verschillende radio-instellingen nodig hebben.
- De RX-poller draait in de normale ESPHome-eventloop, niet in een gegarandeerd real-time task. De diagnose maakt vertraging zichtbaar; hij voorkomt die niet.
- Alleen 26 MHz CC1101-modules zijn hier parametrisch beschreven. Afwijkende kristalfrequenties vereisen andere waarden.
- Jouw GPIO's, Itho-virtuele ID, secrets, bestaande decoders, NVS en apparatenprofielen hoeven niet veranderd te worden.

Bronnen: TI CC1101 SWRS061I (data-FIFO, hoofdstuk 20), TI CC1101 silicon errata SWRZ020E en de RAMSES-II implementations in `wlcrs/esphome-ramses` en `me-processware/orcon-15rf-controller`. Respecteer hun bronlicenties.
