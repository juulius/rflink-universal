# ESPHome protocol adapters

Adapters for Nexa, Byron SX, Dooya, KeeLoq and RC-Switch 1-8 using ESPHome protocol implementations.

## Build model

This decoder pack is auto-discovered by `components/rflink_universal/__init__.py`.
Do not add it to a central registry or constants file.

## Identity

Identity rules are protocol-specific; volatile command/hopping fields are excluded.

## Transmit status

- `tx_encoder.cpp` contains the registered, host-tested Nexa, Byron SX, exact-button Dooya and fixed-frame RC-Switch raw TX
  encoders. `manifest.json` declares exactly these four capabilities; sending
  requires a saved, individually approved RFLink device and a present 433.92 radio.
- KeeLoq remains receive-only here. RC-Switch requires the precisely captured code, protocol and bit length, then individual approval; it does not create inferred switches. Dooya TX only allows individually approved button_N with its own frozen check nibble; not an inferred open/stop/close mapping. Never claim that a
  receive decoder alone implements a safe encoder. Itho virtual packet TX is a
  separate ESPHome script, not this RAW decoder pack.

## Files

- `decoder.cpp` — receive adapters
- `tx_encoder.cpp` — reviewed protocol-specific TX adapters
- `manifest.json` — human/build metadata
- `README.md` — this explanation
