# RFLink v0.7.0 Radioactive — first Experimental ESP32-S3 test board

**Status:** A test plan, **not** a hardware test report. Green GitHub Actions results confirm code/tests/compilation only. No physical board has been tested as part of this change.

## Before starting

1. Use a **spare ESP32-S3 N16R8** with the three CC1101 modules at 433.42, 433.92 and 868.30 MHz. Keep the working v0.6.6 device powered and untouched.
2. Verify the default board profile matches the actual connections. Current reference: SPI SCLK 12 / MOSI 11 / MISO 13; 433.42 CS/GDO0/GDO2 4/5/6; 433.92 9/8/14; 868.30 15/16/17. The initial conservative SPI setting is **200 kHz**. Do not blindly flash if your board differs.
3. Ensure you have a reliable USB data cable, USB serial access through Home Assistant or a PC, and a way to erase/reflash the spare board. A full USB installation uses a *different* partition layout and can erase prior data.
4. Back up device-specific secrets elsewhere. Never share ZIP files containing Wi-Fi passwords or the OTA key.

## Produce a traceable test build

1. First merge the approved experimental-version PR after GitHub's **source checks** and **official ESP-IDF build** pass.
2. Open **GitHub → Actions → Publish RFLink firmware source release → Run workflow**. Choose `version: 0.7.0`, `codename: Radioactive`, `channel: experimental`, `sequence: 1`. Leave promotion and Stable approval blank. An ordinary merge is **not** a release.
3. Check the workflow completed; the immutable tag should be `firmware/v0.7.0-exp.1`. A source-only GitHub Release is *not* a flash-ready image.
4. Open **Home Assistant → RFLink Builder** after upgrading the Builder to the reviewed release. Create a **new test device**, check **Show test firmware**, select `v0.7.0 · Experimental · build 1`, and double-check the per-device GPIO/SPI profile and network credentials.
5. Click **Build firmware** and read the real ESP-IDF log. When complete, check the manifest includes `release_tag: firmware/v0.7.0-exp.1`, `release_channel: experimental`, `release_build: 1`, `test_only: true`, the correct board hash, and SHA-256 file hashes. If any disagree, stop.
6. Install the **validated first-install USB package** to the *spare test board only*, using the Builder's device-specific provisioning package and `TESTBOARD` confirmation. Do not substitute the OTA `.bin` for the initial USB partition table. Do not install to the working v0.6.6 device.

## Check the board before exercising radio TX

1. **Boot:** capture the serial log and note resets, boot-loop errors, chip/flash/PSRAM errors, NVS issues and Wi-Fi status. Keep the update/admin key private.
2. **Network:** the board should connect to the expected Wi-Fi or offer the documented provisioning path. Open its local IP address, **not** a public reverse proxy.
3. **Identity:** on `GET http://BOARD-IP/api/status`, expect `release_channel = experimental`, `release_build = 1`, a friendly `firmware_display`, and the matching `board_id` / `board_hash`. The native page should show the short version with the technical version under expandable details.
4. **Radio:** check presence and reception on 433.42, 433.92, 868.30 MHz individually; capture counts, RF profile, SPI timeouts/errors, and RX recovery information. Do not infer a working RF decoder or functional send control from the driver's presence alone.
5. **Logging:** verify bounded signal/noise/System/Web logs, filtered CSV export, and persistent crash/reset history across at least one *ordinary* restart. Do not deliberately crash the device until a recovery procedure is confirmed.
6. **OTA/rollback:** **defer** until you have a known-good USB recovery path and two explicitly compatible test builds. Native partition migration from v0.6.6 is not supported by OTA. An OTA test must verify A/B boot confirmation and recovery, not just a successful upload.
7. **TX:** **defer** until RX is stable and intentional transmit controls/decoder mappings are implemented. UART/RMT/FIFO driver attempts alone do not prove on-air RF transmissions.

## Information to bring back for the next iteration

Capture the **Builder build log** and firmware `release_manifest.json` (remove any private provisioning/credentials), the device's `/api/status` values, a redacted boot log, the results for all three radios, and the visible System/Radio/Web logs. We can then fix actual problems as **Experimental build 2** without moving the existing build 1 tag.

**Stop conditions:** wrong board hash/pinout, bad boot loops, SPI errors that worsen continuously, inability to recover USB, unexpectedly active TX, or OTA identity mismatch. No Stable release should be published until real hardware, safety and recovery gates have passed.
