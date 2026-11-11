# v0.6.6 Radio Gaga — release fingerprint fix (test candidate)

- Fixed a mismatch between the versioned source fingerprint and the actual runtime source in the distributed v0.6.5 archive.
- Preserved the v0.6.5 pairing timeout correction and shared RX/IRQ recovery paths without changing their logic.
- New version v0.6.6, still in the Radio Gaga series.
- Release fingerprint: `b9038c00af2f` (SHA-256, shortened).

**Important:** source-only test candidate. Not compiled with ESPHome or tested on ESP32-S3 hardware.
Run `00_RFLink_START.bat` and use the normal/default release mode.
