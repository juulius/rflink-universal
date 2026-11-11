# RFLink Universal v0.5 Radiolove — DIAG7

**Basis:** UNIFIED6 + PROFILESAFE1 + LOGROWFIX1. **Type:** experimentele ontwikkelrelease, geen hardware-gevalideerde stable-release.

## Wat is er gebouwd?

- **Boot/crash in Logboeken:** firmware schrijft bij het opstarten een korte Systeemlog-gebeurtenis met bootnummer en reden van de **vorige** reset. Crashes en watchdogs krijgen niveau Fout. De reeds aanwezige NVS-bootgeschiedenis wordt hergebruikt; er is geen extra flash-schrijfopdracht per gebeurtenis.
- **Status:** bij recente crash-reasons een prominente waarschuwing en 'Bekijk crashlog'. Als radios/decoders verder in orde zijn, meldt de statuskaart 'Aandacht nodig' in plaats van misleidend 'Alles gereed'. Geen ongefundeerde tijdsinschatting: NVS-boots hebben geen tijdstempel.
- **Logboeken:** één viewer voor Alles/Radio/Systeem/Web, tijdnotatie `uu:mm:ss,mmm` sinds ESP32-start, filter op gebeurtenistype (crashes/herstarts, TX/koppeling, API/browser), logboekenicoon in navigatie. Vaste vensterhoogte en één ruis-checkbox uit UNIFIED6 behouden.
- **Diagnosepakket:** download JSON met firmware-identiteit, recente boots, geheugenmetingen (inclusief grootste intern geheugenblok), radiogezondheid, recente systeem- en browsersessie-events. Geen raw RF-pakketten, API-bodies, wachtwoorden, TX-sleutels of tokens. **Controleer het bestand vóór delen:** vrije tekst in events kan toch apparaatnamen/IDs bevatten. Een ESP-IDF-backtrace zit niet in de RFLink-status-API en wordt niet verzonnen.
- **TX-lab:** normale systeemgebeurtenissen open je vanuit TX-lab in Logboeken. De oude ruwe systeemtrace is alleen onder een optionele technische uitklapper beschikbaar en blijft voor gespecialiseerde debugging behouden.
- **Bootsessies:** de systeemtrace-API geeft `boot_session_id` mee; bij een waargenomen reboot scheidt de browser de nieuwe tijdlijn van oude sessie-events en voorkomt hij dat oude timestamps als nieuwe uptime verschijnen.
- **Apparaten:** zoekactie over de reeds geladen lijst, geen extra `/api/devices`-verzoek bij iedere toetsaanslag. De apparaatdetailpagina heeft nu gelijkwaardige secties voor functies, Home Assistant, signalen, gevaarlijke acties en technische ID, zonder een extra genest beheerniveau. Goedgekeurde functies zijn te bekijken; verwijderen blijft mogelijk na bewust openen van de beheerlijst. Pending functies blijven direct ter beoordeling zichtbaar.
- Alle wijzigingen zijn ingebed in `components/rflink_universal/web_ui.h`, met EN/NL/FR/DE-vertalingen.

## Wat is niet af / expliciete beperkingen

- **Geen persistente trace van de laatste C++-instructies vóór een panic**. De oude Systeemlog blijft RAM-only; alleen de bestaande bootgeschiedenis staat in NVS. Voor exacte crashlocatie is de seriële ESPHome-backtrace én het exacte bijbehorende `.elf`-bestand noodzakelijk. RTC-breadcrumbs en crashintegratie via de ESPHome crashcomponent zijn nog niet geïmplementeerd wegens betrouwbaarheid en geheugenrisico's.
- Niet alle inhoudelijke UX-verbeteringen uit de audit zijn uitgevoerd. Bibliotheek/updates en decoderontwikkeling blijven grotere expertflows; een volgende ronde moet die op de echte ESP32 nalopen.
- HTTP-beheer mist nog volwaardige authenticatie: beheerinterface **niet publiek toegankelijk maken**.
- Universele TX-router en experimentele Orcon-bediening zijn ongemoeid gelaten; fysieke zending/koppeling niet bewezen.
- Geen echte ESPHome/ESP-IDF-crossbuild, OTA- of hardware-soak in deze omgeving.

