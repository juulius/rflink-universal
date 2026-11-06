# RFLink Universal

Independent RF gateway for an ESP32-S3 with three CC1101 radios (433.42, 433.92 and 868.30 MHz).

**Nederlands:** [Start hier — eenvoudige uitleg](docs/START_HIER.md) · [Documentatie](docs/README.md) · [Hardware en onderdelenlijst](hardware/README.md).

> **Development status — v0.7.0 Radioactive is not yet validated for flashing.**
> The current native ESP-IDF firmware and Home Assistant Builder are under development. An official full ESP-IDF build and hardware tests of RX, TX and OTA are still required.

## Scope of this repository

This repository describes **the current v0.7 Radioactive architecture** and its Home Assistant Builder. Documentation from earlier firmware is not imported unless the behavior is implemented or explicitly retained in v0.7.

Planned directories:

- `firmware/` — native ESP-IDF RFLink firmware, CC1101 radio manager, RX/TX, decoders, WiFi, MQTT and OTA.
- `rflink_builder/` — Home Assistant app for source updates, builds, GitHub Releases, OTA and USB installation.
- `docs/` — current v0.7 user and technical documentation.
- `hardware/` — future KiCad board design, verified BOM and manufacturing files.
- `.github/workflows/` — build checks.

**This repository presently contains a documentation scaffold only. It cannot yet be installed as a Home Assistant app.**

## Architecture

RFLink exclusively manages CC1101 SPI, GDO, RMT, UART and all RX/TX changes. Protocol decoders provide RF profiles and encoding/decoding; they do not own radio hardware. ESPHome is not included in the Radioactive firmware.

The Home Assistant Builder keeps local board overrides (GPIO/SPI), WiFi/MQTT settings and secrets outside GitHub. Default pinouts and default SPI speed may be versioned publicly.

GitHub Releases will contain versioned source and, **only after passing official builds and hardware verification**, firmware binaries. A failed or unverified build must never be labeled stable.

See [current documentation](docs/README.md), [implementation status](docs/V0_7_STATUS.md), and [publication checklist](docs/PUBLICATION_CHECKLIST.md).

## v0.6 reference

Previous versions such as **v0.6.6 Radio Gaga** are used privately for technical comparison during migration. Obsolete v0.6 documentation is not part of this repository; every user-facing instruction must describe the actual v0.7 behavior.

## Licensing

Project-wide licensing and attribution review remain open, especially for migrated protocol code. A public repository does not automatically grant open-source reuse rights. Do not publish third-party source before its terms and provenance are reviewed.
