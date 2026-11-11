# v0.6.1 — Radio Gaga (UARTTRACE61 + DEVICEDETAIL61)

- UART1-driver installeren vóór 8N1/baud-configuratie; hardwarestappen met exacte ESP-IDF-foutcodes en CC1101-registerreadbacks.
- Radio/Systeem-loggen nu iedere fysieke zendvoorbereiding, afwijzing, en RX-herstel; geen onterechte TX-succesmelding vóór het doorlopen van de driver.
- Rustige, minder geneste apparaatsectie met direct zichtbaar Home Assistant/naambeheer en uitklapbare RF-technische onderdelen.
- Het juiste eerste foutdetail blijft behouden bij succesvolle RX-rollback; RX-herstel blijft fail-closed.
- Zie `V061_RELEASE_NL.md`; echte ESPHome-build en fysieke RF-test staan nog open.

---

# v0.6 — Radio Gaga (UI-unificatie)

- Rustiger Status-overzicht: kerncijfers, radio's, recente gebeurtenissen, aandachtspunten.
- Compacte uniforme apparaatkaarten met alleen relevante acties; profiel/koppeling apart.
- Apparaatdetails vereenvoudigd; technische gegevens op verzoek en verwijderen met bevestiging.
- Aansluiting op het bestaande ontwerp voor Instellingen en Logboeken/Diagnose.
- Vier talen, veilige lege logstatistieken, versie-afhankelijke Windows-releasecontroles.
- Bronhash `19a467a723ef`; zie `V060_RELEASE_NL.md` voor acceptatiecriteria, tests en beperkingen.
- Experimentele fysieke TX en hardware-stabiliteit zijn nog niet gevalideerd.

---

# Nieuwste release: v0.5.5 Radiolove

Zie `V055_RELEASE_NL.md`.

## v0.5.4 Radiolove — CRASHTRACE1

ESPHome-crashbacktraces (core, PC, BT0…) worden waar beschikbaar beperkt overgenomen in NVS en verschijnen in Logboeken → Systeem. Bestaande resetredenen blijven behouden. Zie `V054_RELEASE_NL.md`.

# v0.5.3 Radiolove — UPDATE_RFLINK PowerShell-parserfix (3 november 2026)

- Corrigeert drie foutieve variabeleverwijzingen `$label:` in `rflink_universal/tools/update_rflink.ps1`; `${label}:` wordt correct verwerkt in Windows PowerShell 5.1.
- Voeg een Windows parserpreflight toe aan de updater-/releaseregressietest en een platformonafhankelijke interpolatiecontrole.
- Logboeken haalt de opgeslagen NVS-resetgeschiedenis zelfstandig op, zonder afhankelijkheid van Status. Geen verandering aan de gebruikers-YAML, RF-firmwarelogica, TX-encoder of radiohardware. Releasehash herberekend voor de ingebedde versiepatchnotes.
- `UPDATE_RFLINK` blijft veilig bij onzekere PCB-configuratie en gewijzigde firmwareblokken; deze controles zijn niet uitgeschakeld.

Details: `V053_RELEASE_NL.md`.

---

# v0.5.2 Radiolove — Windows-releasehash reproduceerbaar (3 november 2026)

- Herstelt foutieve hashafwijzingen door cultuurgevoelige PowerShell-sortering bij `MAAK_RELEASE.bat`.
- Runtimebronbestanden worden voortaan op een vaste ordinale manier gesorteerd voor SHA-256, net als in de onafhankelijke broncontrole.
- De strikte beveiliging tegen gewijzigde firmware zonder versieophoging blijft aanwezig. Ook bij een echte bronwijziging wordt de release nog steeds geweigerd.
- Korte releaseversie en patchnotes bijgewerkt. RAMSES-II, CC1101, Orcon TX, profielen, bestaande data en YAML-pinout ongewijzigd.

Details: `V052_RELEASE_NL.md`.

---

# v0.5.1 Radiolove — crashhistorie en korte bronhash (3 november 2026)

- Ingebouwde bootgeschiedenis (laatste acht starts) ook zichtbaar in Logboeken → Systeem, zonder onjuiste tijdstempels of dubbele huidige boot.
- Korte firmwareversie en 12-cijferige SHA-256-bronvingerafdruk in plaats van de lange interne revisieketen.
- Windows DEFAULT-release controleert of versie en runtime-bronhash bij elkaar passen; bij gewijzigde code moet de patchversie omhoog.
- De bestaande gecombineerde logviewer, profielcatalogus en Orcon TX-lab blijven behouden.
- Geen garantie dat een nieuwe firmwarepanic is opgelost; NVS bewaart resetredenen, niet de C++-stacktrace.

Details: `V051_RELEASE_NL.md`.

---

## v0.5 Radiolove – PAIRRADIO9 (experimenteel)

Radiolog uitgebreid met TX-aanvragen, routeafwijzingen, redenen en driverresultaten (o.a. Orcon 1FC9-koppeling). Gedeelde begrensde radio-signaalbuffer, geen duplicaat in ruis. Frequente 868 MHz-managementframes dragen het gecontroleerde RF-kanaal in het zendplan. De koppelwizard meldt API-fouten in de Weblog; Systeemlog blijft voor beslissingen. Zie `PAIRRADIO9_RELEASE_NL.md` voor tests, beperkingen en installatie.

## DIAG7 — crashdiagnose en gebruikersflow

Zie [`rflink_universal/DIAG7_RELEASE_NL.md`](source/rflink_universal/DIAG7_RELEASE_NL.md) voor wijzigingen, testresultaten, veilige installatie en openstaande punten.

## LOGROWFIX1 — logregels rekken niet meer uit (1 november 2026)

- Vaste logviewer behoudt compacte regels bij 1, 2 of weinig gebeurtenissen; scrollen bij veel regels.
- CSS Grid sizing aangepast (`align-content:start; grid-auto-rows:max-content`) en embedded UI opnieuw gegenereerd.
- Desktop-/mobiele Chromium-layout en bestaande LOGBOOK1-/PROFILESAFE1-/releasechecks geslaagd.
- Geen verandering aan radio, decoder, RF TX, opslag of de persoonlijke YAML. Zie `LOGROWFIX1_RELEASE_NL.md`.

## PROFILESAFE1 — crash bij gebruikersbibliotheek en lege profielen (1 november 2026)

- **Geverifieerde oorzaak:** de ESPHome-backtrace wees naar `RFLinkWebHandler::user_library_json_()` bij `rflink_universal.cpp:3498`: de complete ingebouwde profielcatalogus inclusief viertalige tekst werd met `std::ostringstream` opgebouwd; bij het vergroten van de string mislukte een geheugenallocatie en ESP-IDF beëindigde het proces met `Abort`.
- `GET /rflink/api/user-library` geeft nu **compacte profielsamenvattingen** en decoder-overzichten zonder RAW-captures. Geen volledige builtin vertalingen/sources/setup-stappen meer in deze grote respons. Dit reduceert de profielpayload ruwweg van 85 KB JSON-brondata naar circa 8 KB compact overzicht (indicatief, geen meting van HTTP-heapallocaties).
- **Nieuwe `/rflink/api/profile-catalog`**: compacte lijst met 26 firmwareprofielen voor de selectie; `/profile-catalog?id=<profile_id>` geeft detailinformatie voor één gekozen profiel. Beschikbaar ongeacht de volledige gebruikersbibliotheek.
- **Nieuwe `/rflink/api/user-decoder-detail?id=...`** geeft één decoder; optioneel `&sample=N` geeft maximaal één opgenomen sample. De UI haalt alleen voor bewerken/export de afzonderlijke samples op. De bestaande import/exportstructuur blijft behouden.
- Web-API fouten worden gecontroleerd voordat JSON als bibliotheekdata wordt aangenomen; een tijdelijke storing mag de geladen profielen niet overschrijven. Apparaatkaarten hebben naast `Apparaat instellen` een knop `Verwijderen`, met verplicht bevestigingsvenster. Geen wijziging aan opgeslagen NVS-data.
- `/rflink/api/status` bevat `internal_free_bytes`, `largest_internal_block_bytes`, en `psram_free_bytes` om geheugenfragmentatie op hardware te onderzoeken. Een seriële logregel toont vóór het bouwen van de gebruikersbibliotheek de vrije interne heap en grootste blok.
- De TX-engine, Orcon-codec, hardware-GPIO en SPI-YAML blijven ongewijzigd. Dit is een **gerichte softwarecorrectie**, niet fysiek bewezen crashvrij: controleer de volledige ESPHome/ESP-IDF-build en herhaal hardwaretests met de ESP32-S3.
- Tests: PROFILESAFE1 browser (bibliotheekuitval, 26 profielen, 16 Orcon, verwijderen annuleerbaar), decoder-sample-fetch, DEVICEUX4, PROFILECATALOG2, pairing/Orcon, LOGBOOK1, PACKFIX1, C++ syntax en pakket/board guards. Windows PowerShell-build en hardware-OTA niet hier uitgevoerd.

## UXREVIEW5 — v0.5 Radiolove (1 november 2026)

- Friendly UI release name, with detailed technical revision and patch notes under About.
- Logging settings grouped by Radio / System / Web; one fixed-height log viewer for all three, with preserved radio filters and older RX records.
- Library summary visible without a second Advanced click; TX lab explained separately from ordinary system log.
- Browser avoids overlapping device/status polls; C++ instruments timing and JSON sizes for device/library APIs.
- Consolidated current documentation into HANDBOEK_RFLINK_NL.md; moved 24 historical, nonrequired root documents into docs/history/.
- No RF encoder, pinout, SPI or physical TX changes. Panic root cause NOT identified; this is an experimental source release.
- Test and scope: docs/UXREVIEW5_GEBRUIKERSAUDIT_NL.md.

## DeviceUX4 — compacte apparaatlijst en gescheiden apparaatdetails (1 november 2026)

