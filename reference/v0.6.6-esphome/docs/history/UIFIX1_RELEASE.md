# Radio Engine V2 — UIFIX1 (26 oktober 2026)

## Opgelost

De Windows `MAAK_RELEASE.bat` stopte vóór ESPHome met:

```
PACKAGING FAILED [RuntimeException, line 21]: Embedded web UI differs from editable HTML or its locale bundle.
```

Oorzaak: in `rflink_universal/components/rflink_universal/web_ui.h` stond al een RX-transporthint in het Advanced-paneel van de decoder, terwijl deze twee JavaScript-regels nog niet waren overgenomen in `rflink_universal/ui/index.html`.

Herstel: alleen `rflink_universal/ui/index.html` bijgewerkt, zodat zijn inhoud na het inbouwen van `ui/locales.bundle.json` byte-voor-byte overeenkomt met de reeds aanwezige embedded HTML in `web_ui.h`.

Er zijn **geen GPIO-instellingen, decoderimplementaties, TX-beveiligingen, firmware YAML of radiocode gewijzigd** ten opzichte van de eerdere Radio Engine V2 SOURCE ZIP.

## Validatie

- Exacte vergelijking `index.html` + `locales.bundle.json` tegen `web_ui.h`: PASS.
- Embedded HTML/SVG/ICO integriteit en UI-/GPIO-/decoder-/release-guardtests: PASS.
- JavaScript syntaxiscontrole (`node --check`): PASS.
- Source ZIP en optionele ESPHome-bundle: archief-integriteit en manifest-inventaris bevestigd.
- Windows PowerShell `MAAK_RELEASE.bat` **niet hier kunnen uitvoeren**; gebruiker test deze stap op Windows.
- Geen daadwerkelijke ESPHome-firmwarecompile of hardwareflash uitgevoerd.

## Windows

1. Pak de **UIFIX1_FULL_SOURCE.zip** uit in een *nieuwe, lege map* (bijvoorbeeld `C:\RFLink`), zodat er geen oude `ui/index.html` uit een vorige versie blijft staan.
2. Voer `MAAK_RELEASE.bat` uit. De standaardrelease wordt in `dist` gezet.
3. Compileren/importeren in ESPHome; pas daarna OTA.

Gebruik geen oude `dist`-bundle. Er is ook alvast een optionele `.esphomebundle.tar.gz` bijgeleverd, gebouwd en op manifest gecontroleerd in deze ontwikkelomgeving (niet met Windows PowerShell).
