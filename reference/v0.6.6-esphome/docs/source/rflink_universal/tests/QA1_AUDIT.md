# RFLink Universal — QA1 firmware-, onderhouds- en UX-audit
**Peildatum:** 25 oktober 2026  
**Basis:** `RFLink_Universal_RXLOG2_NAV4_FULL_SOURCE.zip` (268 bestanden)  
**Scope:** C++ firmwarebron, ESPHome-configuratie, decoderpakketten, logging, apparaatopslag, update-/releasescripts, HTML/JS, vier talen, belangrijkste gebruikersroutes.  
**Niet in scope/geen claim:** live ESP32-S3/CC1101-metingen, echte ESPHome 2026.10.0 cross-build, flash/OTA, echte MQTT-broker, penetratietest, 72-uurs hardware-soak.

## Samenvatting
De belangrijkste dagelijkse workflows en NAV4-browserroutes functioneren in host-/browsertests. De code heeft meerdere goede veiligheidsmechanismen: radio's zijn optioneel bij boot, TX vereist expliciete goedkeuring, decoderupgrades worden transactioneel toegepast, bibliotheken worden tegen corruptie gecontroleerd, en logs hebben een vast PSRAM-budget met ruis als eerste uitwijkmogelijkheid. Dit is een waardevolle basis maar nog **geen productierijp vrijgavebesluit**.

Er blijven twee prioritaire ontwikkelpunten open: (1) de HTTP-beheerinterface mist volwaardige toegangscontrole en toont zijn schrijftoken op leesbare endpoints; (2) het RX-pad bouwt per ontvangst nog tijdelijke JSON-/detailstrings op, wat onder langdurige 868-MHz-ruis geheugenfragmentatie kan versterken. De eerdere JSON-crash is functioneel hersteld, maar niet bewezen onder hardware-soak.

