# Somfy RTS 56-bit

Somfy RTS 56-bit receive decoder and native virtual-remote transmitter.

## Build model

This decoder pack is auto-discovered by `components/rflink_universal/__init__.py`.
Do not add it to a central registry or constants file.

## Identity and transmit state

The 24-bit address received from an existing handset is the RX identity. RFLink
never reuses that handset address for transmission. When the user pairs RFLink,
a separate 24-bit virtual remote address is created and stored together with its
rolling counter and pairing state.

The next rolling counter is committed to device persistence before a radio send
is queued, so a reboot cannot intentionally reuse an already attempted code.
`PROG` is used only by the explicit pairing flow; normal controls expose Up,
Down and My/Stop. The encoder is host-tested. Validation against a physical
Somfy RTS motor is still pending.

## Files

- `decoder.cpp` — receive protocol implementation
- `tx_encoder.cpp` — native 56-bit virtual-remote TX encoder
- `manifest.json` — human/build metadata
- `README.md` — this explanation
