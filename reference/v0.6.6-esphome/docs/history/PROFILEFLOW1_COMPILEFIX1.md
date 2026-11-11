# PROFILEFLOW1 COMPILEFIX1 (26 oktober 2026)

Herstelt twee compilerfouten in `RFLinkWebHandler::devices_json_()` bij kandidaat-profielsuggesties: ontbrekende `;` na `ha_available`, en een ongeldige losse streamoperator `<<` voor `actions`.

De JSON-velden en hun volgorde zijn ongewijzigd. Geen wijziging aan decoderprotocollen, opslag, GPIO, CC1101-radio-instellingen of zendbeveiligingen.

De SOURCE-ZIP bevat de oorspronkelijke YAML met `cc1101_spi_rate_hz: "200000"`. Als je zelf 6 MHz gebruikt, neem dan je eigen `rflink-universal.yaml` mee of wijzig die waarde na uitpakken opnieuw in `"6000000"`. De kleine PATCH-ZIP bevat geen YAML, zodat jouw eigen instellingen intact blijven.

Validatie: host C++ syntax-only webfirmware, automatisch uitvoeren van regressietests, verificatie van ZIP en ESPHome-bundle. Dit is geen volledige ESPHome/ESP-IDF-hardwarebuild.
