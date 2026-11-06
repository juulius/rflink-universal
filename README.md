# RFLink Universal

A do-it-yourself, **universal RF gateway** built around an ESP32-S3 and three CC1101 radio modules (433.42 MHz, 433.92 MHz and 868.30 MHz).

**New here?** Start with the [beginner-friendly guide](docs/GETTING_STARTED.md). You do not need programming or electronics experience to understand how the finished system is intended to work.

> **Development status:** v0.7.0 *Radioactive* is not yet ready to flash. The full official ESP-IDF build and actual hardware tests of RF reception, transmission and OTA updates are still pending.

## What will RFLink do?

RFLink is designed to learn, recognize and control compatible wireless devices using its own web interface. The same universal radio manager handles RX (receiving) and TX (transmitting); protocol profiles provide the specific instructions for individual device types.

The finished Home Assistant Builder is intended to let you upload or update source code without replacing files one by one, compile a selected GitHub Release, and install via USB or local Wi-Fi (OTA). Home Assistant is for building and integration; RFLink must keep operating by itself.

These are **project goals**, not a claim that every function already works.

## Find your way

- [Getting started — simple explanation](docs/GETTING_STARTED.md)
- [Documentation index](docs/README.md)
- [Current implementation and validation status](docs/V0_7_STATUS.md)
- [PCB design, BOM and manufacturing files — planned](hardware/README.md)
- [Project rules for developers and AI agents](AGENTS.md)
- [Using the rules in future AI chats](docs/AI_WORKFLOW.md)
- [Public publication and security checklist](docs/PUBLICATION_CHECKLIST.md)

## Repository organization

- `firmware/` — planned location of the native ESP-IDF firmware source.
- `rflink_builder/` — planned location of the Home Assistant firmware builder app.
- `docs/` — current v0.7 documentation, organized for beginners and advanced users.
- `hardware/` — placeholders for verified KiCad files, BOM and manufacturing outputs.
- `.github/` — GitHub automation and repository instructions.

**The repository currently contains the documentation scaffold.** The firmware and Builder source have not yet been published here, and the Home Assistant app cannot yet be installed from this repository.

## Key architectural choices

- Native **ESP-IDF**, with **no ESPHome runtime** in the RFLink firmware.
- One protocol-independent CC1101 RX/TX driver for all three radio modules.
- The radio manager exclusively owns SPI, GDO, UART, RMT, profile changes and RX recovery.
- Default pinout and default SPI speed are versioned in GitHub; personal overrides and secrets stay in local Builder storage.
- GitHub Releases track firmware versions separately from Builder updates.
- A build is not called hardware-tested unless it was actually tested on the target board.

## History and publication

The older ESPHome-based v0.6.6 *Radio Gaga* firmware is a technical comparison reference. Obsolete v0.6 user instructions will **not** be added to the current v0.7 documentation.

Project licensing and third-party decoder provenance require review before publishing all migrated source code. Public visibility does not itself grant reuse rights.
