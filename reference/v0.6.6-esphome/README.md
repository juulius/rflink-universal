# RFLink Universal v0.6.6 — reference sources (ESPHome)

This directory is an **archival snapshot of the user-provided RadioGaga v0.6.6 SOURCE ZIP**, imported to compare the original ESPHome CC1101/RMT implementation with the native ESP-IDF v0.7 rewrite.

Original archive: `RFLink_Universal_v0.6.6_RadioGaga_RELEASE_HASH_FIX_SOURCE (2)(2).zip`  
Archive SHA256: `d91dde4f579a92e5313b13e6499de45ff4758ad149045817f6014b444a519a8f`

* Firmware and component sources, user-facing UI, tests, release documentation and scripts were kept in their original relative paths. Original line endings and UTF-8 content are preserved by the import process.
* Three build-only artifacts are omitted: two `__pycache__/*.pyc` files and the compiled test executable `rflink_universal/tests/test_radio_engine_v2`. No credentials were detected; `rflink_universal/secrets.example.yaml` contains placeholders, not working credentials.
* A binary `favicon.ico` is included with its original contents.
* **REFERENCE ONLY — not part of the compiled native firmware nor the Builder release process.** Nothing in this directory should become a component of the v0.7 build or be deployed to ESP32 until ported and independently tested.

For 433 MHz, start at `rflink-universal.yaml` (`remote_receiver`, 96 RMT symbols, 8 KB receive buffer, GDO2) and `rflink_universal/components/rflink_universal/rflink_universal.cpp` (raw capture and UI). For 868 MHz, see the same YAML `cc1101` packet profile (63-byte FIFO with RSSI/LQI/frequency offset). See `docs/LOGGING_RUNTIME.md` for the v0.6.6 logging contract.
