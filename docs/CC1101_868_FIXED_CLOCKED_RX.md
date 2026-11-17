# Fixed 868 MHz synchronous serial receive — staged implementation

**Status:** bounded bit capture implemented and host-regression tested in a
development PR. **Hardware connection and over-the-air reception are NOT yet
implemented or validated.** This is not a new firmware mode or a claim of
simultaneous Itho/RAMSES reception.

## What RFLink should eventually do

RFLink should listen continuously on its third CC1101 module at 868.30 MHz,
with **one** validated 2-FSK demodulator configuration. Its synchronous data
and clock outputs will feed a bounded bitstream receiver on the ESP32-S3.
The bitstream will then be examined independently by compatible Itho,
RAMSES-II and user protocol decoders. A decoder must never reconfigure the
CC1101 or stop another decoder.

There is **no automatic FIFO-versus-UART time-sharing** in this design.
Once validated, the synchronous bitstream receiver can replace the active
868-MHz FIFO implementation in a future Experimental firmware revision.
The previous fixed-63-byte FIFO firmware remains the tested rollback
baseline, not a second concurrent listening mode.

433.42 MHz and 433.92 MHz stay on their own independent OOK/GDO2/RMT
receivers; no 433 RF changes are part of this increment.

## Current code in this development PR

`rflink_clocked_bit_capture.hpp/.cpp` provides a pure, platform-independent
clocked-bit buffer suitable for feeding from a future HAL adapter:

- MSB-first packed bits, with an explicit valid-bit count. It does not invent
  UART start/stop bits, CC1101 FIFO bytes, sync words or packet boundaries.
- Two immutable completed blocks of 2,048 bits each, plus a separate
  2,048-bit working buffer. No heap, shared log writes or radio register calls.
- Drop-newest on queue saturation: older evidence is not overwritten; dropped
  bit/block counters are explicit. The next valid block is labelled as
  discontinuous and **must never be stitched into one purported good frame**.
- Explicit acquisition epochs. Previous data is discarded on a new RX lease
  (after boot, TX, failed RX restart or firmware-controlled profile change).
  Stale producer samples are rejected.
- Missing-edge and timestamp-regression reporting, preserving the fact that
  any subsequent decoder attempt is incomplete.
- `end_window()` publishes a partial evidence block only when the *actual*
  transport establishes a capture boundary. A full 2,048-bit block does not
  imply a valid frame and a long inter-bit gap alone does not prove lost bits.
- Single serialized execution context only. **Do not call this C++ object
  directly from a GPIO ISR while also popping on an RX task.** A separate,
  proven ISR-to-task handoff is mandatory.

The existing packet receiver, GPIO pin configuration, SPI, UART TX, RMT,
RadioManager and default RF profiles are unchanged.

## Proposed physical interface — requires bench confirmation

TI CC1101 Table 41 lists:

| CC1101 output selection | Register candidate | ESP32-S3 reference GPIO |
|---|---|---|
| Synchronous demodulated RX data | `IOCFG0=0x0C` | **GDO0, GPIO16** |
| Synchronous serial RX clock | `IOCFG2=0x0B` | **GDO2, GPIO17** |

With non-inverted GDO outputs, data is set up by CC1101 on the falling
clock edge; a candidate receiver samples on the rising edge. **Verify the
actual edges and signal levels with a scope/logic analyzer** before using
an interrupt or asserting valid decoding.

CC1101 synchronous serial receive uses `PKTCTRL0.PKT_FORMAT=01` (candidate
base value `0x10`), with 2-FSK demodulation and intentionally configured
symbol rate, channel bandwidth, deviation, sync and GDO functions. Do not
copy a register subset from the old RAMSES 38,400-baud asynchronous UART
lab without validating Itho receive parameters. The old FIFO Itho profile
uses a different packet-format field (`PKTCTRL0=0x00`).

The existing reference board has GDO0 and GDO2 wired, but the Builder may
generate a different pin profile. Validate the **actual compiled profile**
and PCB. Do not silently change GPIOs in a firmware update.

