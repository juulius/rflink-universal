# RFLink Universal — Radioactive v0.7 native ESP-IDF source

The firmware uses an ESP32-S3 N16R8 and three CC1101 radios. A universal radio manager is responsible for shared SPI and all RX/TX transitions. RF profiles, protocol codecs, network configuration and OTA live in separate components. ESPHome is **not** required or included in this firmware.

**Status:** Native ESP-IDF compilation has passed in GitHub Actions; live RF hardware receive, transmit and pairing remain unverified. **Do not flash this over a working Radio Gaga v0.6.6 device.**

**Stage 21 receive watchdog (bench only):** A radio that is absent at boot can be re-probed every approximately five seconds without resetting it. Lost RMT capture or repeated 868 packet receiver errors trigger manager-controlled RX recovery. An active or switching TX radio is never reset by the watchdog. The existing native UART/RMT/FIFO TX transports are still experimental and have no user-facing TX or pairing API. See [RX recovery bench guide](docs/RX_RECOVERY_S21.md).

**Stage 24 logbooks (host development):** 250 signal records per radio,
2,000 shared noise records filterable by radio, 128 System and 128 Web
records. Fixed-size metadata uses PSRAM and a 1.5 MiB ceiling. The prototype
website adds filtered log views and management-key-protected live retention
settings saved in NVS. Full RF captures, detailed TX traceability and
hardware validation are not yet available. See
[logging and UI acceptance criteria](../docs/LOGGING_AND_UI_V07.md).

The board's default pinout and SPI speed are in [boards/rflink_s3_n16r8.json](boards/rflink_s3_n16r8.json). Local overrides are stored in RFLink Builder outside GitHub.

The home build path is **RFLink Builder** (Home Assistant app), which uses ESP-IDF to produce OTA/USB artifacts after build checks pass. The source manifest detects unexpected changes; release outputs are never accepted from stub or simulated builds.

See [documentation](docs/README.md) and the [repository rules](../AGENTS.md).
