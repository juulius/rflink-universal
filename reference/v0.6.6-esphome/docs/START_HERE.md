> **v0.6.2 — nieuw startpunt:** gebruik `00_RFLink_START.bat` in de hoofdmap. Keuze **1 = veilige persoonlijke update** (eigen YAML blijft behouden); **2 = schone standaardrelease**; **3 = ontwikkeltests**. Eventuele verwijzingen hieronder naar de vroegere `UPDATE_RFLINK.bat`, `MAAK_RELEASE.bat`, `MAAK_UPLOAD.bat` of `TEST_UPDATE_RELEASE.bat` zijn historisch; die afzonderlijke starters bestaan niet meer.

> **Nieuwste ontwikkeling:** RFLink Universal **v0.6.1 Radio Gaga**. Zie [V061_RELEASE_NL.md](releases/V061_RELEASE_NL.md) voor wijzigingen, beperkingen en het hardwaretestplan.

> **Actuele patch:** v0.5.4 Radiolove — CRASHTRACE1. Zie `V054_RELEASE_NL.md` voor details en beperkingen.

> **Nieuwste release: v0.5.3 Radiolove.** Korte versie plus bronhash in de website. Eerdere NVS-resetredenen staan onder **Logboeken → Systeem → Crashes en herstarts**. Raadpleeg `V053_RELEASE_NL.md` voor de actuele wijziging en `V052_RELEASE_NL.md` voor de vorige hashcorrectie en `V051_RELEASE_NL.md` voor crashgeschiedenis. Bouw met je eigen PCB-YAML en 6 MHz SPI.

**Laatste experimentele ontwikkelversie: v0.5 Radiolove – PAIRRADIO9.** Bekijk `PAIRRADIO9_RELEASE_NL.md` voor nieuwe radio-TX-diagnostiek.

## Nieuw: DIAG7 — crash- en UX-diagnose

Zie [`rflink_universal/DIAG7_RELEASE_NL.md`](source/rflink_universal/DIAG7_RELEASE_NL.md) voor wijzigingen, testresultaten, veilige installatie en openstaande punten.

> **Actuele crashfix:** zie `PROFILESAFE1_HERSTEL_EN_TEST_NL.md` voor het herstellen van het lege profielenoverzicht en de `Abort` in `user_library_json_`.

# Begin hier: [RFLink Universal Handboek (actuele routes)](HANDBOEK_RFLINK_NL.md)

> Oudere instructies hieronder zijn historische release-informatie. Voor de huidige v0.5 Radiolove source gebruik je het handboek en de actuele versiekaart in de UI.

## Snelle start — v0.5 Radiolove (UXREVIEW5, 1 november 2026)

