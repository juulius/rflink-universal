# v0.6.5 Radio Gaga — pairing timeout & RX recovery (test candidate)

- Fixes premature RAMSES pairing timeout: RF transmission can advance `millis()` beyond the start-of-loop timestamp; no more unsigned underflow at WAIT_ACK entry. The 180-second timer remains wrap-safe.
- Consolidates CC1101 packet-mode, GDO0 interrupt re-arm and calibrated RX return in one helper used by GDO UART TX, RAMSES UART RX and RAMSES SPI RX.
- Releases the temporary RAMSES UART RX GDO2 GPIO route when stopping UART.
- Includes radio mode manager error reason code in the system trace on state changes, particularly FAULT.
- Preserves fail-closed behavior: unexpected register/state mismatch still marks the radio unavailable.

**Not yet hardware validated:** compile with ESPHome, check known 868 MHz RX events before pairing, use the pairing button once without an ACK, verify that WAIT_ACK stays active (no immediate timeout), wait for the intended expiry or test RX lease return, and verify real RF reception afterwards. Do not assume RF TX from UART enqueue alone.

Source-only candidate. Do not deploy as production firmware without ESPHome compilation and board tests.
