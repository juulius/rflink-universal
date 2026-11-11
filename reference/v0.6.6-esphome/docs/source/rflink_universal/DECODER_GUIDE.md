# Zelf een decoder maken / Creating a decoder

> Dit document is voor **Decoder maken / Developer Mode**. Voor normaal gebruik is `START_HERE.md` genoeg.

## De makkelijkste start

Maak nooit handmatig een kopie ergens in de RFLink-core. Gebruik:

```bash
python tools/new_decoder.py hood_remote_868 "Hood Remote 868"
```

Dit maakt één zelfstandige map:

```text
decoders/hood_remote_868/
├── decoder.cpp
├── manifest.json
├── README.md
└── tests/
```

De RFLink-build ontdekt alle ingeschakelde decoderpacks uit `decoders/` automatisch. Je verandert dus **geen** centrale decoderlijst of constants-file.

## Wat hoef je meestal maar te wijzigen?

### 1. `decoder.cpp`

Hier beantwoord je twee vragen:

- **`match()`** — lijkt dit RF-bericht betrouwbaar op mijn protocol?
- **`decode()`** — welk fysiek apparaat is dit, wat gebeurde er en welke waarden zitten erin?

De vaste regel is:

```text
RFFrame -> DecodeResult
```

Een decoder mag niet rechtstreeks de webinterface, MQTT/Home Assistant, GPIO's of een andere decoder besturen.

### 2. `manifest.json`

Hier staat sinds Decoder API v2 **wat de decoder werkelijk aanbiedt**. `manifest.json`
beschrijft protocolnamen, ontvangstacties, datavelden, Home Assistant-entiteittypen,
en eventuele zenders. Het volledige voorbeeld staat in `DECODER_API_V2.md` in de SOURCE-zip.

Een minimale, nieuwe receive-only decoder ziet er ongeveer zo uit (de bestaande
algemene manifestvelden blijven verplicht):

```json
{
  "schema": 2,
  "id": "hood_remote_868",
  "capabilities": [{
    "protocol": "HoodRemote868",
    "rx_actions": ["button_1"],
    "fields": [{"id": "battery_low", "ha": {
      "label": "Batterij bijna leeg", "type": "binary_sensor",
      "device_class": "battery", "unit": ""
    }}],
    "transmit": {"encoder": "none", "status": "disabled", "actions": [], "default_enabled": false}
  }]
}
```

Dit is bewust een *fragment*; gebruik `tools/new_decoder.py` om een volledige
schema-2-map te maken. De parser, de catalogus en de compiler controleren of de
identificatie en velden overeenkomen met de decoderbron. Een manifest op zichzelf
bouwt geen RF-zendencoder.

Zet `enabled` op `false` als je tijdelijk een decoderpack uit de firmware wilt houden zonder bestanden te verwijderen.

### 3. `tests/`

Bewaar representatieve captures/tests. Minimaal nuttig:

- dezelfde knop meerdere keren;
- alle bekende knoppen;
- een bericht met veranderende counter/rolling-code indien aanwezig;
- een fout/beschadigd bericht dat juist **niet** mag matchen;
- later eventueel een tweede fysiek exemplaar.

## RF-basisbegrippen / RF basics

- **Puls / Pulse** — één periode waarin het ontvangen signaal HIGH of LOW is.
- **Pulslengte / Pulse width** — duur van die puls, meestal in microseconden (µs).
- **Bit** — 0 of 1. Eén bit bestaat vaak uit twee pulsen.
- **Telegram / Frame** — één compleet RF-bericht.
- **Bitstream** — reeks 0/1 na omzetting van pulsen.
- **RSSI** — ontvangen signaalsterkte. Nuttige leerfactor, nooit een device-identiteit.
- **Rolling counter/code** — veranderend veld; hoort niet in `identity_key`.

Voorbeeld:

```text
+350 -1050 -> mogelijk bit 0
+1050 -350 -> mogelijk bit 1
```

## Device identity — dit is belangrijk

Gebruik alleen gegevens die één fysiek apparaat stabiel identificeren:

```cpp
result.identity_key = "mijn_protocol:1234";
```

Niet opnemen in identity:

- ON/OFF;
- knopnummer;
- kanaal als dat slechts een functie is;
- checksum;
- message counter;
- rolling code.

Als je met één afstandsbediening nog niet kunt bewijzen dat een veld werkelijk uniek is, gebruik dan `IdentityConfidence::LOW` of `MEDIUM`. De decoder mag alsnog bruikbaar zijn.

## Developer Mode gebruiken

1. Kies de juiste radio.
2. Houd het testapparaat ongeveer binnen 1 m van die antenne.
3. Meet eerst de achtergrond zonder iets te activeren.
4. Voer één actie meerdere keren uit.
5. RFLink filtert bekende/baseline-signalen en groepeert repeats.
6. Bekijk protocol, mogelijke identiteit, RSSI waar beschikbaar en het codevoorstel.
7. Leer andere knoppen/functies één voor één.
8. Zet de bewezen logica in alleen jouw decoderpack.

Met één fysieke afstandsbediening kan veel worden afgeleid. Een tweede exemplaar is extra bewijs, geen vereiste.

## Buildcheck

Controleer decoderpacks met:

```bash
python tools/check_decoder_packs.py
./tests/run_tests.sh
```

`check_decoder_packs.py` controleert onder andere mapnaam, manifest, self-registration en de stabiele include-route naar de RFLink-core.