- Apparaatkaarten tonen status plus snelle, daadwerkelijk beschikbare bediening. Onbeschikbare knoppen staan niet meer op het overzicht.
- Profielkeuze, de koppelwizard en technische RF- en HA-beheeropties staan in een afzonderlijke detailweergave. Apparaten zonder profiel hebben één duidelijke instelroute; er is geen dubbele profielknop of geneste wizard toegevoegd.
- Vier talen bijgewerkt, HTML opnieuw ingebed in `web_ui.h`; RF-transmit/pairing-implementatie en gebruikersconfiguratie ongewijzigd.
- Acht inhoudelijk byte-identieke dubbele Markdown-rapporten verwijderd, met één origineel per paar behouden. Raadpleeg `docs/DOCUMENTATIE_OVERZICHT_NL.md`.
- Voor echte vrijgave: ESPHome Validate+Compile, Windows MAAK_RELEASE en hardwaretests. Eerdere panic-resets zijn met deze UI-aanpassing **niet** opgelost.

## PACKFIX1 — native releasegenerator voor LOGBOOK1 (28 oktober 2026)

- `MAAK_RELEASE.bat` liep vast op een verouderde `static.038`-controle in `maak_release.ps1`. LOGBOOK1 gebruikt `logbook.title`, terwijl RX/TX-monitor en de nieuwe Systeemlog-API aanwezig zijn.
- De releaseguard controleert nu expliciet de huidige logviewer (Alles/Radio/Systeem/Web), loglimieten, radiomonitor en systeemtrace-API. Een incomplete runtime blijft geblokkeerd.
- Hosttest `tests/test_packfix1_release.py` verifieert positieve en negatieve gevallen. Geen RF-code, decoder of YAML aangepast.
- De Windows PowerShell 5.1-releaseprocedure en de ESPHome-compile moeten op een Windows-/ESPHome-systeem worden bevestigd; uitgeven van de SOURCE ZIP is niet hetzelfde als firmwareflashen.

## ORCONTXLAB2 – opt-in RAMSES-II/Orcon TX en 1FC9 (28 oktober 2026)

- Native `ramses_ii` TX-encoder registreert acht MVS-15R-acties met GDO_SERIAL
  via de universele TX-engine; geen protocolspecifieke radio-driver.
- Nieuwe device-specifieke 1FC9 pairing offer/ACK/confirm/10E0-workflow, unieke
  vooraf opgeslagen virtuele zender-ID en expliciete Auto-test/gebruikersbevestiging.
- Niet-gepaarde apparaten, foutieve profielen, ontbrekende labflags en drukke
  radio worden geweigerd. Web/Systeem/Radio-log blijven gekoppeld.
- Hosttests en Chromium-wizardtests geslaagd; fysieke RF-koppeling nog ongetest.
  Raadpleeg `ORCONTXLAB2_RELEASE_NL.md` voor instellingen, test en beperkingen.

## LOGBOOK1 / SYSDEBUG1 — gecombineerde debugging (28 oktober 2026)

Zie `LOGBOOK1_SYSDEBUG1_RELEASE_NL.md`. Nieuwe logboekweergaven Alles/Radio/Systeem/Web, centrale limieten, browseractie-/API-tracing en SYSDEBUG1 hardware-labtrace. Geen nieuwe Orcon-encoderactivering.

## UX3 — gebruikergerichte apparaatkaart en 3-klikcontrole (28 oktober 2026)

- De apparaatkaart is geordend rond gebruikersdoel: status, echte bediening of volgende instelstap; beheer en RF-details zijn secundair.
- `Alleen ontvangst` en `Bediening mogelijk` worden los van Home Assistant-publicatie getoond. Onbewezen Orcon-TX blijft uitgeschakeld.
- `Apparaatprofiel kiezen` is zichtbaar wanneer `profile_id` leeg is; opslaan via bestaande API, automatische bevestiging en selector inklappen.
- De universele koppelwizard biedt RX-only afsluiting zonder een valse TX-/koppelclaim; uitgebreide RF-details staan niet langer als lange primaire instructies op de kaart.
- Vier talen bijgewerkt; mobiele filters compacter, opslag-succesbanner verborgen, storage-fault blijft zichtbaar.
- Dubbele devicepoll in de user-library UI weggehaald; de tweede module hergebruikt de primaire 3 s-apparaatgegevens. Selectfocus en paneelstatus behouden tijdens verversen; HA-dieplink direct open.
- Firmware-revisie eindigt op `GDOUARTLAB1-UX3`; de radio/TX-fysica is niet vrijgegeven of gewijzigd.
- Browser/host/regressietests uitgevoerd; zie `UX3_GEBRUIKERSAUDIT_EN_TESTRESULTATEN.md` voor resultaten, beperkingen, 3-klikmatrix en installatie-instructies.

# GDOUARTLAB1 (27 oktober 2026) — protocolonafhankelijke CC1101 GDO0 UART TX laboratoriumdriver

Zie `GDOUARTLAB1_RELEASE.md`. Zonder fysieke RF-validatie blijft Orcon-bediening uitgeschakeld.

# TXROUTER2 + DEVICEUI2 – 27 oktober 2026

## Gerealiseerd
- Runtime kiest voorkeurs-/fallbacktransport alleen uit goedgekeurde decoder-manifesten en werkelijke adapterbeschikbaarheid.
- TX-plan heeft een door de ingebouwde encoder vastgelegde protocolidentiteit; radio, profiel, frametype en permissie worden opnieuw gecontroleerd vóór zendactie.
- Generieke `SerialTxAdapter`-interface met fysieke lease-, herstel- en controleverantwoordelijkheid. De core ondersteunt nu RMT RAW, SPI-FIFO en GDO-serieel langs dezelfde transportdispatcher. Er is nog **geen geregistreerde fysieke GDO-adapter**.
- Bestaande Itho-koppelroute gebruikt hetzelfde vertrouwde TX-profiel en behoudt SPI-FIFO.
- Op de apparatenpagina blijven geopende detailpanelen open na automatisch verversen; compacter responsive raster voor apparaat, bediening en profiel.
- Geen nieuwe GPIO-, CC1101- of opslagmigratie. 6 MHz SPI-instelling van bestaande project-YAML blijft intact bij gebruik van de patch.

## Teststatus en beperkingen
- Geslaagd: C++ host-syntax, generieke 100-profiel-routerbenchmark, GDO-adaptermock, ESPHome-padresolutie, TX3-RC/Dooya/MQTT-regressie, browser 1300px/390px detailpanelen, PAIRING1-browser en web_embed-controle.
- Nog niet afgerond: volledige ESPHome/ESP-IDF-build, complete Somfy-suite, fysieke GDO0-transmissie, herstel van Itho-RX en werkende Orcon-koppeling/bediening.
- Orcon-TX blijft niet beschikbaar: het manifest heeft `tx: none`, `enabled: false`, en er bestaat geen fysiek gevalideerde zenderadapter. De knoppen blijven terecht niet klikbaar.
- Dit is een veilige ontwikkelrelease; niet presenteren als een werkende Orcon-zender.

## RADIOENGINEV2 (26 oktober 2026) — uniform decodercontract en centrale routing

- Alle 13 ingebouwde protocollen (9 decoderpacks) hebben hetzelfde `radio_v2`-contract voor RX-routes en TX-status, onafhankelijk van 433/868 MHz.
- Runtime-dispatch controleert na kanaal-/frametype-prefilter meerdere kandidaten en valideert decoderresultaten; conflicterende resultaten met gelijke score worden niet stilzwijgend als apparaten opgeslagen.
- Capture-profielplanner groepeert uitsluitend exact compatibele routes. De 868 MHz Itho/Orcon-combinatie wordt niet zonder fysieke verificatie samengevoegd.
- Scheduler ondersteunt >16 protocollen; hosttests met 100 synthetische decoders. Bestaande User Decoder NVS en TX-toestemming blijven identiek.
- Geen GPIO- of CC1101-configuratie gewijzigd; RAMSES6 fysieke TX/pairing blijft uit. Zie `rflink_universal/RADIO_ENGINE_V2_NL.md` voor teststatus en open hardwarevalidatie.

## RAMSES6 — decoder radio contracts and UART RX lab (26 October 2026)

- Added a schema-2 decoder-declared `radio_transport` configuration, a generated immutable C++ catalog and a generic `RadioTransportPolicy`. RAMSES-II chooses GDO2/UART stream RX first, SPI-FIFO sync fallback; Itho stays in existing SPI packet mode.
- Added a bounded RAMSES UART byte deframer, ESP32-S3 UART1 receive adapter using GDO2 while shared SPI remains active for CC1101 configuration, and fail-closed radio acquisition/restoration. Hardware RX not yet validated.
- Added optional safe-default-off manifest-controlled RX time slices (250 ms / 5000 ms in the RAMSES lab manifest) and a JSON field in decoder catalog exposing primary/fallback mode in Advanced.
- Physical Orcon TX, pairing and Home Assistant control remain disabled. No Itho GPIO changes, radio-flash override or NVS migration. Full ESPHome build and physical Itho/Orcon coexistence verification pending.
- Tests: `tests/test_radio_transport_policy.cpp`, `tests/test_radio_transport_catalog.py`, `tests/test_ramses_uart_stream.cpp` plus existing decoder, board and radio-mode tests. See `rflink_universal/RAMSES6_TRANSPORT_NL.md`.

## RAMSES5 SPI LAB — alternate SPI-only diagnostics (26 October 2026)

