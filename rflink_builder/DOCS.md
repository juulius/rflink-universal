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

### Installation appears stuck

This is a **locally built** app (no pre-built registry image). On first install, Home Assistant must download the official `espressif/idf:v5.5.6` Docker image. The compressed base image is approximately **5 GB**, and unpacking/building it requires additional free disk space. A spinner does not, by itself, prove failure.

1. In **Settings → System → Logs**, select **Supervisor** and look for messages about `rflink_builder`, Docker, disk space, image pulls or build errors. Installation problems appear in the Supervisor log, often **before** the app's own Log tab exists.
2. Check the free storage reported by Home Assistant and that the host has internet/Docker Hub access. Do not repeatedly trigger parallel installations.
3. If the Supervisor reports an error, save the relevant log lines (remove passwords, tokens, internal addresses or private details) before retrying. A repeated spinner without those logs cannot identify the exact cause.
4. When a newer app version is available in this repository, refresh the app store/repository and then retry once. If it is still stuck, send the Supervisor error rather than guessing.

A full first-time Home Assistant OS installation of this Builder has **not yet been confirmed**. This documentation does not claim that the large-image download or USB/OTA features have been hardware-validated.

Subsequent local Docker builds may reuse cached base-image layers unless the cache is cleared or the base image changes.

## Firmware versions

The previous five published stable firmware versions appear in the default selector. **All releases** shows older/newer versions and prereleases. A selected GitHub tag is resolved to a fixed commit and compiled in an isolated workspace using your current local board settings. Earlier successful builds can be selected again after hash and profile checks.

## Local-only configuration

The default board and SPI settings come from GitHub. Hardware overrides, Wi-Fi/OTA secrets, and MQTT credentials live in the Home Assistant app's persistent `/data` storage, never in the public repository. Credentials are **not automatically flashed** into firmware or deployed to a device; firmware configuration support must be tested separately.

## Safety and limitations

Native OTA updates require a compatible Radioactive firmware on the target device; v0.6.6 Radio Gaga is not a supported direct OTA migration target. Use a dedicated ESP32-S3 N16R8 test board for initial flashing. Early OTA and MQTT provisioning operate over HTTP/plaintext LAN protocols and are unsafe for direct internet exposure.
