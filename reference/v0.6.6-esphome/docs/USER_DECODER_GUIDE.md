# Eigen RFLink-decoders en apparaatprofielen

## Werken vanaf de webinterface

1. Open **Learn → Create decoder**. Kies de juiste beschikbare radio. Meet met **Measure background** eerst ongeveer vijf seconden het achtergrondverkeer.
2. Geef de decoder een naam en een vaste ID die met `user_` begint, bijvoorbeeld `user_remote_keuken`. Schrijf een actienaam zoals `on` en kies **Record this action**. Druk die knop minstens drie keer in. Herhaal dit voor bijvoorbeeld `off`. Gebruik **Clear recordings** om helemaal opnieuw te beginnen.
3. Kies **Test and activate decoder**. RFLink maakt een interpreterrecept, vergelijkt alle opgenomen voorbeelden met de verwachte actie, controleert botsingen met bestaande decoders en slaat de decoder in twee NVS-snapshots op. Daarna ontvangt hij direct nieuwe signalen. Bij een onbekende of wisselende code kan de automatische test mislukken; corrigeer de regels onder **Advanced**.
4. Maak onder **Settings → My decoders and device profiles** eventueel een modelprofiel met knoppen en inleerhulp. Bij één passend profiel wordt dit automatisch aan een nieuw bevestigd apparaat gekoppeld; bij meerdere kies je het profiel onder **Devices**.
5. Bevestig een nieuw apparaat zoals gebruikelijk onder **Devices → New signals**. Bij een decoder met TX-regel verschijnt daar **Preview TX**. Dat berekent alleen de uit te zenden data. Kies daarna per apparaat **Allow sending** en **Send**. De radio-opdracht bevestigt niet dat het doelapparaat reageerde.

Vaste RAW-codes tot de begrensde VM-capaciteit (door compacte `MARKUS`/`SPACEUS`-instructies nu aanzienlijk langer dan USERLIB1) en 63-byte packet-opnames krijgen meestal een automatische TX-regel. Bij sterk wisselende telegrammen wordt automatische TX weggelaten. Een herhaalde opname van een rolling code is geen geldig nieuw rolling-codecommando. Voor versleutelde of onbekende protocollen zijn algoritme, sleutels, synchronisatie en testen op echte hardware noodzakelijk.

## Interpreterregels (Advanced)

Elke instructie staat op een nieuwe regel. Commentaar begint met `#`. Maximaal 4096 tekstbytes, 192 instructies, 2048 uitgevoerde stappen en 32 stapelwaarden. Een decoder kan geen eigen C++-code, netwerktoegang, GPIO of bestandsacties uitvoeren.

| Instructie | Betekenis |
|---|---|
| `LEN`, `BYTE n`, `PULSE n` | Plaats lengte, packetbyte of signed RAW-puls op de stapel. |
| `MATCHBYTE n v`, `MATCHRAW n v` | Vergelijk een positie; RAW heeft 25% timingruimte, minimaal 100 µs. Plaatst 1 of 0 op de stapel. |
| `CONST n`, `DUP`, `DROP`, `ABS`, `NOT` | Stapelbewerkingen. |
| `ADD`, `SUB`, `AND`, `OR`, `XOR`, `EQ`, `LT`, `GT`, `SHL`, `SHR` | Bewerk twee stapelwaarden en plaats het resultaat terug. |
| `JMP n`, `JZ n`, `REQUIRE`, `HALT` | Spring naar instructienummer vanaf 0; `JZ` springt bij 0; `REQUIRE` verwerpt bij 0; `HALT` verwerpt. |
| `ID`, `ACTION naam`, `FIELD naam eenheid`, `ACCEPT` | RX: stel identiteit en resultaat samen. `ACCEPT` moet de laatste instructie zijn. |
| `ACTEQ naam`, `STATE naam`, `BUMP naam` | TX: vergelijk gevraagde actie; lees een teller; verhoog een per-apparaat teller. `BUMP` bewaart de volgende waarde in NVS vóór uitzending. |
| `MARK`, `SPACE`, `EMIT_BYTE`, `REPEATS n`, `SEND` | TX: legacy stapelvorm voor pulsen/bytes, 1–10 herhalingen. |
| `MARKUS n`, `SPACEUS n`, `BYTEVAL n` | TX: compacte directe puls/byte-instructies; één vaste puls of byte kost één VM-instructie. `SEND` rondt de uitvoer af. |

