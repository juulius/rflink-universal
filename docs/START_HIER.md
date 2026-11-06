# Start hier — RFLink Universal v0.7

Welkom! RFLink Universal is een zelfbouwontvanger en -zender voor draadloze apparaten. De print gebruikt een ESP32-S3 en drie CC1101-radiomodules. Je hoeft de technische termen niet te begrijpen om RFLink later te kunnen gebruiken.

> **Let op: dit is nog een ontwikkelproject.** v0.7.0 Radioactive is nog niet volledig gecompileerd en getest op de echte print. Installeer deze versie nog niet als vervanging van een werkende RFLink.

## Wat wordt de eenvoudige werkwijze?

Deze stappen zijn het **beoogde gebruik**, niet een al geteste installatieprocedure.

1. **Installeer RFLink Builder één keer** via Home Assistant.
2. **Kies je print** (standaard hardwareprofiel) en vul eventueel je eigen instellingen in.
3. **Kies een firmwareversie** uit de beschikbare GitHub Releases.
4. Klik op **Compileren**. De Builder maakt het firmwarebestand.
5. Kies **Installeren via USB**, **Bijwerken via WiFi (OTA)** of **Downloaden**.
6. Open de RFLink-website om apparaten in te leren en te bedienen.

Een eerdere versie nodig? Kies standaard een van de vijf vorige releases, of open **Alle versies**.

## Waar bewaar je je instellingen?

- **In GitHub:** de algemene broncode, standaard pinout en standaard SPI-snelheid, handleidingen en vrijgegeven firmwareversies.
- **Alleen lokaal in de Builder:** aangepaste GPIO's, SPI-snelheid en persoonlijke WiFi-, MQTT- en OTA-gegevens.
- **Op RFLink zelf:** de instellingen die de firmware op het apparaat opslaat. Een backup en terugzetprocedure worden nog getest.

Je hoeft geen bestanden handmatig stuk voor stuk te overschrijven. De Builder is ontworpen om verschillen vooraf te tonen en alleen gewijzigde bronbestanden bij te werken.

## Wat kan ik nu wel doen?

De ontwikkeling volgen en de documentatie bekijken. We testen eerst de volledige ESP-IDF-build, de drie radio's, zenden/ontvangen en veilige OTA-updates. De eerste echte installatiehandleiding verschijnt pas nadat dat is gevalideerd.

## Ik wil meer weten

- [Huidige technische status](V0_7_STATUS.md)
- [Documentatieoverzicht](README.md)
- [Printplaatbestanden en onderdelenlijst (gepland)](../hardware/README.md)

**Terminologie:** *OTA* betekent een firmware-update via het netwerk; *USB* is een bekabelde installatie; *BOM* is een onderdelenlijst (*Bill of Materials*); *SPI* is de dataverbinding tussen de ESP32 en de radiomodules.
