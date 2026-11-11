# RFLink Universal v0.5 Radiolove — UXUNIFIED6

## Basis
Gebouwd vanaf `RFLink_Universal_v0.5_Radiolove_PROFILESAFE1_LOGROWFIX1_FULL_SOURCE.zip`.

## Wat een gebruiker wil doen

| Doel | Wat moet direct zichtbaar zijn | Resultaat |
| --- | --- | --- |
| Radiolog bekijken | Zelfde tijdlijn als systeem- en weblog, direct filter op frequentie | Eén logviewer, vier brontabs |
| Ruis tonen | Slechts één begrijpelijke instelling | Checkbox `Ruis tonen` bestuurt zowel All als Radio |
| Logregel onderzoeken | Compacte regel, details op aanvraag | Tijd / bron / ernst / gebeurtenis, per regel uitklapbaar |
| Geen regels aanwezig | Stabiele hoogte, geen gerekte rijen | Vaste scrollbox met lege ruimte, duidelijke lege toestand |
| Apparaat instellen | Eén eerstvolgende stap, geen dubbele kaart | Aparte instelpagina met directe profielkeuze |
| Gekoppeld apparaat beheren | Koppeling en test apart, RF-administratie secundair | Stapsgewijze profiel-/koppelschermen, aanvullende opties ingeklapt |

## Implementatie
- Radiolog wordt niet meer als een tweede, groot apart paneel getoond. Alle drie bronnen komen samen in `#logbookTimeline`.
- De oude radiomonitor-elementen blijven alleen als verborgen compatibiliteits-IDs in de DOM voor bestaande routines en tests; ze verschijnen niet dubbel.
- De frequentiekeuze staat als compacte filterband **onder** de gemeenschappelijke werkbalk. `Ruis tonen` is de enige zichtbare ruisoptie.
- Radioverzoeken gebruiken bij uitgevinkt ruis de `kind=0` filter. Bij aanvinken wordt ruis opgehaald; geen tweede selectiefilter dat dit overschrijft.
- De logtabel heeft dezelfde 4-koloms structuur voor alle brontabs. Op mobiel verschuift de nadruk naar tijd, bron en gebeurtenis; waarschuwingen/fouten krijgen een indicatie aan de rand.
- De gedetailleerde apparaatkaart herhaalt niet meer de apparaatnaam in een tweede kop. Voor een apparaat zonder profiel wordt alleen profielkeuze als hoofdactie getoond. Overige instellingen zitten onder één uitklapdeel.
- De bestaande browser-/firmwarecode blijft behouden, inclusief de experimenteler TX-architectuur en de eerder aangebrachte PROFILESAFE1 crashbeperking.
- De interne revisie eindigt op `-LOGROWFIX1-UNIFIED6`.

## Teststatus
- Chromium browser: gecombineerde logviewer, Radio/Systeem/Web-tab, filter, export, schermgrootte 1440/390: geslaagd.
- Chrome: 1/2/5/30 rijen, vaste hoogte en intern scrollen, uitklapdetails: geslaagd.
- Apparaat overzicht → profiel instellen, bestaand gekoppeld apparaat, state-behoud: 1440/390 geslaagd.
- Profielcatalogus: 26 ingebouwde profielen, 16 Orcon, filteren/mobiel: geslaagd.
- Profielveiligheid, Orcon/Itho-wizard, taalwissel NL/EN/DE/FR, releaseguards: geslaagd.
- *Niet* getest: volledige echte ESPHome 2026.10 crosscompile, daadwerkelijke CC1101-hardwarebediening, live langdurige SOAK, definitieve oplossing van panic-resets.

## Installatie en verificatie
1. Gebruik altijd je bestaande persoonlijke ESPHome YAML met eigen GPIO's en 6 MHz SPI. De DEFAULT YAML is niet identiek aan jouw hardwareconfiguratie.
2. Compileer/upload de volledige SOURCE met ESPHome. Lees na OTA `/rflink/api/status` en controleer `firmware_revision` eindigt op `UNIFIED6`.
3. Doe een harde browserrefresh als oude stijlen zichtbaar blijven (`Ctrl+F5`). Als er nog steeds twee panelen staan, controleer dan de revisie van de werkelijke firmware in plaats van opnieuw patches te stapelen.
4. Test op een lege Radio-log en met meerdere live gebeurtenissen. Bij uitgeschakelde ruis kunnen er 0 zichtbare regels zijn terwijl de ruiscounter oploopt; dat is normaal.

## Aandachtspunten
De veiligheidsblokkades voor niet-gevalideerde RF-koppelingen zijn ongemoeid gelaten. Diagnose- en crashesessies blijven hardwareafhankelijk; een UI-test bewijst geen crashvrij gedrag op de ESP32.
