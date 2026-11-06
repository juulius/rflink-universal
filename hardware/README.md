# RFLink Universal — Hardware

In deze map komen later de **echte printplaatbestanden** en de **onderdelenlijst (BOM / Bill of Materials)**.

> Deze map bevat momenteel **nog geen gecontroleerde productie- of assemblagebestanden**. Bestel nog geen PCB's op basis van deze documentatie.

## Wat voegen we later toe?

| Bestand / map | Waarvoor bedoeld? |
| --- | --- |
| `kicad/` | Bewerkbaar elektrisch schema en PCB-ontwerp in KiCad |
| `bom/` | Onderdelenlijst met aantallen, waarden, behuizingen en leveranciers-/onderdeelnummers |
| `manufacturing/` | Gerbers, boorbestanden en eventuele assemblagebestanden voor een printplatenfabrikant |
| `images/` | Duidelijke foto's of renders van de print en aansluitingen |

Deze mappen maken we pas aan als de echte bestanden beschikbaar en gecontroleerd zijn.

## Belangrijk voor eenvoudige installatie

We willen bij elke printrevisie vastleggen:

1. **Boardrevisie** (bijvoorbeeld Rev A); zo weet je welke print je hebt.
2. **Pinout**; welke ESP32-aansluitingen naar welke CC1101 gaan.
3. **Onderdelenlijst (BOM)**; wat je nodig hebt om de print te bouwen.
4. **Assemblage-instructies**; waar de onderdelen komen en hoe ze aangesloten worden.
5. **Bijpassend firmware-boardprofiel**; zodat je niet onbedoeld firmware voor een andere print gebruikt.

De repository bevat later alleen **standaardinstellingen**. Persoonlijke overrides en geheimen blijven lokaal in RFLink Builder.

## Wat moet er voor publicatie worden gecontroleerd?

- Klopt het KiCad-schema met de daadwerkelijk geteste print?
- Zijn aansluitingen, spanningen en CC1101-varianten correct?
- Zijn de BOM-onderdelen leverbaar en passen de footprints?
- Zijn Gerbers en boorbestanden aantoonbaar van dezelfde boardrevisie?
- Kloppen het boardprofiel en de firmware-pinnen met die revisie?
- Zijn alle bestanden geschikt om openbaar te delen?

Een BOM beschrijft de echte, bevestigde onderdelen: we vullen geen ontbrekende partnummers of prijzen in op basis van aannames.