**GDO0 is also used for 868-MHz UART TX.** The future manager-owned serial
RX backend must stop acquisition, detach its clock/data pin handlers, enter
safe IDLE, perform the existing approved TX handover, then reinitialize and
verify this **same synchronous RX** profile. It must not silently resume the
old FIFO receiver or leave the CC1101 driving GDO0 against the ESP32.

## Why not ESP32 UART1 for this stream?

UART1 at 38,400 8N1 turns ten on-air bits into one byte only when the radio
protocol genuinely uses 8N1 formatting. RAMSES-II can use this; Itho and
unknown devices are not guaranteed to. Raw synchronous **data+clock** leaves
the framing decision to independent software decoders.

However, the CC1101 still demodulates at one selected carrier/modulation/
data-rate/bandwidth/deviation combination. Synchronous serial output cannot
turn a CC1101 into an SDR, and it cannot receive incompatible RF profiles
simultaneously. The critical remaining radio test is whether one fixed
2-FSK configuration reliably captures **both real Itho and real RAMSES**.

## Hardware receive adapter: implementation gates

1. **Scope the GDO pins without changing main firmware.** Verify the reference
   GPIO16/GPIO17 signal directions, data/clock polarity and whether the
   selected 2-FSK modem outputs useful symbols for Itho and RAMSES.
2. **Choose an actual capture peripheral based on evidence.** At about
   38.4 ksymbols/s, a GPIO ISR per bit is a high-frequency interrupt source;
   Wi-Fi, OTA, two active RMT receivers and FreeRTOS may make it unreliable.
   Assess hardware-clocked alternatives and buffer loss on ESP32-S3 first.
   An ordinary 8N1 UART is **not** an equivalent generic bit sampler.
3. **Implement a RadioManager-owned, receive-only backend** behind a
   *build-time Experimental flag*, default off. No direct UI or decoder
   pin/register ownership. One permanent RX profile, no automatic time share.
4. **Build the decoder-side bitstream reassembler.** Continuous chunks are
   not packets. Carry arbitrary bit offset and epoch through the RAMSES-II
   bit-packed decoder and an Itho bit-alignment/preamble search. Reject
   overlapping or checksum-invalid candidates; never publish a raw/noise
   capture as a recognized device automatically.
5. **Validate real transmissions and keep an A/B firmware rollback**. Before
   switching the default, test real Itho and Orcon Auto/High/Low messages,
   noise for 24 hours, no lost edges during Wi-Fi/OTA/CPU load, correct
   TX-to-the-same-RX restore, three independent radio indicators, no 433
   regression, bounded PSRAM/logging, and forced power/radio failures.

If the single fixed RF profile cannot receive both protocol families, do
not silently time-share: explain the physical conflict and revisit the
frequency/rate/modulation assumptions or a separate 868 CC1101.

## Host test evidence and boundaries

`firmware/tests/test_clocked_bit_capture.cpp` feeds synthetic bit trains
to the real pure C++ buffer:

- Native RAMSES-II TX codec produces 8N1 *on-air* bits, which are passed
  unchanged to the existing `decodePacked` receiver.
- Native Itho encoder produces 63 bytes of test packet payload; its bits are
  streamed with a deliberately non-byte-aligned prefix and then realigned
  in the host test before `itho::decode_packet`.
- Buffer saturation, retention of earlier evidence, missing edges,
  epoch reset, stale-sample rejection and clock regression are tested.

These tests prove **bit packing and software codec inputs only**.
They cannot prove that a real CC1101 exposes the same physical bitstream,
that an Itho message is 63 bytes in synchronous mode, or that the ESP32-S3
can capture all clock edges under live workload.

Reference: Texas Instruments CC1101 datasheet Rev. I, synchronous serial
operation and GDO output selection (Table 41):
https://www.ti.com/lit/ds/symlink/cc1101.pdf
