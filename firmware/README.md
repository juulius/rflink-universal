# RFLink Universal — Radioactive v0.7 native ESP-IDF source

The firmware uses an ESP32-S3 N16R8 and three CC1101 radios. A universal radio manager is responsible for shared SPI and all RX/TX transitions. RF profiles, protocol codecs, network configuration and OTA live in separate components. ESPHome is **not** required or included in this firmware.

**Status:** Native ESP-IDF compilation has passed in GitHub Actions for development source; the v0.7.0 Experimental release identity and its final test package still need a fresh official IDF build and hardware tests. Live RF hardware receive, transmit and pairing remain unverified. **Do not flash this over a working Radio Gaga v0.6.6 device.**

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

## Experimental test-board build (planned v0.7.0 exp.1)

Once the release-identity pull request passes tests and is merged, the maintainer can run **Actions → Publish RFLink firmware source release → Run workflow** with version `0.7.0`, codename `Radioactive`, channel `experimental` and sequence `1`. This workflow must pass real ESP-IDF compilation and package verification before the immutable source release is created.

In **Home Assistant → RFLink Builder**, select the deliberately published `v0.7.0 · Experimental · build 1` release after enabling test releases. Select or create the **separate test-board configuration**, confirm GPIO/SPI and Wi-Fi settings, and build. The firmware app's `/api/status` reports `firmware_display`, `release_channel`, `release_build`, exact app version and board/profile hashes; the native web page shows a compact friendly version and expandable technical identity. The release manifest must match the selected tag and board profile.

For USB first installation, prefer the Builder's validated per-device package because it provisions the test board's unique local credentials. Do **not** install the experimental binary over a working v0.6.6 board. Test first boot, Wi-Fi setup, logbook visibility, stored crash history and non-intrusive radio detection before any TX/OTA experiments. See [experimental board checklist](docs/EXPERIMENTAL_BOARD_CHECKLIST.md). Source-only GitHub Releases and green CI do not prove RF hardware or on-air transmission.
