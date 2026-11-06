# RFLink Universal

Independent RF gateway for ESP32-S3 with three CC1101 radios (433.42, 433.92 and 868.30 MHz).

> **Work in progress — not ready to flash or install.**
> The Home Assistant Builder and native ESP-IDF Radioactive firmware are under development. A complete ESP-IDF build, on-device RF tests and OTA/rollback validation are still outstanding. Keep the existing v0.6.6 Radio Gaga firmware as a working reference.

## Project structure (planned)

- `rflink_builder/` — Home Assistant app for firmware builds, GitHub Releases, OTA and USB flashing.
- `firmware/` — native ESP-IDF firmware, universal CC1101 RX/TX manager, decoders, local web interface, MQTT.
- `docs/` — current documentation and migration notes.
- `docs/legacy-v0.6/` — preserved v0.6 Radio Gaga documentation (import pending provenance/publication review).
- `.github/workflows/` — automated build, tests and release checks.

**This repository currently contains the initial documentation scaffold only.** It is not yet a Home Assistant app repository.

## Design priorities

1. Hardware access (SPI, GPIO, RMT and UART) is exclusively managed by RFLink's own radio manager, not ESPHome.
2. RF profiles/decoders define the modulation, encoding and timing; drivers remain protocol independent.
3. Home Assistant provides a convenient build-and-update interface, not a firmware runtime dependency.
4. GitHub Releases keep versioned sources and validated binaries separate from ongoing development.
5. Personal pinouts, SPI overrides and secrets are stored locally in the Home Assistant Builder, never committed.

## Previous release

The ESPHome-based **v0.6.6 Radio Gaga** reference package includes extensive documentation, decoders, profiles and diagnostics. The original documentation will be preserved under `docs/legacy-v0.6/` after publication review, rather than silently replaced by the new native implementation.

See [documentation index](docs/README.md), [legacy import notes](docs/legacy-v0.6/README.md) and [publication checklist](docs/PUBLICATION_CHECKLIST.md).

## Publication and license status

The 0.6.6 source package explicitly marks license selection, third-party code provenance and rights review as incomplete. No project-wide open-source license has been selected yet. Do not interpret public visibility as permission to redistribute third-party components.

Created and directed by **Juulius**, with AI-assisted software, testing and documentation. This is an independent project, not an official Espressif, ESPHome, Home Assistant or OpenAI product.