- Added opt-in unframed SPI/FIFO capture beside experimental B32A-sync RX. `ramses_spi_unsynced` defaults false; the lab is disabled by default. No Orcon radio TX, pairing, new live HA controls or automatic multiprotocol switching.
- Explicit 26 MHz modem setup (38.4 kbps 2-FSK, approx. 50.8 kHz deviation), RX FIFO last-byte/race guard, return-to-Itho register verification and end-of-lab log diagnostics.
- Host-tested both synthetic RX routes, 8 RAMSES-II command/bind codec cases, deliberate bad register readback, simulated TX underflow, arbiter restore/failure and decoder priority; not a physical RF or full ESPHome build claim.
- Documentation: `rflink_universal/RAMSES5_SPI_LAB.md` and `rflink_universal/RAMSES5_SPI_LAB_NL.md`.

## RAMSES4 — SPI-only FIFO laboratory implementation (26 October 2026)

- Added a bench-only RAMSES-II FIFO transport using **SPI exclusively**: programmable sync, streaming RX, variable-length TX refill, deterministic header reconstruction and checksum-based detection.
- Added `CC1101SpiBackend` to the existing `RadioModeManager` with exclusive IRQ pause, original register snapshot, RX lease, ESPHome `reset()`/Itho restoration and register readback verification. Itho stays the default mode.
- Added a firmware opt-in `ramses_spi_lab: false` and explicit short `request_ramses_spi_lab_rx(milliseconds)` API. No automatic sniffing, no user-facing live TX, pairing or new MQTT controls.
- Integrated successful RAMSES lab frames via the normal RFLink registry/log/device pipeline, not a parallel decoder.
- Added host FIFO RX/TX simulation (including corrupted frames, overflows, timeouts, FIFO refill and I/O failure), plus the existing RAMSES RX/TX and mode-manager regressions.
- **Engineering limitation:** packet-engine preamble/sync behavior and live TX on-air framing with an Orcon 15RF have NOT been validated. Full ESPHome/ESP-IDF build, hardware timing, physical pairing and TX remain pending. See `rflink_universal/RAMSES4_SPI_LAB_NL.md`.

## RAMSES3 — safe 868-MHz mode arbitration groundwork (26 October 2026)

- Introduced `radio_mode_manager.h`, a fail-closed state machine for exclusive use of radio 2 by a future RAMSES-II async UART driver. Bounded 20–30,000 ms lease; restores original Itho mode before declaring the radio available again. Failed restore is a sticky radio fault requiring explicit recovery.
- Integrated conservative guards in the RFLink core: packet receive callbacks are ignored while reserved, Itho/user packet TX is rejected before any persistent state update, periodic CC1101 watchdog SPI reads are deferred during a lease, and radio health reports unavailable during transition/fault.
- Added `test_radio_mode_manager.cpp` with acquisition, timeouts, failure injection, recovery, concurrency exclusion and tick rollover tests. `RAMSES3_RADIO_SAFETY_NL.md` explains the adapter contract and rollout sequence.
- **NOT a finished RAMSES RX/TX hardware driver.** No backend is armed, and the current Itho 63-byte packet radio remains unchanged by default. No new transmit action or pairing control is exposed. GPIO/YAML, existing Itho protocol, and native packet TX remain unchanged.
- Host checks: arbiter PASS, RAMSES RX and TX codecs PASS, native RFLink core monitor PASS, source syntax PASS, decoder catalog/baselines PASS. Full ESPHome firmware validation/compilation and physical radio tests remain pending.

## RAMSES1 — Orcon 15RF / RAMSES-II RX-only (26 oktober 2026)

- Added normal schema-2 native decoderpack `ramses_ii` (manifest, decoder, reusable codec, tests), separate manually selectable MVS-15R/15RF device profile.
- Two real 868.30-MHz RAMSES frames from the 26 October user log decode as class:serial 29:204810, ventilation mode Away and Auto.
- No changes to CC1101 configuration, GPIO mapping, existing Itho RX/TX, NVS schema, MQTT approvals or radio safety. Orcon TX and pairing **not enabled**.
- Generic shared-protocol device profiles can be `manual_only`, avoiding unproven automatic vendor matching.
- Host tests pass for codec, shared decoder registration, manifest/catalog generation, pack copying/linking, core C++ syntax and YAML. Full firmware build / RF hardware validation pending.
- More information: `rflink_universal/RAMSESII_ORCON1.md` and `rflink_universal/RAMSESII_ORCON1_NL.md`.

## DEVICECARDS1 – 2026-10-25 (Apparaten frequentiekaarten)

- Frequentiefilters op Apparaten in dezelfde visuele kaartstijl als Activiteit; één klik om te filteren.
- Dynamische apparaattellingen en optionele kaart voor apparaten met onbekende radio.
- Zoeken, automatische verversing en Wissen blijven werken; EN/NL/FR/DE bijgewerkt.
- UI-only: geen wijziging in YAML, CC1101, decoder- of API-code. Zie `DEVICECARDS1.md`.

## NAV4 – 2026-10-25 (device discovery, browser history, runtime MQTT option)

- Device search and per-radio filters with refresh-state retention; unknown radio labels avoid guessing.
- Browser Back/Forward and direct settings deep links, without intermediary pages.
- Runtime Home Assistant MQTT broker settings via POST + NVS, when optional MQTT component is compiled in.
- Existing YAML left otherwise unchanged; one-time optional `ha_mqtt_runtime` activation documented separately.
- Full source of use and tests in `NAV4_FINDABILITY_RUNTIME_MQTT.md`.

## NAV3 – 2026-10-25 (navigation consistency and three-click audit)

- Named Settings tools now open their contents immediately (Diagnostics and Custom profiles no longer require another summary click).
- Direct Create decoder links in decoder library and custom protocols, routing to the existing guided advanced editor.
- Settings → Manage HA devices directly reveals HA controls per device, while ordinary Devices view remains compact.
- Clear Back to settings overview buttons in settings sub-tools. Labels added for EN/NL/FR/DE.
- Browser tests and 3-click route audit updated. Radio, RX logs, NVS, YAML and reboot code unchanged.
- See NAVIGATIE_3KLIK_AUDIT.md. Full ESP32 firmware/OTA not tested here.

## ACTIVITYFIX1 – 2026-10-25

- Keeps expanded Activity detail cards open during polling and new reception, including when an open record ages off the latest 20 records.
- Reuses keyed DOM nodes and anchors the reader viewport; filters and boot changes still reset the view.
- No changes to RXLOG2 memory/HTTP paths, radio configuration, reboot fix or YAML.

## RXLOG2 – 2026-10-25 (CRASHFIX)

* Fixed Abort in `/rflink/api/rx-log` by using a chunked ESP-IDF response and temporary PSRAM record buffer.
* Fixed malformed JSON in recorded RX events (missing quote after summary).
* Fixed monitor noise filter displaying no events; improved pagination, visible API errors, safe export.
* No changes to GPIO/YAML, reboot fix, decoder configuration or TX logic.
* Source and host tests passed; real device build/OTA/uptime remain to be checked.

## RXLOG1 (2026-10-25) – memory-bounded multi-radio reception logs

- Added 250 signal events per radio (433.42 / 433.92 / 868.30) and one shared 2,000-event noise queue.
- The radio and category can be filtered independently in the Monitor. Events contain full received frame exports, RSSI/LQI/frequency offset when available, and decoder classifications.
- Dynamic capacities persist in NVS; write controls require the existing web write token.
- Allocates a bounded PSRAM arena (tries 1.5 MB, then 1 MB or 512 KB). Internal decoder/OTA memory is protected; noise is evicted before signals under pressure.
- Browser ZIP includes six per-radio/category CSV files, a merged chronological CSV, and diagnostics.json.
- Older historical hotfix BAT/README items were removed from the main source tree and archived separately. Current release, upload and update BAT files remain available.
- Host-stub compilation, storage simulation and browser regressions passed. Not yet verified by full ESPHome/ESP32-S3 build, OTA or hardware uptime test.

## LINKFIX1 — ESPHome 2026.10 protocol linker repair (2026-10-24)

- ESPHome 2026.10 omits unused `remote_base` protocol source files unless `to_code()` explicitly requests them.
- Enabled `esphome_builtin` decoder pack now calls `remote_base.request_protocol()` for `nexa`, `byronsx`, `dooya`, `keeloq`, and `rc_switch`. Decoder implementations and TX encoders are preserved.
- Guarded for ESPHome 2026.9, which compiles all protocols and lacks `request_protocol()`.
- No changes to CC1101 configuration, 433/868 RX, SPI, GPIO, filters, OTA, or BOOTDIAG1.
- Regression tests run; full ESPHome/ESP-IDF link still requires checking on the actual installation.

## COMPILEFIX2 — ESPHome 2026.10 listener compatibiliteit (2026-10-24)

- `StringRef.data()`-buildfout door een oud, los `rflink_web.cpp`: gebruik de al samengevoegde webserverimplementatie met `ref.c_str()` en verwijder losse legacybron via `APPLY_COMPILEFIX2.bat`.
- RAW-ontvangstlisteners voor 433,42 / 433,92 MHz vanaf ESPHome 2026.10 in `to_code()` geregistreerd via `remote_base.add_listener()`; ondersteuning voor 2026.9 behouden.
- Firmwarediagnostiek en VERSIONUI1 blijven behouden, revisienaam uitgebreid met COMPILEFIX2.
- Host-C++ en codegeneratieregressietests geslaagd; volledige ESPHome-build niet lokaal getest.

## VERSIONUI1 — runtime firmware identity (2026-10-24)

- Settings now shows the running firmware release lineage, installed patch revision and the compiled build timestamp.
- Status API exports `firmware_version`, `firmware_revision`, `firmware_built` and diagnostics text export includes them too.
- No changes to radios, GPIOs, RF handling, SPI, reboot logic or persistent device storage.
- The revision is `BOOTDIAG1-VERSIONUI1`. See `VERSIONUI1_README.md`.

## BOOTDIAG1 — uptime and reboot diagnostics (2026-10-24)

