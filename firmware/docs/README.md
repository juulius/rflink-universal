# RFLink Universal v0.7 — Native firmware documentation

This is the current **Radioactive v0.7** development source, not a production firmware release.

- Read the root repository [Getting Started](../../docs/GETTING_STARTED.md) guide for plain-English explanations.
- Read [implementation status](../../docs/V0_7_STATUS.md) for verified versus experimental functions.
- This firmware uses native ESP-IDF, not an ESPHome runtime. RFLink owns the SPI/GDO/RMT/UART hardware.
- Current RX and TX transports are experimental until they have passed hardware tests.
- The previous ESPHome v0.6 instructions must not be used to install this firmware.

## Build

RFLink Builder within Home Assistant is the intended build and deployment interface. An independent ESP-IDF build is also possible using the firmware CMake project and documented SDK version. No firmware binary has yet been validated for general flashing.

## Safety

Only flash a separate test ESP32-S3 N16R8 board after an official ESP-IDF build succeeds, board/partition identity is checked, and you have a recovery method. MQTT configuration and OTA transfer use HTTP on a trusted LAN in this prototype and must not be exposed to the internet.
