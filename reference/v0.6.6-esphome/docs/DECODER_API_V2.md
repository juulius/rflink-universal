## DECODERUX1 – verplichte per-protocol leerinstructies

Naast de bestaande `transmit`-metadata bevat elke `capabilities[]`-entry nu `learn` met `category` (`sensor`, `control` of `development`), `band`, `instruction`, `ha_result` en `warning`. Voeg deze gegevens toe bij nieuwe decoders; `manifest_schema.py` valideert ze en genereert ze automatisch naar de catalogus. Voor gebruikersuitleg en de bewuste beperkingen zie `DECODER_UX1.md`. Dit verandert geen encoder- of zendtoestemming.

# Decoder API v2C – vier RAW-zendencoders met per-apparaat toestemming

**Huidige testrelease (21 september 2026).** RFLink is leidend voor apparaten en goedgekeurde functies. De generieke `tx_engine` kent geen hardgecodeerde Nexa-/Byron-/Dooya-/RC-Switch-takken: protocollen bieden in hun eigen `tx_encoder.cpp` een `prepare`-, `capture`- en `encode`-implementatie aan; `manifest.json` vermeldt alleen werkelijk gekoppelde, gecontroleerde encoders. Alle RF-zendbediening staat standaard uit; `ha_tx_controls: true`, MQTT en expliciete HA- en TX-goedkeuring per apparaat blijven vereist.

- **Nexa**: bestaande Aan/Uit-encoder ongewijzigd.
- **Byron SX**: bestaande encoder voor afzonderlijk goedgekeurde vaste deurbelcode ongewijzigd.
- **Dooya**: encoder voor *exact ontvangen en goedgekeurde `button_N`*, niet automatisch de semantische namen `open`, `close` of `stop`. Het radiokanaal, Dooya-kanaal en de controlecode van **iedere afzonderlijke knop** worden op het moment van zendtoestemming vastgezet. Een nieuwe knop of een latere ontvangen gewijzigde controlecode verandert bestaande zendcommando's niet; zet TX uit, controleer en keur opnieuw goed om de binding te vervangen. Bestaande Dooya-apparaten krijgen `tx_radio` en `check_button_N` eerst als voorstellen wanneer de knoppen opnieuw worden ontvangen.
- **RC-Switch**: uitsluitend een exact ingeleerd vast-code-telegram met expliciet vastgezette `code`, protocolnummer **1–8**, bitlengte **8–64** en 433,92MHz-radio. Geen automatische aan/uit-interpretatie of truc met verzonnen bitlengte. Bestaande RC-Switch-apparaten missen `bit_length`/`tx_radio` totdat opnieuw een passend signaal wordt ontvangen en de velden worden goedgekeurd.

**Decoderbibliotheek:** toont per protocol de werkelijk gekoppelde RX/TX-status uit het manifest. Itho RFT heeft nu native 868,30MHz packet-TX met een eigen virtuele RFLink-zender per ingeleerd apparaat; Somfy RTS gebruikt zijn native virtual-remote/rolling-codepad. KeeLoq blijft zonder TX zolang geen afzonderlijk gevalideerde sleutel-/tellerimplementatie bestaat. Weersensoren blijven bewust ontvangstsensoren.

**Nog te bouwen / fysiek te bevestigen:** (1) persistent functie-/kanaal-/relatiemodel v2 met gecontroleerde migratie; (2) MQTT-wijzigingsqueue bij brokeruitval; (3) webauthenticatie; (4) fysieke hardwaretest van iedere encoder, inclusief Itho RFT op een echte 868,30MHz-unit, en volledige ESPHome/Windows-test. Ontvangen functies worden niet automatisch goedgekeurd of naar Home Assistant geüpload. Een RF-zendaanvraag bewijst geen uitvoering door het doelapparaat.

**Voor bestaande gebruikers:** gebruik de updater voor je persoonlijke YAML en houd de huidige GPIO/OTA-secretwaarden. Bestaande NVS-records en Home Assistant-IDs blijven intact. Schakel zendbediening niet in voordat je ESPHome Validate en Compile hebt uitgevoerd én de relevante RF-modules op echte hardware hebt getest. Dit is géén productieclaim.

---

# Historische v2B-bouwstap – generieke TX-architectuur en verdere migratie

## Het centrale uitgangspunt

RFLink is de bron van waarheid. Een ontvangen signaal kan een voorstel opleveren,
maar publiceert niet automatisch een nieuwe goedgekeurde functie naar Home Assistant.
Een decoder beschrijft alleen de radio-inhoud, identiteit en protocolmogelijkheden.
De centrale RFLink-core doet opslag, toestemming, MQTT, GPIO en webbeheer.

