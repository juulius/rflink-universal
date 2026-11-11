# ORCONFAMILY1 — Orcon 15RF-profielen

De 13 protocoldecoders zijn onveranderd. Dit pakket voegt **16 Orcon-apparaatprofielen** toe (15 nieuw en een uitgebreide, bestaande MVS-15R profiel-ID), in totaal 26 ingebouwde apparaatprofielen. Een apparaatprofiel is NIET hetzelfde als een nieuwe RF-decoder.

## Profielen

| Familie | Profiel-ID | Opmerkingen |
|---|---|---|
| Orcon 15RF CO2 inbouwbediening | `orcon_15rf_co2_flush` | Knoppen zichtbaar, TX beveiligd |
| Orcon 15RF CO2 ruimtesensor | `orcon_15rf_co2_room` | RX-only sensordata |
| Orcon 15RF CO2 bedieningssensor | `orcon_15rf_co2_wall` | Knoppen zichtbaar, TX beveiligd |
| Orcon 15RF · model nog kiezen | `orcon_15rf_general` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC display inbouw 15RF | `orcon_15rf_hrc_display` | Knoppen zichtbaar, TX beveiligd |
| Orcon 15RF vocht-inbouwbediening | `orcon_15rf_humidity` | RX-only sensordata |
| Orcon 15RF bewegingssensor | `orcon_15rf_motion` | RX-only sensordata |
| Orcon Compact-10 RF | `orcon_compact10_rf` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC-220 / 225 | `orcon_hrc220_225` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC-300 / 400 legacy | `orcon_hrc300_400_old` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC EcoMax / MaxComfort | `orcon_hrc_ecomax` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC EcoSmart / SmartComfort | `orcon_hrc_smartcomfort` | Knoppen zichtbaar, TX beveiligd |
| Orcon HRC Ventiflow / MaxComfort EU | `orcon_hrc_ventiflow` | Knoppen zichtbaar, TX beveiligd |
| Orcon MPV-10 RF | `orcon_mpv10_rf` | Knoppen zichtbaar, TX beveiligd |
| Orcon MVS-15R / 15RF remote | `orcon_mvs15r_15rf` | Knoppen zichtbaar, TX beveiligd |
| Orcon MVS-15RH / CO2 varianten | `orcon_mvs15rh_family` | Knoppen zichtbaar, TX beveiligd |

## Wat werkt en wat nog niet

- De universele decoder `RamsesII` ontvangt reeds echte geldige frames op deze ESP32-S3. Dat bewijst niet dat alle typen CO2-, RH- en bewegingsmeldingen worden gedecodeerd.
- Het 15RF-radioprotocol alleen identificeert **geen specifiek Orcon-model**; daarom stelt RFLink slechts `Orcon 15RF · model nog kiezen` als *mogelijke* familie voor. Selecteer het exacte type zelf. Alle overige Orcon-modellen zijn zichtbaar maar worden niet automatisch gematcht.
- Zowel bestaande als nieuwe profielen geven RX en TX **verschillende statussen**: knoppen bestaan in de catalogus maar blijven uitgeschakeld zolang de fysieke 868MHz TX/1FC9 koppeling niet volledig is gevalideerd.
- De MVS-15R heeft Afwezig/Auto/1/2/3/15/30/60 minuten. HRC EcoMax/MaxComfort toont ook optionele bypass als **niet-actieve** bediening op basis van een externe HRC400-implementatie. Geen filterreset op MVS.
- Bij de Orcon vocht-, CO2- en bewegingssensoren is het precieze payload-decoding nog niet getest. Een profiel is hier een vindbare, gedocumenteerde apparaatdefinitie, geen belofte dat alle sensorvelden nu werken.
- Nieuwe `setup_steps` per profiel in EN/NL/FR/DE worden getoond als uitklapbare begeleidingsstappen. Dit is nog geen uitvoerende koppelwizard of opstarttimer; die vereist de geplande universele workflow-engine.
- De UI-wijziging koppelt **nu echt** profielkeuze en profielknoppen aan bestaande backend-handlers, maar zenden blijft onder huidige radio- en encoderautorisatie.
- Bestaande gekoppelde apparaten en profielen blijven behouden. `orcon_mvs15r_15rf` ID is niet veranderd. NVS-opslag noch radio-/GPIO-configuratie is gewijzigd. De meegeleverde standaard-YAML staat nog op zijn oorspronkelijke SPI-kloksnelheid; behoud je aangepaste 6 MHz instelling bij een update.

## Bronnen
- Orcon 15RF-afstandsbediening, compatibiliteit en knoppen: https://orcon.nl/download/3933/?tmstv=1701956572
- Orcon sensor- en bedieningsassortiment: https://orcon.nl/product-categorie/woningbouw/toebehoren/bedieningen-en-sensoren/
- Orcon vocht-inbouw: https://orcon.nl/product/vocht-bedieningssensor-inbouw-15rf/
- Orcon beweging-inbouw: https://orcon.nl/product/bewegingssensor-inbouw-15rf/
- Orcon CO2 ruimtesensor: https://orcon.nl/product/co2-ruimtesensor-15rf/
- Orcon HRC display: https://orcon.nl/product/inbouw-hrc-bediening-display-15rf/
- Externe experimentele HRC400 implementatie (geen validatie voor alle modellen): https://github.com/me-processware/orcon-15rf-controller
