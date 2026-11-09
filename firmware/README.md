# RFLink Universal

**Stage 28 early diagnostic startup (development):** Crash history, log buffers,
Wi-Fi, HTTP diagnosis and authenticated OTA are started before any CC1101
hardware initialization. HTTP status advertises whether the radio providers
were initialized, and separately whether firmware boot/OTA validation actually
completed. A reachable management endpoint is not an assertion of radio
health. Historical phase numbers stay compatible with Stage 27. Real watchdog,
power-cycle and OTA testing is still required; the device-wide ESP-IDF panic
handler cannot preserve a live Wi-Fi socket across an immediate reset.

**Stage 27 boot checkpoint diagnostics (development):** Native ESP-IDF stores a
bounded list of the last reached startup stage for up to 12 boots in NVS,
alongside the existing (unchanged) reset history. The crash API exposes only
sanitized stage enums. It does not save every serial event or guarantee that
the last checkpoint caused a crash. A separate ESP32-S3 must be used to verify
watchdog/power-loss behavior; do not flash over a working v0.6.6 reference.
 — Radioactive v0.7 native ESP-IDF source

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

## Stage 25: TX troubleshooting and export (development)

Every `RadioManager::transmit_encoded()` call now reports a correlated
rejection or start/completion with its UART/RMT/FIFO transport, physical
failure stage and previous RX restoration state. A driver result is **not
proof of on-air transmission**. There is intentionally no web TX button yet.

Use the prototype Logbooks card to filter by radio and event type, then
download the filtered chronological metadata as CSV. This is volatile
in-memory history, not persistent crash evidence or a raw RF capture;
entries can be overwritten during an extended download. The saved crash
summary remains independent. Hardware verification is still required.
