> **v0.6.2 — nieuw startpunt:** gebruik `00_RFLink_START.bat` in de hoofdmap. Keuze **1 = veilige persoonlijke update** (eigen YAML blijft behouden); **2 = schone standaardrelease**; **3 = ontwikkeltests**. Eventuele verwijzingen hieronder naar de vroegere `UPDATE_RFLINK.bat`, `MAAK_RELEASE.bat`, `MAAK_UPLOAD.bat` of `TEST_UPDATE_RELEASE.bat` zijn historisch; die afzonderlijke starters bestaan niet meer.

## DECODERUX1 – veilige update (22 september 2026)

Pak het SOURCE-pakket uit in een **nieuwe korte map** en draai op Windows `TEST_UPDATE_RELEASE.bat` en daarna `UPDATE_RFLINK.bat` voor je bestaande persoonlijke configuratie. Met `MAAK_RELEASE.bat` maak je alleen de DEFAULT-release. De updater herkent originele v2A/B/C- en DECODERUX1-manifests aan volledige SHA256-paren; gewijzigde eigen decoders vereisen controle. Je hebt op Windows geen Python nodig. De apparaatopslag blijft in het bestaande formaat: er is geen automatische NVS-migratie. De instellingen, YAML/GPIO/OTA zijn ongewijzigd. Zie `DECODER_UX1.md` voor werking en beperkingen. Firmware Validate/Compile en radiopraktijktests zijn nog nodig.

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

# HA2 – instructie voor beheer van apparaten en functies

MQTT één keer activeren volgens `INSTELLINGEN.md` is genoeg. RFLink is de enige bron van waarheid voor welke RF-apparaten en functies bestaan. Een nieuw apparaat gaat pas naar Home Assistant nadat je het in RFLink hebt opgeslagen met het standaard aangevinkte, maar uitschakelbare HA-vinkje (alleen zichtbaar bij MQTT-verbinding en ondersteunde functies). Bij bestaande apparaten zijn nieuw ontdekte functies uitsluitend voorstellen in **Apparaten → Functies beheren**. Goedkeuren of verwijderen past eerst de RFLink-configuratie aan en publiceert daarna de bijbehorende MQTT Discovery-wijziging. Bekende knopacties en bestaande meetwaarden publiceren zonder nieuwe goedkeuring; niets wordt gewist vanwege een tijdelijk ontbrekend signaal. HA-gekoppelde functies verwijderen is geblokkeerd zolang Mosquitto offline is. Een afzonderlijke knop krijgt nog geen vrij instelbaar label. Nieuwe voorstellen verdwijnen na herstart; goedgekeurde functies blijven opgeslagen.

Let op: eerder in dit bestand opgenomen HA1-stappen beschrijven **historisch gedrag**. Volg voor de actuele CONFIG1 MQTT-velden het deel hieronder; de HA2-interface vervangt de eerdere tweede handmatige stap `Toevoegen aan HA` tijdens het inleren.

---

# CONFIG1 – centrale gebruikersinstellingen (21 september 2026)

De actuele stappen voor het activeren van MQTT staan in `INSTELLINGEN.md`. Kies broker-IP/hostnaam, poort (normaliter 1883) en MQTT-gebruikersnaam in de `substitutions` bovenaan `rflink-universal.yaml`; `rflink_mqtt_password` blijft in `secrets.yaml`. Activeer de `ha_mqtt`-include direct onder dit instellingenblok. Eerdere HA1-instructies verderop met broker- en gebruikersnaam-**secrets** zijn vervallen voor CONFIG1. Een ingeschakelde MQTT-package zonder ingevulde broker wordt bij releasebouwen geblokkeerd. Bij een herkende HA1-update met ingeschakelde MQTT behoudt de updater de bestaande broker-/gebruikersnaam-secretverwijzingen in de nieuwe labels. Een schone DEFAULT moet je handmatig invullen; het script raadt geen brokeradres.

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

# Installeren, updaten en release maken — Wizard 4

Lees eerst `START_HERE.md` voor de korte procedure. Historische instructies in oudere overdrachtsbestanden over `MAAK_RELEASE.bat update` en het automatisch overnemen van nieuwe GPIO's zijn **vervallen**.

## Waarom twee onafhankelijke BAT-bestanden?

