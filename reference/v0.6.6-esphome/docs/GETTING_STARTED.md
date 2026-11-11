# Getting started — read before flashing

## Intended audience

A curious hobbyist who can safely wire an ESP32, use ESPHome and check a PCB pinout. Familiarity with power supply, RF modules and basic electrical safety is necessary. Beginners should avoid experimenting with TX until RX and the actual module setup have been tested.

## Hardware overview

The supplied default YAML targets an ESP32-S3 N16R8 profile and three CC1101 modules. It configures approximately 433.42 MHz, 433.92 MHz and 868.30 MHz; **these values are from the supplied firmware**, not universal settings or an assurance that an antenna, board or jurisdiction permits a given configuration. The PCB design files and enclosure are not present in this source ZIP; do not manufacture an assumed board from the software pinout.

## Installation

1. Download or extract a **source/release bundle from a trusted source** to a short, new directory. Never mix scripts and firmware files from different release snapshots.
2. Open `rflink-universal.yaml`. Review the `substitutions` block, board revision, SPI and radio GPIOs, Itho virtual identity and enabled packages. Use the actual board for comparison.
3. Use `rflink_universal/secrets.example.yaml` as a **list of keys only**. Put your real secrets in your existing ESPHome `secrets.yaml`, outside Git.
4. On Windows, `MAAK_RELEASE.bat` creates a clean package and `UPDATE_RFLINK.bat` processes personalized configurations. Review `INSTALLEREN_EN_UPDATEN.md` and read update reports. The originals must remain untouched.
5. In ESPHome, validate and compile first. Do not flash if validation, compilation, hardware checks or the update report fails.
6. After a verified boot, browse to `http://<device-ip>/` (it redirects to `/rflink`) from a **trusted LAN**. The interface is not yet password-protected.
7. Check Diagnostics and the radio statuses. Missing radio modules must not prevent access to the web interface, but cannot receive or transmit.
8. Learn and approve devices one at a time. RF reception alone does not authorize publication or transmission.

## Optional Home Assistant

The native ESPHome API handles the board itself. For independently learned RF devices, enable the optional MQTT package only after configuring a reachable broker and its credentials. Review and approve each device/capability; TX remains separately gated. Read `INSTELLINGEN.md`, `TX1_BEDIENING.md`, and `RELEASE_NOTES.md`.

## Updating

Use the updater for an existing YAML rather than replacing it with the default configuration. Check the selected baseline, merge conflicts, GPIO changes, and the generated `WIJZIGINGEN.txt` report before compiling the output. Preserve OTA/API secrets and identities. If the updater refuses an unknown baseline, supply the exact original base YAML; **do not bypass the conflict guard**. Firmware changes are not performed by this GitHub documentation pack.
