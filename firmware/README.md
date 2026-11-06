# RFLink Universal — Radioactive v0.7 native ESP-IDF source

The firmware uses an ESP32-S3 N16R8 and three CC1101 radios. A universal radio manager is responsible for shared SPI and all RX/TX transitions. RF profiles, protocol codecs, network configuration and OTA live in separate components. ESPHome is **not** required or included in this firmware.

**Status:** Source prototype. Full ESP-IDF build and RF hardware testing are pending. **Do not flash this over a working Radio Gaga v0.6.6 device.**

The board's default pinout and SPI speed are in [boards/rflink_s3_n16r8.json](boards/rflink_s3_n16r8.json). Local overrides are stored in RFLink Builder outside GitHub.

The home build path is **RFLink Builder** (Home Assistant app), which uses ESP-IDF to produce OTA/USB artifacts after build checks pass. The source manifest detects unexpected changes; release outputs are never accepted from stub or simulated builds.

See [documentation](docs/README.md) and the [repository rules](../AGENTS.md).
