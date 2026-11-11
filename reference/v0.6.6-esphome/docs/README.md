# RFLink Universal — documentatie

**Start in Windows met `00_RFLink_START.bat` in de hoofdmap.**

- **Keuze 1 — Bijwerken:** behoud je eigen YAML, GPIO's, 6 MHz SPI en experimentele opties via een conservatieve merge. De bestaande originele YAML wordt niet aangepast; resultaat is een CUSTOM-bundle. Controleer het voorstel voordat je flasht.
- **Keuze 2 — Schone release:** maak de officiële standaard-SOURCE-ZIP en ESPHome-bundle, met de **standaard-YAML**; die is niet hetzelfde als je persoonlijke bordconfiguratie.
- **Keuze 3 — Tests:** voor ontwikkelaars; draait de bestaande Windows-integratietest.
- **Keuze 4 — Documentatie:** opent dit overzicht.

Geen Python nodig voor het Windows-startmenu. De PowerShell-scripts zitten onder `rflink_universal/tools/`.

## Lees verder

- [Starten en uitleg](START_HERE.md)
- [Installeren en veilig bijwerken](INSTALLEREN_EN_UPDATEN.md)
- [Instellingen](INSTELLINGEN.md)
- [Gebruikshandleiding](HANDBOEK_RFLINK_NL.md)
- [Laatste wijzigingen](RELEASE_NOTES.md)
- [Versiehistorie](releases/)
- [Technische uitleg en decoderontwikkeling](source/)
- [Oude patchdocumentatie](history/)
- [Testverslagen](testing/)

**Veiligheid:** `00_RFLink_START.bat` voert geen OTA-flash uit; laad een gemaakte bundle pas na ESPHome Validate en Compile in je eigen systeem. Deel logs pas na controle op RF-identiteiten en geheimen.
