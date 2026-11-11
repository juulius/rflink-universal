# RFLink Universal — v0.5.1 Radiolove

Dit is een **patchrelease** op PAIRRADIO9. De universele TX-route en de bestaande radio- en PCB-configuratie zijn niet gewijzigd.

## Wat is nieuw

- **Korte release-identiteit**: in de webinterface staat `v0.5.1 Radiolove` en onder technische details een 12-tekens SHA-256-bronvingerafdruk, geen lange historiek van patchnamen.
- **Oude crashes en herstarts in Logboeken**: de huidige Systeemlog wordt aangevuld met eerdere resetredenen uit de reeds aanwezige NVS-bootgeschiedenis (maximaal de laatste acht starts, inclusief de huidige). Er komt geen vierde fysieke logbuffer.
- **Geen valse tijdstempels**: oude NVS-records krijgen `Eerdere start` in plaats van een verzonnen uptime. Ze verschijnen op bootnummer, na de huidige getimede gebeurtenissen, en zijn te filteren onder `Systeem → Crashes en herstarts`.
- **Fouten beter herkenbaar**: panic, brownout en watchdog worden als foutniveau getoond; een gewone softwareherstart als informatie.
- **Releasecontrole**: `MAAK_RELEASE.bat` gebruikt de korte firmwareversie en verifieert of de bronhash bij de uitgepakte runtime hoort. Zonder aangepaste versie en bijpassende nieuwe hash weigert de generator een gewijzigde bron onder dezelfde identiteit te verpakken.

## Grenzen

- De NVS-geschiedenis bevat **resetredenen, geen exacte datum/tijd of C++-backtrace**. Voor de onderliggende crashoorzaak blijft de ESPHome-seriële log plus het bijpassende `.elf`-bestand nodig.
- De fingerprint is een **ingekorte SHA-256-vingerafdruk van geselecteerde bronbestanden**, geen Git-commithash en geen SHA-256 van de uiteindelijke ELF/firmwarebinary.
- Bestaande `firmware_revision` blijft beschikbaar in de status-API maar is nu compact (`v0.5.1-<hash>`); oude interne labels zijn voor naslag opgeslagen in `docs/history/LEGACY_FIRMWARE_REVISION_PAIRRADIO9.md`.
- Echte ESPHome/ESP-IDF-compilatie en testen op jouw print zijn hier niet uitgevoerd. Gebruik je eigen YAML met 6 MHz SPI en bestaande pinout.

## Versiebeleid

- Elke opgeleverde codewijziging: patch omhoog (`v0.5.1`, `v0.5.2`, …). Alleen documentatie-/testwijzigingen zonder firmwarewijziging veranderen de firmware-identiteit niet.
- Grotere feature-uitbreiding: minor omhoog (`v0.6`), met een nieuwe speelse codenaam, bijvoorbeeld **Radio Gaga**. Een patch behoudt de codenaam van de featurelijn.
- Bij elke release: korte gebruikersgerichte patchnotes en een afzonderlijke technische fingerprint.

## Na installatie testen

1. Open `Instellingen → Over RFLink Universal` en controleer `v0.5.1 Radiolove`, builddatum en bronhash.
2. Open `Logboeken → Systeem → Crashes en herstarts`; controleer de huidige start en eerder opgeslagen resetredenen.
3. Open een historische regel voor technische resetcode; verwacht geen backtrace in deze UI.
4. Test daarna pas opnieuw de experimentele Orcon-koppeling, terwijl je Radiolog en seriële ESPHome-logs open hebt.
