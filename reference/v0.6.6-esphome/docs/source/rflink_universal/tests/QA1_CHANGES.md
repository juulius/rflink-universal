# QA1 wijzigingen ten opzichte van NAV4

**Runtime:** alleen `ui/index.html`, de daaruit gegenereerde `components/rflink_universal/web_ui.h` en de revisietekst in `firmware_identity.h` gewijzigd (toegankelijkheid: expliciet apparaatnaamlabel, ARIA voor readonly decoderfragment). De radio-, driver-, logger-, decoder-, NVS-, HTTP- en YAML-runtime is ongemoeid.

**Tests:**
- `tests/run_tests.sh`: ongeldige GCC-combinatie van `-c` en twee bronbestanden met één `-o` vervangen door twee objectcompilaties.
- `tests/test_localization_browser.py`: ontbrekende mockroutes voor nieuwe RX-log toegevoegd; vier talen slagen.
- `tests/test_user_flow.cpp`: drie afzonderlijke drukken simuleren met tussenpozen > dedupe-venster; normale leerproces-/TX-/persistencetest slaagt met en zonder MQTT.

**Niet gewijzigd:** Wi-Fi, API/OTA, CC1101-pinnen, radiofrequenties, rebootfix, logs en geheugenlimieten, MQTT-instellingen, TX-policy of decoderregistratie.

Dit is géén bewijs van een echte ESP32-build, geen beveiligingsfix en geen productie-stabiliteitscertificering. Zie `QA1_AUDIT.md`.