- Added 64-bit uptime and per-boot receiver-counter labels in RFLink web status.
- Added NVS-persisted boot counter, last eight reset reasons, free/minimum RAM and exportable diagnostics under Settings > Diagnostics.
- Browser detects a new boot session separately from an RX counter reset while the page is open.
- The CC1101 radio configuration, ESP32 pin mapping, SPI speed and RF receive filters remain unchanged.
- No wall-clock timestamps are stored at boot; previous restarts cannot be reconstructed before installing this version.
- See `BOOTDIAG1_README.md`. Full ESPHome/ESP-IDF hardware compilation remains to be verified.

## COMPILEFIX1 (2026-10-12)
- Fix CC1101* vs CC1101& main.cpp compile errors, no PCB GPIO changes.
- Clean up uint32_t log formatting.
- Source/host-stub verified; full ESPHome build still pending.

## RELEASEFIX REV2 (12 oktober 2026)

- Windows `MAAK_RELEASE.bat` werkte niet met de nieuwe `eigen_rfid_board_rev2_gpio14`: de PowerShell-packer accepteerde alleen rev1/custom. Rev2 toegevoegd met strikte GPIO-mapcontrole.
- Update-assistent controleert nu rev2 als huidige release-revisie; historische rev1-baselines blijven ongewijzigd.
- Alleen release/update scripts, tests en documentatie gewijzigd; radio-runtime en YAML GPIO-pinnen zijn identiek aan BOARDSAFE1.
- Python-bronregressies geslaagd; Windows PowerShell-release en ESPHome-build moeten nog lokaal worden uitgevoerd.

## BOARDSAFE1 — 12 October 2026

GPIO14 PCB mapping and defensive 200 ms rail settling, CC1101 dual-pin 433 MHz GDO0 RX/TX reconfiguration. See BOARDSAFE1_NOTES.md.

## SPIBOOT2 — deterministic shared-SPI startup diagnostic

- Runs before ESPHome SPI setup (priority 1199, after optional power components).
- CSn lines: internal pull-up + driven HIGH.
- SCK/MOSI: internal pull-down + driven LOW before any CC1101 is selected.
- MISO/SO: input with internal pull-up so an un-driven bus is deterministic.
- 250 ms pre-SPI settle delay, 5 CHIP_RDYn attempts per radio, 25 ms quiet gap before SPI setup.
- Logs MISO stuck-low with all CSn HIGH and MISO that fails to release after deselect.
- CC1101 SPI rate is temporarily/conservatively set to 200 kHz before bus setup to remove clock-rate margin as a test variable.
- Existing 10 s watchdog remains active; after 3 consecutive invalid reads it keeps attempting recovery, rate-limited to once per 30 s per radio.

## RXDIAG1 + PACKPATH1 — 2026-10-04

- RX diagnostics now apply to all three radios. Packet RX records LQI and frequency offset; raw radios retain full bounded pulse detail for on-demand export.
- Suspected RF noise is counted separately and hidden from the normal Activity view by default. The current Itho packet profile classifies unmatched fixed 63-byte FIFO frames as noise-suspect instead of new devices; 433 MHz raw filtering is intentionally conservative.
- Activity adds Copy and Export actions. Export includes the complete 63-byte packet payload and bounded raw pulse data from the 32-event history.
- Release packaging now uses a shorter temporary path, validates source directories before staging, and reports the exact failing file/path and PowerShell line instead of only `Kan een deel van het pad niet vinden`.

## BOOTGUARD1 PINCLAIMFIX1 — 2026-10-04

- Fixed ESPHome YAML validation errors `Pin 13/5/9 is used in multiple places`.
- Root cause: BOOTGUARD1 registered the existing SPI MISO and CC1101 CS lines a second time through ESPHome pin schemas.
- Boot guard now validates those entries as raw ESP32-S3 GPIO numbers and accesses them directly through ESP-IDF only during its early startup probe.
- The normal SPI/CC1101 components remain the sole ESPHome owners of those pins.
- Watchdog, 3x CHIP_RDYn boot retries, 10 s health polling and automatic recovery are unchanged.

## RADIO-WATCHDOG1 - runtime CC1101 recovery + digital SPI health (2026-10-04)

- Adds a read-only PARTNUM/VERSION liveness probe every 10 seconds for all three CC1101 radios.
- One or two isolated invalid reads are tolerated; three consecutive invalid reads mark that radio unhealthy and request automatic recovery.
- Automatic recovery is rate-limited to one attempt per radio per 30 seconds to prevent reset storms on disconnected hardware.
- If ESPHome already marked a CC1101 driver failed but the chip responds over SPI again, the component is returned to construction state so normal ESPHome setup can run again.
- If a previously healthy radio loses SPI liveness, CC1101 reset/configure is invoked without rebooting the ESP32.
- Diagnostics now expose SPI checks, failures, consecutive failures, recovery attempts/successes and a digital SPI health percentage.
- The software monitor detects invalid digital transactions only; electrical ringing/reflections/overshoot still require an oscilloscope or logic analyzer.
- BOOTGUARD1 remains unchanged: 100 ms power-up delay, 10 ms CHIP_RDYn timeout and 3 readiness attempts per radio at boot.

## RADIO-BOOTGUARD1 - shared-SPI startup hardening

- All configured CC1101 CS lines are driven HIGH before the SPI bus/CC1101 components start.
- Adds a 100 ms power-up settling delay.
- Probes CC1101 CHIP_RDYn/SO per radio (10 ms timeout, up to 3 attempts) before normal ESPHome CC1101 setup.
- A missing or marginal radio remains non-fatal; network services continue.
- Existing RADIO-DIAG2 PARTNUM/VERSION diagnostics remain unchanged.

## RADIO-DIAG2-COMPILEFIX (2026-10-03)
- Fixes ESPHome C++ generation for CC1101 diagnostic binding: id(cc1101_x) is already a CC1101Component pointer, so the erroneous address-of operator was removed.
- No diagnostic behavior changed.

## RADIO-DIAG2 (2026-10-03)
- Adds a read-only CC1101 SPI self-test for all three radios.
- Reads CC1101 PARTNUM (0x30) and VERSION (0x31) status registers without resetting or reconfiguring the radio.
- Web Diagnostics shows raw PARTNUM/VERSION and whether the register probe returned a plausible response.
- Serial log prints CS pin, PARTNUM, VERSION and SPI probe result.
- Self-test runs once after startup; normal radio availability remains based on the ESPHome CC1101 component.

## RADIO-DIAG1 (2026-10-03)
- Radio status now separates CC1101/SPI detection, RX path and TX path.
- Web portal Diagnostics shows the configured CSN/GDO pins for every radio.
- A failed CC1101 is reported as an initialization/SPI-level fault instead of only "radio missing".
- Serial logs identify the affected radio and its configured CS pin.
- Diagnostic metadata is read-only and does not change GPIO behavior.

## USERLIB2-PROFILES1-UI4-SOMFY1-ITHO1 — native Itho RFT device control (27 september 2026)

- `IthoRFT` heeft nu native 868,30MHz packet-TX via dezelfde centrale device/TX-architectuur als de overige ingebouwde encoders. De bestaande `itho_codec` wordt hergebruikt; snelheid/timerpayloads worden als 63-byte CC1101-pakketten verstuurd.
- Ieder ingeleerd Itho-apparaat krijgt een eigen, niet uit ontvangen RF gekopieerde 24-bit virtuele RFLink-zenderidentiteit, persistente logische verzendteller en device type. De teller wordt vóór uitzending vooruit opgeslagen en restore mag hem nooit terugzetten.
- De normale flow is **inleren → RFLink koppelen → Laag/Midden/Hoog of Timer 10/20/30**. Stand-by, opnieuw koppelen, compatibility-pairing, virtuele zenderstate en ontkoppelen staan onder Advanced.
- Standard pairing gebruikt JOIN; compatibility-pairing blijft beschikbaar voor varianten die de alternatieve join nodig hebben. Unpair gebruikt LEAVE. De firmware houdt pairingstatus per apparaat bij, maar een RF-zendopdracht blijft geen bevestiging van de ventilatie-unit.
- Home Assistant gebruikt dezelfde centrale MQTT/device-flow: pairing eerst en daarna expliciete HA/TX-opt-in. De oude losse ESPHome-templateknoppen en globale `itho_id1/2/3`, `itho_device_type`, teller en scripts zijn uit de hoofd-YAML verwijderd. Bestaande installaties die de oude virtuele Itho-zender gebruikten moeten RFLink na de update opnieuw koppelen.
- Hosttests zijn groen voor native Itho encoder/packet-decode, standard pairing, compatibility-route, unpair, speed/timers, NVS-persistence, monotone restore, radio guard, HA discovery/dispatch en mobiele Chromium-UI. Fysieke 868,30MHz CC1101 + echte Itho-unitvalidatie blijft pending.
- Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-PROFILES1-UI4-SOMFY1-ITHO1`.

## USERLIB2-PROFILES1-UI4-SOMFY1 — native Somfy RTS virtual remote (27 september 2026)

- `SomfyRTS56` heeft nu een native TX-encoder op 433,42 MHz voor Up, Down, My/Stop en de expliciete PROG/pairing-flow.
- RFLink maakt bij de eerste pairing een eigen 24-bit virtueel remote-adres; het ontvangen adres van de bestaande handset wordt nooit als TX-identiteit hergebruikt.
- Rolling-code-state staat in het persistente device-record (NVS schema v4). De volgende tellerwaarde wordt transactioneel opgeslagen vóór de radio-opdracht en restore mag de teller alleen gelijk houden of vooruitzetten. Een backup met een ander virtueel remote-adres wordt geweigerd.
- De webinterface toont vóór pairing één duidelijke **RFLink koppelen**-actie en daarna alleen **Omhoog / My-Stop / Omlaag**. Adres en counter blijven onder Advanced. PROG wordt niet als normale Home Assistant-knop gepubliceerd.
- De Somfy RX-code is gehard voor samengevoegde software-sync/eerste halfbit en laatste LOW-halfbit/inter-frame-gap; de TX-regressie decodeert de zelf gegenereerde waveform opnieuw en controleert adres, commando en rolling code.
- De historische `ha_tx_controls`-hoofdschakelaar staat in deze developmentrelease standaard aan. Er wordt nog steeds nooit automatisch gezonden: Somfy vereist expliciete pairing en HA-bediening vereist nog steeds MQTT + per-device opt-in.
- Hosttests voor native Somfy TX, NVS persistence/restore, monotone counters, radio guard, Home Assistant discovery/dispatch en mobiele Chromium-UI zijn groen. Fysieke CC1101/Somfy-motorvalidatie blijft pending. Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-PROFILES1-UI4-SOMFY1`.

