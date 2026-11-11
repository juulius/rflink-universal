# RFLink Universal – TXROUTER2 development milestone

## Functional changes
- Central, protocol-independent transport resolution from trusted TX manifests and current adapter availability.
- Preferred transport plus fallback handled on each real transmit request, not only when the decoder is registered.
- Device and radio binding rechecked before RF and state reservation.
- Existing RMT and SPI-FIFO support retained, with GDO serial intentionally still unavailable in normal firmware.
- Read-only user-decoder legacy adapter unchanged; RX untouched.
- Device-details refresh bug fixed centrally (open panels survive periodic device refresh).

## Not yet implemented or validated
- Physical GDO serial TX, generic adapter registration, Orcon on-air handshake: still OFF.
- Real ESPHome/ESP-IDF compile and hardware measurements: must be done on your hardware.
- This does not add an operational Orcon TX button. Do not bypass safety gates.
