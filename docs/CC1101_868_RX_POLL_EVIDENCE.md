# 868 MHz FIFO polling evidence (RX Engine v3, observation-only)

## What changes for the user

RFLink's **Receive evidence** panel now shows how often the 868 MHz receiver sampled an empty, partly filled or read-ready CC1101 FIFO, plus the longest interval between actual RX polls.

This helps distinguish two hypotheses behind an `rx868_fifo_overflow`:

1. The RX worker was delayed for too long while the CC1101 kept receiving.
2. The selected fixed 63-byte configuration or continuous receive behavior does not match the physical traffic / packet boundaries.

**A high poll gap does not prove it caused an overflow. A full/ready 63-byte sample does not prove that a complete protocol telegram was received, either.** All values are aggregate driver observations, not per-frame timestamps. The second hypothesis also cannot be proven from FIFO occupancy alone. The correct next step is a controlled, timestamped test of GDO events and packet boundaries, not blindly shortening the FreeRTOS task sleep.

## New read-only /api/status fields

| Field | Meaning |
|---|---|
| `rx868_poll_count` | Number of RX worker iterations reaching this active 868 poll |
| `rx868_status_valid_polls` | Successful paired MARCSTATE + RXBYTES readings |
| `rx868_fifo_empty_polls` | Valid samples with 0 bytes |
| `rx868_fifo_partial_polls` | Valid non-overflow samples with 1–62 bytes |
| `rx868_fifo_ready_polls` | Valid non-overflow samples with at least 63 bytes |
| `rx868_fifo_last_bytes` | Latest valid non-overflow occupancy, not necessarily current |
| `rx868_fifo_peak_bytes` | Highest valid non-overflow occupancy in bytes |
| `rx868_poll_last_gap_us` | Last measured gap between consecutive active poll calls |
| `rx868_poll_max_gap_us` | Longest measured gap in microseconds |
| `rx868_poll_gaps_over_20ms` | Number of measured gaps longer than 20 ms |

The existing `rx868_packets`, `rx868_fifo_overflows`, `rx868_recoveries`, `rx868_spi_errors`, `rx868_idle_rearms` and packet/decoder fields remain unchanged. No packet bytes or private identities are added to telemetry.

A successful status sample in FIFO overflow state (RXBYTES bit7 or MARCSTATE `0x11`) increments `rx868_status_valid_polls` but does **not** affect `last/peak` occupancy or empty/partial/ready buckets. The low seven RXBYTES bits in an overflow state are **not** an honest count of buffered bytes. The already-existing `rx868_fifo_overflows` remains the definitive driver count of observed overflow recoveries.

First gap after (re)starting the packet RX transport is ignored so that TX/OTA recovery does not appear as a giant stalled worker interval. Telemetry totals are cumulative within the current firmware boot.

## How to check on an ESP32-S3 test board

1. Install the Experimental firmware only after the official CI compile passes, keeping the previously working image available for rollback.
2. In **Receive evidence**, record `rx868_poll_count`, partial/ready samples, max gap, gaps >20 ms and existing overflow count; preserve the firmware version and uptime.
3. Leave all three radios listening for at least 30 minutes with **no deliberate button presses**. Compare deltas, not just absolute counters.
4. Repeat with 10 known Orcon RAMSES-II actions and, if available, known Itho messages at normal range; record whether the actual protocol decoder recognized each.
5. Compare FIFO overflow timestamps from the existing System log against the aggregate gap/occupancy trends. This release does **not** supply a timestamp for each long poll gap.
6. Recheck 433.42/433.92 RMT counters, normal OTA, Wi-Fi, TX->RX recovery, heap and PSRAM minimums; revert if any degradation occurs.
7. Only after this baseline evaluate an interrupt-notified FIFO transport and optional 868 GDO2 serial RX on a separate Experimental PR.

## Safety and validation

- No changes to CC1101 modem registers, packet length, SPI burst length, RX polling period, SFRX/SRX behavior, UART or RMT.
- No change to log retention, PSRAM allocation limits, raw capture bytes or TX/OTA workflows.
- Deterministic host-only tests for clock gaps, partial/ready readings, overflow status and reset of the timing baseline.
- **Physical reception, timing and FIFO-overflow improvement are not claimed by these tests.**
- Related roadmap: [RX Engine #163](https://github.com/juulius/rflink-universal/issues/163); [architecture PR #195](https://github.com/juulius/rflink-universal/pull/195).