## USERLIB2-PROFILES1-UI4 — eenvoudig toevoegen zonder RF-kennis (26 september 2026)

- **Apparaat toevoegen** is teruggebracht tot één normale route: frequentie kiezen, Start, apparaat gebruiken, naam/functies kiezen en Opslaan. Na Start verdwijnen frequentiekeuze en algemene uitleg zodat alleen de huidige stap overblijft.
- 433.92 MHz wordt waar beschikbaar als veilige standaard geselecteerd. De drie boardbanden krijgen korte herkenningshulp (meest gebruikt / vaak Somfy-RTS / vaak Itho) zonder protocolinformatie in de normale wizard.
- Protocolnaam, observaties, decoderinformatie en RF-details staan alleen onder **Geavanceerde details**. De protocolbouwer staat apart onder Advanced en is niet onderdeel van de normale inleerflow.
- **Apparaten** toont geen groene opslagmelding wanneer alles goed is en verbergt de pairingfilter als geen opgeslagen apparaat pairing ondersteunt. Alleen problemen vragen aandacht.
- **Activiteit** en de bovenbalk spreken over signalen in plaats van frames/RF-frames. Technische RX/TX/protocolinformatie blijft onder Advanced.
- De bestaande LED-statuslocalisatie blijft gebaseerd op stabiele statuscodes en is gecontroleerd in Engels, Nederlands, Frans en Duits.
- Chromium-regressies bewaken eenvoudige defaults, verborgen technische secties, mobiele layout, device-learning, decoderbibliotheek en vier talen. Geen RF-decoder-, CC1101-, NVS- of TX-algoritme gewijzigd. Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-PROFILES1-UI4`.

## USERLIB2-PROFILES1-UI4 — eenvoudiger UI + complete LED/wizard-localisatie (26 september 2026)

- De webinterface is verder teruggebracht naar **simple by default**. Status heeft nog maar twee dagelijkse snelacties: apparaat toevoegen en apparaten beheren. Pairing staat bij Apparaten in plaats van op meerdere pagina's.
- Instellingen begint nu met drie compacte kaarten: **Taal**, **Home Assistant** en **RGB-statuslamp**. Lange MQTT- en LED-uitleg staat achter een uitklapper in plaats van permanent in beeld.
- **Bibliotheek & updates** toont standaard alleen of er actie nodig is. Decoder-/profielaantallen, schema en technische library-details staan onder Advanced; nul-aantallen voor eigen items of aandacht worden niet meer getoond.
- **Eigen protocollen en apparaatprofielen** en **Diagnose** zijn standaard ingeklapt. De Status-knop *Bekijk diagnose* opent Diagnose nu ook werkelijk. Technische en gevaarlijke apparaatsecties blijven tijdens de periodieke refresh open als de gebruiker ze zelf heeft geopend.
- De oude tweede decoder-taglijst onderaan de Decoderbibliotheek is verwijderd. Op mobiel wordt ook niet meer automatisch het eerste decoder-detail geopend: eerst protocol kiezen, daarna pas de uitleg.
- De LED-status gebruikt nu een stabiele `status_code` uit de API (`ready`, `offline`, `learning`, enz.) en vertaalt die in **Engels, Nederlands, Frans en Duits**. De oude Nederlandse `status`-tokens blijven als compatibiliteitsveld aanwezig en worden door de UI nog als fallback begrepen.
- Bij de taalaudit bleek dezelfde foutklasse ook in de eerste toestand van de leerwizard te zitten. Titel, uitleg, Start/Stop, achtergrondstatus en wachttekst zijn nu eveneens via de locale-catalogus gekoppeld.
- Nieuwe Chromium-regressie bewaakt de simpele standaardweergave, ingeklapte Advanced-secties, diagnose-snelkoppeling, mobiele decoder-on-demand en gelokaliseerde leerwizard. Bestaande localization-, workspace-, decoder-, device-, LED- en profieltests blijven groen.
- Geen wijziging aan RF-decoding, CC1101-configuratie, NVS-deviceformaat, TX-policy of protocolalgoritmen. Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-PROFILES1-UI4`.

## USERLIB2-PROFILES1 — ingebouwde, brononderbouwde apparaatprofielen (25 september 2026)

