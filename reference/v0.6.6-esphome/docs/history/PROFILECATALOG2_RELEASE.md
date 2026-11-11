# PROFILECATALOG2 — universele profiel- en familiebibliotheek

Deze release bouwt voort op ORCONFAMILY1; decoder- en RF-TX-gedrag verandert niet.

## Nieuwe bediening

Open **Instellingen → Decoderbibliotheek**. Kies bovenaan **Protocollen** of **Apparaatprofielen**.

- **Protocollen**: dezelfde 13 decoders als voorheen; bij een decoder staat het aantal gerelateerde apparaatprofielen. Klik **Bekijk apparaatprofielen** om met protocolfilter te openen.
- **Apparaatprofielen**: alle 26 ingebouwde profielen plus opgeslagen eigen profielen doorzoekbaar, gegroepeerd per fabrikant en familie. Filters: merk, familie en protocol; vrije zoektekst doorzoekt ook model-aliassen. Elk item toont de bijbehorende protocoldecoder en een doorkliklink naar die decoder.
- De 16 Orcon-profielen zijn gegroepeerd in MVS, HRC, Compact, MPV, en 15RF bedieningen en sensoren. MVS-15R behoudt zijn profiel-ID.
- Uitgebreide inleerhulp en bronnen zijn per profiel op aanvraag te openen. Bediening blijft uitgeschakeld waar zendhardware of koppeling nog niet bewezen is.
- Optionele metadata `family` is nu verplicht voor *ingebouwde* JSON-profielen. Eigen profielen zonder `family` worden in 'Eigen apparaatprofielen' geplaatst; er is geen NVS-migratie.
- De GUI toont Auto en Afwezig nu begrijpelijk in NL/EN/FR/DE.

## Installatie

Gebruik de **PATCH ZIP over ORCONFAMILY1 SOURCE** in dezelfde map (oude bestanden overschrijven). Laat `rflink-universal.yaml` ongemoeid, vooral bij custom SPI op 6 MHz. Voer `MAAK_RELEASE.bat` uit en compileer in ESPHome voordat je OTA gebruikt. FULL_SOURCE en DEFAULT ESPHome-bundle bevatten de standaard YAML (200000 Hz, als die standaard blijft).

## Teststatus

Geslaagd: profielcatalogusgenerator (26 profielen, 16 Orcon), Arduino/ESPHome host-header C++ syntaxis, 13 decoder-metadata, bestaande officiële-profieltests, 4 talen en JS, echte Chromium-browserinteractie op 1300px en 390px, beide navigatierichtingen en filters, eenvoudige UI-regressie, library UX, release/board guards en pakketintegriteit.

De oorspronkelijke host-websmoke-run met `-DUSE_ESP32` blijft hangen op een reeds ontbrekende `driver/uart.h` stub. De oudere Itho/Somfy-behavior fixtures controleren de voormalige losse apparaatkaarten, die in PROFILEFLOW1 zijn vervangen; deze zijn niet leidend voor deze cataloguswijziging. Er is **geen volledige ESPHome/ESP-IDF build of nieuwe fysieke radio-/TX-test** uitgevoerd.

Er is nog **geen uitvoerende universele koppelwizard**; dit pakket verbetert het overzicht van apparaatprofielen en hun decoderrelatie.
