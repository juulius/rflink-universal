# 868 MHz FIFO fault/recovery timeline — observational milestone

Related roadmap: [#163](https://github.com/juulius/rflink-universal/issues/163).
Input evidence: an exp.49 diagnostic export covering 4h46m reported **30 FIFO overflows and 30 recoveries**, but the old snapshot exposed no corresponding timestamps for individual errors. Eight RAMSES-II packets were validly decoded, including one RAMSES/Itho overlap. The 868.30 modem profile is 63-byte 2-FSK FIFO with CRC disabled; 13,537 FIFO packets were not recognized, primarily due to missing legacy preamble.

## Goal

Record **when** the receiver reports a fault and **whether** the existing recovery routine succeeded, without changing any RF modem register, SPI timeout, RX polling rate, FIFO behavior, decoder, packet size, ISR, OTA or TX control.

## New diagnostic events (system stream; radio 2; also visible in Radio tab)

| Code | Meaning | When emitted |
|---|---|---|
| 47 — `rx868_fifo_overflow` | FIFO full/overflow detected | Immediately after the existing `recover_fifo()` returns |
| 48 — `rx868_idle_rearmed` | CC1101 unexpectedly entered IDLE twice | Immediately after the existing rearm attempt |
| 49 — `rx868_spi_status_error` | SPI MARCSTATE/RXBYTES status read failed | Only on status read failure, no premature rearm |
| 50 — `rx868_fifo_read_error` | Fixed 63-byte RX FIFO read failed | After the existing recovery attempt |

Each compact event includes RFLink's monotone uptime (converted to UTC in CSV if synchronized), radio=868.30, warning/error severity, number of FIFO bytes observed, and the ESP-IDF status or recovery result (**0 = success** except where a status read itself failed). The optional per-event PSRAM detail records *reason*, observed MARCSTATE, raw RXBYTES status bits and recovery result. There are **no RF payload bytes, radio device IDs, credentials or captured pairing information** in these fault records.

No extra timers, separate task, logging in an ISR, extra SPI reads, queue allocations or per-frame log events. Callback registration occurs before the RX worker starts. A typical 30-overflow, ~5-hour run introduces about 30 compact System events, not one record for every 63-byte FIFO packet. The existing System ring is bounded to 512 records and retains its settings.

## Benchmark procedure after flash

1. Take an idle snapshot (radio listening, counters at zero or noted initial values).
2. Wait 1–4 hours while RFLink runs; export full CSV via fixed-length Builder 0.6.41 or directly via IP.
3. Filter **Radio → 868.30 MHz**, and also inspect **System**; search for `868 RX FIFO overflow`, `868 RX unexpectedly IDLE`, or `868 RX SPI status read failed`.
4. Compare timestamped event count with **`rx868_fifo_overflows`**, **`rx868_recoveries`**, **`rx868_spi_errors`** and zero SPI bus transfer errors. Record any missing events if more than 512 system events were evicted or the log buffers were reset.
5. Compare those timestamps with actual nearby RAMSES-II decodes and 868 unknown packet rates; coincidence does **not** prove causality, and a successful rearm does **not** recover a packet already lost in the overflow.
6. Repeat manual Orcon commands High and Auto; verify correct protocol decoding and absence of regressions.

Caution: an observation timestamp is at the **end of recovery**, not necessarily the first RF-overrun microsecond. The new data makes fault chronology visible; it does **not** fix FIFO overflow causes or increase receiver sensitivity. Those require additional hardware/driver testing. Both 433 and 868 RF acceptance gates remain open.
