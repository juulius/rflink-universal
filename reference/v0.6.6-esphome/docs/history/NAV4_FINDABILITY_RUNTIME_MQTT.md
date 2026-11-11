# RFLink Universal NAV4 – zoeken, browsergeschiedenis en MQTT via web

Uitgangspunt: NAV3 / RXLOG2 / ACTIVITYFIX1. Radio-pins, OTA, rebootfix, logbuffers en gebruikersbibliotheek zijn niet gewijzigd.

## 1. Apparaten zoeken
- Op Apparaten staan altijd het zoekveld, een radiofilter en Wissen direct in beeld.
- Zoek op naam, protocol, apparaattype, opgeslagen RF-identiteit en radio.
- Radio is gebaseerd op de laatst bekende RX-radio; voor ingeleerde apparaten waarvan dit na een reboot nog niet beschikbaar is wordt uitsluitend een *ondubbelzinnige* ingebouwde protocolfrequentie gebruikt. Overige radio's staan als Onbekend. Zo wordt de verkeerde radio niet verzonnen.
- Ook bij de drie-seconden-refresh blijven de zoektekst en ingestelde filter bewaard. Kandidaten wachten in hun eigen sectie.
- De bestaande knop 'Alleen apparaten met koppelen' blijft functioneren.

## 2. Browser Terug/Vooruit
- Interne routes zijn URL-hashes: `#status`, `#devices`, `#monitor`, `#learn`, `#settings`, `#settings/diagnostics`, `#settings/custom`, `#settings/library`, `#settings/decoders`.
- Directe Status → Diagnose-snelkoppeling maakt maar één browsergeschiedenisstap, geen verborgen tussenpagina.
- Direct linken/bookmarken naar de vier instellingenhulpmiddelen blijft bij herladen werken.
- De browsergeschiedenis bewaart de navigatie, niet het onafgeronde werk van formulieren. Gegevens opslaan is een aparte bewuste actie.

## 3. Home Assistant / MQTT
- Het bestaande ESPHome MQTT-component is voor RF-Discovery nodig. **Een firmware zonder MQTT kan dit niet vanuit de website alleen aanzetten.**
- NAV4 bevat een nieuwe optionele package: `rflink_universal/options/ha_mqtt_runtime.yaml`. Deze compileert MQTT maar start bewust uitgeschakeld (`enable_on_boot: false`, `reboot_timeout: 0s`).
- Om runtime MQTT mogelijk te maken, activeer eenmalig bij `packages:` in de *huidige werkende* YAML:

```yaml
ha_mqtt_runtime: !include rflink_universal/options/ha_mqtt_runtime.yaml
```

- Gebruik **nooit beide** `ha_mqtt` en `ha_mqtt_runtime` tegelijk. Maak een back-up van bestaande MQTT-instellingen en bewaar de bestaande werkende radio-/rebootconfiguratie.
- Na de eenmalige firmwarebuild: Instellingen → Home Assistant & systeem → MQTT instellen. Brokeradres, poort, gebruiker, wachtwoord en ingeschakeld kunnen tijdens runtime worden gewijzigd.
- De instellingen blijven via een aparte NVS-sleutel behouden na herstart. Wachtwoorden worden **niet via GET, diagnostiek of de logexport getoond** en alleen via een POST-body geaccepteerd. Een leeg wachtwoord behoudt alleen een eerder via de webinterface bewaard wachtwoord; bestaande YAML-secrets worden niet automatisch overgenomen.
- Let op: standaard NVS **is niet versleuteld**; gebruik de beheerinterface uitsluitend op een betrouwbaar, afgeschermd IoT-netwerk. De huidige RFLink-webinterface heeft geen volwaardige gebruikersauthenticatie. Gebruik geen publiek internet, exposeer dit systeem niet rechtstreeks aan buiten.
- Bij omschakeling van broker kunnen oude retained Home Assistant Discovery-entiteiten op de oude broker blijven bestaan; ruim die zo nodig bewust op. De native ESPHome API en OTA blijven afzonderlijk functioneren.
- De daadwerkelijke MQTT-verbindingsstatus wordt afzonderlijk van de opgeslagen/ingeschakelde instelling weergegeven.
- ESPHome heeft ondersteuning voor runtime MQTT `set_broker_address`, `set_broker_port`, `set_username`, `set_password`, `disable` en `enable`. Deze code is met host-stubs gecompileerd, **niet** op een echte ESP32 of MQTT-broker beproefd.

## 4. Validatie / risico
- Browsersmoke Chromium op 1680x900, 1280x800 en 390x844: zoeken, radiofilter, zoektekst bij refresh, browser Terug/Vooruit, MQTT POST zonder wachtwoord in URL: **geslaagd**.
- Bestaande NAV3 browserregressie: geslaagd.
- C++ rflink_universal.cpp webcomponent met `USE_ESP32` + `USE_RFLINK_UNIVERSAL_WEB`, met en zonder `USE_MQTT`: hostcompile geslaagd.
- Diverse bestaande RXLOG2 / instellingen / locales en code-contracttests: geslaagd.
- Bestaande uitgebreide localization_browser.py faalt ook in de NAV3-baseline bij het verwachte label van een lege RX-log; dat is geen door NAV4 geïntroduceerde fout, maar wordt niet als geslaagd geteld.
- **Niet uitgevoerd**: echte ESPHome 2026.10.0 build, hardware-SPI/OTA-soak, daadwerkelijke MQTT brokerconnectie, full integration on hardware. Build eerst met je huidige YAML en maak een backup.