`MAAK_RELEASE.bat` maakt uitsluitend de project-DEFAULT. Ook als `update_output` een vorige succesvolle of mislukte update bevat, raakt die de DEFAULT niet. `UPDATE_RFLINK.bat` kiest bij bekende UX2-versies automatisch een gecontroleerde historische originele YAML (baseline), vergelijkt die met de bestaande YAML en nieuwe release, bouwt bij nul conflicten zelf de CUSTOM. Beide archieven krijgen verschillende namen.

## De juiste oude basis automatisch kiezen (Wizard 3)

De vorige updater gebruikte bij Enter ten onrechte de HF4-project-YAML als basis, zelfs als jouw bestaande YAML uit HF3 kwam. Dat leverde de fout `Existing YAML and project baseline appear to be different releases` op. Wizard 3 bevat de **originele, ongewijzigde** YAML-bestanden uit de HF3 UX2- en HF4 UX2-SOURCE-archieven onder `rflink_universal/maintenance/baselines/`. De updater herkent de oude versie en UX2-kenmerken, controleert de SHA256 van het meegeleverde origineel, kiest deze automatisch en vermeldt de keuze in het rapport. Ook een expliciet opgegeven verkeerde baseline wordt afgewezen.

Andere of niet-herkende versies blijven geblokkeerd totdat je de exacte originele YAML van jouw oude firmware aanlevert. Versieherkenning bewijst niet dat een lokaal aangepaste configuratie op dit origineel is gebaseerd: bij dubbel gewijzigde YAML-blokken blijft conflictcontrole gelden. De fysieke PCB is nooit uit de YAML alleen te bepalen; bij ontbrekende revisie en gewijzigde pinout volgt altijd een vraag zonder standaardkeuze. De historische bestanden komen alleen in SOURCE.zip, **niet** in de ESPHome-bundle.

## Hardware: de PCB beslist, niet de oude YAML

De huidige standaard-`board_revision` is `eigen_rfid_board_rev2_gpio14` en staat in `substitutions` direct boven de centrale GPIO-pinout. Rev2: SPI SCK/MOSI/MISO 12/11/13; radio1 CS/GDO0/GDO2 4/5/6; radio2 9/8/14; radio3 15/16/17; LED 48. De eerdere revisie 1 (radio1 5/6/4, radio2 9/8/10) blijft een historisch apart hardwareprofiel. Werkelijke PCB-bedrading heeft altijd voorrang op de revisienaam. De updater controleert dubbele GPIO's en de gebruikte N16R8-PSRAM-pinnen 35/36. `radio3_gdo2` is uitsluitend gereserveerd. Als je een andere print hebt, kies bewust `Existing`; daarmee wordt de pinout als groep bewaard en krijgt de kandidaat `board_revision: custom_existing`. Daarvoor moet de bestaande YAML alle centrale pinlabels hebben en moeten ze uniek zijn. Oude YAML zonder pinlabels kan **niet** veilig als eigen pinout worden behouden: sluit eerst je echte bedrading aan op een bekend profiel.

| Situatie | Standaard / gedrag |
|---|---|
| Bestaande pinout exact gelijk | Release gebruiken, geen overbodige vraag. |
| PCB-revisie gelijk maar pinwaarden verschillen | `[Y/n]` nieuwe pinout, Enter is Y; controleer de echte bedrading. |
| PCB-revisie anders/onbekend en pinnen verschillen | `[y/n]` fysieke pinout expliciet kiezen, Enter ongeldig. |
| Automatische modus met onbekend/conflicterend profiel | Stop, geen bundle. |
| Bestaande pinout kiezen zonder volledige centrale pinlabels | Stop, geen bundle. |
| GPIO-dubbeling/gereserveerde PSRAM | Stop, geen bundle. |

## Persoonlijke instellingen

De apparaatnaam, friendly name en vier Itho-substitutions worden uit je bestaande YAML behouden. Andere `substitutions` worden per sleutel met drie-wegmerge behandeld. Complexe YAML (bijvoorbeeld gewijzigde radio-, OTA- of scriptblokken) wordt op blokniveau vergeleken: bij twee verschillende wijzigingen stopt de normale/automatische modus en volgt een rapport. In de geavanceerde modus kunnen `wifi`, `logger` en `esphome` bewust oud/nieuw gekozen worden. Firmwarecode, radiodrivers, RMT- en OTA-fixes komen uit de nieuwe release. Deze tool leest geen live ESP32-NVS en bewaart geen apparaten uit het flashgeheugen naar een ZIP; het bestaande apparaat wordt niet gewist.

