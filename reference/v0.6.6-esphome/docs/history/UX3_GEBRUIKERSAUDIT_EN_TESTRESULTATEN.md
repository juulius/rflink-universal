# RFLink Universal — UX3: gebruikersaudit en testrapport

**Datum:** 28 oktober 2026  
**Basis:** `RFLink_Universal_TXROUTER3_GDOUARTLAB1_FULL_SOURCE.zip` (373 oorspronkelijke bestanden)  
**Nieuwe firmware-revisie:** `…-TXROUTER2-DEVICEUI2-GDOUARTLAB1-UX3`  
**Scope van de codewijzigingen:** apparaatkaart, profielkeuze, inleer-/koppelwizard, tekst, polling en bestaande Itho-browsertest. De universele TX-engine, radio-instellingen en decoderlogica zijn niet inhoudelijk gewijzigd.

## Werkwijze — verplicht bij iedere nieuwe functie

Vooraf vastleggen: **Wat wil ik doen? Waarom wil ik dat? Wat moet ik direct zien? Hoeveel navigatieklikken kost het? Hoe weet ik dat het werkelijk gelukt is? Wat als het nog niet kan?**

Na het bouwen *als een onbekende gebruiker* opnieuw de route doorlopen op desktop én mobiel. Daarna als dagelijkse gebruiker en als Advanced-gebruiker. De gebruiker moet doel, status en eerstvolgende actie kunnen begrijpen zonder documentatie te openen. Meer dan drie **navigatieklikken** voor een normale bestemming betekent herontwerpen; een veilige wizard mag meer handelingen vragen. Test daarnaast foutgevallen, focus, achtergrondverversing, data-opslag en hardwarestatus. Een geslaagde hostmock bewijst geen geslaagde RF-uitzending.

## Acceptatiematrix per gebruikerstaak

| Wat wil ik doen? | Waarom? | Wat moet ik meteen zien? | Ingang en verwacht klikpad | UX3-resultaat |
|---|---|---|---|---|
| Systeemstatus begrijpen | Weet ik of RFLink betrouwbaar ontvangt? | Radio-/opslagstatus en heldere fout | Status, 0 klikken | **Behouden**, bestaande NAV3/NAV4-test geslaagd |
| Een apparaat bedienen | Dagelijkse handeling | Naam, ontvangen/bedienstatus, *alleen echte actieve commando's* | Status → Apparaten; direct daarna een actie | **Verbeterd**, mock UX3 geslaagd |
| Een niet-bedienbaar apparaat begrijpen | Ik wil weten waarom knoppen grijs zijn | `Alleen ontvangst`, reden, instelactie | Status → Apparaten | **Verbeterd**, Orcon zonder TX getest |
| Apparaatmodel kiezen | Radio-identiteit bewijst het model niet | Huidig profiel of `Nog niet gekozen`; directe keuze | Status → Apparaten → Profiel kiezen | **Verbeterd**, opslag via API gemockt |
| Instellen/koppelen | Ik wil van ontvangst naar bediening | `Stap voor stap instellen`, stapstatus, eerstvolgende handeling | Status → Apparaten → Instellen | **Verbeterd**, RX-only veilig afgerond; geen valse pairing |
| Ontvangen functies beoordelen | Ik wil weten welke gegevens kloppen | Duidelijke ingang en voorgestelde nieuwe functies | Status → Apparaten → Ontvangen functies | **Verbeterd**, pending goedkeuring en uitklapstatus getest |
| Apparaat beheren / HA | Naam, RF-signalen, HA wijzigen | Compact `Apparaat en RF-signalen beheren`; HA deep link opent direct | Status → Instellingen → HA beheren → knop | **Behouden en getest**, geen extra verborgen klik |
| Een onbekend apparaat leren | Nieuw RF-apparaat toevoegen | Frequentie, Start en detectiestatus | Status → Apparaat toevoegen → Start | **Behouden**, bestaande browser UX-test geslaagd |
| Eigen decoder of profiel maken | Protocol uitbreiden | Directe editor / bibliotheek | Status → Instellingen → Decoderbibliotheek → Aanmaken | **Behouden**, NAV3-test geslaagd |
| Signalen/ruis bekijken en exporteren | Probleem analyseren | Radiofilters, logtype en export | Status → Activiteit; 1 klik | **Behouden**, RX-log-/monitorregressies al aanwezig, niet fysiek getest |
| Diagnostiek openen | Hardwarefout onderzoeken | Direct zichtbaar diagnosepaneel | Status → Diagnose; 1 klik | **Behouden**, NAV3-test geslaagd |
| Instellingen bewaren | Niet twijfelen of iets echt is opgeslagen | Opslaan + duidelijke feedback | Status → Instellingen | **Behouden**, bestaande configuratietests |
| Werkende firmware identificeren | Zeker zijn dat ik niet een oudere webinterface draai | `firmware_revision` | Instellingen → Diagnose/versie | **Nieuwe UX3-revisie ingebouwd**, VERSIONUI1-test geslaagd |

