# 868 MHz fixed synchronous RX — manager-controlled bench route

**Available code:** The manager-owned synchronous RX transport is now built
into **new main and Experimental firmware** (through the usual Home Assistant
Builder release workflow). It reads back the CC1101 configuration and uses
GDO0 DATA + GDO2 CLOCK to acquire bounded bit evidence.

**Hardware timing and on-air protocol compatibility remain unverified.**
This is a dedicated test firmware path, not a verified everyday receiver.
No on-device Itho/RAMSES decoder or automatic device action is enabled.

## What changes for an ordinary user?

**New main / Experimental:** the 868.30 MHz radio starts the **fixed
synchronous DATA+CLOCK receiver**, while 433.42 and 433.92 MHz remain on
their separate OOK/RMT receivers. There is no automatic FIFO/UART switch.

**RC / Stable:** retain the 868.30 MHz 63-byte FIFO receiver until
hardware validation supports promotion. Firmware source tags are immutable:
existing Experimental releases do not gain this feature retroactively.

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
- Main/Experimental builds instantiate the sampler, so it uses its
  bounded internal-memory clock-edge ring and a GPIO clock interrupt.
  RC/Stable builds do not instantiate it.

The existing CC1101 FIFO driver remains a separate rollback/reference
mode. Both 433 receivers, OTA and built-in decoder logic are unchanged.

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

## Build and install a new Experimental firmware

The CMake option `RFLINK_ENABLE_SYNC_868_BENCH` now defaults to **ON**
when building the current main branch or a new immutable
`firmware/v0.7.0-exp.N` release. The Builder already supplies that tag
when the user selects an Experimental release. No manual CMake flag is
needed. Firmware marked RC/Stable is compiled with the option **OFF**;
a mismatched build cache causes a build error rather than a silent
change of radio behavior.

1. In the Builder choose **Devices → RFLink → Firmware → Experimental**,
   refresh the available versions and choose a **newly published** tag
   containing this main change. Older immutable tags remain unchanged.
2. Build against the saved, verified board profile (reference: 868 GDO0
   GPIO16, GDO2 GPIO17). Preserve a known-working firmware image and
   the ability to recover through USB if 868 interrupts affect Wi-Fi.
3. Install on the intended ESP32-S3 N16R8 test device through a
   compatible OTA or USB method. After boot, check System/Console for
   `rflink.sync868` start errors and every-30-second counters.
4. Use the protected [868 capture instructions](CC1101_868_SYNC_BIT_EVIDENCE.md)
   to record real RF activity, freeze it, export CSV and run offline replay.

At about 38.4 ksymbols/s the GPIO ISR can generate **~38,400 interrupts
per second**, including under noisy conditions. This could overload an
ESP32-S3 running Wi-Fi, OTA and two RMT receivers. Real clock polarity,
missed edges, TX handover and Itho/RAMSES compatibility have not yet
been measured. Do not rely on this test receiver for essential equipment.

The current web UI is not a full Itho/RAMSES raw-data decoder; it provides
a protected diagnostics API and numerical counters. No automatic
protocol-mode switching or live decoder/device registration is performed.

## Acceptance tests and explicit non-claims

Both main.cpp compile branches are checked with host-IDF syntax tests.
Official Experimental CI now compiles and verifies the **enabled sync
path**; RC/Stable compile without it. Successful compilation is still
**not hardware validation** of GDO timing, RF receive quality, CPU load
or the TX-to-same-RX handover.

The host unit test `firmware/tests/test_sync_manager.cpp` simulates:
- default transport unsupported and no fallback to FIFO;
- manual sync RX acquisition while no other RX on radio 2 is allowed;
- RAMSES UART TX success -> exact previous sync RX restoration;
- UART TX failure -> same sync RX restoration;
- sync restore failure -> manager FAULT, not online;
- hardware loss -> local GPIO capture stop attempted, despite SPI loss.

Repository regression tests verify the channel-driven CMake option:
main and Experimental select synchronous RX, RC/Stable select FIFO and
conflicting cached settings fail the build.

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
