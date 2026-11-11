# UXREVIEW5 — gebruikersaudit, stabiliteitsanalyse en testresultaten

**Bronbasis:** ORCONTXLAB2 + LOGBOOK1 + PACKFIX1 + DEVICEUX4, aangeleverd 1 november 2026.  
**Nieuwe interne revisie:** `...-DEVICEUX4-UXREVIEW5`  
**Publieke UI-versienaam:** `v0.5 Radiolove` (de technische firmwareversie/revisie blijven ongewijzigd op API-niveau en zichtbaar onder details).  
**Doel:** een begrijpelijke dagelijkse UI met volledig gescheiden detailtaken, behoud van universele TX, en beter onderzoek van herhaalde resets.

## 1. Wat wil ik doen? Waarom? Wat verwacht ik direct?

| Gebruikersdoel | Waarom? | Meteen zichtbaar | UX-uitkomst / test |
|---|---|---|---|
| Status zien | Controleren dat RFLink werkt | Drie radio's, status, gezonde toestand | Bestaand; geen ongegronde werkende-TX-status claimen |
| Apparaat bedienen | Direct een stand/actie kiezen | Alleen daadwerkelijk beschikbare bediening | DEVICEUX4: direct op compacte kaart; Orcon TX afhankelijk van gekoppeld en bevestigd |
| Apparaat inleren | Nieuw apparaat toevoegen | Frequentie en korte stappen | Bestaande wizard; doel max. 3 navigatieklikken, inleren zelf mag meer stappen |
| Profiel kiezen/koppelen | Onbekend apparaat herkenbaar maken | Eén volgende stap, eigen detailweergave | DEVICEUX4 browser 1440/390 PASS; profiel ontbreekt -> keuze, geen kapotte kaart |
| RX/TX onderzoeken | Achterhalen waar signaal/commando stopt | Alles/Radio/Systeem/Web en zo nodig details | **Nu dezelfde logviewer** voor radio én andere events, paginering; 1440/390 PASS |
| Loggen aanpassen | Geheugengebruik en detailniveau aanpassen | Radio, Systeem, Web onder elkaar | Herschikt en in browser gecontroleerd |
| Versie zien | Snel weten wat geïnstalleerd is | `v0.5 Radiolove` en korte omschrijving | Detailknop met patchnotes en volledige technische revisie, browsertest PASS |
| Bibliotheek beoordelen | Profielen/decoders terugvinden | Aantal en update-status meteen zien | Samenvatting zonder tweede Advanced-klik; technische metadata blijft optioneel, browsertest PASS |
| Experimentele TX debuggen | Veilig hardwarefouten onderzoeken | Expliciet TX-lab met status en waarschuwing | Lab apart van Logboeken → Systeem; vanuit lab directe logknop, geen hardwareclaim |
| Firmware veilig updaten | Bestaande PCB-config behouden | Welk BAT-script ik moet starten | `UPDATE_RFLINK.bat` voor persoonlijke 6 MHz YAML, `MAAK_RELEASE.bat` voor DEFAULT |

## 2. Logviewer als één geheel

- Vier bronkeuzes: Alles, Radio, Systeem en Web.
- **Één zichtbare tijdlijn** voor alle bronnen; een radiofilterbalk verschijnt alleen wanneer Radio geselecteerd is.
- De tijdlijn heeft een vaste hoogte en interne scroll; blijft op 390 px mobiel zichtbaar.
- De native radiologfilters op signaal/ruis, frequentie en RX/TX blijven bestaan, inclusief gedetailleerde RAW-/RF-info, ZIP-export en 'oudere gebeurtenissen ophalen'.
- Backend-radiolog en systeemtrace blijven aparte begrensde buffers; de browser voegt alleen de presentatierijen samen.
- Timestamps worden op basis van ESP32-uptime waar mogelijk gesynchroniseerd; browser- en ESP32-klok blijven principieel verschillend. Een geslaagde TX-API-call bewijst geen reactie van het apparaat.
- De webbrowser bewaart alleen eigen webgebeurtenissen; deze bestaan niet automatisch op andere clients of na browserwissel.

## 3. Wat is werkelijk vastgesteld over de crashes?

Het aangeleverde web-logboek (1 november) had GET `/rflink/api/devices` duren van **20.310, 21.717 en 46.365 ms** en fouten bij `/library` en `/user-library`. Dat bewijst **niet** dat een ontbrekend profiel de panic veroorzaakt. De C++-profile-lookup heeft al null-/lege-ID-controles. Ook netwerkstoring, zware JSON-serialisatie, intern geheugen of races zijn mogelijke oorzaken.

**Deze release voegt toe:**

