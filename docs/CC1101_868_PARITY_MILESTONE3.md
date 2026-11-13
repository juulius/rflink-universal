# 868.30 MHz parity audit — CC1101 RX Engine v2, milestone 3

Roadmap: [#163](https://github.com/juulius/rflink-universal/issues/163). Previously merged: [#169](https://github.com/juulius/rflink-universal/pull/169) (433 stage counters) and [#171](https://github.com/juulius/rflink-universal/pull/171) (433 RAW metadata isolation).

## Audit results and decisions

The archived stable **v0.6.6 ESPHome** `reference/v0.6.6-esphome/rflink-universal.yaml` uses an 868.2999 MHz **2-FSK fixed 63-byte FIFO RX** profile, 203 kHz channel filter, 50 kHz FSK deviation, preamble/sync mode 16/16 with sync bytes AB FE, no whitening and no hardware CRC. This is also the transport profile the native code is implementing (`firmware/components/rflink_radio_hal/rflink_rf_profiles.cpp`). The native 868 receiver explicitly verifies configured CC1101 registers, enters a confirmed RX state, polls MARCSTATE + RXBYTES, recognizes FIFO overflow/IDLE and reads 63 bytes atomically; it records optional RSSI/LQI/FREQEST, without claiming hardware CRC validity. The decoder attempts the installed user decoder, RAMSES-II or Itho, tracks preamble rejection and decoder failure separately and reports ambiguity, retaining only one accepted protocol event when both would match.

**On-air evidence so far:** an actual RAMSES-II `0x22F1` fan-high command was received and verified by the user on the working experimental firmware. This is useful positive evidence, **not** proof of lossless reception or of correct handling of every 868 telegram. Earlier logs also had many unknown FIFO packets, some FIFO overflows, and a fan Auto command not confirmed as decoded.

### Key 868 difference from 433

At 433 MHz the RMT observer provides **signed pulse durations**. At 868 MHz the CC1101 **has already demodulated and framed** a 63-byte FIFO packet; we can replay those delivered bytes (and accompanying modem-quality estimates). We cannot recover on-air symbols rejected before reaching the FIFO or infer a valid CRC from a CRC-disabled profile. A future optional synchronous/asynchronous digital GDO mode belongs in a separate, isolated radio-profile investigation.

## Milestone 3 implementation: unknown 868 retention is independent

| Evidence | Previous | New |
|---|---|---|
| Recognized 868 RAMSES-II / Itho compact signal entries | Existing 868 signal ring, 250 default | **unchanged** |
| Unknown 868 FIFO compact entries | Shared 2,000-entry noise ring with 433 glitches | New independent **1,500-entry** metadata ring; still `stream:noise` in APIs |
| Recognized 868 exact 63-byte packet details | Existing 868-specific **256-KiB** arena | **unchanged** |
| Unknown 868 exact 63-byte packet details | Shared **448-KiB** noise arena | New 868-only **256-KiB**, up to 1,500 entries |
| 433 short noise detail arena | Shared 448 KiB with unknown 868 | Shared **192 KiB** (smaller, but no 868 unknown packets) |
| Other RF details | 433.42 512 KiB; 433.92 512 KiB; System 384 KiB; Web 256 KiB | **unchanged** |

- Total physical detail **byte arenas stay at 2,368 KiB**. Increasing the detail index from 3,774 to 5,274 entries grows the object to under 89 KiB; combined object + byte arenas are compile-time bounded by **2,457 KiB**, less than 30% of 8 MiB PSRAM. The early-boot 4 MiB free-PSRAM floor and fallback at 50% remain intact.
- Two previous 433 RAW-capture metadata rings remain independent; all six original configurable log settings/NVS schema remain unchanged.
- Legacy log JSON/CSV stream codes are preserved: unknown 868 packet events still say `noise`, need *Include noise* enabled to display in live logs, and are included in **full CSV regardless of filtering**.
- `/api/logs/config` includes `unknown868_fifo: {limit, used, overwritten}` and a **seventh detailed arena** in its details counters. Builder and direct-device web show the counts. CSV progress event estimate includes this extra ring.
- `/api/radio/capture` and full CSV RF detail export resolve the new dedicated unknown 868 bucket first, while preserving old fallback lookups and recognized 868 details.
- Device data remains volatile; no flash writes. Both metadata and detailed byte rings have finite retention. Under sustained same-band unknown traffic, *older 868 unknown packets can still be evicted by later unknown packets*. Existing FIFO detail storage also evicts oldest records to make room; **this milestone deliberately preserves that existing retention policy** rather than silently switching FIFO drop behavior.

## Gate status

**Phase 1 review (868):**
- [x] Source-level audit of known v0.6.6 868 radio preset, native fixed 63-byte FIFO RX, SPI readiness/readback, recovery and decode-path counters
- [x] Preserve current CC1101 register table, FIFO poll, hardware CRC-disabled status, packet length, 868 radio manager, RX/TX recovery
- [ ] Fresh bench checks: FIFO overflow recovery, repeated high/Auto Orcon messages, additional Itho/RAMSES sample and a longer background run
- [ ] Compare against exact upstream ESPHome CC1101 and RadioLib revision-specific recovery implementation and document any actionable differences

**Phase 2 retention/replay evidence (868):**
- [x] Independent, bounded unknown-868 metadata ring, separate from 433 noise and recognized RAMSES
- [x] Independent 868 unknown FIFO-byte detail arena with existing RF capture API/CSV links, optional signal quality
- [x] Host tests: 1,900 unknown packets with real `LogStore`, page_after cursor replay, radio filtering, preservation of recognized messages/433 shared noise and unchanged NVS limits
- [x] Host `DetailStore` tests: 868 unknown 63-byte packets and quality data remain independent of recognized 868 and shared noise under a flood, with honest evictions
- [ ] Measure actual PSRAM free/largest block and 24h detail retention on ESP32-S3; compare first/last 868 unknown captures after 433/868 activity
- [ ] Reconcile desired *drop-newest* vs current *evict-oldest* detail policy in a **separate** explicitly tested PR; don't accidentally change existing semantics in this storage patch

**Next:** common offline Replay Engine accepting 433 signed RMT pulses **and 868 FIFO hex/bytes**. Every replay vector must identify the modem mode, what raw data actually exists, decoder validation/rejection reasons, original boot/UTC if synchronized and an immutable capture ID. Optional 868 digital RAW/GDO capture needs a separate transport mode, not a fallback claimed to already exist.

**No hardware-level improvements are claimed until on-air validation.**
