# ACTIVITYFIX1 – 2026-10-25

## Probleem
Bij automatische verversing van **Activiteiten** werd de volledige lijst vervangen via `innerHTML`. Daardoor klapten geopende technische details van RX-signalen of ruis elke paar seconden dicht en verschoof de leespositie.

## Oplossing
- Gebruik stabiele sleutels: de RX-volgnummer-ID (`seq`), of voor legacy/TX-activiteiten hun originele opnametijd plus brongegevens.
- Bestaande DOM-kaarten worden hergebruikt in plaats van opnieuw opgebouwd. Alleen nieuwe en verdwenen regels worden aangemaakt/verwijderd.
- Geopende records blijven zichtbaar, ook wanneer ze door nieuwe ontvangsten net buiten de nieuwste 20 API-regels vallen. Zodra je ze sluit, worden ze bij de volgende verversing normaal opgeschoond.
- De browser corrigeert zijn scrollpositie wanneer er regels vóór het geopende record bijkomen.
- Bij wisselen van radio, logtype, richting, taal of boot-sessie worden de oude paginaresultaten terecht losgelaten.
- De bestaande RXLOG2-API, NVS-instellingen en geheugenlimieten blijven ongewijzigd.

## Controle
- Browserregressie `tests/test_monitor_expanded_persistence.py`: automatisch verversen, 20-regels-paginering, identieke DOM-node, behoud van scrollpositie, sluiten van oude pinned regels, nieuwe filterkeuze: geslaagd.
- Bestaande RX-log-browser-, logging-integratie-, RXLOG2- en compact-settings-tests: geslaagd.
- Geen volledige ESP32/ESPHome build en geen echte OTA uitgevoerd; compilatie en testen op het apparaat blijven noodzakelijk.

## Installatie
Gebruik de volledige broncode-ZIP met je **bestaande werkende YAML**. De YAML in deze ZIP is ongewijzigd gebleven ten opzichte van RXLOG2; je hoeft geen GPIO's of loginstellingen aan te passen.
Na compileren/OTA: open Activiteiten → Ruis, klap een regel open en laat nieuwe signalen binnenkomen. Het record moet geopend blijven.
