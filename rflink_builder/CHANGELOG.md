# RFLink Universal Builder — Changes

## v0.6.1 (ESPHome-style device dashboard)

- Made Devices the homepage with compact cards, online/offline indicators, search and filters.
- Added discovered/unmanaged gateway cards and an Add device wizard with adoption and manual IP fallback.
- Added persistent management for names, saved devices and **independent OTA passwords** per RFLink, including explicit password visibility controls.
- Periodically rechecks known native RFLinks and preserves offline cards instead of deleting them.
- Shows newer **published releases** without falsely claiming that a compatible OTA binary exists.
- Current discovery is bounded local /24 scanning, not ESPHome mDNS; native firmware discovery and stable identity remain separate firmware tasks.
- **Home Assistant OS installation and hardware validation are not yet verified.**

## v0.6.0 (lightweight Home Assistant app + simplified interface)

- Replaced the large pre-provisioned ESP-IDF Docker base with a smaller Python/Debian app image. The web interface starts before the official ESP-IDF v5.5.6 toolchain is downloaded.
- Added persistent background ESP32-S3 toolchain setup, live installation console, clear failure status and retry control.
- Activated the downloaded toolchain for firmware builds and USB flashing only after setup verification.
- Reorganized the interface into Build, Versions & GitHub and Settings pages; kept existing advanced features without crowding the home screen.
- Selected `juulius/rflink-universal` automatically; changing repository is an Advanced action.
- Added a dedicated 128×128 Home Assistant icon and 250×100 logo with the approved RFLink artwork.
- **Home Assistant host install and hardware tests are still pending.**

## v0.5.1 (source migration candidate)

- Prepared a single GitHub repository for the Home Assistant app and native v0.7 firmware.
- Updated the bundled default source to Stage 20 native firmware.
- Retained the five-previous-releases selector, full release history, isolated version builds and local hardware/secret storage.

## v0.5.0

- GitHub Releases history with version-pinned builds, SHA-256 checks and board compatibility validation (prototype).
