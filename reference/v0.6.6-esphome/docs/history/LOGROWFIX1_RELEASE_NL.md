# LOGROWFIX1 — compacte regels in de vaste logviewer

Basis: RFLink Universal v0.5 Radiolove PROFILESAFE1.

## Gebruikersprobleem
Bij één of twee systeemgebeurtenissen verdeelde de logviewer de beschikbare hoogte over de regels. Ze werden honderden pixels hoog. Zodra veel logregels binnenkwamen, werden ze vanzelf compact.

## Oorzaak en oplossing
De globale `.list` gebruikt CSS Grid. In de vaste `#logbookTimeline` werd bij te weinig regels de resterende ruimte automatisch over de gridrijen verdeeld. In `ui/index.html` toegevoegd aan `#logbookTimeline`:

```css
align-content: start;
grid-auto-rows: max-content;
```

Daardoor blijven gesloten regels inhoudshoogte houden en wordt de rest van de logviewer leeg gelaten. Als er voldoende regels zijn, scrollt uitsluitend de logviewer. Uitgeklapte details behouden hun natuurlijke hoogte. De logviewerhoogte (desktop responsive, mobiel 440px) is onveranderd.

`web_ui.h` is opnieuw gegenereerd met `tools/embed_web_ui.py`, dus de wijziging zit in de firmwarewebinterface, niet alleen in een losse HTML.

## Tests
- Chromium 1440px/390px: 1, 2, 5 en 30 logregels blijven compact; paneelhoogte blijft constant; scrollbar pas bij overflow.
- Uitklappen van technische details geeft de rij alleen de benodigde extra ruimte.
- Bestaande LOGBOOK1 browserflows inclusief filters, export, talen en schermbreedtes: PASS.
- PROFILESAFE1 (26 ingebouwde profielen, Orcon-selectie, verwijderbevestiging): PASS.
- PACKFIX1 releaseguard, source-ZIP/tar-integriteit en geheimuitsluiting: PASS.
- Geen wijziging aan RX/TX, flashconfiguratie, opslag, loglimieten of ESPHome API.
- Geen claim van voltooide ESPHome-crosscompile of hardwarevalidatie.

## Installatie
Gebruik de volledige SOURCE-ZIP en je bestaande aangepaste YAML, met behoud van de 6MHz SPI- en GPIO-instellingen. Bouw en flash zoals gebruikelijk. Firmware `firmware_revision` eindigt daarna op `-LOGROWFIX1`.
