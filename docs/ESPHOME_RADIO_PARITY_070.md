# v0.7 native CC1101 and RMT parity with Radio Gaga v0.6.6

## Scope and reference
The archived `reference/v0.6.6-esphome/rflink-universal.yaml` uses ESPHome 2026.9 CC1101, two independent 433 MHz RMT receivers, and 868.2999 MHz 2-FSK fixed packets. This change narrows **receiver compatibility** differences, not every ESPHome implementation detail. It is not proof of over-the-air decoding.

## 868 MHz modem parity (RX)
The source values were derived from ESPHome 2026.9 `cc1101.cpp`, `cc1101defs.h` and `__init__.py` defaults plus the archived YAML. The C++ `split_float` rounding is important: 203 kHz filter gives MDMCFG4 0x8A (not 0x9A), and 50 kHz FSK deviation gives DEVIATN 0x50 (not 0x40 or 0x47, effective ~50.78 kHz on 26 MHz XTAL).

| CC1101 register | Former native | v0.6.6-based receive profile |
| --- | --- | --- |
| IOCFG2 (0x00) | 0x06 | 0x0D |
| FIFOTHR (0x03) | chip reset | 0x0F |
| MDMCFG4 (0x10) | 0x9A | 0x8A |
| MDMCFG1 / MDMCFG0 (0x13/0x14) | 0x20 / 0x00 | 0x22 / 0xF8 |
| DEVIATN (0x15) | 0x40 | 0x50 |
| MCSM2 (0x16) | chip reset | 0x07 |
| MCSM0 (0x18) | 0x18 | 0x14 |
| FOCCFG / BSCFG (0x19/0x1A) | 0x16 / chip reset | 0x36 / 0x6C |
| AGCCTRL2/1/0 (0x1B/1C/1D) | 0x43 / 0x40 / 0x91 | 0xC7 / 0x00 / 0xB1 |

**Deliberately preserved native policy:** IOCFG0=0x06 because the native driver polls the FIFO rather than using ESPHome GDO0=0x01 interrupt. MCSM1=0x3C retains native `RXOFF_MODE=RX` (bits 3:2) instead of ESPHome's IDLE (0x30), while also applying ESPHome's `CCA_MODE=3` (bits 5:4). Changing RXOFF_MODE to IDLE without rearming after each packet would stop the existing native receiver. This also retains `enter_rx_verified`, PLL checks, CHIP_RDYn/CS guards, isolated recovery and FIFO overflow diagnosis. This is **not full 868 driver parity**: GDO0 IRQ and after-packet flushing are intentionally not introduced here. Since the same modem profile is used for a native 868 packet TX bench preset, validate TX separately before enabling regular transmit.

## 433 MHz pulse-frame parity
Keep both RMT channels at 96 hardware symbols, non-DMA, 512-symbol host frames, three immutable capture slots, immediate ISR rearm, 30 us software filter and 5 ms idle time. ESPHome's `decode_rmt_` also appends an explicit 5 ms idle-level termination to a nonempty pulse train. After filtering a **complete** frame with at least two retained signal pulses, native now appends this synthetic termination in the existing bounded receiver-owned scratch buffer. The appended sign is the opposite of the *last raw RMT level*, matching the reference's `idle_level=!last_level`. It is not an extra RF measurement. Short/full/noise captures remain rejected; raw completion ownership, queue-drop policy, RMT driver errors, no allocations in ISR and PSRAM logging isolation are unchanged.

The native software merging of short/same-level pulses is still an independent implementation and the native decoder/device catalog is not yet feature-equivalent to v0.6.6. This patch does not claim byte-for-byte ESPHome output for every glitch waveform.

## Bench acceptance criteria
1. Backup logs; confirm build, board pinout, all three radios present, 0 RMT rearm errors, and OTA safe.
2. Compare v0.6.6 vs this native firmware with a **known good 868.30 MHz Itho/Ramses source**, same antenna and distance: compare 63-byte FIFO samples (privately), preamble hits, decoded Itho/Ramses count, SPI faults, FIFO overflows, and recovery. Unknown-prefix packets must not be called decoded messages.
3. Compare 10 deliberate presses from a known 433.92 MHz transmitter, then 433.42 MHz if a valid source exists. Compare full captures, filtered, forwarded and raw pulse lengths (a synthetic idle tail should be present on forwarded frames), and queue drops.
4. Verify continuous 868 packet polling under noise, 433 concurrent RX, RX→TX→RX, OTA reboot and retained crash diagnostics. If any RX/TX regression appears, use the proven v0.6.6 fallback; do not call this production-safe on CI success alone.