- Gebruik `MAAK_RELEASE.bat` voor de schone **DEFAULT**-bundle. Gebruik voor een bestaande eigen YAML (zoals 6 MHz SPI en eigen GPIO's) `UPDATE_RFLINK.bat` of controleer en behoud jouw aangepaste YAML handmatig; de DEFAULT neemt persoonlijke instellingen niet automatisch over.
- Open **Apparaten** voor status en de direct beschikbare bediening. Kies **Apparaat instellen** voor een ongeconfigureerd apparaat; **Instellen en koppelen** voor de uitgebreide details.
- De huidige broncode bevat **geen hardwarebewezen Orcon-TX** en de eerder gemelde panic-resets zijn hiermee niet gerepareerd.
- Documenten en BAT-uitleg: `docs/DOCUMENTATIE_OVERZICHT_NL.md`.

---

## ORCONTXLAB2 – experimentele Orcon MVS-15R-bediening (28 oktober 2026)

Nieuw: acht echte RAMSES-II-TX-acties via de universele GDO-serial-zendroute;
1FC9-ACK-gestuurde koppelwizard met expliciete Auto-test en verificatie.
De drie logboeken (Radio/Systeem/Web) blijven beschikbaar voor debugging.
Alles is standaard uitgeschakeld en **niet fysiek op de Orcon-box gevalideerd**.
Gebruik je eigen PCB-YAML (inclusief 6 MHz SPI), niet de standaard-YAML.
Lees `docs/history/ORCONTXLAB2_RELEASE_NL.md` vóór een experiment.

> **Nieuwe LOGBOOK1 debugrelease:** raadpleeg `docs/history/LOGBOOK1_SYSDEBUG1_RELEASE_NL.md` voor de gecombineerde logviewer, instellingen en experimentele GDO UART-onderzoekstappen. De echte Orcon-bediening is nog niet hardwarebevestigd.

**UX3 (28-10-2026):** De apparaatkaarten hebben nu compacte status, duidelijke instelacties en aparte bediening/ontvangst. De nieuwste broncode heeft revisie `…GDOUARTLAB1-UX3`. Lees [UX3-gebruikersaudit en testresultaten](history/UX3_GEBRUIKERSAUDIT_EN_TESTRESULTATEN.md) vóór een OTA-update. Orcon-zenden blijft uitgeschakeld totdat de fysieke test geslaagd is. Gebruik jouw eigen 6 MHz SPI-YAML.

## USERLIB2-PROFILES1-UI4-SOMFY1-ITHO1 — Itho RFT centraal bedienen (27 september 2026)

Itho RFT zit nu in dezelfde apparaatflow als Somfy. Leer eerst een Itho-afstandsbediening in, open daarna het opgeslagen apparaat en kies **RFLink koppelen**. RFLink maakt per Itho-apparaat een eigen virtuele 24-bit zenderidentiteit en bewaart de verzendteller persistent. Na koppelen zijn **Laag, Midden, Hoog, Timer 10, Timer 20 en Timer 30** direct beschikbaar; Stand-by en compatibility-pairing staan onder Advanced.

De oude globale Itho-ID en losse ESPHome-templateknoppen zijn uit de hoofd-YAML verwijderd. Home Assistant-bediening loopt nu via hetzelfde gecontroleerde device-pad als de rest: pairing eerst, daarna expliciete HA/TX-toestemming. De native packetencoder, pairing/unpair, telleropslag, backup/restore en MQTT-dispatch zijn hostgetest; een fysieke Itho-unit + 868,30MHz CC1101-test blijft nodig.

## USERLIB2-PROFILES1-UI4-SOMFY1 — Somfy RTS bedienen (27 september 2026)

Somfy RTS kan nu niet alleen worden ontvangen, maar ook als **aparte virtuele RFLink-afstandsbediening** worden gekoppeld. Open het opgeslagen Somfy-apparaat, zet de motor met een bestaande afstandsbediening in programmeermodus en kies **RFLink koppelen**. Daarna verschijnen **Omhoog, My/Stop en Omlaag**.

RFLink bewaart een eigen 24-bit virtueel zenderadres en rolling counter in de apparaatopslag. De volgende tellerwaarde wordt opgeslagen vóór een zendopdracht naar de radio gaat en een backup kan de teller nooit terugzetten. De PROG-knop wordt niet als gewone Home Assistant-knop gepubliceerd. Deze TX-route is met hosttests en terugdecoderen van de gegenereerde 56-bit frames getest; een echte Somfy-motor/CC1101-test staat nog open.

## USERLIB2-PROFILES1-UI4 — zonder handleiding te bedienen (26 september 2026)

De normale interface is bedoeld voor iemand die niets van RF-protocollen hoeft te weten. **Apparaat toevoegen** vraagt alleen de frequentie, daarna Start en het apparaat gebruiken. Zodra RFLink het apparaat herkent, geef je een naam, kies je de functies en druk je op Opslaan. Technische protocolinformatie staat onder **Geavanceerde details**.

Een gezonde toestand blijft stil: Apparaten toont geen opslagmelding als alles goed is en pairing verschijnt alleen als het relevant is. De bovenbalk en Activiteit spreken over **signalen** in plaats van frames. Decoderbouw, ruwe RF-data, RX/TX-regels en diagnostiek blijven beschikbaar onder Advanced.

## USERLIB2-PROFILES1-UI4 — simpele dagelijkse UI (26 september 2026)

Deze iteratie maakt de webinterface rustiger zonder functies weg te halen. **Status** toont alleen de twee normale acties; pairing en apparaatbeheer zitten onder **Apparaten**. **Instellingen** toont eerst taal, Home Assistant en de RGB-statuslamp. Lange uitleg, eigen protocol-/profielbouw en diagnose staan ingeklapt of onder **Advanced**. De Decoderbibliotheek gebruikt op desktop de brede werkruimte, maar opent op mobiel pas details nadat je zelf een protocol kiest.

De LED-status is nu ook echt taalafhankelijk. De firmware geeft naast het historische statusveld een stabiele machinecode terug en de UI vertaalt die naar Engels, Nederlands, Frans of Duits. Tijdens dezelfde audit zijn de nog Engelstalig blijvende beginteksten van **Leren → Apparaat toevoegen** meegenomen.

Voor dagelijks gebruik is de bedoelde route nu: **Status → Apparaat toevoegen → opslaan → Apparaten**. Decoderbouw en eigen profielen blijven beschikbaar voor gevorderd gebruik, maar staan niet meer in de weg.

## USERLIB2-PROFILES1 — bekende apparaatprofielen in de firmware (25 september 2026)

Bij ondersteunde decoders kan RFLink nu een **bekend apparaatprofiel** tonen en selecteren. De eerste catalogus bevat 10 brononderbouwde product/familieprofielen voor AcuRite, Alecto/Ventus, Fine Offset/ClimeMET, Itho Daalderop, Byron, Nexa/Jula Anslut, Dooya/Zemismart, Rubicson/TFA en Somfy Telis RTS. Open **Instellingen → Decoderbibliotheek** om bij een protocol modellen, leerhulp, compatibiliteitsnotities en de bronlinks te zien. Bij een opgeslagen apparaat verschijnt hetzelfde profiel in de profielkeuze.

Een profiel is metadata bovenop een decoder; het verandert het RF-protocol niet. Daarom staat een nieuw ingebouwd profiel zolang het exacte model nog niet op deze RFLink Universal-hardware is beproefd als **hardware validation pending**. Bij twijfel kiest RFLink niet automatisch tussen meerdere productprofielen. Eigen device profiles blijven daarnaast mogelijk en apart opgeslagen. Zie [LIBRARY_AND_PROFILES.md](LIBRARY_AND_PROFILES.md) en `rflink_universal/profiles/README.md`.

## USERLIB2-UI2 — brede desktopwerkruimte voor bibliotheek, profielen en decoderbouw (25 september 2026)

Deze UI-iteratie gebruikt op desktop de beschikbare schermbreedte in plaats van de hele app op 1240 px af te knijpen. **Instellingen** groepeert taal/snelkoppelingen en RGB/Home Assistant in twee kolommen, terwijl Bibliotheek & updates en Decoderbibliotheek de volledige breedte gebruiken. **Mijn decoders en apparaatprofielen** heeft een echte tweepaneelwerkruimte en **Leren → Decoder maken** zet de basisvelden naast elkaar; RX en TX staan in Advanced naast elkaar zolang er genoeg ruimte is. Tablet en mobiel stapelen deze onderdelen weer zonder horizontale overflow.

Open **Leren → Create decoder**, neem elke knop minstens drie keer op en kies **Test and activate decoder**. Daarna kun je een modelprofiel aanmaken, het herkende apparaat bevestigen en de TX-uitvoer vooraf bekijken. **Allow sending** verleent per apparaat apart toestemming; **Send** vraagt een radio-uitzending aan. Lees eerst [USER_DECODER_GUIDE.md](USER_DECODER_GUIDE.md) voor de precieze stappen, interpreterregels en grenzen.

Met **Convert to firmware decoder → Download evaluation export** exporteer je de recepten. Het meegeleverde `promote_user_library.py` voert een controle uit en voegt na `--merge` geteste recepten toe aan de firmwarebron. De hosttests zijn uitgevoerd; een ESPHome-build, Windows-releasebouw, bedienproef in de browser en fysieke ontvangst/zendtest op jouw print blijven nodig voordat deze versie als hardwarebevestigd geldt.

## DECODERUX1 – eenvoudiger leren en apparaatbeheer (22 september 2026)

De decoderbibliotheek groepeert protocolinformatie en toont de zend-teststatus duidelijk. Via Leren controleer je functies voor de eerste opslag; apparaatbeheer verbergt technische instellingen en gevaarlijke acties achter uitklappers. Zie `DECODER_UX1.md`. Geen opslagmigratie of nieuwe zendencoder; eerste Validate/Compile en Windows-releaseproef blijven vereist.

**Webadres (ROOTFAV1):** Open `http://<IP>/`; de browser gaat automatisch naar `http://<IP>/rflink`. Het tabblad heeft een eigen RFLink-icoon. Gebruik HTTP, tenzij je zelf een HTTPS-proxy instelt.

# Release-/updatercorrectie RELUPFIX2 – 22 september 2026

**RELUPFIX2: correctie op DEFAULT-releases (22 september 2026).** De releasecontrole zocht nog naar de oude `set_radio_available(radio, beschikbaar)`-signatuur. De meegeleverde YAML gebruikt terecht vier argumenten (`radio`, CC1101, RX en TX). De releasebouwer controleert nu alle drie radio's binnen de actieve `interval`-sectie en accepteert DEFAULT zonder dat de YAML handmatig veranderd hoeft te worden. De Windows-integratietest heeft nu een positieve DEFAULT-test en een negatieve test die een defecte radiocontrole afwijst. Geen firmware-, GPIO- of RF-protocolwijzigingen. Werk vanaf deze nieuwe SOURCE.zip; RELUPFIX1 bevat de foutieve releasecontrole.


Dit is een **script- en packagingcorrectie** op Decoder API v2C, geen nieuwe RF-protocolrelease. De firmwarebron en centrale GPIO-instellingen blijven gelijk aan de oorspronkelijke v2C, behalve de aanpassingen aan documentatie en hosttests.

- `MAAK_RELEASE.bat` stopte doordat `maak_release.ps1` de niet-gedefinieerde functie `Read-Utf8` aanriep. De releasebouwer bevat nu zijn eigen UTF-8-lezer.
- In de expliciete CUSTOM-bouw controleerde de zendveiligheidscheck de DEFAULT-YAML en DEFAULT-core in plaats van de gecontroleerde updatebestanden. Nu worden `$configSource` en `$runtimeRoot` consequent gebruikt.
- De updater identificeerde de standaard-YAML van Decoder API v2B/v2C ten onrechte als de oudere TX1-baseline, omdat de historische TX1-commentaarmarkering bleef bestaan. Gehashte, versie-eigen v2B/v2C-baselines worden nu vóór TX1 geselecteerd.
- Een ongewijzigde, officiële oudere decoder kon ten onrechte worden afgekeurd omdat de nieuwe release zijn `decoder.cpp` of manifest had vernieuwd. Per release zijn de oorspronkelijke hashparen vastgelegd. Alleen **beide** bij elkaar passende, geverifieerde bestanden worden als stock-upgrade geaccepteerd; een afwijkende decoder blijft een conflict.
- De Windows-integratietest had verouderde archiefnamen, een niet-bestaande loggertekst en een verkeerde verwachte baselineomschrijving. Die zijn hersteld. Er zijn controles voor v2B → v2C en een echt gewijzigde oude decoder toegevoegd.
- Archiefnamen zijn bewust ingekort om de kans op Windows-padlengteproblemen bij het uitpakken te beperken.

**Gebruik:** Pak de nieuwe `...RELUPFIX2_DEFAULT_SOURCE.zip` uit in een korte, nieuwe map, bijvoorbeeld `C:\RFLink`. Meng de scripts niet met oude v2C-bestanden. Dubbelklik `MAAK_RELEASE.bat` voor een schone DEFAULT; gebruik `UPDATE_RFLINK.bat` voor jouw persoonlijke bestaande YAML. De updater maakt bij succes zelf een afzonderlijk CUSTOM-pakket. `MAAK_RELEASE.bat update` is geen geldige opdracht.

**Belangrijk:** deze omgeving heeft geen Windows PowerShell. De Python-regressies, hosttests en archiefcontroles zijn onafhankelijk; een echte uitvoering van `TEST_UPDATE_RELEASE.bat` op Windows en daarna ESPHome Validate/Compile blijven vereist. Geen OTA/flash of automatische wijziging aan jouw bestaande YAML.

---

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

# Historische TX2 / Decoder API v2B – wat was er nieuw?

De gemeenschappelijke TX-motor verwerkt MQTT-toestemming, fysieke 433-MHz-
radioselectie en dispatch; iedere decoder houdt zijn eigen encoder in het pack.
**Nexa Aan/Uit** blijft behouden; **Byron SX command_0 t/m command_15** is als
hostgeteste optionele zender toegevoegd. Alle zendbediening is standaard uit:
zet MQTT en `ha_tx_controls: true` bovenaan de YAML en geef per ingeleerd
apparaat in RFLink expliciet toestemming. Gebruik geen radio zonder fysieke
CC1101-test. Itho gebruikt vanaf ITHO1 de centrale native device-/packet-TX-route; de oude losse ESPHome-knoppen zijn verwijderd.

Voor decoders en de resterende werkzaamheden: `DECODER_API_V2.md`.
Gebruik voor bestaande installaties eerst de Windows-updater, niet zomaar een
schone DEFAULT-YAML; oorspronkelijke instellingen en NVS-identiteiten behouden.
De updater en release-BAT gebruiken geen vooraf geïnstalleerde Python.

---

# Decoder API v2A – beheerbare protocollen (testrelease)

De decoderbibliotheek in de RFLink-webinterface toont nu per protocol
ontvangstacties, velden en beschikbare zendacties uit het manifest.
**Alleen Nexa on/off heeft in deze fase een bestaande zender.** Voor Somfy,
KeeLoq, Byron, Dooya en RC-Switch worden geen nieuwe HA-zendknoppen
geactiveerd. GPIO, OTA en de apparaatopslag zijn niet gewijzigd.

Lees `DECODER_API_V2.md` voor ontwikkeling en de beperkingen. ESPHome Validate
én Compile zijn verplicht voordat je deze testrelease zelf installeert.

---

# HA2 – apparaten en functies beheren via RFLink (21 september 2026)
## TX1 – gecontroleerde Home Assistant-bediening (testversie)

Nieuw: een expliciet ingeleerd Nexa-apparaat (433,92 MHz) kan na eenmalige
MQTT-configuratie en twee goedkeuringen (`ha_tx_controls: true` bovenaan de YAML
en `HA-bediening toestaan` op het apparaat in RFLink) Home Assistant-knoppen
**Aan** en **Uit** krijgen. Zonder die twee toestemmingen blijft TX uit.

Ondersteunde commando's: alleen gevalideerde Nexa aan/uit; GEEN raw-replay,
Somfy, KeeLoq of generieke Itho-zendfuncties. De knop bevestigt alleen dat
RFLink een zendopdracht heeft gestart, niet dat het doelapparaat reageerde.
Radio/GPIO en oudere RF-apparaten blijven onveranderd. Als een oud Nexa-record
`tx_radio` mist, ontvang het signaal opnieuw en keur die herkenningsfunctie in
RFLink goed voordat je zendtoestemming kunt geven.

Lees `TX1_BEDIENING.md` voor stappen, veiligheidsregels en tests. De volledige
ESPHome-compile en praktijktest op Mosquitto, Home Assistant en echte radio
moeten nog plaatsvinden; niet flashen voordat Validate en Compile slagen.


**Begin bij `INSTELLINGEN.md`:** zet Mosquitto eenmalig aan met brokeradres, poort, gebruikersnaam en wachtwoord in je eigen `secrets.yaml`. De MQTT-package blijft standaard uit; de bestaande ESPHome API blijft het board beheren.

1. Importeer de DEFAULT ESPHome-bundle of gebruik `UPDATE_RFLINK.bat` voor een bestaande persoonlijke YAML. Voor de SOURCE-zip kun je zonder updater `MAAK_RELEASE.bat` starten.
2. Controleer je PCB, voer **ESPHome Validate en Compile** uit en installeer daarna alleen zelf via OTA.
3. Leer een RF-apparaat in op de RFLink-webpagina. Als MQTT verbonden is en het apparaat een ondersteunde functie heeft, staat **Toevoegen aan Home Assistant** standaard aangevinkt in het opslaan-scherm. Bevestig het apparaat zelf; losse RF-signalen voegen nooit automatisch een apparaat toe.
4. Bij een bestaand apparaat: open **Apparaten → Functies beheren**. Nieuwe sensorvelden of knopacties zijn voorstellen. **Goedkeuren** verandert de door RFLink beheerde functie-indeling en publiceert voor HA-gekoppelde apparaten de bijpassende Discovery-configuratie. **Verwijderen** vraagt bevestiging en ruimt alleen de betreffende HA-functie op.
5. Goedgekeurde meetwaarden en bekende knopacties worden automatisch live bijgewerkt. Een functie verdwijnt niet doordat een radiosignaal tijdelijk uitblijft. Bij brokeruitval worden HA-gerelateerde verwijderingen geblokkeerd.

**Let op:** voorstellen voor nieuwe functies worden nog niet over een ESP32-herstart bewaard. Bestaande goedgekeurde functies wel. Home Assistant ontvangt wijzigingen via MQTT, maar de firmware heeft geen applicatiebevestiging van Home Assistant zelf. Verdere details en huidige grenzen: `RELEASE_NOTES.md`.

---

# CONFIG1 – instellingen en Mosquitto (21 september 2026)

**Begin bij `INSTELLINGEN.md`.** WiFi, MQTT-IP/hostnaam, poort, MQTT-gebruikersnaam, identiteit, Itho, GPIO, OTA/API-secretverwijzingen en voorkeuren staan gegroepeerd bovenaan `rflink-universal.yaml`. Onder `packages` staat direct daaronder de optionele MQTT-aan/uitregel. Laat de optie uit zolang het brokeradres op `VUL_IP_OF_HOSTNAAM_IN` staat. Het MQTT-wachtwoord hoort uitsluitend in `/config/esphome/secrets.yaml` onder `rflink_mqtt_password`. Gebruik jouw bestaande Mosquitto; geen tweede broker installeren.

De RFLink-webinterface blijft leidend: HA1 vereist nog handmatige goedkeuring per apparaat; CONFIG1 implementeert niet automatisch toevoegen of verwijderen van RF-functies. De oude HF4-firmware en bestaande fysieke pinout blijven verder ongewijzigd.

---

# HA1 – Home Assistant-koppeling (optioneel, 20 september 2026)

Dit is de eerste testrelease met MQTT Discovery voor *ingeleerde* RF-apparaten. De ESPHome native API blijft de bestaande koppeling voor de ESP32 zelf. MQTT is **standaard uitgeschakeld** zodat je bestaande firmware zonder broker en zonder nieuwe secrets blijft werken.

## Zo zet je Home Assistant aan

1. Zorg voor een bereikbare MQTT-broker en de MQTT-integratie in Home Assistant. Gebruik je eigen brokeradres en apart aangemaakte gebruikersgegevens.
2. Open `rflink-universal.yaml`, zoek de `packages:` sectie en wijzig `# ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml` in `ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml` (laat de indentatie staan).
3. Voeg aan je **eigen** ESPHome `secrets.yaml` deze drie sleutels toe: `rflink_mqtt_broker`, `rflink_mqtt_user`, `rflink_mqtt_password`. Waarden staan niet in de release; zie `rflink_universal/secrets.example.yaml`.
4. Voer in ESPHome eerst Validate en Compile uit en installeer daarna pas. De vorige OTA-configuratie blijft voor deze update behouden.
5. Open de RFLink-webinterface, ga naar **Apparaten** en kies bij een herkend apparaat `Toevoegen aan HA`. Het apparaat krijgt op basis van beschikbare en ondersteunde gegevens bijvoorbeeld temperatuur-, vocht- en batterij-entiteiten, of een knop-event. Laat het apparaat nogmaals zenden voor de eerste actuele meetwaarde.
6. Met `Hernoemen` verandert de zichtbare naam bij de volgende succesvolle MQTT-publicatie. `Verwijder uit HA` verwijdert alleen de MQTT Discovery-configuraties; het apparaat blijft in RFLink. `Verwijderen` verwijdert het record uit RFLink én bij actieve HA-koppeling de Discovery-registraties.

De MQTT-package zet `reboot_timeout: 0s` zodat een tijdelijke brokerstoring de bestaande ESPHome-API, webinterface en OTA niet elke 15 minuten opnieuw opstart.

**Belangrijke grenzen:** alle HA-wijzigingen vereisen een verbonden broker. Bij een brokerstoring blokkeert uitschakelen/verwijderen van een HA-gepubliceerd apparaat, om achterblijvende registraties te voorkomen. MQTT-publicatie bevestigt niet dat Home Assistant de gegevens heeft verwerkt: controleer de MQTT-integratie bij problemen. RFLink exporteert alleen expliciet ondersteunde sensormetingen en knopacties; geen RF-identiteiten of rolling codes. Een afstandsbediening die alleen ontvangst levert, verschijnt als event en niet als fictieve schakelaar. Native ESPHome API en standaard GPIO-pinout worden niet omgezet. Het dashboard zelf is alleen bedoeld voor een vertrouwd lokaal netwerk.

**Tests:** C++-hosttests dekken Discovery JSON, toevoegen, waarden/acties, hernoemen, herverbinden, broker-uitval, verwijderen en niet-ondersteunde gegevens. De volledige ESPHome 2026.9 firmwarecompile, Windows PowerShell en broker/HA-hardwaretests moeten op de eigen installatie worden uitgevoerd.

---

# COMPILEFIX1 – belangrijke opmerking

Deze versie verhelpt de compileerfout uit Wizard 4 bij de OTA-RGB-led en twee C++-formatwaarschuwingen. Laat GPIO en de huidige OTA-authenticatie bij de eerste firmware-upgrade ongewijzigd. Gebruik de SOURCE ZIP voor scripts/updater, of de DEFAULT ESPHome-bundle om een nieuwe configuratie te importeren. Heb je al een aangepaste YAML uit de updater, voer de updater opnieuw uit met deze SOURCE-versie zodat ook die aangepaste YAML de OTA-correctie krijgt; gebruik niet de oude CUSTOM-bundle. Controleer daarna in ESPHome Validate en Compile en pas na een succesvolle build Install. Geen secrets.yaml wordt meegeleverd.

# RFLink Universal RC4-HF4 UX2 / Update Wizard 4

Deze revisie verbetert alleen de Windows-updater en releaseprocedure. De RFLink-firmware en radio-RMT-oplossingen blijven HF4 UX2. Dit is geen vooraf gecompileerde firmware.

## Nieuwe installatie / schone standaardrelease

1. Pak de SOURCE.zip uit in een nieuwe map.
2. Voer `MAAK_RELEASE.bat` uit, **zonder argumenten**.
3. De map `dist` bevat een `_DEFAULT_SOURCE.zip` en een `_DEFAULT.esphomebundle.tar.gz`. De standaardpinout komt uitsluitend uit de YAML van deze projectmap; `update_output` wordt genegeerd.
4. Importeer de `.esphomebundle.tar.gz` in ESPHome Device Builder. Controleer jouw fysieke PCB, behoud `secrets.yaml`, voer Validate en Compile uit en kies zelf Install/OTA.

## Bestaande installatie updaten

1. Bewaar zelf een kopie van jouw bestaande YAML, secrets en werkende ESPHome-project. Gebruik `UPDATE_RFLINK.bat` en wijs de bestaande YAML aan. Sleep de YAML desgewenst op de BAT.
2. Optioneel: selecteer de nieuwe SOURCE-map/ZIP, originele YAML van de oude versie en de uitgepakte oude SOURCE-map voor vergelijking van eigen decoders. De updater vindt zelf de ongewijzigde basis van de bekende HF3 UX2- of HF4 UX2-release. Voor andere releases geef je de exacte oorspronkelijke YAML aan; oude GPIO-indelingen worden nooit stilzwijgend overgenomen.
3. De updater controleert de centrale GPIO's **als een geheel**. Rev1 + dezelfde pinout: automatisch release; rev1 + afwijkende pinnen: vraag `[Y/n]` (Enter = release); onbekende of andere PCB: verplichte keuze `[y/n]` zonder default. De hardware kan niet automatisch fysiek herkend worden.
4. Persoonlijke identiteit/Itho-ID blijven behouden; andere YAML wordt conservatief per sectie samengevoegd. Bij twijfel over complexe blokken of aangepaste decoder-code wordt gestopt met een rapport, niet gegokt.
5. Na jouw akkoord (`[Y/n]`, Enter = ja) bouwt de updater bij nul conflicten zelfstandig een aparte `_CUSTOM_SOURCE.zip` en `_CUSTOM.esphomebundle.tar.gz` in `dist`. `MAAK_RELEASE.bat update` is **vervallen**.
6. Lees `update_output/update_.../WIJZIGINGEN.txt`, controleer de PCB-revisie en de YAML; in ESPHome nog altijd Validate, Compile en handmatig OTA.

## Modi

- `UPDATE_RFLINK.bat`: normaal; relevante hardwarevragen. Identieke externe componenten met uitsluitend gewijzigde commentaarregels worden automatisch herkend. Een exact bekende oudere zenderconfiguratie krijgt de HF4-correcties. Bij werkelijk afwijkende `external_components`/`remote_transmitter` verschijnt een keuze `[y/N]`: Enter = stoppen, Y = oude afwijkingen bewust vervangen door de release. Overige conflicten blijven geblokkeerd.
- `UPDATE_RFLINK.bat advanced`: geavanceerd; bij een overlap in `wifi`, `logger` of `esphome` expliciet oud/nieuw kiezen. Beide keuzes moeten de releasechecks doorstaan.
- `UPDATE_RFLINK.bat automatic`: geen keuzes bij onzekerheid; stopt bij onbekende PCB of conflicten. Bouwen en flashen zijn verschillende handelingen; geen automatische flash.

Technisch/scripts: `powershell.exe` (Windows PowerShell 5.1) en ingebouwde `tar.exe`, geen Python nodig bij de Windows-BAT-workflow. Extra beheerdersopties voor tests: `-HardwareChoice Release|Existing|Unknown`, `-NonInteractive`, `-AcceptReleaseFirmwareChanges` (expliciet oude aangepaste firmwareblokken vervangen), `-ExistingRuntime <uitgepakte oude SOURCE-map>` en `-BaseYaml <originele oude YAML>` (alleen bij niet-herkende versies). Geef hardware-keuze alleen op na controle van je PCB.

**Grenzen:** eigen decoderbron uit een oude SOURCE-map kan worden gedetecteerd, maar wordt niet ongemerkt overgenomen: gewijzigde en extra code vraagt handmatige compatibiliteitscontrole en blokkeert een CUSTOM-release. Zonder oude SOURCE-map is aanwezigheid van eigen decoders onbekend. Firmwarecompile, fysieke CC1101-werking en Windows PowerShell-uitvoering zijn hier nog niet geverifieerd.

**Privacy:** de lokale `update_output/.../backups` bevat jouw oorspronkelijke YAML en kan gevoelige waarden bevatten. De distributie-archieven nemen geen secrets, back-ups of update-rapporten op. Deel backups niet ongewijzigd.

### Wizard 3: fout bij oudere HF3-YAML

Als je YAML nog de oudere variant zonder RMT TX-correcties bevat, herkent de updater deze nu via een streng gecontroleerd referentieprofiel. Plaats de broncode in een nieuwe map, voer `UPDATE_RFLINK.bat` uit en selecteer je bestaande YAML. Kies de GPIO-pinout uitsluitend na controle van je fysieke print. Raak de originele YAML niet aan. Bij nieuwe/onbekende verschillen stopt de updater nog steeds en vraagt om de originele baseline.

### Wizard 4: `external_components` / `remote_transmitter` conflict

De updater toont nu het ingelezen pad, SHA256 en of 48-symbolen-TX-instellingen aanwezig zijn. Controleer dat hij de bedoelde YAML inleest. Commentaarwijzigingen bij exact gelijke externe componenten leiden niet meer tot een conflict. Een exact bekende historische zendersectie wordt automatisch vervangen door de HF4-versie met RMT- en radiomonitorfixes. Alle **andere** wijzigingen in deze firmwaresecties vereisen afzonderlijk akkoord `[y/N]` (standaard N = stop). `Y` betekent dat je eigen wijzigingen in die sectie bewust weggooit. Er wordt geen oude code in een nieuwe release geforceerd en geen firmware automatisch geïnstalleerd.
