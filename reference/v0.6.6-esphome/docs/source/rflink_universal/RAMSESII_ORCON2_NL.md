# Orcon MVS-15R / 15RF — RX + TX-voorbereiding (RAMSES2)

## Wat werkt in deze versie?

**Firmware RX blijft hetzelfde als RAMSES1.** De bestaande Itho-, Somfy- en 433 MHz-radio-instellingen en je persoonlijke YAML blijven ongewijzigd. Deze update bevat nu een zelfstandige, compileerbare **RAMSES-II TX-codec** met:

- `22F1` Afwezig / Stand 1 / Stand 2 / Stand 3 / Auto;
- `22F3` timer 15, 30 en 60 minuten;
- `1FC9` koppelverzoek, bevestiging en verwerking van de reactie;
- `10E0` Orcon 15RF apparaatinfo;
- logische RAMSES-frameopbouw, 8-bit checksum, Manchester en UART-8N1 on-air framing;
- een koppelstatusmachine met time-out en harde controle op de virtuele afstandsbediening-ID en het ventilatiebox-ID;
- softwaretests voor alle bovenstaande zaken.

**Nog niet fysiek actief:** de TX-codec is geen radiozenddriver. Onze ESPHome/CC1101-radio draait momenteel in vaste 63-byte Itho-pakketmodus, terwijl RAMSES-II een 38,4 kbit/s continue (asynchrone) datastroom over CC1101/GDO en een antwoordkanaal vereist. Een `22F1`-pakket door de Itho `transmit_packet()` sturen zou dus fout zijn. Daarom is in `manifest.json` RX-only gehandhaafd en zijn er nog geen Orcon-TX-knoppen zichtbaar. Dit is opzettelijk. *Niet flashen in de verwachting dat zenden al werkt.*

## Koppelen van RFLink aan een Orcon MVS-15R (nadat de asynchrone radio-implementatie gereed is)

De Orcon MVS-15R accepteert een extra 15RF-afstandsbediening via een koppelvenster van 3 minuten na herinschakelen.

1. Controleer dat je gewone Orcon-bediening nog werkt en laat die gekoppeld.
2. Trek de stekker van de ventilatiebox circa 10 seconden uit het stopcontact. Steek weer in. De groene status-LED brandt tijdens het inleren.
3. Open straks in RFLink Universal **Apparaten → Orcon MVS-15R → Meer beheren → RFLink koppelen**. Dit UI-onderdeel is in deze versie **nog niet actief**.
4. RFLink moet een **nieuwe, persistent bewaarde 37:xxxxxx-ID** gebruiken; gebruik nooit de ID van je bestaande handzender.
5. RFLink verstuurt een `1FC9` aanbod voor `22F1`, `22F3`, `10E0`, wacht op het `1FC9`-antwoord van de MVS en verstuurt de bevestiging en `10E0`-apparaatinfo. De koppelstatus mag pas `Gekoppeld` worden nadat de gebruiker of de ventilatiebox dit daadwerkelijk heeft bevestigd.
6. Orcon: bij succesvolle koppeling knippert de groene LED circa tien keer en toert de ventilator kort op.
7. Test vervolgens **Stand 1 → Stand 2 → Stand 3 → Auto**. Zet zenden uitsluitend vrij na de normale RFLink per-apparaat goedkeuring en controleer dat de box reageert.

Als de koppeling niet lukt: kijk of de drie minuten verstreken zijn; herstart de koppelprocedure, maar verwijder **niet** je huidige afstandsbediening en doe geen fabrieksreset.

### Wat is nodig voor een echte TX-release?

- Een apart, hardware-getest CC1101-preset voor RAMSES-II (868,3 MHz 2-FSK, ongeveer 38,4 kbps / 50,8 kHz deviatie), plus UART TX via GDO0 en RX via GDO2.
- De ESP32-S3-GPIO-matrix aan de aangesloten GDO-pinnen koppelen; GPIO16/17 zijn in deze YAML de 868-MHz GDO0/GDO2.
- Veilige radio-arbitrage (één 868-MHz-radio): tijdelijk naar RAMSES schakelen, Itho pauzeren, registers/sensorpaden volledig herstellen, foutpad/failsafe, watchdog, geen dubbele claims op pinnen.
- Een persistente, willekeurig toegewezen **virtuele Orcon-ID**, een vertrouwde ventilatiebox-identiteit, koppelstatus, herstel/back-up, deduplicatie van inkomende replies.
- Een niet-blokkerende pairingwizard in webinterface, bestaande TX-toestemming en MQTT/Home Assistant via de centrale `tx_engine`.
- ESPHome Validate+Compile én fysieke radiotests met de echte MVS-15R; controleren dat andere radios en OTA niet achteruitgaan.

## Technische bestanden

- `rflink_universal/decoders/ramses_ii/tx_codec.h/.cpp` — protocol TX, onafhankelijk van ESPHome.
- `rflink_universal/tests/test_ramses_ii_tx.cpp` — hosttests incl. volledige RX-roundtrip en simulatie pairing.
- `rflink_universal/tests/run_tests.sh` — uitvoering van de nieuwe TX-codec tests.

### Bronvermelding

Protocoldetails en voorbeelden uit [ramses_cc wiki, Faking Remotes/Orcon](https://github.com/ramses-rf/ramses_cc/wiki/5.2-Faking-Remotes-%28switches%29) en [Orcon 15RF-controller, MIT](https://github.com/me-processware/orcon-15rf-controller). De RAMSES-II-pakketten zijn nieuw geschreven in het bestaande RFLink-pack. Voor de 15RF- en MVS-15R-handleiding: [Orcon 15RF](https://orcon.nl/download/3933/?tmstv=1701956572) en [MVS-15R](https://orcon.nl/wp-content/uploads/sites/7/Downloads/installatiehandleiding-mvs-15-r-v7-interactief.pdf).

Protocolvariant-aantekening: een aantal HRC400-voorbeelden gebruikt een afwijkende derde `22F1`-payloadbyte of andere `22F3`-timerbytes. Deze voorbereiding gebruikt de gepubliceerde *MVS-15R* `ramses_cc`-voorbeelden (`000004` / `00020F03040000`); echte controle op de aangesloten MVS-15R blijft verplicht.
