# RFLink Universal v0.6.2 — Radio Gaga (PROJECTCLEAN62)

## Wat is er veranderd?

- Eén hoofdstartbestand: `00_RFLink_START.bat`, met een duidelijke keuze voor veilig bijwerken, schone release, developer-tests en hulp.
- De vier oude losse BAT-starters zijn niet langer nodig en niet opgenomen. De PowerShell-functies blijven behouden onder `rflink_universal/tools/`.
- Alle uitleg en historisch materiaal staan onder `docs/`, inclusief releasehistorie en testverslagen. Bestaande broncode- en radiofunctionaliteit zijn bewust ongemoeid gelaten.
- Releasegenerator en Windows-updatertests gebruiken de nieuwe locatie en controleren de aanwezigheid van de documentatie.

## Gebruik

1. Pak de volledige SOURCE-ZIP uit, bij voorkeur naar een kort pad zoals `C:\RFLink`.
2. Dubbelklik `00_RFLink_START.bat`.
3. Kies voor jouw print **1. Mijn bestaande RFLink veilig bijwerken**.
4. Lees het wijzigingsvoorstel; de oorspronkelijke YAML wordt niet gewijzigd. Controleer 6 MHz SPI, radio-GPIO's en TX-labopties.
5. Laad het gemaakte CUSTOM-bundle in ESPHome en controleer Validate + Compile voordat je OTA installeert.

## Beperkingen

Deze wijziging betreft verpakking, scripts, documentatie en firmware-versienaam. Er is geen fysieke RF-test of ESPHome-compile uitgevoerd. Windows PowerShell 5.1 blijft vereist voor de ingebouwde update- en releaseprocedures.

Firmwarebronhash: `0e22292101ed` (hash van de runtimebronnen, exclusief de identiteitsheader).