1. Metingen van de bouwduur en grootte van de JSON-antwoorden van `/devices`, `/library` en `/user-library`; uitzonderlijk grote/trage antwoorden verschijnen in System Trace en de ESP-log.
2. Logmeldingen vanaf het begin van de betreffende C++-serialisatiepaden (ESP_LOGD) voor gerichte seriële debug.
3. UI-guards: nooit meerdere gelijktijdige automatische `/devices`- of `/status`-aanvragen als de vorige aanvraag nog bezig is. Eerder was er bij langzame device-responses een wachtende vervolgrequest; die keten wordt nu niet meer eindeloos opgebouwd.
4. De experimentele systeemdebugpoll loopt alleen als dat scherm daadwerkelijk actief is, met één gelijktijdig verzoek. Hij belast de ESP32 niet ongemerkt in andere pagina's.

**Niet opgelost of bewezen:** er is geen fysiek gemeten panic-backtrace, geen volledige ESP32-S3 ESP-IDF-build, geen hardware-soak. Noem dit daarom **diagnose & belastingbeperking**, niet CRASHFIX. Het is niet veilig om ontbrekende profielen te verwijderen of NVS te wissen op basis van alleen de browserlog.

**Wat bij een volgende panic vastleggen?**

- `GET /rflink/api/status`: `firmware_revision`, `boot_session_id`, `boot_count`, `reset_reason`, vrij/min. geheugen.
- Logboeken → Alles exporteren; kijk naar `api` events met duur en omvang.
- Seriële ESPHome/ESP-IDF panic-backtrace en decodeerbare symbolen van exact dezelfde build (`.elf`), indien beschikbaar.
- Of de crash ook voorkomt bij gesloten browser, of alleen bij Apparaten/Bibliotheek en hoe lang de uptime was.
- Echte fysieke stabiliteit: bij voorkeur meerdere dagen, RX-ruis en pagina-API's tegelijk.

## 4. Documentatie en Windows-scripts

- Eerst waren er 96 Markdown-bestanden, waarvan 8 exact dubbele bestanden in DEVICEUX4 opgeschoond zijn.
- In deze iteratie zijn **24 niet door de releasegenerator verplichte historische documenten** van de projectroot naar `docs/history/` verplaatst; inhoud blijft behouden.
- `HANDBOEK_RFLINK_NL.md` is nu het centrale actuele overzicht. Het indexbestand `docs/DOCUMENTATIE_OVERZICHT_NL.md` is bijgewerkt.
- `MAAK_RELEASE.bat`: DEFAULT. `UPDATE_RFLINK.bat`: CUSTOM. `TEST_UPDATE_RELEASE.bat`: regressies. `MAAK_UPLOAD.bat`: legacy alias, bewust behouden vanwege compatibiliteit.
- Kritieke root-documenten (door tests/releasegenerator verwacht) zijn ongemoeid gelaten.

## 5. Testmatrix — onderscheid maken tussen simulatie en hardware

| Controle | Uitkomst |
|---|---|
| DeviceUX4 browser 1440/390 px | PASS |
| Unified LOGBOOK browser 1440/390 px, filters, export, limieten, version details en library | PASS |
| NAV4 browser (navigatie, zoeken, filter, history) | PASS |
| Orcon experimentele 1FC9-wizard browser (mock) 1440/390 px | PASS |
| Itho browserbediening (mock) | PASS |
| Bibliotheek/browser (mock) | PASS |
| PACKFIX1 releasecheck plus negatieve incomplete-UI-gevallen | PASS |
| Pakketintegriteit en broncompleetheid | PASS |
| DEFAULT-releaseradioguard en versie-/PCB-contract | PASS |
| Profileflow host C++-syntax op stubs | PASS |
| Volledige oudere NAV3-test | NIET AFGEROND (liep vast/timeout) |
| Volledige ESPHome/ESP-IDF cross-compile | NIET BESCHIKBAAR op dit systeem |
| Windows PowerShell `MAAK_RELEASE.bat` uitvoering | NIET GETEST op Windows |
| Echte CC1101 TX/RX, Orcon koppeling, 72 uur soak | NIET GETEST |

De browserfixtures zijn **gesimuleerde API's**. Ze bevestigen de gebruikersroute, niet een daadwerkelijke radio-uitzending. De volledige oudere testsuite is niet als geheel als geslaagd verklaard.

## 6. Wat staat nog open?

1. **Panic-rootcause** achterhalen met echte ESP-IDF crashdata en geheugenmeting vóór de volgende hardwarevrijgave.
2. Website-authenticatie voor alle muterende HTTP-routes; huidige interface uitsluitend op vertrouwd LAN.
3. Universele TX en Orcon-RAMSES-II-methode op echte CC1101 meten; pas na bewijs normale stabiliteit claimen.
4. Langlopende RF-/web-/log-stresstest, inclusief opgeschoonde pagina-API's.
5. Eventuele verdere consolidatie van de nog verplichte rootdocumenten pas na aanpassen van de releasegenerator en updater; niet blind verwijderen.

**Vrijgavestatus:** experimentele SOURCE, geen stabiele, op hardware bewezen firmware.
