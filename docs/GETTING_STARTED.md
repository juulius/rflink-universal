# Getting Started with RFLink Universal v0.7

Welcome! **RFLink Universal** is a do-it-yourself gateway that listens to and controls compatible wireless devices. It uses an ESP32-S3 board and three small CC1101 radio modules.

You do **not** need to understand electronics to use the finished RFLink web interface. This page explains what to expect without assuming technical experience.

> **Not ready to install yet:** v0.7.0 *Radioactive* is still in development. A complete official firmware build, real radio tests and safe update/rollback tests are not finished. Do not replace a working v0.6.6 device.

## What should using RFLink look like?

**This is our intended workflow, not a tested installation guide.**

1. **Install RFLink Builder** in Home Assistant once, from the project's GitHub app repository.
2. **Select your board profile.** The default pinout and SPI speed are supplied; your personal changes stay local.
3. **Choose a firmware version** from GitHub Releases. Five previous stable releases are shown by default; choose **All releases** to see the full history.
4. Select **Build firmware**. The Builder will compile the selected native ESP-IDF source.
5. Choose **Install via USB**, **Update over Wi-Fi (OTA)** or **Download firmware**. Options will only be enabled if the firmware and board pass their safety checks.
6. Open the RFLink web interface to learn and control supported wireless devices.

**Want to go back?** Select an older compatible release and rebuild or reuse a verified previous build. Downgrade safety and settings compatibility still require testing.

## Updating source code without replacing files one by one

The planned Builder compares a downloaded or uploaded source package with the existing project and shows what is **new**, **changed** or **unchanged**. It preserves unchanged files, keeps local overrides separate and offers a backup before replacing changed files. It must not silently delete files.

## What is stored where?

| Location | Information |
| --- | --- |
| Public GitHub repository | RFLink source, documentation, example/default pinout and SPI speed, published release history |
| Home Assistant Builder (local only) | Your optional GPIO and SPI speed overrides, Wi-Fi/MQTT/OTA secrets, build preferences and approved firmware builds |
| RFLink device | The configuration the firmware actually stores on your ESP32; backup and restore behavior still needs verification |

Never upload passwords, device-pairing secrets, private keys or personal diagnostic captures to a public repository.

## What can I do today?

Read the project status and follow development. The first trustworthy installation walkthrough will be published after the firmware has been compiled with the official SDK and tested on an ESP32-S3 with the three radios.

## Common terms

- **Firmware:** Software that runs on the ESP32.
- **OTA (Over-the-Air):** Updating firmware using the local network instead of a USB cable.
- **USB:** A wired connection that can be used to initially install or recover firmware.
- **BOM (Bill of Materials):** The list of parts needed to build a board.
- **SPI (Serial Peripheral Interface):** The data connection between the ESP32 and the radio modules.
- **GitHub Release:** A saved version with notes and, after validation, downloadable firmware.

## Continue reading

- [Current feature and test status](V0_7_STATUS.md)
- [Documentation overview](README.md)
- [PCB, assembly and BOM status](../hardware/README.md)
