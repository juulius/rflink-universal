# Versioning and safe firmware recovery

Three separate versions are tracked: **Home Assistant Builder** (app version from `rflink_builder/config.yaml`), **RFLink firmware** (GitHub Release tag, for example `firmware/v0.7.0`), and **the specific compiled binary** (source commit, board profile hash and SHA-256).

The Builder defaults to the **five previous stable firmware releases**. **All releases** includes earlier and newer versions and previews. A selected tag is resolved to its exact commit SHA and compiled in an isolated temporary project with locally stored pinout/SPI overrides. An available previous build can be reused only if its hashes and current board profile match.

A downgrade is not the same thing as the bootloader's automatic OTA rollback. Data migrations, partition compatibility and any anti-rollback eFuse restrictions can make older firmware unsafe. The original ESPHome v0.6.6 is **not** supported by Radioactive's native OTA update API.

**Release policy:** Publish a stable binary only after a full official ESP-IDF build, test-board radio RX/TX verification, safe OTA/USB and recovery tests. No tags or released source commits should be moved. Release notes must include known limitations and recovery instructions.
