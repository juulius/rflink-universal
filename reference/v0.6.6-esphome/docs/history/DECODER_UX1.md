# DecoderUX1 — gebruikersinterface en inleren (22 september 2026)

## Wat je als gebruiker doet
1. Open `http://<IP>/`; de browser gaat naar `/rflink`.
2. Kies **Leren** en de radio waarop het apparaat uitzendt. De wizard meet kort de achtergrond, daarna bedien je het apparaat enkele keren. Je hoeft geen protocolnaam of hexcode te kennen.
3. Kies de naam, controleer de **werkelijk ontvangen** functies en vink aan wat RFLink mag bewaren. Een ontbrekende functie wordt niet geraden. Bij een actieve MQTT-verbinding staat *Toevoegen aan Home Assistant* standaard aan; laat het uit voor een lokaal apparaat.
4. Klik op **Apparaat opslaan**. Eventueel later gevonden nieuwe functies verschijnen als voorstellen bij Apparaten; pas na goedkeuring wijzigen de functies in HA. Gewone meetwaarden en bekende knopacties worden direct bijgewerkt.

**Decoderbibliotheek:** Instellingen -> Decoderbibliotheek. Filter bediening/sensoren; elke kaart toont frequentie, instructie, HA-resultaat en een duidelijk onderscheid tussen ontvangstcode, native/softwaregeteste zendcode en niet-beschikbare TX. Voor ontwikkelvoorbeeld kies het filter *Ontwikkelvoorbeelden*; het verschijnt standaard niet. De echte radiotest is nog NIET uitgevoerd, ongeacht welke protocolnaam op het scherm staat.

**Apparaatbeheer:** standaard één overzicht met naam, HA-status, goedgekeurde functies en eventuele voorstellen. Nieuwe voorstellen zijn direct uitklapbaar. Naam wijzigen, HA-koppeling, zendtoestemming en RF-codebeheer zitten onder *Meer beheren*. Volledig verwijderen zit nog een niveau dieper achter *Gevaarlijke acties* én een bevestiging. Technische velden kunnen zendgegevens bevatten: ze staan apart, zonder gevaarlijke losse verwijderknop.

## Per decoder: welke aanwijzingen krijg je?

| Decoder | Wat de wizard vraagt | Wat je na bevestiging ontvangt / beperking |
| --- | --- | --- |
| Nexa | Een paar keer Aan en vervolgens Uit voor hetzelfde kanaal | Ontvangen Aan/Uit; TX uitsluitend na aparte toestemming; meerdere lampen/kanalen worden niet stilzwijgend samengevoegd. |
| ByronSX | Meermalen dezelfde deurbel indrukken | Deurbelgebeurtenis; zendknop alleen na toestemming, fysieke test ontbreekt. |
| Dooya | Elke knop afzonderlijk meerdere keren indrukken | De werkelijk ontvangen knopnummers; betekenis als Omhoog/Stop wordt niet automatisch geraden. |
| RCSwitch | Eén vaste code herhaald ontvangen, volgende knop afzonderlijk leren | Codegebeurtenis, alleen hostgeteste TX bij exacte code/protocol/bitlengte en toestemming. |
| IthoRFT | Stand- of timerknop herhaald indrukken | RX-standen/timers; daarna RFLink één keer koppelen als extra virtuele afstandsbediening. Native Low/Medium/High + Timer 10/20/30; Stand-by en compatibility-pairing onder Advanced. |
| SomfyRTS56 | Eén knop herhaald, niet de programmeerknop | RX + native virtuele RFLink-remote; na expliciete pairing Omhoog/My/Omlaag. Rolling counter blijft persistent. |
| KeeLoq | Herhaald één knop van dezelfde afstandsbediening | RX en eventuele batterijmelding; geen generieke code-replay of TX. |
| AlectoV1 | Wachten op meerdere uitzendingen | Bevestigde temperatuur/vocht/neerslag/batterij waar gedecodeerd; ontbrekende waarden worden niet automatisch verwijderd. |
| Acurite609TXC | Wachten op meerdere berichten | Bevestigde temperatuur/vocht/batterij; nieuwe ID na batterijwissel alleen handmatig koppelen. |
| FineOffsetWH2 | Wachten op berichten | Bevestigde temperatuur en vocht; geen bedieningsknoppen. |
| RubicsonTemp | Wachten op berichten | Bevestigde temperatuur/batterij; kanaal/CRC niet als losse sensoren. |
| ExampleSwitch | Alleen via ontwikkelfilter | Sjabloon, geen echt te leren apparaat. |