Voorbeeld van een eenvoudige RX-regel:

```text
LEN
CONST 4
EQ
REQUIRE
MATCHRAW 0 500
REQUIRE
CONST 123
ID
ACTION on
ACCEPT
```

Bij `STATE` en `BUMP` wordt de teller per apparaat en per decoder opgeslagen. Het formaat en de versleuteling van een echt rolling-codeprotocol moeten nog steeds correct worden geïmplementeerd. TX voor de 868 MHz packet-radio vereist precies 63 bytes omdat de meegeleverde CC1101-configuratie `packet_length: 63` gebruikt. Beide 433 MHz kanalen gebruiken signed RAW-pulsen. De API noemt deze contracten `board_packet_63` en `board_raw`. Zie `RADIO_PROFILES.md`: USERLIB2 laat bewust nog geen willekeurige CC1101-registers vanuit user recipes toe.

## Exporteren, evalueren en toevoegen aan firmware

**Convert to firmware decoder → Download evaluation export** downloadt één JSON-bestand met gebruikersdecoders, voorbeelden, profielen, protocoltellers en een momentopname van opgeslagen apparaten. Bewaar dit bestand zelf; het bevat RF-codes en mogelijke tellers. **Restore library backup** kan USERLIB2-exports weer inlezen. Elk decoderrecept wordt opnieuw gecompileerd en gevalideerd; protocoltellers mogen alleen vooruit. Zendtoestemming wordt nooit automatisch hersteld. Een profielbinding aan een bestaand apparaat wordt alleen teruggezet wanneer UID én protocol nog overeenkomen.

In de uitgepakte bronmap:

```sh
python3 rflink_universal/tools/promote_user_library.py rflink-user-library-export.json --check
python3 rflink_universal/tools/promote_user_library.py rflink-user-library-export.json --merge
```

Op Windows kan `rflink_universal\tools\promote_user_library.ps1 -Backup .\rflink-user-library-export.json` controleren; voeg `-Merge` toe om de vaste recepten toe te voegen. De wrapper zoekt eerst de Python-launcher en daarna onder andere de Python-runtime van PlatformIO. Backup/restore en dagelijks aanleren in de webinterface hebben geen Python nodig; alleen maintainer-promotie gebruikt Python en `g++` voor de hostvalidatie.

`--check` wijzigt niets: het valideert grenzen, compileert de gegenereerde C++-header met de echte interpreter, speelt RX/TX-voorbeelden na en weigert ongerelateerde framelengten. `--merge` voegt de recepten toe aan `firmware_user_recipes.h` en houdt een manifest met dezelfde IDs bij. Een gewijzigd bestaand recept wordt geblokkeerd; na beoordeling kan `--replace user_naam` het expliciet vervangen. Met `--candidate-out review` (PowerShell: `-CandidateOut review`) wordt daarnaast per user-decoder een **uitgeschakelde RX-only decoder-packcandidate** gemaakt met `decoder.cpp`, `manifest.json`, `README.md` en het oorspronkelijke recipe. Dit is een reviewstap, geen automatische native-TX-vrijgave.

Firmware-recepten worden bij het opstarten apart geladen en lezen de lokale NVS-collectie niet over. Bij dezelfde ID gebruikt RFLink het lokale gebruikersrecept; onder **Settings → Library & updates** verschijnt een inhoudsconflict ter beoordeling. Het verwijderen of bijwerken van firmware-recepten vergt een nieuwe firmwarebuild. Nieuwe gebruikersrecepten en profielen op het apparaat blijven normaal gesproken bij een OTA-update in NVS. Een gewijzigde partitietabel of het wissen van flash kan NVS wissen; automatische terugzetting uit de export ontbreekt nog.

## Grenzen en verificatie

- De ingebouwde webserver is bedoeld voor een vertrouwd lokaal netwerk en heeft in deze versie geen aparte beheerlogin. Stel hem niet rechtstreeks aan het internet bloot.
- Decoderprofielen beschrijven model en verwachte acties; RF-ontvangst en TX hangen van het decoderrecept en de apparaten af.
- Hosttests controleren parsing, ontvangst, simulatie van zenden, opslag, conflicten en firmwarepromotie. Een echte ESP32-S3-build en RF-test met de drie CC1101-modules moeten op het fysieke bord worden uitgevoerd voordat zendgedrag als hardwarebevestigd geldt.
