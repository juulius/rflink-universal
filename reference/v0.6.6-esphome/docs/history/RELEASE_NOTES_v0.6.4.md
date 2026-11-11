# RFLink Universal v0.6.4 — Radio Gaga (RX/IRQ recovery candidate)

## Changed
- Keeps the v0.6.3 fix for invalid CC1101 DEVIATN (0x48 -> 0x40).
- On every completed/aborted GDO UART TX session: delete UART1 driver and release GPIO16; explicitly detach the stale packet IRQ, reset the CC1101 profile, restore the GDO0 input, re-arm the ESPHome CC1101 packet IRQ and re-enter calibrated packet RX.
- Adds host regression assertions for IRQ rearming after both a successful transmission and failed TX preparation.
- Does not modify the other two radio GPIO owners.

## Known limits — important
- Host tests simulate the GPIO ISR and ESPHome CC1101 driver. They cannot prove that real GDO pulses are delivered, and do not validate the RF waveform.
- ESPHome's `set_packet_mode` and `begin_rx` return void; software verification proves modem registers/state, not delivery of real packets.
- The ESPHome CC1101 API remains a shared owner of the 868-MHz SPI register state and IRQ. The current repair is feasible without replacing ESPHome immediately, but a central native radio manager is preferable long-term.
- Do not regard this as a production-ready pairing release until checked on an ESP32-S3 board with before/after RX events and actual RF TX.

## Hardware acceptance test
1. Reboot, verify all 3 radios and collect baseline RX/noise counters for each radio.
2. Perform a single 1FC9 pairing attempt, inspect stages including `ESPHome packet IRQ rearmed after UART release`.
3. Verify 868MHz RX events continue **after** the attempt, using a known 868MHz transmitter; mere RX-ready/green status is not enough.
4. Test another pairing attempt and failed TX preparation, recheck continuous RX and 433MHz reception.
5. Export a fresh diagnostic JSON before rebooting if problems recur.

Source build only: compiled ESPHome firmware and physical RF tests were not available in this environment.
