# RFLink Universal — RX Engine v3: design and implementation plan

**Status:** architecture proposal; **no live radio firmware changed**.  
**Target:** native ESP-IDF v0.7, ESP32-S3 N16R8, three CC1101s.  
**Date:** 2026-11-16; architecture decision updated 2026-11-17. **Tracking roadmap:** [#163](https://github.com/juulius/rflink-universal/issues/163).

## Overview: what this means for users

RFLink should listen to as many **compatible** wireless protocols as the existing radios physically support, including unknown messages and different payload lengths. A new decoder should not require rewriting the radio driver. We will preserve the working receiver as a rollback image while developing one continuous fixed-profile 868 MHz synchronous-bit receiver, gated by physical validation.

One CC1101 cannot simultaneously receive unrelated frequencies, incompatible modulations or arbitrary data rates, and its output is not a software-defined radio (SDR) I/Q stream. The system must **show** missed-listening windows if it changes RF configuration. Do not promise "all 433/868 signals at once."

**Decision (2026-11-17): one permanent RX method per physical radio:**
- 433.42 and 433.92 MHz: unchanged OOK/GDO2/RMT pulse capture, one capture evaluated by all compatible decoders.
- 868.30 MHz target: one **fixed, hardware-validated** 2-FSK profile with synchronous CC1101 GDO DATA+CLOCK capture, then software bitstream framing and parallel protocol decoding.
- **No automatic FIFO/UART time-sharing** on the 868 radio. The legacy 63-byte SPI FIFO path remains the operational baseline until the new design passes real board tests, and then remains a firmware/OTA rollback reference. The host-only RAMSES UART byte framer is a reusable software decoder stage, **not a request to enable 8N1 UART hardware RX**.

A single CC1101 cannot receive incompatible RF profiles simultaneously. If Itho and RAMSES need different RF modem parameters, the fixed-profile goal cannot be achieved with one radio; report that limitation and consider another CC1101 rather than silently switching modes.

Every transport delivers immutable evidence to a separate framing/decoder engine. No decoder owns SPI, CC1101 registers, GPIO, RMT, UART or radio mode changes. The sole radio owner remains `RadioManager`.

### Reliability requirements

- Keep both 433 receivers independent and listening while the 868 data+clock receiver starts, recovers or temporarily pauses for TX.
- Preserve today's FIFO firmware as the rollback baseline; any new 868 TX-to-RX handover must restore the *same fixed synchronous RX profile* rather than switching to FIFO.
- Never confuse **RF activity**, **raw capture**, **complete framed data**, **checksum-validated message**, and **recognized device**.
- No automatic TX or pairing from a learned or unrecognized signal.
- No hardware code, allocation, logs, HTTP, or SPI transactions in an RX interrupt service routine (ISR).
- Bounded internal RAM, CPU, and PSRAM; per-radio drop counters, frame truncation markers and explicit reasons.
- Firmware remains native ESP-IDF, with legacy ESPHome v0.6.6 strictly a source/reference.

## 1. Verified source inventory

| Radio | Native v0.7 implementation | v0.6.6 reference | Unresolved |
|---|---|---|---|
| 433.42 MHz (R0) | CC1101 OOK -> GDO2 -> RMT, 96 hardware symbols, 512 host symbols, 3 immutable slots, 30 us software filter, 5 ms idle boundary | ESPHome remote_receiver and OOK pulse decoder | Different valid protocols may require different pulse filters/idle windows; long captures saturate |
| 433.92 MHz (R1) | As above, with distinct capture metadata/PSRAM buffers | As above | Same limitations; continuous FSK needs another profile |
| 868.30 MHz (R2) | 2-FSK, fixed `PKTLEN=63`, `PKTCTRL0=0x00`, CRC disabled; worker polls MARCSTATE/RXBYTES and reads exactly 63 bytes | Itho fixed FIFO **plus** optional RAMSES6 GDO2/UART1 RX, 38,400 8N1 with bounded 240-byte software deframer | Native has no RAMSES6 serial RX transport; 63-byte FIFO captures do not guarantee valid on-air frames |

Relevant source:
- `firmware/components/rflink_radio_hal/{rflink_packet_rx.cpp,rflink_rf_profiles.cpp,rflink_cc1101_spi.cpp,rflink_rmt_rx.cpp}`
- `firmware/components/rflink_decoder_core/src/rflink_native_decoder.cpp`
- `reference/v0.6.6-esphome/rflink_universal/components/rflink_universal/{ramses_uart_esp_backend.h,ramses_uart_stream.h,ramses_uart_stream.cpp}`
- `reference/v0.6.6-esphome/rflink-universal.yaml`, `reference/v0.6.6-esphome/docs/source/rflink_universal/RAMSES6_TRANSPORT_NL.md`.

**Crucial legacy correction:** RAMSES6 was implemented as experimental asynchronous UART RX: it pauses the Itho FIFO consumer, applies `PKTCTRL0=0x30`, `IOCFG2=0x0D`, `MDMCFG2=0x00`, and restores/verifies the old packet profile afterwards. The standard legacy YAML has `ramses_radio_lab: false` and `decoder_auto_scan_lab: false`. Software tests do **not** prove the UART hardware path was active, or that FIFO and UART were concurrent.

## 2. Hardware capabilities and limits

**CC1101 FIFO:** 64-byte RX and TX queues. Fixed-length 63 in our firmware is a selected profile, not a universal 868-MHz packet size. TI describes fixed length, length-byte-controlled variable length, and infinite length/streaming plus synchronous/asynchronous serial operation. Hardware variable mode is only suitable if the **actual on-air protocol** transmits a length byte in the required position. Changing 63 to 62 or enabling variable mode globally would break established captures.

**FIFO underflow race:** TI section 20 recommends waiting for the **full packet** before reading a packet shorter than 64 bytes. With streams longer than 64 bytes, read in controlled chunks while leaving a byte in the FIFO until the end is known: emptying the FIFO at the precise instant another RF byte arrives can duplicate data. Use GDO watermarks/packet boundaries and status, not naive "flush every receive" or unrestricted FIFO reads.

**Available GDO signals (TI Table 41):**
- `0x01`: RX FIFO threshold or packet end;
- `0x06`: sync seen until packet end;
- `0x0B`: synchronous serial clock;
- `0x0C`: synchronous serial data;
- `0x0D`: asynchronous serial data;
- `0x16` / `0x17`: hard symbol data bits;
- `0x1D`: symbol tick.

**No concurrent FIFO+GDO approach is proposed for live RX.** For the target synchronous 2-FSK profile, the **candidate** GDO mapping is `IOCFG0=0x0C` (DATA on reference GPIO16) and `IOCFG2=0x0B` (CLOCK on GPIO17); `PKTCTRL0.PKT_FORMAT=01` selects synchronous serial. Verify actual signal polarity, capture edge and non-overlap with TX using a logic analyzer. GDO0 is shared with existing UART TX and must be safely released/reclaimed by `RadioManager`; GDO1 is the shared MISO line and is not an additional free GPIO. Do not present the candidate register values as board-tested.

**Supported modulation formats:** CC1101 can demodulate OOK/ASK, 2-FSK, GFSK, 4-FSK and MSK, but each distinct combination of frequency, bandwidth, deviation and data rate has to be configured and tested. The two 433 radios are currently dedicated to OOK. Switching them to 433 FSK means a listening gap for OOK; use a separate CC1101 if two incompatible 433 profiles must be received continuously.

**Pin/speed caveat:** reference pinout is R0 CS/GDO0/GDO2 4/5/6, R1 9/8/14, R2 15/16/17; shared SPI 12/11/13. The current versioned default SPI clock is conservative (200 kHz), while a user's Builder-generated board profile can override it. Verify **actual active board profile** before testing; never silently change pins over OTA.

Primary hardware reference: [TI CC1101 SWRS061I datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf), sections 15, 20, 27 and Table 41.

## 3. Shared RX architecture

```text
RadioManager — one physical RX owner / radio, TX->same-RX restore and epoch
  +-- 433.42: OOK GDO2 -> RMT -> immutable pulse capture
  +-- 433.92: OOK GDO2 -> RMT -> immutable pulse capture
  +-- 868.30 target: fixed 2-FSK GDO0 DATA + GDO2 CLOCK
                          -> hardware-verified sampler -> bit capture
                          -> continuous bitstream reassembly
                 |
                 v
       Raw evidence (epoch, length, gaps, dropped bits, provenance)
                 |
                 v
       Shared framing/interpretation (Itho, RAMSES-II, user decoders)
                 |
                 v
       Validated / rejected / ambiguous messages
                 |
                 v
       Device registry, logs, MQTT/Home Assistant and web UI
```

The existing 868 FIFO receiver is active in the current firmware; the
synchronous transport is **not connected to physical CC1101 outputs** yet.
The independent [fixed synchronous receiver plan](CC1101_868_FIXED_CLOCKED_RX.md)
is being implemented in PR #200 and must pass a bench test before activation.

Suggested versioned interfaces (design contract; only some pure capture helpers exist):

| Object | Fields/requirements |
|---|---|
| `RxProfile` | radio, carrier, modulation, bandwidth, deviation, bit rate, **one** RX transport, pin owner, profile identity, verified decoder compatibility |
| `RxCapture` | radio, boot ID, epoch, monotonic time, bit count, partial/complete/discontinuity/dropped evidence |
| `RxFrameCandidate` | bounded bit start/end, framing/sync hypotheses, corruption and provenance |
| `DecodeVerdict` | unrecognized/candidate/validated/ambiguous/rejected with checksum/repeat evidence; no implicit device creation |
| `RxLease` | manager-owned GPIO/SPI/clock/data resources; TX stop and **restore same RX profile** or mark FAULT |

A bit-block boundary is **not** necessarily a radio message boundary. Multiple
compatible decoders can inspect the same immutable capture without modem switching.
Never stitch fragments across epochs, missing clock edges, TX interruptions,
buffer overflow or boots.

## 4. 868 MHz implementation — fixed synchronous data and clock

### Step 4.1 — Preserve the previous FIFO firmware, prove GPIO semantics

- Baseline remains the native 63-byte 2-FSK FIFO receiver with no changes to
  active SPI/CC1101 registers in the current firmware. Its polling diagnostics
  (PR #198) are **not** a prerequisite for the fixed synchronous RX architecture.
- On a dedicated test build, configure a candidate synchronous serial modem:
  GDO0=DATA (`IOCFG0=0x0C`), GDO2=CLOCK (`IOCFG2=0x0B`) and synchronous
  serial `PKT_FORMAT=01`. Measure the data setup/hold and sampling edge with
  an oscilloscope or logic analyzer. Do not assume UART 8N1 framing.
- Before selecting the modem parameters for a permanent RX profile, capture
  **real Itho and real RAMSES** traffic. One fixed 2-FSK rate/deviation/
  bandwidth must receive both; if not, a single CC1101 cannot meet this
  no-switching requirement.

### Step 4.2 — Reliable ESP32-S3 clocked-bit acquisition

- Choose a hardware-backed clock/data sampling path after measuring
  approximately 38.4 ksymbols/s under Wi-Fi, OTA and two active RMT receivers.
  A naive GPIO ISR per bit is not automatically safe.
- Use bounded ISR-to-task transfer: ISR does not decode, allocate, log,
  access SPI or call the HTTP service.
- The host-side `ClockedBitCapture` in PR #200 already implements MSB-first
  bit packing, 2,048-bit blocks, drop-newest and epoch isolation. **It is
  not a physical capture driver.**
- Counters for dropped edges, task backpressure, clock discontinuities and
  recoveries must be surfaced independently per radio.

### Step 4.3 — Shared software decoding, one active RF profile

- Implement streaming bit alignment and preamble search for Itho from raw
  captured bits; preserve exact provenance and the existing checksum rules.
- RAMSES-II may be decoded directly from bit-packed 8N1 on-air data using
  existing `decodePacked`, or deframed into validated bytes for the pure
  RAMSES UART stream framer from PR #196. **Do not enable physical UART1 RX**
  or alternate to FIFO just because the protocol wire format uses 8N1.
- Multiple decoders evaluate a capture. Explicitly flag ambiguous matches
  instead of creating duplicate devices or accepting noise as a command.

### Step 4.4 — TX/restoration and promotion gates

- GDO0 is also a hardware UART TX line. `RadioManager` must pause data/clock
  sampling, release the pins, perform approved TX, and **restore the same
  synchronous data/clock RX** (register readback, PLL/RX state, ownership,
  fresh epoch). A failed restore means FAULT, not online.
- Bench test known Itho/RAMSES messages, truncated and noisy bitstreams,
  long captures, 433 concurrent RX, Wi-Fi reconnect, OTA, watchdogs, TX and
  recovery. Run a long listen test before early-adopter or Stable promotion.
- If the fixed profile cannot support both families, keep the older FIFO
  firmware as a separately selectable rollback, document the limitation,
  and evaluate a dedicated additional radio. **Never introduce silent mode
  hopping as an unapproved fallback.**

## 5. 433 MHz implementation

- Preserve the two independent OOK/RMT receivers and existing early ISR rearm, 3 immutable buffer slots, pre-filter RAW details, separate signal/candidate/noise storage and per-band queue-drop counters.
- Treat `30 us` filter and `5 ms` frame-idle as **legacy defaults**, not a universal signal definition. Keep compatibility for current decoders while exposing optional decoder-specific filtering and timing hypotheses **after** the raw capture copy.
- Explore ESP-IDF `en_partial_rx` / `is_last` for >512 RMT-symbol signals. Reassemble contiguous partial captures using a stable capture ID; never decode a saturated/partial buffer as an ordinary complete frame.
- Offline replay should produce bounded PWM, PPM, Manchester, RC-Switch-style timing candidates, polarity/bit order hypotheses, repeat grouping and reason codes. Do not infer a valid device without protocol checksum or repeat/identity proof.
- Learn a device via UI only after independent matching captures and validation; keep source evidence and ambiguity visible.
- Different 433 FSK/GFSK radio profiles may be added, **but require separate RF configuration** and an explicit listening-gap policy. No silent scan that interrupts regular OOK reception.

ESP-IDF RMT RX reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/rmt.html (partial RX and last-part flag; verify v5.5.6 availability before implementation).

## 6. Runtime ownership, memory, error and security policy

**Priority:** preserve RX scheduling > drop/incomplete counters > bounded decode > optional raw/detail retention > UI/HTTP. RF noise must not starve Wi-Fi, IDLE0 watchdog or other radios.

Preserve normal event logs: three independent recognized-signal rings (default 250 each), shared noise, separate System/Web, independent 433 RAW candidate metadata, existing settings. Detailed payload arenas stay within the agreed PSRAM budget (~30% maximum plus existing free-memory safety margin); on a full detail class **drop newest**, never overwrite another band's protected detail. A ring's overwrite policy is **not** a radio hardware FIFO overflow.

Show per-radio:
- RF/raw observation counts;
- complete vs partial vs RMT-saturated vs FIFO overflow;
- transport queue-drop and worker latency;
- framed candidates, verified-checksum decodes, decoder ambiguities;
- actual profile, expected next mode, listening gap, last restore result;
- explicit `hardware CRC not configured` where applicable.

**Safe TX/recovery sequence (not periodic RX switching):** acquire manager lease -> pause synchronous sampler -> mark incomplete capture and increment epoch -> release GDO ownership -> execute approved TX/recovery -> restore and verify **the same synchronous 2-FSK RX profile**, clock/data pins and PLL/RX -> start a new capture epoch. Failed restore means **FAULT**, not fake listening. Radio 0/1 remain unaffected.

Use read-only exported captures deliberately: RF payloads may contain device IDs, security or rolling-code data. No credentials, unfiltered RF bytes or pairing tokens in ordinary unattended console/System/Web logs. API/export protection must remain at least as strict as the existing full-diagnostics capture endpoints.

## 7. Implementation plan with independent PRs

| Phase | Deliverable | Objective acceptance gate |
|---|---|---|
| **A: architecture (#195)** | One fixed physical RX profile per radio and source inventory | Docs review, no active radio changes |
| **B: RAMSES byte decoder (#196)** | Hardware-independent bounded RAMSES UART wire-byte framing | Host tests; **not** physical UART1 RX |
| **C: FIFO baseline diagnostic (#198)** | Optional observation of *existing* FIFO polling, not part of the new RX path | Defer while the new architecture is validated; close separately if no longer useful |
| **D: clocked bits foundation (#200)** | Epoch-aware, bounded bit capture and synthetic native decoder parity | Host tests and official ESP-IDF compile; no live GPIO claims |
| **E: physical clock/data acquisition** | Experimental manager-owned 868 2-FSK GDO0/GDO2 driver | Scope and edge-level trace; zero unsafe TX pin conflicts |
| **F: streaming framing / software decoders** | Bit-contiguous Itho and RAMSES validators, unaligned sync and bounded ambiguity | Controlled on-air packets from **both** on **one** fixed modem configuration |
| **G: flexible 433** | OOK/RMT partial capture and decoder-specific timing hypotheses, no RF switching | Repeat-based validation and bounded drops |
| **H: user library / release readiness** | Learn UI, memory-safe decoder dispatch, long soak, OTA/rollback | 24–48h bench evidence, healthy TX->same-RX and no 433 regression |

All numbers are future acceptance targets, not field measurements.
Do not promote Experimental RX to Stable from compilation or host fixtures.

### Physical and simulated test matrix

- 433.42/433.92: independent continuous OOK reception during all 868
  tests, multiple decoder matches, long pulses/partial frames and buffer-full.
- 868: Itho + RAMSES/Orcon repeated actions on a **single fixed RF profile**,
  GDO0 DATA and GDO2 CLOCK edge correctness, true missing-edge detection,
  bit orientation and reassembly, ambiguous-frame rejection and long streams.
- TX/recovery: receiver paused and **same synchronous profile restored**,
  GPIO ownership, CPU/RMT pressure, power faults, SPI timeout, Wi-Fi/OTA.
- Retain board identity, NVS, crash history, logs, CPU/PSRAM safety margin.
- Evidence levels: source reviewed; host tested; ESP-IDF compiled;
  physically received; long-duration validated.

## 8. Rollback and release

Keep the current 63-byte FIFO firmware and dual OOK/RMT profile as the **rollback baseline**, rather than an automatically alternating runtime listening mode. Every experiment must retain a safe firmware fallback that was tested on the same board. Profile/GPIO overrides belong to the Builder device configuration, never hard-coded into an OTA revision. Preserve Stable / Early adopter / Experimental gating, and never silently activate a new modulation search.

In the UI, unknown frames are named **candidates**. A valid protocol message requires integrity evidence; unsupported modulation/frequency is called out as an **RF profile mismatch**, not treated as a decoder bug.

## 9. Open hardware questions

1. Can **one fixed 2-FSK profile** demodulate both real Itho and RAMSES
   without a listening-mode change? **Must be proved on air.**
2. Which hardware-assisted ESP32-S3 peripheral reliably captures ~38.4k
   clocked symbols/s with both 433 RMT listeners, Wi-Fi and OTA?
3. What is the verified data/clock polarity and sampling edge on this PCB,
   and can RX/TX safely share 868 GDO0 with exclusive ownership?
4. What framing/bit-alignment boundaries and noise limits are required to
   avoid false Itho/RAMSES matches in continuous capture?
5. Which 433 FSK protocols would need a separate dedicated receiver instead
   of interrupting the current OOK service?

## Source and validation statement

This design is based on inspected v0.7 main-branch source, archived v0.6.6 legacy source, existing [RX Engine v2 roadmap #163](https://github.com/juulius/rflink-universal/issues/163), TI CC1101 datasheet SWRS061I and Espressif RMT documentation. **No physical ESP32-S3 capture/flash was carried out as part of this documentation PR.** It does not change firmware, enable transmission or publish a release.
