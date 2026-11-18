# 868 MHz fixed synchronous RX — manager-controlled bench route

**Available code:** Manager-owned, build-time opt-in proof-of-feasibility RX
transport, with CC1101 profile readback, GDO0 DATA + GDO2 CLOCK sampling and
bounded bit/drop counters. **Not hardware tested or supported in ordinary
firmware.** This increment does not activate any new decoder, device or
everyday web setting. No OTA image has been physically validated.

## What changes for an ordinary user?

**Nothing.** The default native firmware, Home Assistant Builder and
Stable/Early adopter/Experimental releases continue to start the existing
868.30 MHz **63-byte FIFO receiver**, plus independent 433.42 and
433.92 MHz OOK/RMT receivers. The optional synchronous mode does **not**
switch between FIFO and UART. It is selected only at compile time for
a separate local laboratory firmware build.

## What this implementation actually includes

- `RfProfileId::sync_868_bench` is a receive-only 2-FSK candidate profile.
  It is separate from the existing `itho_868_packet` (63-byte FIFO) and
  `ramses_868_packet` (software/TX reference) profiles.
- `RadioManager` recognizes `RxTransport::gdo_sync`, but rejects it unless
  its hardware transport explicitly opts in. Start, stop, loss handling and
  TX->RX restoration use the same transport route as the selected profile.
- `Cc1101Sync868BenchRx` exclusively configures the third CC1101 under
  the manager lease. It makes GDO pins safe ESP32 inputs, requests IDLE,
  writes and reads back the selected frequency + modem registers, arms the
  separate GPIO DATA/CLOCK ISR sampler, then verifies actual CC1101 RX/PLL.
  Any initialization error stops sampling and attempts IDLE; manager must
  report FAULT rather than claim the FIFO receiver resumed.
- The receive task drains captured edges into bounded 2,048-bit blocks
  without performing decoding, and counts gaps and dropped samples. This
  adds a periodic bench-only numerical console summary (30 s), never
  raw private RF payloads or fake protocol-recognition results.
- A new explicit capture epoch begins after every RX start (including
  a completed TX handover). Events from different epochs cannot be joined.
- A RAMSES UART transmit request, when supported, pauses the synchronous
  capture and restores **that same synchronous RX profile**. It does not
  start FIFO in the background or after a failure.
- The normal firmware does not instantiate the sampler, so there is no
  additional 32 KB clock-edge ring allocation or extra GPIO ISR at runtime.

The existing CC1101 FIFO driver, 433 receivers, API, web UI, OTA and
built-in decoder code remain unchanged in standard builds.

## Candidate modem settings — still not a verified universal profile

The test profile is based on the existing 868-MHz Itho radio parameters
(`868299900 Hz`, 2-FSK, nominal ~38.4 ksymbols/s, 203-kHz bandwidth,
nominal ~50-kHz deviation) with:
- `IOCFG0 (0x02)=0x0C` for GDO0 DATA;
- `IOCFG2 (0x00)=0x0B` for GDO2 CLOCK;
- `PKTCTRL0 (0x08)=0x10` for synchronous serial data;
- `MDMCFG2 (0x12)=0x00` to disable hardware sync gating for raw analysis.

These are **development hypotheses**. Whether one such fixed configuration
properly receives BOTH real Itho and RAMSES-II signals is not yet known.
The `Cc1101SyncGdoSampler` currently samples on the *rising* clock edge
as the candidate for non-inverted outputs. Verify polarity and phase using
a logic analyzer before treating any captured bits as on-air evidence.

**Important:** A CC1101 synchronous bitstream is still the output of
one configured RF demodulator. It cannot simultaneously demodulate
incompatible modulation, deviation, bit rate or frequency profiles.

## How to enable a separate lab build

The sole opt-in is the CMake variable `RFLINK_ENABLE_SYNC_868_BENCH`,
**OFF by default**. A developer building locally with a complete, isolated
ESP-IDF v5.5.6 environment can set it explicitly using the project's
CMake invocation (e.g. `idf.py -DRFLINK_ENABLE_SYNC_868_BENCH=ON build`).
Use a separate build directory/artifact set, and do not publish to Stable
or install over the only known-working firmware.

**Do not enable this on the user's active RFLink device yet.** The GPIO
interrupt path generates roughly 38,400 interrupts per second at this
candidate rate even during receiver idle noise, and could overload
an ESP32-S3 running Wi-Fi, OTA, logging and two other RMT receivers.
The standard status UI still describes the FIFO diagnostics and does not
yet present a dedicated complete synchronous receiver analysis screen.

A bench image must be flashed only after the user has an A/B rollback image
and an approved test plan. Do not silently turn this on in the Builder,
reuse prior board IDs with different pin assignments or add an automatic
mode scheduler.

## Acceptance tests and explicit non-claims

The normal and explicitly opted-in main.cpp branches are both checked with
the repository's host-IDF C++ syntax test. The official CI firmware job
compiles the **normal OFF-by-default** image; its success alone is **not**
proof that the opt-in hardware mode works on the ESP32-S3, nor a substitute
for a separate full bench-on ESP-IDF build and on-air sampling.

The host unit test `firmware/tests/test_sync_manager.cpp` simulates:
- default transport unsupported and no fallback to FIFO;
- manual sync RX acquisition while no other RX on radio 2 is allowed;
- RAMSES UART TX success -> exact previous sync RX restoration;
- UART TX failure -> same sync RX restoration;
- sync restore failure -> manager FAULT, not online;
- hardware loss -> local GPIO capture stop attempted, despite SPI loss.

Repository regression tests verify that the actual firmware starts FIFO by
default, without passing the build option in any Builder path.

The official ESP-IDF build compiles the GDO ISR and manager integration but
does **not** validate GPIO timing, modem parameters or on-air messages.

Still needed on real hardware:
1. Logic analyzer traces on GDO0/GDO2, including a measured sampling edge.
2. Clock-count/CPU-load/drop testing with Wi-Fi, OTA and concurrent 433 RMT.
3. Actual Itho *and* RAMSES-II receptions on the SAME fixed 2-FSK profile.
4. Complete framed messages and checksum verification, not just 2048-bit
   chunks or FIFO-sized evidence.
5. Successful and failed TX->same-RX restore, fault injection, power loss
   and long-duration receive tests.
6. A reviewed receive-evidence UI before enabling a wider user release.

If the fixed profile proves incompatible with the required protocol families,
do not silently switch between UART and FIFO. Revisit modem calibration or
provision a separate 868 MHz radio.

References: [TI CC1101 data sheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf),
[ESP-IDF GPIO interrupts](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/gpio.html),
[the data+clock sampler bench](CC1101_868_GDO_CLOCK_ISR_BENCH.md).