## Bevindingen en prioriteiten
| ID | Niveau | Bevinding (bewijs in bron) | Advies / status |
| --- | --- | --- | --- |
| SEC-01 | **Hoog, vóór publieke inzet** | `rflink_universal.cpp` `handle_device_`, `handle_candidate_`, `handle_developer_`, `handle_led_settings_` voeren POST-wijzigingen uit zonder login; `handle_mqtt_settings_` en `handle_rx_log_config_` vergelijken een token dat via niet-geauthenticeerde GET terugkomt. `README.md` vermeldt expliciet dat webauthenticatie ontbreekt. | Niet naar internet publiceren. Ontwerp echte sessie/authenticatie en consistente autorisatie voor alle mutaties met browser-/API-regressietests. Niet geautomatiseerd gewijzigd in QA1 om apparaten, leren en TX niet kapot te maken. |
| SEC-02 | Hoog bij onbetrouwbaar netwerk | Runtime MQTT-wachtwoord wordt in gewone NVS opgeslagen (`mqtt_runtime_save`); de NAV4-handleiding bevestigt dat NVS standaard niet versleuteld is. | Optionele flash/NVS-encryptie en veilig credentialbeheer ontwerpen; op dit moment alleen vertrouwd netwerk gebruiken. |
| MEM-01 | **Hoog, stabiliteit** | `record_activity_()` bouwt complete RAW-details, een escaped JSON-record én andere `std::string`-kopieën vóór een item de begrensde PSRAM-arena bereikt. Een event kan tot 32 KiB zijn; veel ruis vergroot allocatie-churn. | Refactor naar bounded PSRAM-schrijver/producer, zorg voor zichtbare `internal_free`, `largest_internal_block` en dropped counters, voer urenlange gecombineerde RF/HTTP/OTA-stress uit. Niet gewijzigd zonder hardwarebasis. |
| MEM-02 | Middel | Logarena reserveert bij start 1,5 MiB PSRAM (fallback 1 MiB/512 KiB) ongeacht aantal gebeurtenissen. Daardoor geen onbeperkte groei, maar vrije logplaatsen zijn **niet** puur dynamisch. | Correct vermelden in gebruikersdocumentatie en geheugenbudget; fallback testen. |
| UX-01 | Middel | Twee verschillende loops verversen Apparaten: `loadDevices()` om de 3 s en `refreshDevices()` om de 4 s vanuit dezelfde `index.html`. | Samenvoegen tot één datastroom met in-flight/abort-regie; controleer focus, uitklapstatus, invoervelden en klikrespons bij updates. Nog niet gewijzigd. |
| UX-02 | Laag — **hersteld** | Het apparaatnaamveld in de naamdialoog had geen expliciet gekoppeld label; het readonly decoder-tekstvak evenmin. | `label[for=deviceName]` en vertaalbaar ARIA-label voor het tekstvak toegevoegd in `ui/index.html`; `web_ui.h` deterministisch opnieuw gegenereerd. |
| QA-01 | Middel — **hersteld** | `run_tests.sh` combineerde `-c` met twee C++-bronnen en één `-o` object, een ongeldige GCC-invocatie. | Separate objectbestanden bouwen; beide compile-opdrachten op host geslaagd. |
| QA-02 | Middel — **hersteld** | `test_localization_browser.py` verwachtte de oude monitor-API; NAV4 gebruikt `/rflink/api/rx-log`. | Browsermock aangepast naar nieuwe RX-logconfig- en paginerings-API; alle vier talen slagen. |
| QA-03 | Middel — **hersteld** | `test_user_flow.cpp` simuleerde drie radiodrukken in hetzelfde millisecondevenster, die terecht als duplicaat worden behandeld door de 80 ms dedupe. | Tussenpozen van 120 ms gebruikt; volledige host user-flow slaagt zonder én met MQTT. De echte deduplicatiecode is ongemoeid gelaten. |
| REL-01 | Middel | `maak_release.ps1` hanteert de oude lijn `v0.3.3-RC4-HF4-...` als pakketnaam; nieuwe functionele revisies staan alleen in `firmware_identity.h`. | Houd één monotone, centraal gegenereerde release-ID naast de bronlijn en exporteer hem naar release- en update-manifests. Niet wijziging zonder migratieplan. |
| RF-01 | Informatief | Developer Session houdt maximaal 12 gelabelde captures vast en weigert RAW-samples boven 512 pulsen. De algemene RX-log kan meer data bevatten maar is een andere subsystem. | In Advanced duidelijk maken waarom een ruwe ontvangst wel in Activiteiten maar niet in Decoderontwikkelaar kan verschijnen. |
| HW-01 | **Hoog voor vrijgave** | De bron bevat hardwarebescherming en teststubs maar geen bewijs van een volledige succesvolle echte ESPHome/ESP-IDF-build of langlopende hardwaretest voor NAV4. | Bouw, upload en meet eerst op ESP32-S3 N16R8, met drie echte CC1101's; geen stable-tag vóór succesvolle soak. |

## Positieve controles
- **Radio/boot:** 3 CC1101-kanalen, bij ontbrekende radio blijven Wi-Fi/OTA/web bereikbaar. Watchdog-/resetregressies aanwezig. Hardwarepinnen/YAML in geautomatiseerde tests gecontroleerd.
- **RF-log:** 250 signalen per radio, één gedeelde ruislog (default 2000), byte-budget, signalen beschermd tegen ruisverdringing; 30 KiB testrecord en 3500 noise-inserts doorlopen.
- **Firmwaredecoders:** gegenereerde catalogus, manifests en built-in linkregistratie; tests herkennen 8 decoderpakketten/12 decoderklassen.
- **Eigen decoders:** beperkte VM, replay en overlapchecks; opslag wordt transactioneel gewijzigd en bij falen teruggedraaid.
- **TX:** Somfy/Itho/Nexa/Byron/Dooya en MQTT-opt-in hebben host-tests, radioselectie en rolling-code/sequence-regels. Geen echte RF-zendbevestiging getest.
- **Updateproces:** stock/custom scheiding, migratieconflictcontrole, GPIO-verificatie en oude scripts verwijderden niet automatisch de gebruikersconfig.
- **Privacy:** user-TX-programma's en MQTT-wachtwoord gaan via POST-bodies, niet in URL's; geen wachtwoord in GET-antwoord. Dit vervangt **geen** echte authenticatie.

