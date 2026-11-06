# RFLink Universal Builder — Home Assistant

> **Development app:** The official ESP-IDF build, USB/OTA deployment and hardware recovery process still require real-world verification. Do not use this app to replace a working v0.6.6 device.

## Installing the app (after publication)

1. In Home Assistant OS, open **Settings → Apps → Install app → ⋮ → Repositories**.
2. Add `https://github.com/juulius/rflink-universal` once.
3. Install **RFLink Universal Builder** from that repository and open its web interface.
4. Under **GitHub**, enter `juulius/rflink-universal` and select **Check**.
5. Preview the source changes and apply them. Unchanged files remain untouched; local overrides and secrets stay in the Builder.
6. Check your board's default profile under **Hardware and SPI**, then build the firmware.
7. Only after a **real successful build** and test-board compatibility checks should you try USB/OTA installation.

The first build environment download may be large and requires free disk space and internet. Later builds reuse the installed toolchain.

## Firmware versions

The previous five published stable firmware versions appear in the default selector. **All releases** shows older/newer versions and prereleases. A selected GitHub tag is resolved to a fixed commit and compiled in an isolated workspace using your current local board settings. Earlier successful builds can be selected again after hash and profile checks.

## Local-only configuration

The default board and SPI settings come from GitHub. Hardware overrides, Wi-Fi/OTA secrets, and MQTT credentials live in the Home Assistant app's persistent `/data` storage, never in the public repository. Credentials are **not automatically flashed** into firmware or deployed to a device; firmware configuration support must be tested separately.

## Safety and limitations

Native OTA updates require a compatible Radioactive firmware on the target device; v0.6.6 Radio Gaga is not a supported direct OTA migration target. Use a dedicated ESP32-S3 N16R8 test board for initial flashing. Early OTA and MQTT provisioning operate over HTTP/plaintext LAN protocols and are unsafe for direct internet exposure.
