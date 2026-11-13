# CC1101 RX Engine v2 — Milestone 2: dedicated 433 MHz RAW candidate metadata

Roadmap: [#163](https://github.com/juulius/rflink-universal/issues/163).
Previous milestone: [#169](https://github.com/juulius/rflink-universal/pull/169).

## Problem verified in Milestone 1

The M1 implementation moved long 433 MHz pre-filter RMT candidates out of the **shared noise log** and into the **per-radio signal log**. This prevented 868 MHz noise from overwriting 433 metadata, but continuous 433 OOK interference could still fill the 250-record *signal log* and overwrite a genuine previously recognized device message. A raw RMT capture is not a validated protocol message and must not compete for the same retention budget.

## Implemented M2 design

**Two new separate fixed-size metadata rings** in `LogStore`:

| Index | Storage | Capacity | Stream reported to API |
|---|---|---:|---|
| 0–2 | Existing per-radio signal logs | User-configured (default 250 each) | `signal` |
| 3 | Existing shared three-radio noise | User-configured (default 2,000) | `noise` |
| 4–5 | Existing system and web | Fixed 512 each | `system`, `web` |
| 6 | RAW 433.42 candidate metadata | Fixed 250 | `signal` for backwards-compatible viewing/export |
| 7 | RAW 433.92 candidate metadata | Fixed 250 | `signal` for backwards-compatible viewing/export |

Only `LogCode::rmt_raw_capture` with `LogStream::signal` on radio 0/1 is redirected to ring 6/7. Every other `signal` record (decoded, user match, ambiguous, etc.) still uses its original per-radio ring; even thousands of raw captures cannot evict those recognized messages. Short glitches still use the original shared noise ring.

The two extra metadata rings add `2 * 250 * sizeof(LogRecord)` (at most 20 KiB) to the existing PSRAM log allocation. `LogLimits` / NVS schema and the six existing configuration/used/overwritten arrays remain unchanged. Fixed capture stats are exposed separately as `raw433_capture` in `/api/logs/config`, including `limit`, `used[2]` and `overwritten[2]`. The web UI includes two additional read-only buffer tiles; they are **not** adjustable log settings.

**Log querying and export**: the two new physical rings participate in the existing `/api/logs`, `/api/logs.csv`, snapshot paging and 433 frequency filtering as normal `stream:signal` records, retaining globally monotonic sequence numbers. Estimated CSV record count includes the new rings, so download-progress estimates do not omit them. Existing `pre_filter_rmt` RF detail links continue to resolve by sequence + band from the separate PSRAM `DetailStore`.

### Remaining bounded-retention limitations

- Capture rings retain the newest **250 metadata records per 433 band**, not every capture forever. Same-band interference can still evict *older capture metadata* from its own ring, which is explicitly counted. It cannot evict recognized device signal records.
- Exact RAW signed durations remain **optional volatile per-band PSRAM details**; their storage and retention policy is unchanged. They can expire independently of the compact capture metadata. Loss of detailed PSRAM must be shown honestly, not treated as proof of absent RF.
- The RMT capture observer still samples very short frames at most once/750 ms per radio; this is not lossless over-the-air capture, and the metadata ring is not a protocol decoder.
- None of these changes adjusts the CC1101 register presets, RMT timing, FIFO packet mode, SPI recovery, TX, OTA or network time.

## Required tests

- [x] Host C++ test: >400 long RAW candidates on radio 433.92; previously recognized device message stays in the original signal ring.
- [x] Host C++ test: 868 noise flood and RAW captures stay isolated; 433.42 and 433.92 independently bounded.
- [x] Host C++ test: existing `/api/logs` query semantics, snapshot chronological paging and CSV streaming across all 8 rings; NVS/configure resize preserves capture records; usage and overwrite counts honest.
- [x] Source contracts for firmware API, native web UI and Builder Ingress.
- [ ] Complete ESP-IDF build and Home Assistant Builder amd64/aarch64 checks.
- [ ] Actual on-air validation with five 433.92 doorbell button presses, plus 868.30 RAMSES-II `0x22F1` regression.

**Gate 1 from the roadmap remains open** until physical on-air captures are collected. Hardware observations, not static tests, are required before altering RF sensitivity or assuming the native 433.92 reception is fixed.

## Next milestone

Build a **reproducible offline RAW capture replay tool** that can import RFLink capture JSON/full CSV and evaluate candidate PWM/PPM/Manchester bit timings without claiming recognized devices; compare those captures with legacy ESPHome v0.6.6. Live software demodulation in firmware should only follow replay quality gates.
