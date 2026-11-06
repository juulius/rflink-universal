# RFLink Universal v0.7 Radioactive — implementation status

This page distinguishes **code present**, **locally checked** and **verified on hardware**. As of the current development source, none of the native firmware has been verified as a production-ready ESP32-S3 firmware.

| Area | v0.7 state |
| --- | --- |
| Native ESP-IDF project | Source code present; complete official build pending |
| Three CC1101 radios / shared SPI | Code and host checks present; hardware verification pending |
| RX 433.42 and 433.92 MHz via RMT | Experimental implementation; hardware tests pending |
| RX 868.30 MHz packet FIFO | Experimental implementation; hardware tests pending |
| Universal UART, RMT, FIFO transmit | Source present; RF on-air verification pending |
| Itho and RAMSES-II decoding | Native code and host tests; end-to-end verification pending |
| Web status and firmware upload | Prototype; security and hardware tests pending |
| OTA A/B rollback | Code present; end-to-end update and rollback tests pending |
| MQTT client / hub discovery | Code present; broker integration and hardware tests pending |
| Native device registry, pairing and controls | Not completely migrated |
| Home Assistant Builder | Prototype source, app installation and official compile not yet proven |
| GitHub release selector | Builder code present; release and install flow not yet validated |

## Architectural contract

**RFLink**, rather than ESPHome, exclusively owns all CC1101 SPI and GPIO access, RMT/UART peripherals and radio profile state transitions.

- Three radios use a shared SPI bus and separate chip-select and GDO pins.
- The decoder specifies RF profile and TX/RX transport, not hardware pin manipulation.
- The radio manager tracks module presence independently of its active receive or transmit mode.
- Personal pinout and SPI speed overrides belong to the Builder's persistent local settings; GitHub contains only defaults.
- Source versions and binaries are managed through versioned GitHub Releases. Compiler success and hardware validation must be tracked separately.

## Documentation requirements before first use

Once the complete firmware builds, write separate, tested guides for initial USB installation, network setup, website OTA updates, recovery, radio diagnosis, MQTT setup and rollback. Do not represent the previous ESPHome v0.6 procedures as valid for v0.7.
