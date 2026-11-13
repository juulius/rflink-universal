# CC1101 RX Engine v2 — Milestone 1: 433 MHz capture evidence

Related roadmap: [#163](https://github.com/juulius/rflink-universal/issues/163).

## Scope (first P0 development increment)

This milestone improves the **observability and retention** of native 433.42/433.92 MHz asynchronous OOK reception. It is **not** an on-air proof of doorbell reception and does **not** implement a new modulation or software protocol decoder.

### Transport path

`CC1101 RF front end (ASK/OOK)` → `GDO2 asynchronous high/low` → `ESP32-S3 RMT RX ISR` → `one RMT active slot + two immutable completed slots` → `RX worker` → `RAW capture evidence` → `30 μs software filter` → `5 ms synthetic idle tail` → `NativeDecoder / user RX recipes`.

- **ISR** does not allocate, access SPI, log or decode. It rearms the next owned slot; a full completion queue discards the newest completion.
- Capture metadata includes the **pre-filter signed pulse durations** (if PSRAM detail survives), frequency, monotone uptime and (in the full CSV) SNTP-derived UTC time and boot ID if synchronized.
- `full_frames` is the legacy public name of **RMT host buffer saturation/truncation** at 512 symbols. It is **not** a decoded or valid RF frame. This milestone adds explicit alias fields instead of removing old status keys.
- Before this change, the `short_frames` aggregate treated `event.symbols < 2` as too short. One symbol can include *two* RF durations; the decoder could receive that capture while the short counter incorrectly incremented. The counter now uses `n < 2` *raw durations*, with saturated captures still included in legacy `short_frames`.
- `last_raw_pulses` and `last_filtered_pulses` distinguish the RMT result from software filtering. Filtered count excludes the *synthetic* 5 ms tail. A zero filtered count also applies when the filter was not run (short/saturated), so check `last_outcome` rather than inferring why from the number alone.

### Status API additions (per channel)

`rx43342_...` and `rx43392_...` gain:
- `rmt_non_saturated` — captures that **did not fill** the RMT host buffer; *does not prove a complete RF message*.
- `rmt_saturated` — captures equal to the 512-symbol host target, dropped without decoding.
- `raw_short_only` — captures with fewer than two raw measured durations; excludes saturated frames.
- `last_raw_pulses` — last observed raw signed GDO2 duration count.
- `last_filtered_pulses` — last resulting post-filter duration count before any synthetic idle tail; zero when no filter run.

Legacy `rx433*_frames`, `full_frames`, `short_frames`, `filtered_frames`, `forwarded_frames`, `last_outcome`, `queue_drops` remain intact.

### Log retention policy (incremental)

The compact *metadata* event `rmt_raw_capture=46` is routed as:
- **Six or more raw signed durations**, or a **saturated RMT capture**: existing **per-radio signal log ring** (433.42 or 433.92). Independent of the shared noise ring. These are *candidates*, not decoded protocol messages.
- **Fewer than six raw durations, not saturated**: existing shared noise ring (max one tiny sample per 750 ms per radio when count <= 2).
- **Exact raw timings** continue exclusively in the existing 512 KiB radio-specific PSRAM detail arena, with a high-bit pre-filter key linked to the metadata event. If detail PSRAM cannot allocate, numeric metadata still logs; raw timings cannot be fabricated or recovered.
- **Limits**: the per-radio signal metadata ring remains bounded (default 250 entries) and can evict earlier records from **the same band**, including protocol events, under sustained same-band activity. The detail arena itself drops newest when full; old captures are not persistent. A dedicated separate capture-metadata budget/replay store remains a follow-up milestone, *not* solved here.

### Parity baseline (v0.6.6 vs native, source-audited)

| Aspect | Archived v0.6.6 ESPHome | Native v0.7 current |
|---|---|---|
| Radio interface | CC1101 ASK/OOK asynchronous GDO2 | same (CC1101 profile `kOok433`) |
| Input pull | pulldown | `GPIO_PULLDOWN_ONLY` after RMT assignment |
| RMT channels | two non-DMA, 96 symbols each | same |
| Pulse filter | configured 30 μs | safe hardware threshold clamped to source clock; software 30 μs |
| Idle boundary | 5 ms | `signal_range_max_ns=5ms`; synthetic 5ms tail for decoder |
| Capture memory | ESPHome `buffer_size: 8kb` | native host target 512 RMT symbols (not byte-for-byte equivalent) |
| Decoder | ESPHome remote_receiver + v0.6.6 recipes | native pulse sink + user decoder recipes; protocol migration incomplete |
| RF validation | former known-working hardware | **not yet demonstrated for 433.92 doorbell** |

The hardware CC1101 front-end still demodulates RF; ESP32 handles durations and protocol logic. **No IQ/sample-level SDR functionality.**

### Later benchmarks and proof (not checked off)

- [ ] Compare all CC1101 RX registers and transitions with upstream ESPHome 2026.9, RadioLib, rtl_433_ESP and OpenMQTTGateway. Record exact source versions/licenses and critical differences.
- [ ] Reproduce five real 433.92 MHz doorbell button presses with timestamps and 5-second gaps. Export complete CSV immediately; correlate RMT captures, full/saturated outcomes, pre-filter samples, post-filter/decoder counters, queue drops, and protocol decisions. Repeat with v0.6.6 on the same PCB if needed.
- [ ] Measure whether same-band long OOK noise crowds the 250-entry signal ring. If so, add a separate capture-metadata arena without impacting actual decoded signal retention.
- [ ] Confirm no watchdog, OTA, SPI lock or 868.30 packet regression during a noisy RX duration test.
- [ ] Implement deterministic capture replay/demodulation only after RF transport has passed physical Gate 1.

## Acceptance for this incremental PR

1. One RMT symbol with two valid raw durations does **not** increment the short-only count.
2. Two raw durations collapsing below two filtered durations increment `filtered_frames`, not `short_frames`.
3. Saturated 512-symbol captures are not forwarded as protocol frames.
4. The status and web UIs describe stages truthfully, without conflating packet recognition and RF capture.
5. Long 433 MHz candidate metadata survives arbitrarily many **868 MHz shared noise** entries; same-radio bounded retention is stated explicitly.
6. Existing 868 packet reception, radio profiles, TX, and OTA code remain unmodified.

**Physical Gate 1 is not considered passed until actual on-air evidence is reviewed.**
