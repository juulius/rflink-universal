# DeviceUX4 — gebruikersaudit en besluit (1 november 2026)

## Doel en grenzen

Bron: `RFLink_Universal_ORCONTXLAB2_LOGBOOK1_PACKFIX1_FULL_SOURCE` (388 bestanden vóór deze wijziging). De werkzaamheden gaan over **webinterface en documentatie**, niet over RF-TX, pairingprotocol of stabiliteitsfixes. Een browserfixture is geen ESPHome/ESP-IDF-hardwaretest.

**Keuze:** geen grote bedieningspopup voor alle functies. De hoofdlijst is voor **status + snelle dagelijkse bediening**; `Instellen en koppelen` opent een volledige, interne detailweergave binnen de bestaande Apparaten-pagina. De koppelwizard is al een dialoog. Een tweede, geneste popup zou ingewikkeld zijn op mobiel en bij timeouts. Dit blijft één navigatiestap vanuit Apparaten. Apparaten zonder profiel tonen direct `Apparaat instellen`, gevolgd door **één** profielselectie op de detailweergave.

## Herbeoordeling als leek

| Intentie (wat en waarom) | Wat meteen te zien? | Nieuwe route / limiet | Beoordeling |
|---|---|---|---|
| Ik wil zien of mijn ventilator werkt | Naam, radio, huidige beschikbaarheid | `Apparaten` → kaart: 1 klik | Compact, geen lege linkerkolom |
| Ik wil de stand wijzigen | Geldige, gereedstaande knoppen | `Apparaten` → knop: 2 klikken | Normale bediening niet achter een technische dialoog |
| Ik zie een onbekend apparaat; hoe stel ik het in? | Eén knop `Apparaat instellen`, geen dubbele dropdown en wizard | `Apparaten` → `Apparaat instellen` → directe profielkeuze | Maximaal 2 navigatiestappen, profiel selecteren is taakhandeling |
| Ik wil koppelen | Koppelstatus in detail, beschikbare wizard | `Apparaten` → `Instellen en koppelen` → wizard | Geen tweede geneste popup |
| Ik wil RF-identiteiten/naam/HA wijzigen | Beheer pas in apparaatdetails | `Apparaten` → detail → sectie beheren | Geen technische ballast op gewone kaart |
| Ik wil gegevens ontvangen van een sensor | Status, daarna details/metingen | `Apparaten` → kaart | Sensor krijgt geen nutteloze TX-knoppen |
| Ik wil een niet gevonden opgeslagen profiel herstellen | Duidelijke fout + behouden RF-identiteit | Detail → profiel opnieuw kiezen | Geen crashclaim; fallback alleen in frontend |

## Wat dit niet oplost

- Geen aanpassing van de backend `/rflink/api/devices` en geen bewijs dat eerdere panic-resets zijn opgelost.
- Geen volledige hardware-/OTA-compile. TX blijft experimenteel en beschermd door dezelfde firmwarevoorwaarden.
- Geen algemene herstructurering van de instellingen-/logboekschermen. Deze audit raakt de apparaatkaarten; de eerdere wensen over korte firmwareversies, vaste loghoogte en instellingen blijven aparte backlogitems.

## Documentencontrole

Exacte documentparen (behouden → vervallen):

- `TXROUTER2_DEVICEUI2_RELEASE.md` → `rflink_universal/TXROUTER2_DEVICEUI2_RELEASE.md`
- `rflink_universal/RXLOG2_CRASHFIX.md` → `RXLOG2_CRASHFIX.md`
- `ACTIVITYFIX1.md` → `rflink_universal/ACTIVITYFIX1.md`
- `DEVICECARDS1.md` → `rflink_universal/DEVICECARDS1.md`
- `NAVIGATIE_3KLIK_AUDIT.md` → `rflink_universal/NAVIGATIE_3KLIK_AUDIT.md`
- `PROFILECATALOG2_RELEASE.md` → `rflink_universal/PROFILECATALOG2_NL.md`
- `NAV4_FINDABILITY_RUNTIME_MQTT.md` → `rflink_universal/NAV4_FINDABILITY_RUNTIME_MQTT.md`
- `PAIRING1_RELEASE.md` → `rflink_universal/PAIRING1_RELEASE_NL.md`

Het verwijderen van de acht exacte kopieën is een opschoning van documenten, geen herschrijving van hun betekenis. De 4 BAT-launchers en de historische hotfix-BAT blijven aanwezig zodat tests, compatibiliteit en CUSTOM-updates niet stilzwijgend veranderen.

## Controle uitgevoerd

Browser-simulaties met de echte HTML/JS en lokale fixturedata (geen radio-uitzending):

- `test_deviceux4_browser.py`: overzicht, één duidelijke instelactie, direct profiel kiezen, details en terug, 1440/390 px: **PASS**.
- `test_itho_ui_browser.py`: pairing + bedienknoppen op detail en veilig versturen: **PASS**.
- `test_orcon_experimental_browser.py`: experimentele wizard, expliciete ACK/test/verificatie, 1440/390 px: **PASS**.
- `test_pairing1_browser.py`: RX-only Orcon zonder ongevalideerde TX en Itho-koppeling: **PASS**.
- `test_device_panel_state_txv3.py`: details blijven bij live verversing open, desktop en mobiel: **PASS**.
- `test_ux_browser.py`: gesimuleerd inleren, goedkeuring, RF-identiteit vervangen, offline HA, verwijderen: **PASS**.
- `test_nav4_browser.py`, `test_simple_ui_browser.py`, `test_localization_browser.py`, `test_logbook_combined_browser.py`, `test_qa1_accessibility.py`, `test_settings_compact1.py`: **PASS**.
- `test_packaging.py`, `test_packfix1_release.py`, `test_release_modes.py`, `test_radio_optional.py`, `test_board_config.py`, `test_radio_transport_catalog.py`: **PASS**.
- `test_monitor_expanded_persistence.py`: **FAIL**, ook op de onaangepaste originele PACKFIX1-bron. Er ontbreekt een verwachte nieuwe signaalrij. Deze bestaande regressie is niet opgelost of weggewerkt door de DeviceUX4-wijzigingen.
- Niet uitgevoerd: complete ESPHome/ESP-IDF compile, Windows PowerShell `MAAK_RELEASE.bat`, hardware-OTA, radio/TX-metingen en crash/stress-soak.
