# RAMSES1 – Standard RAMSES-II decoder for Orcon 15RF-class devices

Based on **RFLink_Universal_NAV4_QA1_DEVICECARDS1_FULL_SOURCE** as supplied 2026-10-26. No hardware/YAML/CC1101 register changes.

## Added

- Standard decoder pack `decoders/ramses_ii`: decoder.cpp, codec.cpp/h, manifest.json, README/attribution.
- Explicit receive-only `RamsesII` protocol identity and a separate, manually selectable Orcon MVS-15R / 15RF device profile (`profiles/orcon_mvs15r_15rf.json`).
- Both valid packets #653 and #661 (2026-10-26) decode to class:serial `29:204810`; #661 is Auto.
- Validated RAMSES messages outrank accidental Itho matching; unrelated packets remain ignored.
- The generic `manual_only` official profile compatibility marker skips automatic brand attribution (normal manual profile selection unchanged).
- Native firmware revision suffix `RAMSES1` for distinguishing source variants.
- Host build harness now compiles all decoder-pack C++ files instead of only `decoder.cpp` (required for reusable codec.cpp). Updated dynamic catalog/count/profile regressions.

## Results verified locally

PASS: standalone native C++ decoder/adapter tests; real packet log replay (2/690 valid); 13 decoder protocols / 9 packs schema validation; 11 device profiles; codegen + strong link anchors; standalone core C++ host-stub compilation; YAML pinout; HA MQTT configuration; existing firmware source remains otherwise unchanged.

NOT VERIFIED: complete ESPHome/ESP-IDF firmware build, real Orcon hardware RX of every button, TX/pairing, full test suite (timed out during large `-O2` host core compile), browser UI regression (timed out). Keep normal Itho hardware unchanged and TX disabled.

## Installation

Use the full source ZIP as an updated source snapshot, preserving your private local YAML/secrets. Review the diff then perform ESPHome Validate and Compile before any OTA flash. The included `maak_upload.py` can create a source ZIP and `tar.gz` but is **not** a precompiled firmware image.

## Known radio caveat

The current CC1101 868.30 MHz radio uses a fixed 63-byte Itho packet profile. It can capture some RAMSES frames but may miss others. Proper transparent-mode GDO2/UART capture is a separate firmware/radio preset task, not part of this safe receive-only release.
