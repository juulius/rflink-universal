# Radio Engine V2 — INCLUDEFIX1 (26 oktober 2026)

## Opgeloste ESPHome-compilatiefout

De standaard ESPHome-build genereert radiocatalogi onder `src/rflink_decoders/`.
De eerdere generator voegde `#include "radio_transport_policy.h"` toe,
terwijl dat bestand onder `src/esphome/components/rflink_universal/` staat.
Daardoor stopte de echte ESP-IDF C++-build met:

`fatal error: radio_transport_policy.h: No such file or directory`

**Oplossing:** beide catalogusgeneratoren krijgen expliciete, bestaande
paden voor de ESPHome-structuur; de lokale host-testlocatie blijft werken.
Dezelfde correctie is toegepast op `radio_contract_v2.h` om de volgende
include-fout te voorkomen.

Bij iedere normale release controleert `maak_release.ps1` nu deze verplichte
includepaden. De test `test_esphome_generated_include_paths.py` bouwt
beide catalogi in een nagebootste ESPHome-mapstructuur en compileert ze met g++.

## Tests

- Beide gegenereerde C++-headers in de ESPHome-mapstructuur: PASS (g++).
- Decodertransport- en manifestvalidatie: PASS.
- Radio Engine V2: 100 decoders, 100 scheduler entries: PASS.
- Decodercatalogus en pakketcompleetheid: PASS.

**Beperking:** deze container heeft geen volledige ESPHome/ESP-IDF-toolchain.
De echte Xtensa/ESPHome-firmwarebuild moet nog worden bevestigd. De vorige
include-fout is rechtstreeks gereproduceerd in de hostlayout en verholpen.

Er zijn geen GPIO-, CC1101-register-, protocoldecoder-, web-UI- of TX-veiligheidsinstellingen gewijzigd.
Gebruik de nieuwe **INCLUDEFIX1** volledige broncode voor `MAAK_RELEASE.bat`;
gebruik niet de oudere `UIFIX1`-bundle uit je bestaande `dist`-map.