## UX-route-audit (klik = navigatieklik, niet elke actie tijdens een wizard)
| Doel vanaf hoofdoverzicht | Verwachte route | Navigatieklikken | Resultaat |
| --- | --- | ---: | --- |
| Diagnose/statusproblemen | Status → Diagnose | 1 | Getest: inhoud zichtbaar, geen extra kop vereist |
| Ontvangstgeschiedenis per radio | Activiteiten → type → radio | 1–3 | Getest: signalen/ruis, radiofilter, export |
| Apparaat terugvinden | Apparaten → zoeken of radiofilter | 1–2 | Getest: zoekterm blijft bij refresh |
| Nieuw apparaat leren | Apparaat toevoegen → frequentie → Start | 1–3 naar de leerfunctie | Getest met mocks; echte detectie niet geverifieerd |
| Eigen decoder-editor | Instellingen → Decoderbibliotheek → Aanmaken | 3 | Getest: editor direct zichtbaar |
| Eigen profielen | Instellingen → Eigen protocollen | 2 | Getest: inhoud open |
| MQTT-configuratie | Instellingen → MQTT instellen | 2 | Browsertest; vereist MQTT-package in firmware |
| Terug naar vorige pagina | Browser Terug | 1 | Getest: directe deep-links en geschiedenis |

**Interpretatie:** de drie-klikregel helpt bij *het vinden* van een functie; koppelen, TX-toestemming en wissen kunnen bewust meerdere bevestigingen vereisen.

## Kwaliteitscriteria voor volgende stabiele release (voorstel, nog niet uitgevoerd)
1. Echte ESPHome 2026.10.0-build: alle drie radio's, web, native API en OTA starten zonder fouten.
2. Minimaal 72 uur gecombineerde test met echte 433/868-signalen, intensieve 868-ruis, logging en periodieke browserexports; geen onverklaarde panics.
3. Per 10 minuten trend opnemen van intern vrij RAM, grootste aaneengesloten interne heap, vrij PSRAM, loggebruik, RX/TX-counters en geweigerde gebeurtenissen; geen aanhoudende afname zonder verklaarde oorzaak.
4. De logviewer openhouden met een uitgeklapt RAW-frame terwijl nieuwe ontvangsten binnenkomen, filters worden gewijzigd en export plaatsvindt.
5. MQTT verbinden/verbreken, broker wisselen, HA Discovery behouden/opruimen en terugrollen van NVS-fouten testen met echte broker.
6. Veiligheidscontrole: anonieme mutaties geblokkeerd (na auth-ontwerp), geen webpoort via reverse proxy of WAN voordat dat af is.
7. Updateproef: huidige werkende YAML en alle gebruikersdecoders, deviceprofielen, Somfy/Itho-toestanden behouden; rollback beschikbaar.

## Vervolgvolgorde
1. **Stabiliteit bevriezen:** QA1 host-/browserregressies bewaren, NAV4 op één echte ESP32-S3 verifiëren.
2. **Beveiligingsrelease:** toegangscontrole en consistent POST-beleid, credentialbescherming; apart van decoderontwikkeling.
3. **Memory-/pollingrelease:** tijdelijke RX-strings verminderen, één verversingsmechanisme, lange SOAK; per fix gerichte regressietest.
4. **UX-nabewerking:** focus-/veldbehoud bij live refresh, duidelijkheid over capturelimieten, foutmeldingen en herhaalbare wizardtests.
5. **Opruimen onderhoud:** dubbele/historische scripts en MD's uitsluitend op basis van verwijzingen en pakketinventaris archiveren, nooit blind verwijderen.

## Waarom geen brede codewijziging in QA1?
RXLOG2 heeft al een echte panic gehad. Een ongeteste gelijktijdige refactor van radio-, HTTP- en opslagcode zou de betrouwbaarheidswinst ondermijnen. Daarom bevatten de QA1-codewijzigingen alleen expliciet geteste UI-toegankelijkheid en de testinfrastructuur. Risicovolle punten staan als open taken met duidelijk acceptatiecriterium.
