# RFLink Universal — Handboek (v0.5 Radiolove, experimentele bron)

Deze pagina beschrijft **de huidige gebruikersroutes**. Oudere releasehistorie en tests staan in `docs/history/`; het volledige technische changelog blijft in `RELEASE_NOTES.md`.

## Welke actie wil ik uitvoeren?

| Ik wil… | Ga naar | Wat verwacht ik? |
|---|---|---|
| Mijn radio's controleren | **Status** | Online/offline en laatste ontvangst per radio |
| Een apparaat bedienen | **Apparaten** | Beschikbare knoppen direct op de compacte overzichtskaart |
| Een apparaat herkennen of instellen | **Apparaten → Instellen** | Profielkeuze en vervolgstappen; geen technische kennis nodig |
| Een apparaat aan RFLink toevoegen | **Apparaat toevoegen** | Frequentie kiezen, signaal ontvangen, bevestigen en opslaan |
| Een ontvanger koppelen | **Apparaten → Instellen → Koppelen** | Begeleide wizard; pas gereed na de juiste verificatie |
| Ontvangsten en fouten onderzoeken | **Logboeken** | Alles/Radio/Systeem/Web op één vaste tijdlijn, zonder dubbele logopslag |
| Logging aanpassen | **Instellingen → Logging** | Radio-, systeem- en weblog apart gegroepeerd |
| Decoders/profielen bekijken | **Instellingen → Decoderbibliotheek** | Herkenning, functies en compatibiliteit |
| Bibliotheek bijwerken | **Instellingen → Bibliotheek & updates** | Samenvatting direct zichtbaar; technische details apart |
| Firmwareversie en wijzigingslijst zien | **Instellingen → Over RFLink** | v0.5 Radiolove; volledige revisie en buildtijd in details |

## Logboeken

- **Radio:** drie CC1101-radio's, 433,42 MHz, 433,92 MHz en 868,30 MHz, inclusief gedeelde ruislog, RX en TX. De Radio-view biedt frequentie-/richting-/soortfilters en oudere gebeurtenissen ophalen.
- **Systeem:** interne beslissing na herkenning, leren, koppelen en zenden; dit is geen RAW-signaallog.
- **Web:** handelingen en API-fouten in de huidige browsersessie; na sluiten van de browser zijn deze niet standaard op de ESP32 opgeslagen.
- **Alles:** een gecombineerde chronologische weergave met filter en export, geen vierde opgeslagen buffer.

Radiosignalen zijn standaard 250 per radio, maar de huidige opslag/instellingen kunnen hiervan afwijken. Systeemlog is begrensd tot 256 gebeurtenissen (standaard 128, RAM, weg na reboot); weblog standaard 300, maximum 2000 browsergebeurtenissen. De logbuffer en tijdreferenties zijn niet over een volledige stroomuitval heen persistent. Details per event worden ingeklapt getoond. De weergave heeft een vaste hoogte en eigen scrollbalk.

## Experimentele Orcon-bediening

De ingebouwde RAMSES-II-encoder kan acht Orcon-acties opbouwen. **Dit bewijst geen werkende RF-uitzending.** De universele TX-router kiest de transportmethode en reserveert de radio. Orcon MVS-15R koppelen vereist een eigen virtuele identiteit, 1FC9-aanbod, passend antwoord, bevestiging en een handmatig geverifieerde Auto-test. Volg de stappen in [Orcon TX-labhandleiding](history/ORCONTXLAB2_RELEASE_NL.md). Gebruik nooit blind herhaaldelijke TX-pogingen. Gebruikersbevestiging is iets anders dan radiotechnische bevestiging.

## Welk BAT-bestand?

| Script | Functie | Gebruik |
|---|---|---|
| `MAAK_RELEASE.bat` | Schone standaard SOURCE + DEFAULT ESPHome-bundle | Nieuwe standaardinstallatie of releasecontrole |
| `UPDATE_RFLINK.bat` | CUSTOM-bundle voor je bestaande YAML en wijzigingen | **Voor jouw bestaande PCB (6 MHz SPI)** |
| `TEST_UPDATE_RELEASE.bat` | Release/updater-regressies | Alleen bij ontwikkeling en controle |
| `MAAK_UPLOAD.bat` | Compatibiliteitsalias van `MAAK_RELEASE.bat` | Niet nodig in normale workflow; behouden voor oudere scripts |

Een DEFAULT-bundle neemt niet automatisch je persoonlijke pinout, SPI-snelheid, secrets, API-instellingen en OTA-instellingen over. Maak eerst een backup. Test eerst ESPHome Validate + Compile; pas daarna OTA installeren.

## Crashes en trage API's

Een ontbrekend of ongeldig profiel mag niet als bewezen panic-oorzaak worden gepresenteerd. Een recente browserlog liet zeer trage `/rflink/api/devices`-aanvragen (20–46 seconden) en mislukkende bibliotheekaanvragen zien. In v0.5 is timingdiagnose toegevoegd voor `/devices`, `/library` en `/user-library`, en de browser stapelt bij een langzame aanvraag niet oneindig nieuwe automatische requests. **Dit is nog geen bewezen reparatie van een panic.** Exporteer bij nieuwe crashes het logboek, firmwarestatus inclusief `boot_count` en `reset_reason`, en de seriële panic-backtrace indien beschikbaar. Zie [crashaudit](UXREVIEW5_GEBRUIKERSAUDIT_NL.md).

## Veiligheid en beperkingen

Dit is een experimentele source-release. Er is geen volledige fysieke ESPHome/ESP-IDF-build of 72-uurs soak op jouw eigen ESP32-S3 uitgevoerd. HTTP-beheer is niet voor directe internetblootstelling beveiligd. De gebruikersdecoders, opslag en Orcon-koppeling moeten ook na OTA expliciet worden gecontroleerd.
