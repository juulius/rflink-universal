# RFLink Universal v0.7 Radioactive — implementation status

This page distinguishes **code present**, **host-tested**, **built with official ESP-IDF**, and **verified on hardware**. Native v0.7 remains a development prototype and is **not** verified for production hardware.

| Area | v0.7 state |
| --- | --- |
| Native ESP-IDF project | **Official ESP-IDF v5.5.6 build passed** in GitHub Actions [run #10](https://github.com/juulius/rflink-universal/actions/runs/42224090381) on 2026-11-06; no hardware test yet |
| Three CC1101 radios / shared SPI | Code and host checks present; hardware verification pending |
| RX 433.42 and 433.92 MHz via RMT | Experimental implementation; hardware tests pending |
| RX 868.30 MHz packet FIFO | Experimental implementation; hardware tests pending |
| Universal UART, RMT, FIFO transmit | Source present; RF on-air verification pending |
| Itho and RAMSES-II decoding | Native code and host tests; end-to-end verification pending |
| Web status and firmware upload | Prototype; security and hardware tests pending |
| OTA A/B rollback | Code present; end-to-end update and rollback tests pending |
| MQTT client / hub discovery | Code present; broker integration and hardware tests pending |
| Native device registry, pairing and controls | Not completely migrated |
| Home Assistant Builder | v0.6.9 targets measured initial discovery RAM spike (33 MB → 466 MB Python RSS; later 880 MB idle) using 12 full-scan / 4 health-check workers, per-worker HTTP opener reuse and glibc arena cap. All /24 network and VLAN discovery coverage preserved, tests added; HA OS memory reduction and physical device testing pending; GHCR publication remains disabled |
| GitHub release selector | Builder code present; release and install flow not yet validated |

## Architectural contract

**RFLink**, rather than ESPHome, exclusively owns all CC1101 SPI and GPIO access, RMT/UART peripherals and radio profile state transitions.

- Three radios use a shared SPI bus and separate chip-select and GDO pins.
- The decoder specifies RF profile and TX/RX transport, not hardware pin manipulation.
- The radio manager tracks module presence independently of its active receive or transmit mode.
- Personal pinout and SPI speed overrides belong to the Builder's persistent local settings; GitHub contains only defaults.
- Source versions and binaries are managed through versioned GitHub Releases. Compiler success and hardware validation must be tracked separately.

## Documentation requirements before first use

The full compiler build has passed. Before first device use, write **hardware-tested** guides for initial USB installation, network setup, website OTA updates, recovery, radio diagnosis, MQTT setup and rollback. Do not represent the previous ESPHome v0.6 procedures as valid for v0.7.
