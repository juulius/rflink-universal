# Historische release-notities (vervangende updaterinstructies staan in START_HERE.md)

# RFLink Universal RC4-HF4 PINOUT / UX2 (20 september 2026)

## Belangrijk: veilige standaardrelease / updater (scriptrevisie 20-09-2026)

- `MAAK_RELEASE.bat` zonder argument maakt ALTIJD de **DEFAULT**-release uit de YAML en code direct in deze uitgepakte projectmap; `update_output/` wordt genegeerd, ook als een eerdere updater faalde. Outputs heten `..._DEFAULT_SOURCE.zip` en `..._DEFAULT.esphomebundle.tar.gz`.
- `UPDATE_RFLINK.bat` is alleen nodig om een BESTAANDE aangepaste ESPHome-YAML over te zetten. Het maakt een voorstel in `update_output/`, verandert geen bronbestanden, en gebruikt voor alle 13 hardware-GPIO-labels altijd de waarde van de NIEUWE release. Afwijkende oude pinnen worden in `WIJZIGINGEN.txt` gerapporteerd; bekijk die pinout voor je flasht.
- Alleen `MAAK_RELEASE.bat update` kiest de nieuwste updateruitvoer, controleert hashes en conflicten en maakt een afzonderlijke `..._CUSTOM`-release. Bij een fout stopt uitsluitend de CUSTOM-modus. Het gewone DEFAULT-archief wordt nooit door CUSTOM overschreven.
- Ook voor DEFAULT blijft de YAML bovenaan het aanpasbare uitgangspunt. Bij fysiek afwijkende PCB's stel je de pinnen daar bewust in; het updater-script zet ze niet stilzwijgend terug naar je oude YAML. Voor je apparaat: eerst ESPHome Validate en Compile; hardware/OTA nog te testen.


