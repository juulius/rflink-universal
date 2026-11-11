# RFLink Universal HF3 BOOTFIX — 18 september 2026

Deze build combineert de eerdere compilefix en radio-optional-wijzigingen met twee
problemen die in de echte ESP32-S3-N16R8-USB-log zijn vastgesteld.

## 1. Fatale reboot: HTTP-server begint te vroeg

In de oorspronkelijke firmware riep RFLinkUniversal::setup() `WebServerBase::init()`
aan bij de standaard component-prioriteit DATA (600). Hierdoor ontstond in
`httpd_start -> lwip_socket -> sys_mutex_lock -> xQueueSemaphoreTake` een assert.
ESPHome initialiseert wifi op prioriteit WIFI (250); de officiële ESPHome
web_server gebruikt WIFI - 1 (249). RFLink gebruikt voortaan dezelfde
opstartprioriteit via `get_setup_priority()`. De eigen webinterface blijft
beschikbaar onder `http://<IP>/rflink` zodra de netwerkinitialisatie slaagt.

## 2. Twee RX-kanalen konden niet tegelijk starten

De twee 433 MHz `remote_receiver`-componenten vroegen elk `use_dma: true`.
ESP32-S3 heeft één DMA-capabele RMT RX (kanaal 7). Het tweede exemplaar faalde
met `no free rx channels`. Beide ontvangers gebruiken nu `use_dma: false` en
`rmt_symbols: 96`, samen de 192 RX-symbolen op de S3. Dit is een veilige
resource-indeling om beide kanalen te starten, maar langere RF-bursts kunnen
worden afgekapt. De ontvangstkwaliteit moet met echte modules nog worden
beproefd en zo nodig verbeterd.

## 3. Optionele hardware blijft optioneel

Drie niet-aangesloten CC1101-modules geven ieder `Failed to verify CC1101`.
Dit is op zichzelf geen reden om wifi, webinterface of OTA te laten crashen.
De eerder gemaakte radio-optional-status- en zendchecks blijven aanwezig.

## Let op: er is een andere openstaande observatie

De meegeleverde log zegt `0 decoder(s) registered` ondanks de decoderpacks.
De oorzaak daarvan is nog niet bewezen; decoders in de daadwerkelijke
firmware moeten afzonderlijk op registratie én ontvangst worden getest.
Deze bootfix claimt GEEN oplossing voor dat probleem.

## Herstellen / testen

1. Maak een backup van je persoonlijke `rflink-universal.yaml` en `secrets.yaml`.
2. Importeer de nieuwe `.esphomebundle.tar.gz` en vervang de componentbestanden.
   LET OP: gebruik ook de nieuwe hoofd-YAML, of voeg de TWEE RX-wijzigingen
   handmatig samen via de YAML-updater, met behoud van je eigen pinouts/secrets.
3. De bestaande crashende firmware is mogelijk alleen via USB te herstellen.
   In ESPHome Safe Mode kan OTA soms bereikbaar zijn, maar vertrouw er niet op.
4. Log vanaf een koude start; controleer `setup() finished successfully!` in
   NORMALE modus, geen assert, beide remote_receivers zonder RMT-fout, en de
   verwachte `Failed to verify CC1101` voor afwezige radio's.
5. Test HTTP `http://<IP>/rflink`, ESPHome API, daarna OTA. OTA niet starten
   voordat de webserver/API stabiel blijken.

## Wat daadwerkelijk getest is

- Negatieve bronregressie: oudere build wordt door nieuwe test afgekeurd.
- Positieve bronregressie: aangepaste build is geslaagd.
- C++ host/web-vtable-, YAML-, UI- en overige meeleverde releasechecks.
- Bundle manifest, inhoud, bestandsinventaris, secrets- en toolinguitsluiting.

NIET uitgevoerd: Windows BAT uitvoeren, ESPHome 2026.9.0 firmware volledig
compileren, ESP32 hardware booten, wifi/API/OTA of echte radio-ontvangst meten.


### RMTFIX vervolg
De eerdere bootfix liet de twee `remote_transmitter`-kanalen nog op ESPHome defaults staan: die konden op de ESP32-S3 geen TX-RMT-memory reserveren. Beide 433MHz TX-kanalen zijn nu expliciet 48 symbolen non-DMA; GPIO48 WS2812 ook 48 non-DMA. De 868MHz 2-FSK packet radio blijft behouden en gebruikt geen RMT. Deze wijziging moet zowel in de hoofd-YAML als in het RGB-package terechtkomen bij een update. Het is een configuratiecorrectie; niet op het fysieke board gevalideerd.