## Concreet veranderde gebruikerservaring

1. **Compacte apparaatkaart**: naam, frequentie en onderscheidende status bovenaan; werkelijk bruikbare bediening en de instelactie direct zichtbaar. Normaal beheer en ruwe RF-details ingeklapt. Geen breed, half leeg schermpaneel met zes uitgeschakelde Orcon-knoppen.
2. **Geen profiel opgeslagen:** het apparaat toont meteen `Apparaatprofiel kiezen`, niet een verborgen of ontbrekende wizard. De knop opent en focust de selector. De bestaande profiel-API bewaart de selectie, de selector klapt daarna in en de instelactie wordt zichtbaar.
3. **Gekozen profiel:** overzicht blijft compact. `Apparaatprofiel wijzigen` is altijd te vinden zonder de standaardweergave te vullen met een groot selectievak.
4. **TX niet beschikbaar:** `Alleen ontvangst` plus een korte, begrijpelijke uitleg; details over 1FC9/RAMSES-II en hardwarevalidatie pas in de wizard. Geen actieve Orcon-TX-knoppen.
5. **TX wel beschikbaar volgens de backend:** actieknoppen zichtbaar; een verzendverzoek wordt *niet* gepresenteerd als bevestigde uitvoering door het apparaat. Tests gebruiken alleen een simulatie van de API, geen echte hardware.
6. **Ontvangen functies:** de secundaire sectie opent bij nieuwe pending functies en behoudt expliciet geopende/dichtgeklapte status tijdens updates.
7. **HA-deep link:** vanuit Instellingen opent de specifieke HA-bediening meteen, ook wanneer het beheer eerder gesloten was.
8. **Gebruikersinstellingen:** vier talen hebben vertalingen voor alle nieuwe UI-labels. Mobiliteit: compacter frequentiefilter, permanente opslag-succesmelding niet meer dominant; bij opslagproblemen verschijnt de waarschuwing nog wel.
9. **Verversen:** de extra apparaatpoll om de vier seconden uit de tweede UI-module is verwijderd. De primaire drie-secondenpoll blijft; de aanvullende module ontvangt daarvan een lokaal event. Gelijktijdige API-calls worden geserialiseerd, een actieve profielselector wordt niet verstoord.
10. **Koppelwizard zonder bewezen TX:** kan correct worden afgerond als `alleen ontvangst`, zonder een verzendverzoek of fictieve succesmelding.
11. **Filterlabel:** `Koppelbare apparaten` vervangen door `Apparaten met instelstappen`, omdat niet ieder getoond apparaat fysiek koppelbaar is.

## Uitgevoerde tests en controles

**PASS — browser/hosttests:**
- `test_ux3_user_journeys.py`: 1440 px desktop / 390 px mobiel, profiel ontbreekt, profielkeuze en opslag, focus tijdens 3 s verversing, RX-only Orcon met instelwizard, Itho met gesimuleerde toegestane opdracht, vier talen, geen horizontale overflow.
- `test_pairing1_browser.py`: onbeschikbare Orcon-TX blijft geblokkeerd, wizard biedt begeleiding, geen RF-verzendactie.
- `test_device_panel_state_txv3.py`: uitklapbare details blijven open na refresh.
- `test_ux_browser.py`: sensorfuncties, HA, verwijderen van RX-identiteiten en opslagfouten.
- `test_nav3_browser.py`, `test_nav4_browser.py`: 3-kliknavigatie, zoeken/filteren, HA-deeplink, browser terug/vooruit, mobile.
- `test_localization_browser.py`: EN, NL, FR, DE en rollback na fout bij taal opslaan.
- `test_itho_ui_browser.py`: gemigreerd naar huidige profielgestuurde bedieningsroute, koppeling en gesimuleerde opdrachten.
- `test_profile_catalog2_browser.py`: 26 profielen, catalogusfiltering en mobiel.
- `test_orcon_family1.py`, `test_qa1_accessibility.py`, `test_settings_compact1.py`, `test_radio_monitor_config.py`, `test_board_config.py`, `test_esphome_generated_include_paths.py`, `test_radio_engine_v2_manifests.py`, `test_packaging.py`, `test_version_ui.py`.
- Host-C++: `test_gdo_serial_tx_session.cpp`, `test_gdo_uart_wire_equivalence.cpp` (met `tx_codec.cpp` en `codec.cpp` gelinkt), `test_radio_mode_manager.cpp`.
- Ingebedde UI opnieuw gegenereerd vanuit dezelfde vier JSON-vertaalbestanden; packaging- en localechecks geslaagd.