Deze bron is gemaakt van de door de gebruiker aangeleverde volledige UX2_SOURCE-zip.
De firmwarefunctionaliteit (opstarten zonder radio's, OTA, monitor, decoders, LED en RMT) is behouden.
**Alle radio- en SPI-GPIO's staan nu centraal bovenaan `rflink-universal.yaml` in `substitutions`.**
De oude radio 2-pinset 15/16/17 is verplaatst naar 9/8/10; 868 is verplaatst van 8/9/10 naar 15/16/17.
Radio 1 blijft CS/GDO0/GDO2 = 5/6/4, SPI SCK/MOSI/MISO = 12/11/13.
**RGB-led blijft GPIO48**, zoals in de werkende UX2-bron; voor een LED op GPIO47 wijzig alleen `status_led_gpio` bovenaan.
De 868 GDO2 op GPIO17 is alleen als gereserveerde pin vastgelegd: er is geen nieuwe listener toegevoegd.

Voor een complete Windows-release dubbelklik `MAAK_RELEASE.bat` (zonder Python).
Die maakt een `SOURCE.zip` en een `.esphomebundle.tar.gz` onder `dist/`.
Importeer de **.esphomebundle.tar.gz** in ESPHome Device Builder; de SOURCE.zip is het bewerkbare project.
De tar.gz is geen vooraf gecompileerde firmware en import installeert niets automatisch.
Maak eerst een backup van je bestaande YAML/secrets. Controleer daarna in ESPHome Validate, Compile en OTA.
Dit pakket heeft een statische/hosttest; echte CC1101-radiotests, volledige ESPHome-compile en Windows-scriptuitvoering moeten nog plaatsvinden.

---

# UX2 — RF-codes veilig beheren (19 september 2026)

- Apparaten > Signalen beheren toont de daadwerkelijke RF-identiteiten per apparaat; de interne `dev_...`-identiteit blijft stabiel.
- Een afzonderlijke RF-code kan worden ontkoppeld zolang minimaal één andere RF-code overblijft. Voor de laatste RF-code: vervang hem met een opnieuw ontvangen code of verwijder het hele apparaat bewust.
- Vervangen kan uitsluitend door een minstens drie keer ontvangen kandidaat met hetzelfde protocol en apparaattype, en niet door een code die al bij een apparaat hoort. Een willekeurige apparaatcode intypen is bewust niet toegestaan.
- Naam, overige codes en apparaat-ID blijven behouden; wijzigingen worden via de bestaande twee NVS-snapshots opgeslagen, met rollback bij opslagfouten.
- Gekende acties worden apparaatbreed bewaard. Bij decoders die meerdere knoppen met één stabiele RF-identiteit aanduiden is nog geen betrouwbare koppeling tussen één knop en één RF-code beschikbaar. De UI zegt dit expliciet en doet niet alsof één specifieke knop kan worden gewist.
- Het apparaat wordt niet verwijderd, `secrets.yaml` en de persoonlijke hoofd-YAML veranderen niet. HTTP lokaal blijft behouden; geen browsercertificaten toegevoegd.
- Host-tests: gecodeerde opslag/reboot, vervangen, mislukte vervanging, voorkomen van ontkoppelen van de laatste code, webservercompile en browserinteracties. Geen volledige ESPHome-compile of echte RF-test uitgevoerd. Windows BAT-verpakker moet op Windows zelf nog worden uitgevoerd.

# UX1 — eenvoudiger gebruik en permanente apparaten (19 september 2026)

- De webinterface blijft **HTTP op het lokale netwerk**. Geen TLS-certificaat, geen verplichte HTTPS. Native ESPHome-API-encryptie en OTA blijven ongewijzigd; dit zijn andere verbindingen. Publiceer de webinterface niet op internet: er is nog geen aparte webauthenticatie.
- Dashboard toont één systeemkaart: wifi/webinterface online, radio's afzonderlijk, verwacht/werkelijk aantal decoders, NVS-opslag en aantal bewaarde apparaten. Hardware ontbreekt betekent niet dat de ESP offline is.
- Leren: kies een beschikbare radio, start, laat de achtergrond automatisch vijf seconden meten, bedien de afstandsbediening en geef een naam op hetzelfde scherm. Ontbrekende radio's blijven grijs, geen wizard zolang opslag niet werkt. Ontwikkelaarshulpmiddelen blijven apart beschikbaar.
- Bevestigde apparaten worden voortaan in de ESP32-NVS bewaard met twee gecontroleerde snapshots, versie en checksum. Na herstart worden namen, protocol, RF-identiteiten, acties/velden en koppelingen hersteld; tijdelijke kandidaten en RF-monitorhistorie niet. Opslaan gebeurt bij bevestigen, koppelen, hernoemen en verwijderen; bij een mislukte opslag wordt de wijziging teruggedraaid. Maximum 24 apparaten en maximaal 8192 bytes per snapshot (bij grote records kan de grens eerder bereikt worden).
- Apparatenpagina heeft hernoemen, verwijderen met bevestiging en opslagstatus. Er is geen export/import-back-upfunctie in UX1; deze is apart te bouwen. Apparaten die in oudere firmware alleen in RAM stonden, kunnen niet met een update achteraf worden teruggehaald.
- De bestaande hoofd-YAML, GPIO48-status-LED, RMT-verdeling, drie CC1101-radioprofielen en decoderpakketten zijn in deze UX1-update ongewijzigd. Update de nieuwe C++/header-bestanden EN web-UI; behoud persoonlijke YAML en secrets. Clean Build Files, compileren en OTA naar het bestaande device; nooit eerst verwijderen.
- Belangrijkste rooktest op hardware: apparaat leren → naam geven → apparaat zichtbaar → ESP32 herstarten → naam en identiteiten nog aanwezig; opslagfout moet een duidelijke melding geven. Full ESPHome compile, echte NVS/herstart, native Windows BAT en radio-hardware zijn hier niet fysiek geverifieerd.

## RADIOLED hotfix (bovenop REGFIX)

- Laat de groene gereed-hartslag alleen zien als alle drie CC1101-modules en bijbehorende benodigde RX/TX-drivers gereed zijn.
- Toon dubbel oranje bij ontbrekende radio's; blijf de website/API/OTA aanbieden. Wit OTA gaat vóór hardwarewaarschuwingen; rood blijft de echte foutstatus.
- Bij native ESPHome OTA: stuur de WS2812-driver fysiek aan op begin/voortgang/einde/fout; `light.turn_on()` alleen zet tijdens de geblokkeerde normale applicatielus niet altijd de LED aan.
- Bij OTA einde wordt de werkelijke toestand hersteld, niet altijd wit.
- Host-test compileert werkelijke YAML-callbacks en controleert directe hardware-output met gesimuleerde LightState (geen claim over echte ESP32-compilatie of OTA).
- Behoud persoonlijke YAML en secrets; neem wijzigingen in OTA-callbacks in de hoofd-YAML en twee RFLink-componenten over.

## REGFIX — decoderregistratie (19 september 2026)

De voorgaande release kon stilzwijgend een leeg decoderregister opleveren wanneer
de gegenereerde linker-header niet aanwezig was. Nu is die header verplicht
bij ingeschakelde decoders en registreert een linkeranker de decoder expliciet.
De bootlog vergelijkt werkelijke registratie met verwacht aantal (8 packs,
12 decoders). Build-stale decoders worden opgeschoond. Zie REGISTRATION_FIX.md.
Dit is hostgetest, niet met een volledige ESPHome-build of op hardware bewezen.

## LED UI HEARTBEAT FIX (2026-09-18)
- Positieve checkbox voor groene gereedhartslag; bewaarde quiet/NVS-voorkeur wordt correct omgekeerd.
- UI toont GPIO48 in plaats van oude GPIO47-tekst.
- Werkelijke JS-gebeurtenissen en API-POST zijn als regressietest toegevoegd.
- Geen wijzigingen aan pinouts, radiocode, bootvolgorde of LED-hardwaredriver.

# LEDFIX addendum 2026-09-18

GPIO47 in de oorspronkelijke RC4-HF3-YAML was voor deze CH343P/N16R8-boardrevisie waarschijnlijk onjuist. De nieuwe standaard is GPIO48; zie LEDFIX_README.md. De originele web- en radio-bootfix blijven behouden. De live hoofd-YAML moet ook worden bijgewerkt bij een ESPHome-bundle-update. Hardware/OTA-test nog vereist.

# RADIO-OPTIONAL PATCH (2026-09-18)

Drie CC1101-modules mogen ontbreken zonder dat RFLink die als actieve radio meldt. Softwarestatus via ESPHome `is_failed()`, geen RF-ontvangst of Itho-TX op mislukte modules, GDO2 pulldown, 409 bij starten leren zonder radio. Web/API/OTA blijven onafhankelijk van RF-status.

**Status:** host-C++-compile, static YAML/JS en regressiechecks uitgevoerd; volledige ESPHome-firmwarecompilatie, USB-log en fysieke boot op ESP32-S3-N16R8 zijn hier niet bevestigd. Het ontbreken van radio's is niet bewezen als oorzaak van het originele OTA- en webprobleem. Zie RADIOLOOS_TESTEN.md.

# RFLink Universal v0.3.3-RC4-HF3 — releaseoverdracht (18 september 2026)

## Nieuw ten opzichte van HF2

- RGB-statusregeling `status_led.h` met 11 toestanden, patronen en vaste
  voorrang: opstarten, verbinden, gereed, offline, inleren, capture, bevestigd,
  Itho-koppelopdracht, TX, OTA en fout.
- Verbonden met echte ESPHome-Wi-Fi- en OTA-callbacks, RFLink setup,
  inleer- en bevestigings-API, Itho-scripts en de beschikbare TX-acties.
- Witte OTA-indicatie gebruikt ook OTA-voortgangscallbacks omdat ESPHome de
  gewone animatieloop tijdens flashen kan blokkeren. Dit is niet gegarandeerd
  een volkomen vloeiende puls; firmwarecompile en echte OTA-test ontbreken nog.
- Itho-koppelpatroon is de *uitgaande opdracht*, geen bewezen pairing bij de
  ventilatiebox. Paars heeft timeout zodat scriptfouten niet tot een
  permanent actieve LED-status leiden.
- Optie voor helderheid (1–100%, standaard 20%) en het uitschakelen van de
  normale groene hartslag in `/rflink`; instellingen in ESP32-preferences.
- Leerpagina: knop `Stoppen en meting wissen` beëindigt de echte leersessie.
- Uitvoerbare C++-policytests en controles voor de daadwerkelijke productie-
  koppelingen, webcontrols en packaging toegevoegd. De bewerkbare HTML en
  ingebedde `web_ui.h` zijn identiek gecontroleerd.
- YAML, programmaversie, installer-BAT en packer bijgewerkt naar RC4-HF3.
  Oude losstaande hotfix niet opnieuw draaien; geen nieuwe RF-decoders.

## Uit HF2 en HF1 behouden

- WebHandler-linkerfix: reeds onderdeel van de oorspronkelijke ontvangen ZIP.
- Nieuwe printmapping: SPI MOSI11/SCK12/MISO13; RF1 CS5/TX6/RX4,
  RF2 CS15/TX16/RX17, RF3 CS8/GDO0 9; GPIO10 fysiek gereserveerd.
- Oorspronkelijk WS2812 op GPIO47 ingesteld; gecorrigeerd naar GPIO48 in LEDFIX. GRB en boardrevisie fysiek bevestigen.
- Eén BAT + Python-verpakker genereert volledige ZIP én tar.gz,
  controleert de inhoud en sluit secrets, backups en buildbestanden uit.
- De oorspronkelijke RC4 WebHandler Fix ZIP vermeldde zelf dat deze uit een
  RC3-codebasis is opgebouwd. **Niet aantoonbaar hetzelfde als originele RC4.**

## Testbereik en risico's

De in `TEST_RESULTS.txt` beschreven korte regressiestappen zijn uitgevoerd.
Statische YAML-analyse en C++-hoststubs bewijzen geen volledige firmwarecompile.
ESPHome/ESP-IDF is in deze uitvoeromgeving niet geïnstalleerd; er is nog geen
fysieke ESP32-S3-, WS2812-, radio- of pairingtest uitgevoerd. Voer dus eerst
Validate/Clean Build/Compile in ESPHome uit, controleer fysieke PCB-pinnen en
maak een backup van je oude werkende firmware.

De webinterface bevat geen ingebouwde HTTP-authenticatie: vertrouw uitsluitend
op een afgeschermd LAN, zonder internetportforwarding. Bestaande secret-namen
zijn niet gewijzigd; de virtuele Itho-ID is behouden, controleer of die uniek is.

Voor kleuren, prioriteiten en eerlijk geformuleerde betekenissen zie
`rflink_universal/options/README_RGB.md`; voor installatie `START_HERE.md`.

## Dual-mode distributie (verpakkingsaanvulling, GEEN nieuwe firmwarecode)

- `MAAK_RELEASE.bat` + `rflink_universal/tools/maak_release.ps1` maken met native
  Windows-tools een complete SOURCE.zip en een geldige Device Builder-bundle
  met manifest.json, lokale custom componenten, decoders en RGB-package.
- Dezelfde bundle kan worden gebruikt voor een nieuwe deviceconfiguratie of om
  conflicterende bronbestanden van een bestaand device te vervangen.
- Bij updates `secrets.yaml` behouden en de hoofd-YAML zo nodig veilig mergen;
  voor RADIOMON is een YAML-merge altijd vereist. Bronbestanden importeren
  is niet hetzelfde als OTA-installeren. Zie INSTALLEREN_EN_UPDATEN.md.
- Het oude Python-verpakscript blijft alleen voor oudere ontwikkeltests staan;
  de user-facing BAT-route gebruikt helemaal geen Python.
- Dit is nog **geen** geteste Windows-run, ESPHome-firmwarecompile of fysieke OTA.

## Addendum — Windows YAML-updater + newest-output release selectie

- `UPDATE_RFLINK.bat` en `rflink_universal/tools/update_rflink.ps1` toegevoegd: veilige conservatieve 3-weg YAML-merge van baseline/huidige/nieuwe YAML, substitutions per sleutel, ongewijzigde originelen en drie afzonderlijke back-ups per uitvoermap. Bij versiemismatch zonder correcte baseline of overlappende wijzigingen: geen `READY`.
- Unieke mappen in `update_output/` bevatten steeds een vaste `rflink-universal.yaml`, het bijbehorende runtimebestandensnapshot, wijzigingsrapport en hashmanifest. Persoonlijke backups of metadata komen nooit in de ESPHome-bundle.
- `MAAK_RELEASE.bat` selecteert bij meerdere updater-uitvoermappen de nieuwste op manifest-UTC-datum (niet op alfabetische mapnaam), neemt de YAML **en de runtime uit dezelfde map**, controleert SHA256, weigert conflicten/incomplete mappen en valt niet terug op oudere outputs.
- Het releasearchief `.esphomebundle.tar.gz` bevat uitsluitend ESPHome-data; het volledige `_SOURCE.zip` bevat bewust ook tools en tests voor ontwikkelaars. Oude dist-archieven worden niet meer vooraf gewist bij een mislukte build.
- `TEST_UPDATE_RELEASE.bat` bevat Windows-integratietests voor nieuwste map, clean bundle, conflict, hashcorruptie en behoud van vorige release bij fouten. Windows-executie en echte ESPHome/OTA-tests zijn nog niet bevestigd in de huidige Linux-testomgeving.
- Extra fail-closed: als een updater-run al vóór een nieuwe output faalt, zet hij `update_output/LAST_UPDATE_FAILED.txt`; de release-maker weigert oude READY-outputs te hergebruiken totdat een nieuwe updater-run slaagt.

## Windows-startscriptcorrectie (geen firmwarewijzigingen)

De drie BAT-starters geven geen `-ProjectRoot "%~dp0"` meer door; PowerShell bepaalt de projectmap via `$PSScriptRoot`. Hiermee wordt de extra eindquote in `Resolve-Path` voorkomen. De Windows-regressietest roept nu de BAT-bestanden rechtstreeks aan vanuit paden met spaties. Dit is een packagingfix, géén bewezen firmware- of OTA-test.

## HF3 BOOTFIX / ESP32-S3 boot-crash
- Oorzaak uit USB-log: `RFLinkUniversal::setup` startte webserver op DATA-prioriteit vóór wifi/LwIP, waarna `xQueueSemaphoreTake` assertte. `get_setup_priority()` is nu WIFI - 1, gelijk aan ESPHome web_server.
- Tweede fout: beide remote_receiver-instanties vroegen DMA RX, terwijl ESP32-S3 maar één DMA RX-kanaal heeft. Beide gebruiken nu non-DMA en 96 RMT-symbolen.
- Het vorige RADIO-OPTIONAL-gedrag is behouden. De log met `0 decoder(s)` blijft een aparte, onbewezen firmware-registratiekwestie.
- Zie BOOTFIX_README.md voor exacte teststappen en beperkingen.


### RMTFIX (2026-09-18): ESP32-S3 two transmitters + on-board WS2812
- User logs: two remote_transmitter components failed to reserve RMT symbol memory, despite successful ESPHome API and functioning website.
- For ESP32-S3 non-DMA TX hardware: tx_433_42=48, tx_433_92=48, onboard WS2812=48; total 144/192 TX symbols, one 48-symbol block remains free.
- Receivers remain 96+96=192 separate RX symbols; use_dma:false.
- The third CC1101 (868.2999MHz Itho) is NOT removed: remains 2-FSK SPI packet RX/TX, no RMT.
- Both YAML files need updating when importing onto existing device: rflink-universal.yaml AND rflink_universal/options/rgb_status_ws2812.yaml. Preserve secrets and personal pinouts; if merging, ensure TX allocations survive.
- Release script now verifies each named RX/TX channel, LED allocation and 868 packet mode before publishing, including when selecting latest update_output folder.
- Static allocation/negative regressions and existing host checks only; actual ESPHome 2026.9 firmware compilation, physical TX/LED testing, and Windows BAT execution not verified in this environment.


### RADIOMON (2026-09-18): radioselectie en beperkte activiteitenmonitor
- Nieuwe Monitor-pagina op desktop/mobiel: totaal of afzonderlijk 433,42, 433,92 en 868,30 MHz; RX-, TX- of beide richtingen.
- Afwezige radio's verschijnen uitgegrijsd en zijn niet selecteerbaar bij Leren en in de monitor. De leerknoppen blokkeren wanneer geen enkele ontvanger werkt. Radiostatus wordt elke 2 seconden gepolld, pagina elke 3 seconden ververst.
- Per 433-radio checkt de firmware afzonderlijk CC1101, RMT RX en RMT TX; bij de 868MHz-radio blijft de bestaande CC1101 SPI-pakketmodus behouden.
- Geschiedenis maximaal 32 gededupliceerde RX-frames en TX-opdrachten, zonder permanente opslag. 433 RX: hash + pulsaantal; 868 RX: eerste 16 pakketbytes + RSSI. TX-registratie bewijst GEEN fysieke uitzending of ontvangst. De nieuwe UI stuurt geen willekeurige raw TX-commando's.
- Beide 433MHz on_transmit-callbacks en de drie Itho-scripts melden TX-opdrachten alleen binnen bestaande checks op fysieke radiobeschikbaarheid. Niet-detecteerde hardware genereert geen nep-TX-log.
- Bestaande BOOTFIX, HEARTBEAT UI, LED GPIO48 en RMTFIX zijn behouden. Radioselectie en activiteitenmonitor zijn getest in headless Chromium met afwezige/aanwezige radio's, dynamische statusverandering, RX/TX-filtering en 868MHz-selectie; firmwarelogica is ook met host-C++ getest. Full ESPHome-compile, echte aangesloten radio's, fysieke TX/RX, OTA en Windows BAT-uitvoering zijn hier NIET getest.
- De hoofd-YAML wijzigt voor de vier-argumenten beschikbaarheidspoll en TX-registratie. Bij updates zorgvuldig mergen; een ongewijzigde oude YAML is niet compatibel met de nieuwe C++-signatuur.


## Decoderuitbreiding 1

Nieuw: Acurite 609TXC, Rubicson/TFA 30.3197 (36 bit) en Fine Offset WH2 (433,92 MHz RX), met checksums/CRC en hosttests. Alle bestaande radio's, de website en het LED-gedrag blijven intact. Bekijk `DECODER_EXPANSION.md` voor beperkingen en update-instructies.

Decoderlinker-fix: registraties hebben nu expliciete gegenereerde symbolen. Negatieve statische-archieftest bewijst dat de oude methode kon opstarten met nul decoders.