- RFLink levert nu **10 read-only ingebouwde device profiles** bovenop bestaande decoders: AcuRite 609TXC, Alecto V1 weather, Fine Offset WH2/ClimeMET CM9088, Itho Daalderop RFT 536-0124, Byron SX21, Nexa/Jula Anslut self-learning, Dooya AC123-16/Zemismart AM15, Rubicson/TFA 30.3197, Somfy Telis 1 RTS en Somfy Telis 4 RTS.
- Ieder profiel heeft een stabiel ID, protocol, fabrikant/modellen, apparaattype, ondersteunde acties waar eenduidig, viertalige leerhulp/notities en bronlinks. De JSON-brondata staat onder `rflink_universal/profiles/`; `tools/generate_profiles.py` valideert die tegen de ingeschakelde decoder-capabilities en genereert de firmwarecatalogus.
- De webinterface toont passende bekende profielen rechtstreeks bij een decoder en biedt ze aan bij opgeslagen apparaten. Ingebouwde profielen zijn niet verwijderbaar; eigen NVS-profielen blijven een aparte bron en mogen een ingebouwd ID lokaal overschrijven.
- Automatisch koppelen gebeurt alleen wanneer exact één profiel bij het protocol past. Bij meerdere mogelijke modellen, zoals Somfy Telis 1 versus Telis 4, kiest RFLink niet voor de gebruiker.
- Alle profielen staan bewust op `hardware_validation: pending_on_rflink_universal`: een gedocumenteerde model/protocolkoppeling is geen bewijs dat elke revisie al fysiek op deze ESP32/CC1101-boardrelease is getest. De UI toont die status en de gebruikte bronnen.
- Geen speculatieve productprofielen voor generieke KeeLoq of RCSwitch. Grenzen van de huidige decoder zijn expliciet bewaard (o.a. WH2 ≠ WH2A/WH5, Rubicson 36-bit ≠ langere PT-310, Itho 536-0124 ≠ RFT Auto/RFT-N).
- Geen wijziging aan het RF-decode-algoritme, TX-policy, NVS-schema of CC1101-configuratie. Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-PROFILES1`.

## USERLIB2-UI2 — brede desktopwerkruimte en decoder-editor (25 september 2026)

- De globale 1240px-desktoplimiet is verwijderd. De RFLink-webinterface benut nu de beschikbare contentbreedte; 1920px-schermen houden geen grote ongebruikte rechter- en linkermarges meer over.
- **Instellingen** is opnieuw gegroepeerd: taal + snelle acties, RGB + Home Assistant, daarna Library & updates en Decoderbibliotheek over de volledige breedte. Onderhoud en diagnose blijven naast elkaar.
- **Mijn decoders en apparaatprofielen** gebruikt op desktop twee interne panelen: bestaande eigen decoders links en de profiel-editor rechts. De profielvelden staan in een compact tweekolomsformulier en stapelen op kleinere schermen.
- **Leren → Decoder maken** gebruikt drie naast-elkaar staande basisvelden. In Advanced staan RX- en TX-regels naast elkaar voor vergelijken en debuggen; onder 1100px worden de editors weer verticaal.
- Het geselecteerde Decoderbibliotheek-detail blijft op brede schermen zichtbaar terwijl je door de protocollijst scrolt. De protocollijst krijgt iets meer vaste ruimte en het detailpaneel de rest.
- Focusmarkering voor toetsenbordbediening is toegevoegd en selects gebruiken dezelfde systeemtypografie als de overige velden. De ongedefinieerde CSS-kleurvariabele `--fg` is vervangen door `--text`.
- Nieuwe Chromium-regressie controleert 1920/1280/390 px: fluid desktop, tweepaneel user library, drie decoderbasisvelden, RX/TX naast elkaar, mobiele stacking en geen horizontale overflow.
- Geen wijziging aan RF-decoders, NVS, radio-instellingen, VM, TX-policy of protocolgedrag. Runtime/release-identiteit: `v0.3.3-RC4-HF4-USERLIB2-UI2`.

## USERLIB2-UI1 — decoderbibliotheek gebruikt volledige desktopbreedte (25 september 2026)

- **Instellingen → Decoderbibliotheek** beslaat op desktop nu beide kolommen van het instellingenraster. De onderhoudsregel en diagnose komen eronder weer naast elkaar.
- De protocollijst blijft links begrensd; het detailpaneel gebruikt de overige breedte. Daardoor breken uitlegblokken niet meer af tot smalle woordkolommen.
- Detailblokken schalen met `auto-fit` en een minimale leesbare breedte; onder 850 px blijft de bestaande éénkoloms mobiele/tabletweergave actief.
- Een Chromium-regressietest controleert op 1280 px dat de bibliotheek daadwerkelijk vrijwel dubbel zo breed is als een normale instellingenkaart en dat het detailpaneel ruimer is dan de selector.
- Geen wijzigingen aan decoderlogica, NVS-formaat, VM, TX-beleid of radio-contracten.
- Runtime/release-identiteit voor deze UI-patch: `v0.3.3-RC4-HF4-USERLIB2-UI1`.

## USERLIB2 — veilige edit/restore, langere TX en promotiecandidates (25 september 2026)

- Advanced wijzigingen aan een actieve user-decoder zijn nu transactioneel: de nieuwe RX/TX-regels worden eerst gecompileerd, samples worden opnieuw afgespeeld en overlap met actieve user/firmware-recepten wordt gecontroleerd. Alleen daarna wordt één NVS-snapshot geschreven; bij een fout blijft de vorige actieve decoder intact.
- JSON-backups kunnen vanuit de webinterface worden hersteld. User-decoders, samples, profielen en tellers worden opnieuw gevalideerd; tellers gaan alleen vooruit; TX-toestemming wordt nooit automatisch hersteld; device-profielbindingen alleen bij dezelfde UID én hetzelfde protocol.
- Device profiles mogen nu ook normale ingebouwde protocollen gebruiken. De UI toont builtin protocollen in de profielkeuze en vult vaste acties voor waar dat eenduidig kan.
- De bounded TX-VM heeft compacte `MARKUS`, `SPACEUS` en `BYTEVAL` instructies. Automatisch gegenereerde vaste RAW-TX kan daardoor veel langere telegrammen bevatten; een hostregressie valideert 120 pulsen.
- Capture-analyse markeert wisselende packetposities en geeft heuristische hints voor oplopende tellers en eenvoudige XOR/SUM-checksumkandidaten. Dit is diagnose, geen automatische protocolaannname.
- Radio-contracten zijn expliciet zichtbaar als `board_raw` en `board_packet_63`. USERLIB2 opent nog geen willekeurige CC1101-registerinstellingen; zie `RADIO_PROFILES.md`.
- `promote_user_library.py` accepteert user-profielen bovenop builtin protocollen en kan met `--candidate-out` uitgeschakelde RX-only decoder-packcandidates genereren. Native TX blijft een afzonderlijk reviewed/hardware-getest traject.
- Runtime/release-identiteit is bijgewerkt naar `v0.3.3-RC4-HF4-USERLIB2`.

## USERLIB1 — eigen RX/TX-decoders en apparaatprofielen (25 september 2026)

- De webinterface neemt gelabelde volledige RAW- of packetframes op, maakt voor stabiele vaste codes een RX-recept en waar mogelijk een TX-recept, speelt minimaal drie voorbeelden per actie terug en activeert een nieuwe decoder direct. Voor andere protocollen is een begrensde RX/TX-interpreter met regels en persistente tellers beschikbaar. Onbekende of versleutelde algoritmen vragen nog steeds kennis van het echte protocol.
- Eigen decoders, voorbeelden, profielen en protocoltellers hebben twee NVS-snapshots; opgeslagen apparaten hebben een achterwaarts leesbare v3-indeling met profielkeuze en aparte TX-toestemming. Een normale OTA laat de NVS-partitie intact; maak wel zelf een export voordat je partitietabel of flash wist.
- Zendopdrachten zijn per apparaat afzonderlijk goed te keuren en hebben voorbeeldweergave, radio- en actiecontroles, bindingsvergrendeling, een pauze tussen opdrachten en opslag van gewijzigde protocoltellers vóór uitzending. RAW op de twee 433 MHz-kanalen en vaste 63-byte-pakketten op het meegeleverde 868 MHz-profiel zijn aangesloten; de TX-regel zelf moet de juiste golfvorm en protocollogica beschrijven. Home Assistant-knoppen blijven afhankelijk van MQTT en `ha_tx_controls: true`.
- Export, evaluatie, hosttest en gecontroleerd samenvoegen tot firmware-recepten zijn beschikbaar via `promote_user_library.py` en de PowerShell-wrapper. Recepten zijn in de firmware ingebedde interpreterprogramma's, met conflictcontrole bij vervanging; ze worden niet automatisch omgezet naar handgeschreven C++-protocolimplementaties.
- Hosttests voor de volledige gebruiksstroom, MQTT-optie, versieherstel, decoderreplay, opslag, web-API-compile, scripts en pakketopbouw: zie `TEST_RESULTS.txt`. Zonder ESPHome-toolchain, fysieke ESP32-S3/CC1101-print en toegestane interactieve lokale browser is er geen claim van fysieke RF-werking of volledige apparaatbediening.

## LIBRARYUX1 — library/update UX foundation (24 September 2026)

- Added a formal reconciliation model for future built-in vs user decoders/device profiles. Exact duplicates are safe to resolve; newer official items, user forks and same-version/different-content conflicts require review. Decoder identities never run twice in parallel; device profiles may be kept as a renamed custom variant.
- Added Dashboard attention handoff that stays hidden when no library decision is required. No blocking update popup is used.
- Added `Settings > Library & updates`, contextual `?` help and Advanced details. Normal decoder details stay concise; stable IDs, source, version, pack and raw capability lists are shown only on demand.
- Added a versioned `/rflink/api/library` schema and stable decoder IDs. In this source snapshot the persistent user decoder/profile library is intentionally **not yet enabled**; the API reports that honestly instead of fabricating user content.
- Added `LIBRARY_AND_PROFILES.md` and `UX_PRINCIPLES.md` as architecture contracts for the next persistent-storage/import-export phase.
- Added host reconciliation tests plus Chromium coverage for Dashboard attention, Settings handoff, Advanced and contextual help.
- No RF decoder behavior, GPIO mapping, device NVS snapshot format, TX policy or radio code changed.

## LANGFIX1 — complete RFLink web-interface localization (23 September 2026)

- Fixed the web UI showing Dutch dynamic text while English was selected. The original localization pass covered mostly static `data-i18n` elements; generated status cards, radio states, device actions, dialogs and wizard messages still contained hard-coded Dutch.
- English remains the default. English, Dutch, French and German now use the same keyed translation layer for static and dynamic RFLink web-interface messages.
- Added localized decoder-onboarding overlays and Home Assistant field labels without translating protocol identifiers, learned RF identities, user-entered device names or raw diagnostic payloads.
- Language changes re-render dynamic panels immediately; no reboot is required. A browser-storage failure no longer overrides a valid language returned by the ESP32.
- Extended Chromium regression coverage to disconnected-radio states matching the bench setup, Learn, Monitor, Devices and placeholders in all four languages.
- Root redirect hotfix remains included: `http://<device-ip>/` uses ESPHome's native 302 redirect to `/rflink`.
- No GPIO, RF decoder, TX policy, NVS device-record format or MQTT topic logic changed.

## DECODERUX1 – decoderbibliotheek, functie-review en eenvoudiger beheer (22 september 2026)

## GitHub draft root redirect hotfix

- Fixed `http://<device-ip>/` returning HTTP 500 on the ESP-IDF web server.
- Root navigation now uses ESPHome's native `request->redirect("/rflink")`, which emits a valid HTTP 302 response.
- `/rflink` behavior and API routes are unchanged.


De inleerinstructies staan voortaan per decoder in `manifest.json` en worden automatisch in de C++-catalogus en webinterface opgenomen. De wizard laat alleen ontvangen publieke functies kiezen; technische radio-/zendvelden blijven behouden en ongekozen functies worden voorstellen. Weersensor-berichttypen zijn geen HA-knopgebeurtenissen. Apparaatkaarten tonen standaard status en functies; zeldzame en risicovolle acties zijn ingeklapt. `ExampleSwitch` staat uitsluitend in het ontwikkelfilter. De updater bevat gehashte UX1-stockdecoderbaselines; eerdere v2A/B/C-baselines blijven behouden. Zie `DECODER_UX1.md` voor alle twaalf protocollen en grenzen. Geen nieuwe TX-vrijgave, opslagmigratie of GPIO-aanpassing.

## ROOTFAV1 — Webadres en favicon (22 september 2026)

- `http://<IP>/` verwijst met HTTP 302 (`Location: /rflink`) door naar de RFLink-webinterface.
- Eigen RFLink-favicon als SVG, met echte ICO voor browserfallback (`/favicon.ico`).
- Het SVG/ICO-paar wordt ingebed in de firmware en opgenomen in SOURCE/bundle en updaterinventaris.
- Radio's, MQTT, GPIO en persoonlijke YAML-instellingen zijn ongewijzigd.
- Nog uit te voeren op de ESP32: volledige ESPHome-compile en browsertest op fysieke hardware.

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

**Decoderbibliotheek:** toont nu per protocol een verklaring in plaats van voor Itho en rolling-code-protocollen misleidend `Alleen ontvangst`. De `note`-metadata komt uit het protocolmanifest; de centrale webinterface bevat geen lijst met protocollenamen. Itho heeft eigen ESPHome-bediening, maar nog geen generieke 868MHz MQTT-pakketencoder. Somfy RTS en KeeLoq krijgen geen TX zonder afzonderlijk gevalideerd rolling-code-/sleutel-/tellerbeheer. Weersensoren blijven bewust ontvangstsensoren.

**Nog te bouwen:** (1) uniforme packet-TX-adapter met afzonderlijke **virtuele Itho-zenderidentiteit** en telleropslag, zonder een ontvangen ID over te nemen; (2) persistent functie-/kanaal-/relatiemodel v2 met gecontroleerde migratie; (3) MQTT-wijzigingsqueue bij brokeruitval; (4) webauthenticatie; (5) hardwaretest van elke encoder en volledige ESPHome/Windows-test. Ontvangen functies worden niet automatisch goedgekeurd of naar Home Assistant geüpload. Een RF-zendaanvraag bewijst geen uitvoering door het doelapparaat.

**Voor bestaande gebruikers:** gebruik de updater voor je persoonlijke YAML en houd de huidige GPIO/OTA-secretwaarden. Bestaande NVS-records en Home Assistant-IDs blijven intact. Schakel zendbediening niet in voordat je ESPHome Validate en Compile hebt uitgevoerd én de relevante RF-modules op echte hardware hebt getest. Dit is géén productieclaim.

---

