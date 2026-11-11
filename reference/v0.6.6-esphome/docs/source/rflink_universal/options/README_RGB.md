# RGB-statuslamp — RC4-HF3

Dit pakket is standaard ingeschakeld voor het getoonde ESP32-S3 N16R8-board
met CH343P en twee USB-C-poorten: **één WS2812 op GPIO48**.
De eerder ingestelde GPIO47 is onjuist voor veel exemplaren: sommige
pinoutplaatjes zeggen 47, terwijl het werkelijke LED-spoor 48 gebruikt.
Een PCB-revisie kan een soldeerbrug voor 47/48 hebben; controleer die bij afwijkend gedrag. ESPHome-driver: `esp32_rmt_led_strip`,
`channel_colors: GRB`, `internal: true` en `restore_mode: ALWAYS_OFF`.
De LED is aangeduid in de door de gebruiker getoonde hardwarefoto. GPIO48
volgt uit de documentatie van het CH343P/dual-USB referentieboard en meldingen
over de onjuiste GPIO47-pinout; fysieke werking op dit concrete exemplaar
is nog niet gecontroleerd.

## Kleuren en patronen

| Kleur | Patroon | Gebeurtenis |
|---|---|---|
| Blauw | Pulseren gedurende 3 s | Opstartfase software |
| Geel | Rustig pulseren | Wi-Fi probeert te verbinden |
| Groen | Twee korte hartslagen per 6 s | Wi-Fi verbonden / systeem gereed |
| Turquoise | Pulseren | Baseline-meting of luisteren / inleren |
| Turquoise | Twee flitsen | Frame op gekozen leer-kanaal ontvangen; ook onbekende frames tellen mee |
| Helder groen | Drie flitsen | Apparaat succesvol bevestigd of aan bestaande registratie gekoppeld |
| Paars | Pulseren | Uitgaande Itho-koppelopdracht; GEEN bevestiging van pairing aan ontvangerzijde |
| Blauw | Korte flits | RF-verzendactie gestart; niet per se op de ether bevestigd |
| Wit | OTA-callback-puls | Firmware-update bezig |
| Oranje | Twee flitsen met pauze | Wi-Fi offline |
| Rood | Drie flitsen | RFLink detecteerde een fout, bv. ontbrekende raw receiver |

Eén LED kan maar één status tegelijk tonen. Prioriteit: OTA > fout >
bevestiging > Itho-koppelopdracht > leer-ontvangst > inleren > opstarten >
verbinding offline > RF verzenden > gereed. Een koppelopdracht verloopt na
maximaal 15 seconden wanneer een script onverwacht stopt, zodat paars niet
onbeperkt blijft staan. Normale ontvangst tijdens bedrijf geeft bewust GEEN
flits om bij druk RF-verkeer voortdurend knipperen te voorkomen.

**Belangrijk:** software kan niet vaststellen dat een fysieke CC1101 echt
gezond is enkel doordat de configuratie de radio definieert. "Gereed"
betekent daarom niet een bewezen hardwaregezondheid. Rood geeft alleen
herkenbare softwarefouten aan; dit is geen universele RF-diagnose.

## Helderheid en bediening

Ga op het vertrouwde LAN naar `/rflink` en open LED-instellingen.
Helderheid 1–100%, standaard 20%. Optie "Groene hartslag bij gereed"
toont bij aanvinken de normale gereed-hartslag; uitvinken maakt uitsluitend de gereedstand stil. Instellingen worden in
ESP32-preferences bewaard. De animatie wordt via een 100ms-interval
getekend en doet niets als de RGB-uitvoer niet veranderd is.

OTA blokkeert volgens ESPHome mogelijk de gewone main loop. Daarom wordt
het witte LED-signaal behalve via de statusregeling ook direct aangestuurd
vanuit de OTA-start- en voortgangscallbacks. Gedurende de update is dat
**callback-gestuurd en mogelijk niet vloeiend pulserend**. Bij OTA-fout wordt
rood getoond; bij succes start het apparaat doorgaans opnieuw.

De inleerstatus volgt de bestaande ontwikkelaars-API; `Stoppen en meting wissen`
in de leerpagina beëindigt het luisteren. Itho paars toont de verzendactie,
niet bewezen koppelsucces; groen bij bevestigen slaat op de eigen software-store.

## Andere hardware / eerste fysieke test

De hoofd-YAML bevat:

```yaml
substitutions:
  status_led_gpio: GPIO48
packages:
  rgb_status: !include rflink_universal/options/rgb_status_ws2812.yaml
```

Dit is een fragment, niet een complete vervangende configuratie. Zonder de
onboard WS2812 op deze pin: verwijder de `packages:`-include, of wijs de
werkelijke pin toe. Een gewone RGB-led met vier pennen is geen WS2812.

Voor de eerste flash: PCB-pinout vergelijken; ESPHome Validate, Clean Build
Files en Compile. Test daarna geel tijdens verbinden, groen gereed, turquoise
inleren, blauw TX en wit OTA. Test de kleuren met de echte WS2812; als R en G
verwisseld zijn, pas `channel_colors` aan. Deze test is nog niet uitgevoerd.

## GPIO48 LEDFIX (2026-09-18)

Als de webinterface werkt maar de LED donker blijft, controleer in de
**daadwerkelijk actieve** `rflink-universal.yaml` dat
`status_led_gpio: GPIO48` staat. De bundle-import kan de oude hoofd-YAML
behouden en daarmee GPIO47 actief laten; alleen het pakket vervangen helpt
dan niet. Controleer in `/rflink` helderheid (test 100%) en schakel
"Groene hartslag bij gereed" tijdelijk aan. De gereed-indicatie knippert slechts
twee keer per 6 seconden. Voor een volledig donkere LED: check ESPHome logs
op `esp32_rmt_led_strip` / `no free tx channels` en de PCB-jumper.
De GPIO48-correctie wijzigt geen radio-, wifi-, API- of OTA-instellingen.


## ESP32-S3 RMT allocatie (RMTFIX)

De twee 433MHz raw OOK-zenders (`tx_433_42` en `tx_433_92`) hebben elk
`use_dma: false` en `rmt_symbols: 48` in de hoofd-YAML. De RGB-LED hier
gebruikt dezelfde twee instellingen; samen is dit 144 van de 192 TX-symbolen.
De twee 433MHz ontvangers gebruiken de aparte RX-bank met elk 96 symbolen
non-DMA. De 868MHz Itho-CC1101 blijft 2-FSK packet-mode via SPI, dus hij
gebruikt geen RMT. Bij het importeren moeten beide YAML-bestanden worden
bijgewerkt, anders blijven de TX-fouten bestaan.

## Statuswaarschuwingen en OTA (RADIOLED fix)

- Groen hartslag: wifi werkt en alle **drie** radio's plus bijbehorende RX/TX-drivers zijn gereed.
- Oranje dubbele flits: minstens één geconfigureerde radio ontbreekt of RX/TX-driver is defect; wifi, website, API en OTA blijven beschikbaar.
- Rood: algemene RFLink-fout; wit: OTA (heeft voorrang op de oranje waarschuwing).
- OTA-handlers sturen de fysieke WS2812-uitvoer rechtstreeks aan, omdat de gewone ESPHome-light-loop tijdens OTA geblokkeerd kan worden. De start wordt wit gezet; voortgang wordt slechts getoond wanneer ESPHome een callback aflevert.
