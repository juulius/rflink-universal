# RFLink Universal — SYSDEBUG1 / LOGBOOK1 (experimentele TX-diagnose)

## Wat is toegevoegd?

**Logboeken** bereikbaar vanaf de hoofdnavigatie, met vier direct schakelbare weergaven:

1. **Alles:** gecombineerde chronologische tijdlijn van de drie bronnen; standaard zonder ruis.
2. **Radio:** de bestaande radio-eventviewer blijft volledig beschikbaar met per-radio filters, signalen/ruis en eigen ZIP-export.
3. **Systeem:** firmwarebeslissingen en acties: decoderselectie, inleren/opslaan, profielwijzigingen, HA-publicatie, TX-router, GDO UART sessie en terugkeer naar RX, plus foutmeldingen waar deze instrumentatie al bestaat.
4. **Web:** acties, navigatie, wijzigingen aan interfacebediening, API-aanvragen (duur/status als debug ingeschakeld), browserfouten en netwerkfouten.

**Instellingen → Logging:** drie radiosignaal-limieten en de gedeelde ruislimiet blijven werken zoals daarvoor (persistente ESP32-instellingen). Nieuw: systeemtrace 0–256 events (standaard 128, runtime RAM; reset bij reboot), weblog 0–2000 events (standaard 300, *alleen deze browser*, limiet en detailniveau via localStorage onthouden).

Het gecombineerde log is **geen vierde opgeslagen kopie**: de browser voegt on demand max. 80 recente radiologregels, max. 256 systeemregels en max. 2000 browserregels samen, waarvan maximaal 250 tegelijk worden getoond. De losse radiolog ondersteunt bestaande paginering en is niet beperkt tot die 80 regels.

### Tijdlijn en relaties

De radio- en systeemlogs gebruiken dezelfde ESP32-uptime. De browser logt met een tijdsschatting die bij elke opgehaalde systeemtrace wordt gesynchroniseerd; daarop is jitter/afwijking mogelijk. Apparaat-ID's komen mee in bestaande systeemtrace-registraties en in webklikgebeurtenissen bij apparaatacties. **Een echte end-to-end unieke transactie-ID is nog niet voor alle subsystemen geïmplementeerd**; gebruik apparaat-ID en tijd bij debugging. De browserbewaring is sessiegebonden en wordt niet vanuit een tweede computer zichtbaar.

### Praktisch debuggen

1. Open **Logboeken → Alles**, zet **Live** aan en laat **Ruis tonen** uit.
2. Probeer de gewenste bediening. Filter desgewenst op apparaat-ID, `tx`, `learn`, `radio` of `error`.
3. Kijk of het probleem optreedt bij webactie, decoder/TX-regel, radio reserveren, UART, of RX-herstel.
4. Open details om fouttekst, UID en sequence-ID te zien.
5. Download **Weergave exporteren** (JSON), of kies **Radio** voor de volledige eigen radiolog-ZIP.

Bij tijdelijk veel informatie kan in Instellingen **Webdebug: ook geslaagde automatische API-verversingen** worden ingeschakeld. Normaal worden snelle succesvolle GET-polls niet opgeslagen om ruis in Weblog en CPU-belasting te beperken.

## Experimentele fysieke zendtest: uitsluitend eigen apparatuur

Deze source combineert de eerdere SYSDEBUG1 handmatige **RAMSES-II 10E0 labtest** met LOGBOOK1. Dit is **geen Orcon stand-, pairing- of bedieningsopdracht**. De bestaande `RamsesII` manifestregels `encoder: none` en `status: disabled` blijven intact. De gewone Orcon-knoppen blijven daarom terecht uitgeschakeld.

1. **Bewaar je bestaande YAML, 6-MHz SPI-keuze, andere GPIO's en werkende firmware.** De meegeleverde standaardconfiguratie is geen vervanging voor jouw eigen PCB-configuratie.
2. Compileer eerst met `gdo_serial_tx_lab: false` en `ramses_tx_probe_lab: false` en controleer alle drie radio's, RX, OTA en de gecombineerde logs.
3. Voor een **bewust gekozen hardware-labtest** kun je in de eigen YAML de beide opties `gdo_serial_tx_lab: true` en `ramses_tx_probe_lab: true` zetten. Controleer de extra `gdo_serial_tx_gpio` instelling en fysieke GDO0-pinout voor de 868 MHz-radio. Zet dit niet per ongeluk op een andere radio.
4. Compileer/flash na ESPHome-validatie. Open **Instellingen → System trace → Advanced: physical RAMSES transmission test**. De status moet **ready** tonen; anders is de knop terecht geblokkeerd.
5. Gebruik de expliciete bevestiging voor **Send one test frame**. Verifieer fysiek RF en timing met een tweede ontvanger of meetinstrument. Een softwaremelding `sent` betekent niet dat het radiopakket echt is uitgezonden of door een apparaat is uitgevoerd.
6. Kijk in **Logboeken → Alles** welke stappen in de firmware zijn uitgevoerd (acquire, prepare, UART send, RX restore). Stopt de driver of blijft een fault actief, stop de TX-tests en exporteer logs. Controleer ook of Itho/RX weer ontvangt.
7. Zet na het experiment de YAML-labflags weer uit zolang de fysieke verificatie nog niet bewezen is.

## Testresultaten van deze build

- LOGBOOK1 Chromium-browser: 1440 px en 390 px, gecombineerd/radio/systeem/web, filters, zoeken, ruis, open details tijdens refresh, export, runtime-loglimiet, vier talen/responsief: **PASS**.
- SYSDEBUG1 bestaande Chromium-browser: 1440/390 px, disabled testknop, lab-success-mock, export en vertalingen: **PASS**.
- UX3 gebruikersroutes, NAV4 browser, PAIRING1 browser, RX-loggingcontract, settings compact DOM, firmwareversie: **PASS**.
- Systeemtrace C++: deduplicatie, limieten 0/20/128/256, ringbuffer, truncate message: **PASS**.
- GDO TX sessie C++: sessiebeveiliging, herstel, fout-afhandeling: **PASS**.
- Boardguard host-stubs: **PASS**.

**Niet getest**: complete doelplatform-ESPHome/ESP-IDF build, flash/OTA, fysieke TX-golfvorm, werkelijke Orcon-koppeling of bediening, langdurige RAM/PSRAM-soak en een volledige beveiligingsaudit. Deze release is een debug-/labbronpakket, geen hardwarebevestigde stabiele firmware.

## Grenzen en beveiliging

De bestaande RFLink web-API heeft nog geen betrouwbare gebruikersauthenticatie. HTTP schrijftokens die via een GET terugkomen zijn niet gelijk aan toegangscontrole. Gebruik dit **alleen op een vertrouwd LAN; publiceer de beheer-API niet op het internet**. Exportbestanden kunnen RF-identiteiten bevatten; bekijk vóór delen. Geen RAW-radioframes, tokens of wachtwoorden worden opzettelijk in de nieuwe systeem- of weblog opgenomen.

De systeemtrace is bewust beperkt tot maximaal 256 vaste records omdat de ESP32 veilig moet blijven functioneren. Een eventuele grotere PSRAM-backed trace en echt gedeelde transactie-ID's verdienen een aparte geheugen- en architectuurtest.