# Historische TX2 / Decoder API v2B – generieke zendmotor (21 september 2026)

- Protocolspecifieke encoders zijn uit de RFLink-core verplaatst naar
  `rflink_universal/decoders/<pack>/tx_encoder.cpp`. Centrale `tx_engine`
  valideert manifest, goedgekeurde actie, fysieke radio en immutabele TX-binding.
- Nexa Aan/Uit is omgezet naar dezelfde encoderinterface; bestaande
  apparaat-UIDs, MQTT-commandotopics en opgeslagen NVS-records blijven behouden.
- Byron SX: nieuwe optionele 433,92-MHz fixed-code-zender voor één ingeleerde
  goedgekeurde `command_0`–`command_15`, met expliciete HA-bedieningstoestemming.
  Een willekeurige `command_16` wildcard, onbekend adres of radio wordt geweigerd.
- Bij opt-in wordt het fysieke RF-kanaal vastgezet. De manifestfrequentie wordt
  centraal gecontroleerd tegen het echte CC1101-kanaal. Ontbrekende radio = geen TX.
- De ESPHome-build genereert sterke linkerankers voor ingeschakelde TX-encoders;
  manifestclaims zonder encoder stoppen de configuratie-/releasecontrole.
- Bestaande Itho-standby/low/medium/high/pair/unpair blijven via native ESPHome
  API werken en worden niet als ongecontroleerde decoder-TX vrijgegeven.
- **Nog niet**: generieke Itho MQTT, Dooya/RC-Switch TX, Somfy/KeeLoq TX,
  multi-channel opslag v2, offline-HA-deletequeue, webauthenticatie.
- Beveiliging: `ha_tx_controls` blijft standaard uit, plus goedkeuring per apparaat.
  De webinterface is nog niet zelfstandig geauthenticeerd; niet publiek bereikbaar
  maken. Een geslaagde TX-call bewijst geen ontvangst door het doelapparaat.
- Alleen C++-hosttests, decoder-codegen en statische releasecontroles; volledige
  ESPHome/ESP-IDF compile, Windows PowerShell, Mosquitto/HA en fysieke RF-test
  zijn niet hier uitgevoerd. Testrelease, geen productieclaim.

---

# Decoder API v2A – capability-catalogus (21 september 2026)

- Acht aanwezige decoderpacks (twaalf protocollen) bevatten nu schema-2-manifests:
  protocolnaam, ontvangstacties, velddefinities en een expliciet veilig TX-profiel.
- ESPHome genereert bij iedere build een C++-catalogus rechtstreeks uit de
  *ingeschakelde* decoder-manifests; een gewone source-ZIP bevat een gecontroleerde
  fallback voor hosttests. Onbekende of dubbel aangemelde protocollen falen gesloten.
- De Home Assistant-sensormetadata komen uit die catalogus en niet uit een
  handmatig bijgehouden lijst in `ha_discovery.h`. Privévelden als rolling_code
  en code krijgen geen sensoren.
- Zendtoestemming vereist nu behalve de bestaande Nexa-veiligheidschecks ook
  dat de manifestcatalogus het protocol plus de actie expliciet toestaat.
- De webinterface heeft per decoder een leesbaar functieoverzicht.
- De nieuwe decoderwizard produceert standaard een schema-2 receive-only pack.
- De Windows-release controleert schema-2-manifests. De updater mag een
  schema-1-manifest van een oudere versie alleen als metadata migreren wanneer
  de bijbehorende `decoder.cpp` byte-voor-byte gelijk is; afwijkende code
  blijft een conflict.
- De bestaande NVS-opslag (schema 1), RF-decodersemantiek, GPIO, RMT en MQTT-
  commandotopics zijn in deze fase ongewijzigd. **Dit is nog geen universele
  TX-engine, geen multi-channel apparaatschema en geen offline HA-deletequeue.**
- Geen volledige ESPHome 2026.9-compile, Windows runtime-test of fysieke
  Nexa/Mosquitto-test uitgevoerd. Het is een architectuurtestrelease.

---

# TX1 – gecontroleerde MQTT-aansturing voor Nexa (21 september 2026)

- RFLink blijft in de lead: nooit automatische zendtoestemming op basis van een ontvangen frame. Centrale `ha_tx_controls: false` en per ingeleerd Nexa-apparaat aparte bevestiging.
- Alleen Nexa **Aan/Uit** op CC1101 radio 2 (433,92 MHz), mits decodeerbaar apparaat-id en correct kanaal/groep/niveau en precies één bevestigde RF-identiteit. Een oud record zonder herkende bronradio mag niet zenden totdat de bron opnieuw is ontvangen en goedgekeurd.
- MQTT Discovery publiceert twee momentane HA-knoppen uitsluitend voor goedgekeurde acties. `command_topic` is per knop; willekeurige, oude of vervalste payloads, verkeerd apparaat, ontbrekende radio en herhaalde opdrachten binnen 600 ms worden geweigerd.
- Bij goedkeuring worden relevante kanaal/groep/niveau-waarden als vergrendelde TX-binding opgeslagen. Later ontvangen radioframes mogen deze binding niet stilzwijgend veranderen. Wijzig identiteit of RF-binding pas na uitschakeling en hernieuwde goedkeuring.
- MQTT disconnect verwerpt de actuele sessie; reconnect genereert een nieuwe sessie en actualiseert retained discovery. Uitschakelen/verwijderen publiceert per knop retained tombstones en bewaart overige functies.
- NVS-opslag is achterwaarts compatibel: TX-permissie gebruikt een eerder onbenut vlagbit in v1. Bestaande records zijn standaard uitgeschakeld. ESPHome API en alle bestaande 433/868 ontvangst, GPIO, radio/RMT en Itho-code blijven behouden.
- Validatie: C++ hostpolicy en MQTT/TX-integratie, UI/API-gates, pakket-/releasecontroles. **Niet uitgevoerd:** volledige ESPHome 2026.9-compilatie, Windows scripts, fysieke RF-test en echte Mosquitto/HA-test. TX1 is een testrelease.

---

# HA2 – RFLink beheert functies en Home Assistant volgt (21 september 2026)

- Alleen een expliciete **Apparaat opslaan**-actie maakt van ontvangen RF-signalen een apparaat. Tijdens inleren staat "Toevoegen aan Home Assistant" aangevinkt als Mosquitto verbonden is en het apparaat een ondersteunde functie heeft. Je kunt het vinkje uitzetten. MQTT wordt nog steeds één keer via `rflink-universal.yaml` ingesteld en blijft standaard uit.
- Bij een bestaand apparaat zijn onbekende knopacties en sensorvelden uitsluitend **voorstellen** onder Apparaten → Functies beheren. Goedkeuren bewaart de functie in RFLink, waarna MQTT Discovery voor een gekoppeld apparaat wordt bijgewerkt. Zonder goedkeuring verandert het HA-apparaat niet.
- Bestaande goedgekeurde temperatuur- en andere meetwaarden en bekende knopgebeurtenissen gaan direct naar HA; een gewijzigde berichtinhoud voor dezelfde bekende knop houdt dezelfde event-identiteit. Tijdelijk geen RF-ontvangst verwijdert geen functie.
- Goedgekeurde functies kunnen afzonderlijk worden verwijderd na bevestiging. RFLink stuurt voor een verwijderde sensor een leeg retained MQTT Discovery-bericht, laat de andere entiteiten intact en werkt bij knoppen de lijst event_types bij. De event-entiteit verdwijnt pas bij het verwijderen van de laatste knopfunctie. Apparaat volledig verwijderen of HA uitzetten behoudt de bestaande HA1-veiligheidsregels.
- Verwijderen van een HA-gekoppelde functie is geblokkeerd als Mosquitto offline is. Toevoegen kan in RFLink offline, maar synchroniseert pas na herstel van MQTT. De publicatie naar MQTT is een enqueue-resultaat, geen bevestiging dat Home Assistant de wijziging heeft verwerkt.
- **Beperking:** voorstellen voor nieuwe functies zijn vluchtig: na herstart moet het signaal opnieuw ontvangen worden voordat je ze kunt goedkeuren. Goedgekeurde functies staan in de bestaande apparaatopslag. Een knopfunctie krijgt nog geen afzonderlijk instelbare weergavenaam; de protocolactie blijft zijn sleutel.
- GPIO-pinout, RMT-verdeling, native ESPHome API, secrets-structuur, historisch vastgezette update-baselines en overige firmwarefunctionaliteit zijn ongewijzigd. MQTT blijft optioneel; niet-geconfigureerde HA-integratie blokkeert het ontvangen en beheren van RF-apparaten niet.

**Validatiegrens:** C++-hosttests en browsersmokes controleren goedkeuringspoort, sensormetingen, MQTT Discovery-wijzigingen, herverbinden en afzonderlijk verwijderen. Volledige ESPHome 2026.9-compile, Windows BAT/PowerShell en test op de echte print met Mosquitto en Home Assistant zijn hier niet uitgevoerd. Flash niet voordat Validate en Compile slagen.

---

# CONFIG1 – centrale instellingen (21 september 2026)

- Alle normale gebruikersinstellingen staan voortaan in `substitutions` bovenaan de hoofd-YAML. De optionele MQTT-include staat er direct onder.
- MQTT-adres, poort en gebruikersnaam zijn zichtbare labels; het MQTT-wachtwoord blijft uitsluitend in `secrets.yaml`. De nieuwe release controleert of een ingeschakelde MQTT-package nog de invultekst voor broker bevat en stopt dan.
- WiFi, API/OTA-verwijzingen, logniveau en RF-dedupe/candidate-voorkeuren gebruiken nu eveneens labels. Identiteit, Itho-ID, GPIO, radio/RMT-budget, decodercode en HA1-apparaatbeheer zijn niet functioneel gewijzigd.
- HA1-gebruikers die MQTT eerder via `rflink_mqtt_broker` en `rflink_mqtt_user` in secrets configureerden, moeten de waarden één keer in de nieuwe centrale labels invullen; de updater gokt geen brokeradressen.
- Documentatie: `INSTELLINGEN.md`. Volledige ESPHome-compile en Windows-scriptuitvoering nog vereist.

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