## De indeling: één decoderpack, één manifest

In `rflink_universal/decoders/<pack>/manifest.json` staat nu `schema: 2` en een
lijst `capabilities`. Eén pack mag meerdere protocollen beschrijven: het
bestaande `esphome_builtin` bevat bijvoorbeeld Nexa, ByronSX, Dooya, KeeLoq
en RCSwitch als **afzonderlijke** protocollen met verschillende mogelijkheden.

Per protocol vul je in:

- `protocol`: exact gelijk aan de `DecodeResult.protocol` in `decoder.cpp`.
- `rx_actions`: acties die je decoder levert, bijvoorbeeld `on`, `off` of
  een herkenbare patroonfamilie zoals `button_*`. Een onbekende actie wordt
  door RFLink niet automatisch goedgekeurd.
- `fields`: de technische veld-ID en optioneel `ha`. Alleen velden met
  een expliciet `ha`-profiel kunnen in MQTT Discovery als sensor verschijnen.
  RF-identiteiten, rolling codes en interne zendbindingsgegevens blijven intern.
- `transmit`: `encoder`, teststatus, radio, toegestane acties en verplichte
  gegevens. `default_enabled` MOET `false` zijn.

De in de historische v2B-stap geïmplementeerde en hostgeteste RAW-zendencoders waren
**`nexa`** en **`byron_sx`**. De andere protocollen hebben `encoder: none`,
`status: disabled`, lege TX-acties en `default_enabled: false`.
Een encodernaam in een manifest verzinnen maakt geen zendcode: een gelijknamige,
gekoppelde C++-encoder en geldige radioconfiguratie zijn verplicht.
`host_tested` is uitdrukkelijk GEEN bewijs van verzending op het echte PCB.

## Voorbeeld van het volledige Nexa-capabilitydeel

```json
{
  "protocol": "Nexa",
  "rx_actions": ["on", "off"],
  "fields": [
    {"id": "channel", "ha": null},
    {"id": "group", "ha": null},
    {"id": "level", "ha": null},
    {"id": "tx_radio", "ha": null}
  ],
  "transmit": {
    "encoder": "nexa",
    "status": "host_tested",
    "radio": "433.92",
    "required_fields": ["channel", "group", "level", "tx_radio"],
    "actions": [
      {"id":"on", "label":"Aan", "entity":"button"},
      {"id":"off", "label":"Uit", "entity":"button"}
    ],
    "default_enabled": false
  }
}
```

Dit is alleen het `capabilities[0]`-deel, niet het complete manifest.

## Eenvoudige nieuwe decoder maken

Het Windows-releaseproces gebruikt uitsluitend PowerShell en ingebouwde
Windows-hulpmiddelen; geen Python vooraf installeren. De Python-hulpscripts
hieronder zijn voor ontwikkelaars en worden door ESPHome's eigen Python-
codegenerator niet als handmatige installatie gevraagd.

1. Kopieer het veilige `decoders/_template` met
   `python tools/new_decoder.py <id> "Naam"` vanuit de
   `rflink_universal`-map. De sjabloon is **receive-only**.
2. Werk alleen de nieuwe `decoder.cpp`, `manifest.json` en tests bij.
3. `python tools/generate_catalog.py` herbouwt de fallback
   voor lokale hosttests. `python tools/check_decoder_packs.py`
   controleert of manifest en source overeenkomen. Bij een normale ESPHome-build
   wordt de daadwerkelijke catalogus automatisch opnieuw gegenereerd op basis
   van alle **ingeschakelde** decoderpacks.
4. Voor RX volstaat `decoder.cpp` + `manifest.json` + tests. Voor TX zet je
   **`tx_encoder.cpp` in dezelfde decodermap**. Implementeer
   `tx_engine::Encoder::{protocol,encoder_id,prepare,capture,encode}` en
   `RFLINK_REGISTER_TX_ENCODER(JouwEncoder)`. In `prepare` controleer je
   exacte apparaatidentiteit, goedgekeurde actie, alle velden en
   uitgegeven/vergrendelde TX-binding. `capture` bewaart de benodigde
   `tx_lock_*`-velden bij de expliciete gebruikersgoedkeuring.