**Niet aangetoond:**
- De volledige ESPHome/ESP-IDF-crosscompile, OTA, werkelijke bediening/Orcon-pairing, GDO0-golfvormmeting, RX-herstel op de PCB, Home Assistant met een echte MQTT-broker en meerdaagse RF-/geheugenstresstest zijn **niet** uitgevoerd.
- De afzonderlijke `run_web_smoke.sh` C++-webcompile met host-stubs blokkeert op ontbrekende `driver/uart.h` uit ESP-IDF. De frontend/host- en losse GDO-tests slagen, maar deze ontbrekende stub is **geen bewijs** van een geslaagde firmwarebuild.
- Beveiligingsprobleem uit eerdere QA1-audit blijft bestaan: HTTP-beheer mist sterke toegangscontrole. Niet rechtstreeks op internet publiceren.

## Tweede UX-review: beoordeling na implementatie

**Nieuwe gebruiker:** ziet direct welke bediening mogelijk is, welk profiel gekozen is en hoe te beginnen met instellen. Geen dummy-transmit. **PASS in simulatie.**

**Dagelijkse gebruiker:** uitklappers zijn secundair, bruikbare actie zonder extra uitklap zichtbaar. De mobiele kaarten staan eerder op het scherm; zoeken en radiofilter blijven rechtstreeks bereikbaar. **PASS in simulatie.**

**Advanced gebruiker:** RF-signalen en risicovolle beheeracties blijven vindbaar; HA-deep link opent het juiste beheer. **PASS in beschikbare browsertests.**

**Nog te verbeteren:** de volle app bevat nog meerdere specialistische pagina's en grote tabellen die niet in deze UI-edit opnieuw zijn ontworpen. De RX/TX-architectuur moet ook nog echte hardwarevalidatie krijgen. Niet als 'alle UX af' presenteren.

## Firmware installeren zonder persoonlijke instellingen te verliezen

- Gebruik deze **volledige SOURCE-ZIP** als nieuwe ontwikkelbasis, niet een stapel losse patches.
- **Neem jouw bestaande aangepaste ESPHome-YAML over**, inclusief je bekende SPI-instelling van 6 MHz, GPIO-mapping, sleutels en overige persoonlijke instellingen. De meegeleverde standaard-YAML heeft de conservatieve standaardconfiguratie en is niet jouw actuele persoonlijke YAML.
- Gebruik de meegeleverde updater waar passend; voer ESPHome Validate en Compile uit en controleer de ongewijzigde fysieke pinout. Maak altijd eerst backup voordat je zelf OTA uitvoert.
- Controleer na het flashen `/rflink/api/status`: `firmware_revision` moet eindigen op `-GDOUARTLAB1-UX3`. Test dan RX van alle drie radio's en de browser op mobiel/desktop. **Schakel `gdo_serial_tx_lab` niet aan als poging om Orcon-knoppen te ontgrendelen.**

## Volgende acceptatiepunten

1. Full ESPHome 2026.10-build op de eigen YAML en ESP32-S3; boot, drie radio's, RX, website, taal en OTA.
2. Werkelijke simulatie met een open/gesloten detailssectie tijdens ontvangst, na reboot, na aanpassen van profiel, bij defecte radio en bij offline MQTT.
3. Hardwarevalidatie van **protocolonafhankelijke** TX-transporten, met oscilloscoopmeting, veilige lease, foutpad, RX-herstel en afzonderlijke pairing/ACK-verificatie. Geen specifiek Orcon-pad buiten de universele TX-router.
4. Webtoegangscontrole en langdurige geheugentest vóór publiek gebruik of stabiele release.