# Wizard 4 – COMPILEFIX1 (20 september 2026)

- Opgelost: ESPHome 2026.9.0 C++ compileerfout in 4 OTA RGB callbacks. De onjuiste `write_state(&id(rflink_status_rgb))` gaf een pointer-naar-pointer door; nu wordt de reeds geplande LED-state via `id(rflink_status_rgb).loop()` verwerkt. ESPHome LightState::loop() schrijft de pending state door naar de driver.
- De oude hosttest simuleerde `id()` onjuist en eiste de foutieve expressie. Hij is aangepast en bevat nu een expliciete regressie tegen diezelfde fout.
- Opgelost: ESP-IDF formaatwaarschuwingen voor `uint32_t` via PRIu32 en PRIX32.
- Firmwarefunctionaliteit, radionamen, GPIO's, PCB-revisie, radio/RMT-budget, WiFi en OTA-wachtwoord zijn verder niet aangepast.
- OTA-passwordwaarschuwing is alleen informatief. Oude installaties eerst op ESPHome 2026.9+ brengen en pas daarna een afzonderlijke migratie naar `ota: encryption:` doen; niet beide in dezelfde migratie combineren.
- Historische HF3/HF4-baselines inclusief SHA256 blijven exact ongewijzigd. Er is een aparte gecontroleerde baseline voor COMPILEFIX1 toegevoegd zodat een latere update zijn eigen bron herkent.
- Teststatus: hosttests/archiefcontrole hieronder vastgelegd; ESPHome 2026.9.0 volledige compile, Windows BAT/PowerShell en fysieke RF/OTA zijn niet op dit systeem beschikbaar en moeten op het apparaat worden gecontroleerd.

# Wizard 4 – betere baseline-diagnose en firmwareconflicten (20 september 2026)

- Controleert het volledige bekende legacy-HF3-profiel vóór heuristische UX2-detectie.
- Rapporteert ingelezen YAML-pad, SHA256 en RMT-TX-aanwezigheid; zo is een verkeerd gekozen lokale YAML herkenbaar.
- Commentaarverschillen naast identieke `external_components` zijn geen firmwareconflict meer.
- Een exact herkende oudere zendersectie krijgt automatisch de **nieuwe** HF4-RMT/monitorimplementatie; willekeurige afwijkende code niet.
- Echte dubbele wijzigingen in `external_components` en `remote_transmitter` krijgen in Normal/Advanced expliciete vraag `[y/N]` (standaard nee). Automatisch of niet-interactief stopt; voor beheerde uitvoering is bewust `-AcceptReleaseFirmwareChanges` mogelijk, met vermelding in WIJZIGINGEN.txt.
- De gebruiker-YAML en persoonlijke data zijn niet mee verpakt. De standaardfirmware, pinout en RF-code zijn identiek aan Wizard 3.
- Windows PowerShell 5.1, ESPHome-compile en fysieke radiotests nog op doelplatform uitvoeren.

# Wizard 3 – historische baseline correct herkennen (20 september 2026)

Bugfix: de updater gebruikt niet langer blind de nieuwe project-YAML als baseline voor een HF3-installatie. De oorspronkelijke HF3 UX2- en HF4 UX2-YAML's zijn met een vaste SHA256 opgenomen in de SOURCE-map. Automatisch selecteren mag alleen bij een herkende UX2-versie; een onjuiste expliciete baseline wordt eveneens geweigerd. Een oude PCB zonder bekende revisie blijft een expliciete hardwarekeuze vereisen. Geen wijzigingen aan de RFLink-firmware, GPIO-defaults of radiodrivers. Bij een fout vóór de uitvoermap verwijst de BAT nu naar `LAST_UPDATE_FAILED.txt` in plaats van een nog niet bestaand WIJZIGINGEN.txt. Windows-integratietest toegevoegd voor oudere HF3-variant en verkeerde baseline.

# Update Wizard 1 — scriptrevisie op RC4-HF4 UX2 (20 september 2026)

Firmware/RF-functionaliteit niet gewijzigd ten opzichte van HF4 UX2. Nieuwe `board_revision: eigen_rfid_board_rev1` in de centrale YAML. Eén GPIO-profielkeuze (release of bestaande volledige pinout), onbekende hardware zonder default, expliciete revisiecontrole bij bouwen, behoud van identiteit/Itho-ID, decoderbronvergelijking (conflict bij nieuwe of gewijzigde decoder), updater bouwt zelf een CUSTOM-bundle. `MAAK_RELEASE.bat` bouwt uitsluitend DEFAULT zonder argumenten. Native Windows PowerShell/tar, geen Python-installatie voor release/update. Alle originelen blijven onaangetast, geen automatisch flashen.

De originele UX2-firmware, radiomonitor, RGB, decoderlinker, boot- en RMT-fixes zijn overgenomen. Hosttests worden apart gedocumenteerd in TEST_RESULTS.txt; Windows BAT/PowerShell, volledige ESPHome-compile, GPIO- en radiohardware vereisen verificatie op het apparaat.


Historische versie-notities, met inmiddels vervangen instructies, zijn bewaard in `rflink_universal/maintenance/legacy_hotfix/RELEASE_NOTES_HF4_ORIGINAL.md`.

### Wizard 3: oudere HF3-variant herkennen (2026-09-20)

- Een eerder uitgeleverde HF3-configuratie met alleen `rmt_symbols: 96` en zonder de latere RMT TX/OTA-correcties werd in Wizard 2 onterecht geweigerd.
- Er is een strikt gecontroleerd legacy-referentieprofiel toegevoegd, gebaseerd op de fout veroorzakende YAML, waarbij de persoonlijke apparaatnaam en Itho-ID zijn vervangen door placeholders. De originele gebruikers-YAML zit **niet** in deze release.
- Alleen als alle overige inhoud exact overeenkomt (na normalisatie van regeleinden) wordt dit profiel automatisch herkend; onbekende wijzigingen blijven geblokkeerd.
- De updater neemt de nieuwste RMT-, OTA-, radio-status- en GPIO-code uit HF4 over. Persoonlijke apparaatnaam/Itho-ID blijven behouden, en onbekende fysieke PCB-indeling vraagt nog steeds expliciete bevestiging.
- De referentie is uitsluitend voor migratie en is niet geschikt voor ESPHome-flashen.
- Windows PowerShell/Batch-uitvoering en een daadwerkelijke ESPHome-compile blijven tests die op een Windows/ESPHome-systeem uitgevoerd moeten worden.

## DECODERUI1 - single-decoder settings view

- Replaced the decoder settings accordion/drop-down presentation with a selectable decoder list and one detail panel.
- Only one decoder is expanded at a time. Category selection now uses buttons instead of a select menu.
- The selected decoder shows learning instructions, Home Assistant result, TX status, a short reason, what is still needed, warnings and technical fields without nested drop-downs.
- TX explanations distinguish receive-only sensors, host-tested fixed-code TX, Itho separate packet TX, Somfy RTS rolling-code requirements and KeeLoq cryptographic rolling-code limitations.
- Added EN/NL/FR/DE translations and Chromium regression coverage for desktop and mobile.

**WATCHDOG1-PACKFIX1 (4 oktober 2026).** Corrigeert de DEFAULT-releasevalidator voor WATCHDOG1. `maak_release.ps1` vereiste nog letterlijk de pre-watchdog `set_radio_available(...)`-vorm en wees daardoor de geldige YAML af. De validator vereist nu per radio exact één vier-argument call met de bijpassende `radio_watchdog_healthy(index)`-gate; verkeerde of ontbrekende watchdog-indexen worden fail-closed geweigerd. De Windows release-integratietest en Python guardtest zijn hierop bijgewerkt. Geen GPIO-, RF- of runtime-watchdogwijziging.

## RAMSES2 — Orcon 15RF zendcodec en koppelprotocol (26 oktober 2026)

- Nieuwe bronbestanden `ramses_ii/tx_codec.h/.cpp` ontwerpen `22F1`, `22F3`, `1FC9` en `10E0` correct als logische RAMSES-II-frames, Manchester-symbolen en UART-8N1 bitstream.
- RX/TX-roundtriptests en strikte 1FC9-ACK- en koppel-time-outtests toegevoegd. Geen hergebruik van een fysieke afstandsbediening-ID.
- Veiligheid: bestaande `board_packet_63` is NIET geschikt voor RAMSES-TX. Daarom **geen** TX-encoder geregistreerd, geen manifest-TX vrijgegeven en geen niet-werkende pairingknop getoond. GPIO, NVS, MQTT, bestaande decoders, bestaande fysieke RF-functies zijn ongewijzigd.
- Officiële koppelprocedure, ontwikkelgrenzen en vereiste RAMSES-radio-driver staan in `rflink_universal/RAMSESII_ORCON2_NL.md`.
- Softwaretests bewijzen geen radio-TX. Echte CC1101 UART TX/RX, veilige Itho/RAMSES radio-arbitrage en UI-wizard volgen in een afzonderlijke hardware-geteste fase.

## PROFILEFLOW1 (2026-10-26)
- One unified profile-first device wizard and saved-device UI for all radios and decoder types.
- Built-in profile action buttons are declared in `profiles/*.json`, verified independently against available native TX and hardware/paired state.
- Manual review for uncertain brand/model suggestions (including Orcon).
- Dismiss pending unsaved signal candidates, remove approved technical fields via Advanced (safe HA/TX guards).
- Four-language UI updates and official profile catalog regeneration. No GPIO/radio default change.


## ORCONFAMILY1
Zie rflink_universal/ORCONFAMILY1.md voor Orcon-brede profielondersteuning.
