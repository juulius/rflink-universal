# Itho RFT

Itho ventilation remote protocol on 868.3 MHz 2-FSK packet mode.

## Build model

This decoder pack is auto-discovered by `components/rflink_universal/__init__.py`.
Do not add it to a central registry or constants file.

## Identity

3-byte source ID is stable identity; message counter is transient.

## Files

- `decoder.cpp` — protocol implementation
- `manifest.json` — human/build metadata
- `README.md` — this explanation
