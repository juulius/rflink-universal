# Protocol sources used for v0.3.2-RC3

The release deliberately distinguishes verified/documented paths from experimental ones.

## ESPHome native protocol adapters

Current ESPHome Remote Base source/API documentation:

- Nexa: https://api-docs.esphome.io/nexa__protocol_8h_source
- Byron SX: https://api-docs.esphome.io/byronsx__protocol_8h_source
- Dooya: https://api-docs.esphome.io/dooya__protocol_8h_source
- KeeLoq/HCS: https://api-docs.esphome.io/keeloq__protocol_8h_source
- RC-Switch: https://api-docs.esphome.io/rc__switch__protocol_8h_source
- RemoteReceiveData API: https://api-docs.esphome.io/remote__base_8h_source

## CC1101 / ESPHome

- https://esphome.io/components/cc1101/

Used for async-vs-packet mode behavior, GDO requirements, modulation configuration and packet metadata (RSSI/LQI/frequency offset).

## Alecto V1

rtl_433 device implementation:

- https://github.com/merbanan/rtl_433/blob/master/src/devices/alecto.c

RC3 includes the checksum-backed temperature/humidity and rain paths. The wind path is omitted because that source labels it untested.

## Somfy RTS

Cross-checked against public RTS protocol descriptions and implementations for the 56-bit frame layout, nibble XOR checksum, byte-wise obfuscation, Manchester timing, 24-bit address and 16-bit rolling code. RC3 only advertises RX; TX/pairing remain blocked until rolling-code persistence and a physical motor are tested.

## Itho RFT

Cross-checked against the public ESPHome/CC1101 Itho RFT implementation and packet codec used with CVE-S ECO-class systems. RC3 preserves the documented packet profile and repeat timing for normal commands, join variants and leave.

## RADIOMON Decoder Expansion 1 — rtl_433 cross-check

- Acurite 609TXC: https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c
- Rubicson 36-bit: https://github.com/merbanan/rtl_433/blob/master/src/devices/rubicson.c
- Fine Offset WH2: https://github.com/merbanan/rtl_433/blob/master/src/devices/fineoffset.c

See DECODER_EXPANSION.md in the source ZIP for limitations, offline regression methodology and hardware verification status.