## Tests in deze ontwikkelronde

**Geslaagd**
- `test_diag7_browser.py` — 1440/390 px, bootincident en filters, mm:ss,mmm, diagnose-export, lokaal zoeken.
- `test_ux_browser.py` — sensorfunctie ontvangen, goedkeuren, verwijderen, Home Assistant-offline, naam, RF-identiteiten; aangepast aan correcte nieuwe UI-indeling.
- `test_profilesafe1_browser.py` — 26 profielen / 16 Orcon, falende gebruikersbibliotheek, bevestiging verwijderen.
- `test_logbook_combined_browser.py` — alle logboeken, filters, export, limieten, vier talen.
- `test_orcon_experimental_browser.py`, `test_deviceux4_browser.py`, `test_pairing1_browser.py` — gesimuleerde apparaat- en koppelroutes.
- `test_profileflow1_compilefix1.py` — C++-webbron via host-stubs `-fsyntax-only`.
- `test_packfix1_release.py`, `test_release_modes.py`, `test_release_radio_guard.py`, `test_release_rev2_guard.py` — release- en PCB-guardcontroles.
- `test_localization_browser.py`, `test_bootdiag_browser.py`.

**Niet volledig geslaagd/afgerond**
- Een grotere NAV4- en device-panel-testopdracht liep tegen tijdslimiet aan. Die is niet als PASS geclaimd.
- Geen volledige C++-ESPHome-build voor de echte ESP32 en geen RF-hardwaretest.

## Gebruikersacceptatie / 3-klikregel

| Wat wil ik doen? | Waarom? | Direct zichtbaar | Controle |
|---|---|---|---|
| Zien waarom RFLink herstart | Herhaald panic-probleem begrijpen | Statuswaarschuwing en Bekijk crashlog | 1 actie naar Systeem > crashfilter in browsermock |
| Eerdere gebeurtenissen onderzoeken | Debuggen zonder drie logs openen | Alles/Radio/Systeem/Web + typefilter | Alle vier bronnen en filters getest |
| Logtijd exact vergelijken | Beslissingen rondom TX in volgorde zetten | uu:mm:ss,mmm sinds start | Browsermock 380 ms -> 00:00:00,380 |
| Eén diagnose exporteren | Informatie verzamelen voor problemen | Download diagnosepakket | JSON-content gecontroleerd |
| Sensorfunctie goedkeuren | Nieuwe meting beschikbaar maken | Pending functie zichtbaar | Sensorflow-browserproef geslaagd |
| Goedgekeurde functie beheren | Ongewenste functie verwijderen | Beheren na bewust openen | Geen prominente zeven verwijderknoppen, verwijderproef geslaagd |
| Apparaat zoeken | Snel apparaat vinden | Naamzoeker direct | Geen nieuwe API-call tijdens typen |

## Installeren / flashen

- Gebruik **`UPDATE_RFLINK.bat`** met je eigen YAML en controleer de bestaande **6 MHz SPI** en pinout. `MAAK_RELEASE.bat` bouwt de DEFAULT, niet automatisch jouw persoonlijke configuratie.
- De nieuwe `firmware_revision` eindigt op `...-UNIFIED6-DIAG7`.
- Leg na installatie `boot_count`, `boot_session_id`, `reset_reason` en geheugenwaarden vast. Test Apparaten en Logboeken en verzamel pas daarna nieuwe crashinformatie.
- Bij een nieuwe panic: download de **bijbehorende ELF van diezelfde build**, ESPHome-serial log en DIAG7-diagnosepakket. Alleen resetreden geeft niet de precieze C++-oorzaak.
