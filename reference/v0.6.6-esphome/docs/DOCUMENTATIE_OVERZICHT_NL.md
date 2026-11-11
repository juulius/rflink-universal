# Documentatie-overzicht — RFLink Universal

## Dagelijks gebruik

- [HANDBOEK_RFLINK_NL.md](../HANDBOEK_RFLINK_NL.md): centrale Nederlandstalige handleiding voor Status, Apparaten, Logboeken, Instellingen en de vier BAT-bestanden.
- [START_HERE.md](../START_HERE.md): ingang plus historische releasecontext.
- [INSTALLEREN_EN_UPDATEN.md](../INSTALLEREN_EN_UPDATEN.md): volledige installatie- en updateprocedure.
- [USER_DECODER_GUIDE.md](../USER_DECODER_GUIDE.md): eigen decoders/profielen en beperkingen.
- [RELEASE_NOTES.md](../RELEASE_NOTES.md): volledige tijdlijn van firmwarewijzigingen.

## Ontwikkeling

- [UXREVIEW5_GEBRUIKERSAUDIT_NL.md](UXREVIEW5_GEBRUIKERSAUDIT_NL.md): gebruikersdoelen, UX-beoordeling, risico's en tests.
- [LIBRARY_AND_PROFILES.md](../LIBRARY_AND_PROFILES.md): opslag en bibliotheekcontracten.
- [LOGGING_RUNTIME.md](../LOGGING_RUNTIME.md): technische logopslag en API.
- [DECODER_API_V2.md](../DECODER_API_V2.md): decodercontract.
- [UX_PRINCIPLES.md](../UX_PRINCIPLES.md): 3-klikstrategie en visuele uitgangspunten.
- [Historisch archief](history/README.md): oudere releaseverslagen zonder verlies van broninhoud.

## De Windows-startscripts

- **MAAK_RELEASE**: nieuwe DEFAULT-bundle.
- **UPDATE_RFLINK**: aangepaste persoonlijke YAML inclusief 6 MHz SPI.
- **TEST_UPDATE_RELEASE**: alleen regressiecontrole.
- **MAAK_UPLOAD**: oudere alias; niet nodig voor normaal gebruik, maar behouden voor bestaande verwijzingen.

Het pakket bevat vier BAT-bestanden om bestaande tools niet te breken. Het opschonen beperkt zich tot documentatiebestanden die niet verplicht zijn in de releasegenerator of updater.
