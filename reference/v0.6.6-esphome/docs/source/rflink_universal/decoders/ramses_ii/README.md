# RAMSES-II (Orcon 15RF) decoder pack

This is a **standard RFLink Universal schema-2 decoder pack**: `decoder.cpp`, `manifest.json`, reusable `codec.h/.cpp` and host regression tests. The Orcon MVS-15R/15RF is a **separate device profile**, not a separate radio driver. That profile is `manual_only`, because RAMSES-II frames alone cannot prove the vendor.

## Verified scope

* RX-only on the existing 868.30 MHz 63-byte CC1101 packet path.
* Captured October 26 samples #653 (31D9 mode 0) and #661 (31D9 mode 4) decode with RAMSES checksum, Manchester and UART framing.
* Invalid captures and previous-day noise are not accepted as RAMSES.
* The `31D9` and `22F1` known mode fields are interpreted; other commands are classified but raw payload is left uninterpreted until confirmed.
* Address class:serial format is stable within this logical decoder; RAMSES-II itself is shared by other vendors.

## Not verified / deliberately not enabled

* Full physical Orcon receive on all button actions, pairing, TX, device status from MVS-15R, and Home Assistant live operation.
* The existing 868.30 MHz CC1101 is configured for Itho fixed-length packet mode. Some RAMSES packets may never be delivered. True RAMSES async/GDO2 UART receive requires a separately audited radio preset and scheduling with Itho.
* We have not changed GPIOs, radio register presets, YAML, or device TX authorizations.

## Technical

The pack is auto-discovered by `components/rflink_universal/__init__.py`, validated and included in the build as an enabled decoder. It is regular native C++, not a runtime User-Decoder VM recipe. `tools/promote_user_library.py` handles VM recipes; this pack is a reviewed native decoder with the same manifest/DecodeResult interface.

Tests: `tests/test_ramses_ii.cpp` via the normal host `tests/run_tests.sh` suite.

Source inspiration: https://github.com/me-processware/orcon-15rf-controller (MIT, see CREDITS) and community observations from https://github.com/peeter123/orcon-15rf-protocol-decoder (GPL-3.0; no code copied). Original RFLink decoder content adapted from the prior user-owned C++ prototype.

## ORCONTXLAB2 – experimentele fysieke TX en koppeling (opt-in)

`tx_codec.h/.cpp` verzorgen de RAMSES-II-telegrammen, `tx_encoder.cpp` registreert
acht acties bij de universele TX-engine en de centrale RFLink-core beheert
voorlopig een conservatieve, device-specifieke 1FC9-koppelsessie. Deze werkt
uitsluitend na expliciete YAML-labvlaggen, een nieuwe virtuele class-37-ID,
een passend ACK, een aparte Auto-test en bevestiging door de eigenaar. De
CC1101/GDO-UART-driver bevat geen Orcon-commando's. Fysieke koppeling is
**nog niet bevestigd** op de ESP32-S3/CC1101-board. Zie
`ORCONTXLAB2_RELEASE_NL.md` in de SOURCE-root en de bijbehorende softwaretests.