5. Werk `manifest.json` (`transmit.encoder`, `status`, `radio`,
   `required_fields`, `actions`) en het conservatieve allowlistcontract in
   `manifest_schema.py` bij. Dit is een **bewuste firmware-reviewstap** voor
   een nieuwe zender; normaal RX-onderhoud raakt de centrale C++ niet.
   De ESPHome-build kopieert en force-linkt alleen de gekozen encoderobjecten.
   Ontbrekende objecten mogen niet stilzwijgend als werkende TX verschijnen.
6. Voer decoder- en transmittests uit, inclusief negatieve acties en ongeldige
   velden. Controleer ESPHome Validate/Compile en test uiteindelijk de radio
   fysiek voordat `hardware_verified` waar mag zijn.

`GET /rflink/api/decoders` levert naast de bestaande decodernamen de
`capability_schema` en de protocolcatalogus. De webinterface toont deze onder
**Instellingen → Decoderbibliotheek**. Het wijzigen van een manifest is
firmwareontwikkeling: opnieuw bouwen en flashen, geen dynamische pluginupload.

## Compatibiliteit / uitgevoerde keuzes

- Bestaande NVS-apparaten, apparaat-UIDs en MQTT-entity-IDs blijven onaangeraakt.
- Goedgekeurde functies en bestaande Nexa-aan/uit blijven zoals in TX1.
- Ontvangstdecoders met `transmit_supported: true` maar zonder getest
  TX-profiel krijgen GEEN HA-zendbediening. Dit is bewust conservatief.
- De updater neemt bekende legacy-schema-1 manifests alleen over als de
  decoder-C++ bron identiek blijft; bij maatwerk moet je decoder en metadata
  zelf beoordelen. De originele configuratie wordt nooit overschreven.

## Wat nu daadwerkelijk werkt in v2B

- Protocolneutrale `tx_engine.h/.cpp`: C++ encoderregistry en `Plan`,
  manifest/actie-controle, immutable binding en radiofrequentievalidatie.
- `decoders/esphome_builtin/tx_encoder.cpp`: Nexa is uit de core verplaatst;
  Byron SX krijgt een encoder met exact de goedgekeurde vaste chimecode (0–15),
  een 8-bit adres en onveranderlijke radioselectie. Commando 16 is verboden.
- De centrale core verwerkt MQTT/HA, toestemming en fysieke verzending zonder
  een Nexa/Byron-specifieke protocolswitch. Beide 433-MHz-zenders hebben een
  generieke configuratieverwijzing, bestaande GPIO's zijn ongewijzigd.
- Aantal geclaimde C++-encoders wordt vergeleken met manifestmetadata; sterke
  linkerreferenties voorkomen stille verdwijnende objecten bij ESP-IDF-linken.
- `GET /rflink/api/decoders` bevat per protocol `tx_registered`; de bestaande
  weergave maakt onderscheid tussen niet gebouwd en hostgetest.
- **Historische v2B-status:** de toenmalige ESPHome-Itho-knoppen stonden nog los van
  de decoder. Vanaf ITHO1 is dit vervangen door een native `IthoRFT` packetencoder,
  per-device virtuele zenderstate, pairing/unpair en centrale web/MQTT-dispatch.

## Wat nog NIET gebouwd is

Dit was **v2B**, dus de onderstaande status is historisch; voor de huidige v2C-status zie bovenaan:

1. Opslag v2 voor afzonderlijke functies, kanalen en relaties tussen meerdere
   afstandsbedieningen en fysieke ontvangers, met bewezen behoud van NVS data.
3. MQTT-wijzigingen opslaan en na brokerherstel gecontroleerd afhandelen.
   HA-gerelateerde verwijderingen zijn voorlopig nog geblokkeerd als MQTT
   offline is, precies zoals in HA2.
4. Authenticatie/autorisatie van de webbediening en afzonderlijke RF-hardware-
   tests per encoder, vooral voor rolling codes. Geen automatische Somfy/
   KeeLoq-zendmogelijkheden zonder afzonderlijke beveiligde implementatie.
4. Dooya vereist validatie van command- en checkbyte per apparaatvariant;
   RC-Switch vereist ook exacte bitlengte en veilige gebruikersbinding.
   Dit beschrijft de historische v2B-status. In de huidige ITHO1-branch is Itho
   geïntegreerd met een afzonderlijke virtuele zenderidentiteit en native packet-TX;
   een ontvangen Itho-ID wordt nog steeds nooit automatisch de eigen TX-identiteit.

Geen Home Assistant-installatie of fysieke CC1101 is hier getest. Valideer en
compileer altijd eerst in je eigen ESPHome 2026.9-omgeving, zonder automatische
flashstap.