## Eigen decoders

Een losse YAML bevat geen decodercode. Geef desgewenst `-ExistingRuntime` op of kies in de interactieve wizard een uitgepakte SOURCE-map. Extra of aangepaste decoders worden gedetecteerd en in het rapport vermeld. Bij verschillen komt er geen CUSTOM-bundle totdat je de code handmatig in de nieuwe bron hebt verwerkt en opnieuw hebt getest. De updater claimt geen compatibiliteit zonder echte controle.

## Wat wordt in mappen gemaakt?

`update_output/update_.../`: merged YAML, nieuwe runtime, `WIJZIGINGEN.txt`, `update-manifest.json` (hashes, status, board revision) en de 3 oorspronkelijke YAML-back-ups. Een mislukte run kan `update_output/LAST_UPDATE_FAILED.txt` achterlaten: verhindert dat een oudere READY-CUSTOM per ongeluk wordt gebruikt. `dist/`: `_DEFAULT` van MAAK_RELEASE of `_CUSTOM` na geslaagde updater. Nooit dezelfde bestandsnaam. Back-ups en secrets worden niet opgenomen in bundels. Het archief bevat geen `.bin`, er wordt niet geflasht en de deviceconfig wordt niet automatisch vervangen.

## Testen en grenzen

`TEST_UPDATE_RELEASE.bat` test onder Windows de wrappers, default-isolatie, hashcontroles, automatische CUSTOM-bouw, conflicten en hardwarekeuzes. `rflink_universal/tests/run_release_checks.sh` test de host/firmwarebron op Linux en controleert niet de uitvoering van PowerShell op Windows. Ook na een geslaagde hosttest blijven ESPHome Validate/Compile en een proef op de echte print nodig.

**Oudere HF3-variant:** Wizard 3 herkent één extra, exact gecontroleerd historisch YAML-profiel (persoonlijke apparaatnaam en Itho-ID uitgezonderd). De referentie bevat placeholders en mag niet rechtstreeks in ESPHome gebruikt worden. Nieuwe firmware- en RMT-fixes worden nooit uit die oude variant gekopieerd. Bij niet-herkende aanpassingen blijft een exacte originele baseline nodig.

**ITHO1-migratie:** de huidige hoofd-YAML bevat geen globale `itho_id1/2/3`, `itho_device_type` of losse Itho-templateknoppen meer. Historische baselines mogen die waarden nog bevatten zodat de updater een oude installatie kan herkennen, maar ITHO1 kopieert die identiteit niet naar een nieuw ingeleerd apparaat. Leer het Itho-apparaat in en koppel RFLink daarna opnieuw als extra virtuele afstandsbediening; vanaf dat moment staan zender-ID en teller per apparaat in de persistente device-state.

**Wizard 4 – conflict `external_components` / `remote_transmitter`:** controleer bij elke poging `INPUT YAML` en `INPUT SHA256` zodat je niet per ongeluk een ander lokaal bestand selecteert. Een wijziging van alleen commentaar bij ongewijzigde externe componenten is geen conflict. Een precies herkende oudere HF3-zendersectie wordt veilig vervangen door de nieuwe HF4-zender met 48-symbolen-RMT-configuratie en radiomonitor. Bij andere firmwareverschillen vraagt Normal/Advanced: `Afwijkende firmware ... vervangen ...? [y/N]`. Enter = nee, conflict en geen bundle. Y = uitdrukkelijk de eigen wijzigingen aan die sectie weggooien ten gunste van HF4. Automatic stopt bij twijfel. Gebruik geen handmatig gemengde oude `remote_transmitter` met nieuwe HF4-code. De oorsprong, gekozen instellingen en onzekerheden staan in `update_output/.../WIJZIGINGEN.txt`.


## TX1: optionele Home Assistant-zendknoppen

Lees eerst `TX1_BEDIENING.md`. Standaard uit. Schakel MQTT en
`ha_tx_controls: true` in de centrale YAML in en valideer/compileer de firmware.
Na installatie moet je Nexa of Byron SX 433,92 MHz-zendtoestemming expliciet per
ingeleerd apparaat verlenen in RFLink. Een bestaande HA2-apparaatregistratie
leidt op zichzelf nooit tot uitgaande RF-signalen. Controleer knopfunctionaliteit
met het echte eindapparaat; queued TX is geen fysieke statusbevestiging.