De instructies komen uit `decoders/<pack>/manifest.json`, niet uit twaalf hardgecodeerde UI-uitzonderingen. Nieuwe decoderpacks moeten gebruikersinstructie, band, HA-uitkomst en waarschuwing declareren; de manifestvalidatie toetst die op het bouwen.

## Wat technisch is aangepast
- Protocolmetagegevens in 8 decoderpacks/12 protocollen, gegenereerde `capability_catalog_generated.h`, uitleesbaar in `/rflink/api/decoders` en dezelfde metadata in de webinterface.
- Kandidaat-API levert ontvangen acties en veldnamen aan de wizard; de bevestigings-API valideert de geselecteerde sleutels tegen de werkelijke kandidaat en weigert onbekende, dubbele of te lange invoer.
- Eerste inleeractie bewaart alleen gekozen **publieke** functies; interne RF-/TX-velden blijven intact. Niet-geselecteerde publieke functies zijn runtimevoorstellen; sensor-berichttypen blijven intern en zijn geen HA-knopgebeurtenis.
- Bestaande NVS-gegevens, UID, radio-GPIO, TX-beleid, API/OTA en releasefunctie blijven behouden; geen migratie of automatische vrijgave van TX. Updater kent v2A/B/C en deze UX1-decodermanifests als afzonderlijke, gehashte officiële baselines; eigen aangepaste code blijft een conflict.

## Kritische controles op gebruiksgemak
- **Te veel acties?** Standaard één bevestiging bij inleren, bij apparaten slechts één uitklapbare lijst met functievoorstellen. Risicovolle opties een niveau dieper. Technische veldnamen niet op normale kaarten.
- **Duidelijk welk protocol?** Ontvangen protocol is leidend; kiezen vanaf de bibliotheek geeft uitleg en een passende radio, maar filtert geen radiosignalen weg. Bevestiging toont het *daadwerkelijk* herkende protocol.
- **Geen valse claims?** Softwaretest is geen hardwaretest; weersensoren hebben geen kunstmatige bedienknop en RX-berichttypen worden niet als HA-gebeurtenis aangeboden.
- **Onbedoelde wijzigingen?** Nieuwe functies pas na selectie; verwijdering extra bevestiging; apparaat verwijderen achter een extra uitklapper; MQTT offline blokkeert destructieve HA-wijzigingen.
- **Gedeeltelijke storing?** Ontbrekende radio, opslagfout of ontbrekende decoderinformatie blokkeert inleren/goedkeuren waar nodig.
- **Onderhoudbaarheid?** Metadata per pack en één generieke wizard; syntaxischeck, catalogustest, backend- en browserregressies en embedded-UI-pariteit.

## Nog open — geen volledige v2 claim
- Voorstellen staan nog in RAM tot goedkeuring: na herstart moet het signaal opnieuw worden ontvangen. Later een zorgvuldig gemigreerd versie-2-opslagformaat ontwerpen en NVS/rollback testen.
- Er is nog geen algemeen model met afzonderlijke lampen/kanalen per afstandsbediening of automatisch afgeleide Dooya-knopbetekenis. Itho packet-TX is vanaf ITHO1 native geïntegreerd.
- KeeLoq TX, webauthenticatie en een MQTT offline-queue voor configuratiewijzigingen ontbreken nog. Somfy en Itho hebben inmiddels een expliciete pairing/device-flow; fysieke hardwarevalidatie blijft open.
- Er is geen volledige ESPHome/ESP-IDF-compile, echte Windows PowerShell/BAT-uitvoering of fysieke RF-/HA-test in deze omgeving bevestigd. Eerst Validate/Compile en `TEST_UPDATE_RELEASE.bat` op Windows; zendfunctie alleen na echte hardwaretest inschakelen.
