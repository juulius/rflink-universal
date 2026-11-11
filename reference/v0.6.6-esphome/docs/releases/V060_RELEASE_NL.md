# RFLink Universal v0.6 — Radio Gaga

**Releasetype:** grote UX-/ontwerpronde op basis van v0.5.5 Radiolove DIAG55 + TXQUEUE.  
**Bronhash:** `19a467a723ef` (SHA-256-fingerprint van 131 relevante runtimebronbestanden; geen Git-commit).  
**Hardwarestatus:** experimentele bronrelease; een volledige ESPHome/ESP-IDF-build en echte RF-zendtest zijn nog niet uitgevoerd.

## Waarom deze versie?

Het goedgekeurde donkere ontwerp van **Logboeken** en **Instellingen** wordt de gemeenschappelijke ontwerpstijl voor Status, Apparaten, apparaatdetails en Apparaat toevoegen. Uitgangspunt: belangrijke dagelijkse informatie staat meteen in beeld; overige inhoud blijft bereikbaar, maar neemt niet onnodig schermruimte in.

## Geïmplementeerd

- **Status:** vier kernwaarden (systeem, radio's, apparaten en aandachtspunten), drie compacte klikbare radioregels, maximaal vier recente activiteiten, een duidelijke sectie met aandachtspunten, snelle acties. De bestaande diepe systeemdiagnose blijft beschikbaar achter één uitklapper, niet als tweede groot dashboard.
- **Apparatenoverzicht:** compacte, gelijk uitgelijnde kaarten met naam, protocol/frequentie, profiel, laatst ontvangen signaal, toestand en een relevante actie. Werkelijke age_ms uit de API wordt gebruikt; er wordt nooit een fictieve tijd ingevuld. Details zijn op verzoek zichtbaar.
- **Bediening:** als er volgens de API zenden mogelijk is, staat Bedienen direct op de kaart. De knop vouwt alleen de werkelijke beschikbare bediening uit. Geen bedienknoppen wanneer de API alleen ontvangst bevestigt. Sensoren kunnen hun huidige gerapporteerde waarden bekijken.
- **Apparaatdetails:** bovenaan profiel en koppelwizard, daarna bediening/functies, gevolgd door één afdeling voor ontvangen functies, RF-signalen, HA en technische gegevens. Verwijderen is één afzonderlijke waarschuwing onderaan, met de bestaande verplichte bevestiging.
- **Apparaat toevoegen / Instellingen / Logboeken:** behoud van goed werkende dagelijkse routes, consistente kleuren, knoppen, randen en onderlinge afstanden. De bestaande gecombineerde tijdlijn met Diagnosetab blijft ongewijzigd in functionaliteit.
- **Logginginstellingen:** ontbrekende tellerwaarden worden veilig als een streepje weergegeven in plaats van `undefined` of `NaN`.
- **Talen:** nieuwe UX-teksten zijn toegevoegd aan Engels (standaard), Nederlands, Duits en Frans.
- **Release:** v0.6 Radio Gaga en een korte buildhash; Windows-integratietests verwijzen naar de werkelijk ingestelde releaseversie in plaats van een vaste oude bestandsnaam.

## Controle van de 3-klikstrategie

| Scherm | Wat wil de gebruiker? | Meteen zichtbaar | Doorgang |
|---|---|---|---|
| Status | Weten of RFLink functioneert | kernstatus, aantal radio's, aandachtspunten | radio → logboek in 1 klik |
| Apparaten | Snel herkennen en bedienen | naam, status, laatste activiteit, Bedienen indien toegestaan | Bedienen → keuze commando; instellingen in 1 klik |
| Apparaatdetails | Profiel kiezen of koppelen | profiel, wizard en beschikbare functies | profiel / koppelen binnen 2 klikken vanaf overzicht |
| Apparaat toevoegen | Onbekend apparaat inleren | frequentiekeuze en Start | begeleide flow direct zichtbaar |
| Logboeken | Problemen volgen | Alles / Radio / Systeem / Web / Diagnose | technische details per gebeurtenis op verzoek |
| Instellingen | Normale voorkeuren aanpassen | taal, LED, loglimieten, HA | geavanceerd apart bereikbaar |

## Wat is bewust niet gewijzigd?

- ESP32-pinout, CC1101-SPI-configuratie, persoonlijke YAML, RX-/TX-drivers, fysieke UART-/GDO-route en radioherstel.
- Protocoldecoders, 26 ingebouwde apparaatprofielen, eigen bibliotheek, gekoppelde apparaten en opslagstructuur.
- De queue-correctie uit v0.5.5. Die is nog niet fysiek bewezen vrij van stack-overflow of succesvol Orcon-zenden.
- Technische logbuffers zijn niet vergroot.

## Tests in deze omgeving

**Geslaagd in gesimuleerde Chromium-/API-tests** (geen echte ESP32):

- Nieuwe Radio Gaga Status-, apparaten- en detailroute op 1440 en 390 px; geen horizontale overflow.
- Orcon-profiel aanwezig maar alleen ontvangst: geen onterecht zenden; koppelwizard bereikbaar.
- Itho-apparaat met vrijgegeven bediening: directe bedienknoppen blijven bereikbaar.
- Detailpaneel gesloten bij openen, signaalbeheer na uitklappen, verwijderen maar één keer met bevestigingsfunctie.
- DeviceUX4 bestaande profiel/refresh-routes, DIAG55 logboek en instellingentest, CRASHTRACE-browser, Itho-bediening, Orcon-wizard.
- 26 profielen (waarvan 16 Orcon), 4 talen met browserkeuze en save/rollback, bibliotheek, accessibility, ZIP/favicons en PowerShell-interpolatiecontrole.
- JavaScript-syntax, gebundelde UI, bronfingerprint op 131 runtimebestanden, hash bij gewijzigde bronbestanden afgekeurd.

**Niet volledig bevestigd:** oude NAV3/PAIRING1-browserflows hebben in deze omgeving een time-out gegeven; dit is geen bevestigde regressie maar ook géén geslaagde test. De echte Windows PowerShell 5.1-uitvoering, complete ESPHome/ESP-IDF-compile en fysieke radio-/koppeltests zijn niet uitgevoerd.

## Installeren op jouw PCB

Gebruik bij voorkeur `UPDATE_RFLINK.bat` om **jouw bestaande YAML met 6 MHz SPI, GPIO-pinout en experimentele TX-labopties** over te nemen. Controleer de voorgestelde wijzigingen en werk niet rechtstreeks over het origineel heen. `MAAK_RELEASE.bat` maakt de standaardrelease en neemt je eigen aanpassingen niet vanzelf over.

Na flashen via `/rflink/api/status` controleren:

- `firmware_version`: `v0.6`
- `firmware_name`: `Radio Gaga`
- `firmware_build_hash`: `19a467a723ef`

Test vervolgens eerst het openen van de pagina's, de profielcatalogus en het logboek. Onderzoek daarna pas de experimentele 1FC9-koppeling met seriële ESPHome-logs bij de hand.
