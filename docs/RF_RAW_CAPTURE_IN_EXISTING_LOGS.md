# Raw 433 MHz RF evidence in existing Logs (native v0.7)

This is an incremental improvement to the existing six-buffer LogStore/DetailStore, **not** a separate RF tool, diagnostic page, or persistence path. Main UI path: **Logs → Radio → 433.92 MHz → Include noise**, expand the `433 raw RMT capture (pre-filter)` event.

## Why regular CSV lacks the signal
`/api/logs.csv` intentionally contains only timestamps, counters, transport and correlation IDs; it does not include raw RF IDs, rolling-code counters or waveform samples. The existing `/api/radio/capture` endpoint already returns full signed GDO2 microsecond timings and 868 FIFO hex for decoded/forwarded events. But before this patch, 433 pulses rejected by the decoder (too short/filtered/full) never reached that storage.

## New evidence path
1. Immediately after the RMT ISR reparms a free slot, the worker copies a completed RMT capture into its existing receiver-owned scratch; it releases the completed slot before any decoding or logging.
2. A worker-only observer sees the original unfiltered signed microseconds before the 30us cleanup and 5ms synthetic idle tail. No callback from ISR, no extra task, no extra RMT resources.
3. The observer appends a compact **noise** entry with code 46 to the **existing** LogStore. Exact sample durations are saved in the same **radio-specific** PSRAM DetailStore arenas as other RF details (independently for 433.42 and 433.92), using a dedicated high-bit key namespace to avoid colliding with decoder event sequences.
4. Tiny captures (<=2 duration values) are sampled at most once per 750ms **per radio** to avoid flooding the shared compact noise log. Full/incomplete frames are still retained as original physical samples; never claim they are decodable.
5. Expanding the row reuses `GET /api/radio/capture?radio=1&seq=<log_sequence>&pre=1`; no new HTTP endpoint. It shows exact signed durations, high/low counts and binned duration summary (bins are descriptive, not a guessed protocol).
6. **Export RF capture JSON** is an explicit browser-local download. The exported file includes exact `pulses_us` (or 868 `hex` when applicable), firmware/build if known, radio/frequency, uptime, hash and a format/phase marker, allowing offline analysis in a future chat. Neither normal CSV nor web audit includes waveform or rolling-key bytes.

## Preserved
- Dual non-DMA 96-symbol RMT, 512-symbol host buffers, 3 immutable completion slots/radio, FIFO newest-drop policy, ISR rearm, worker sleep/watchdog, RX/TX lock ownership, SPI/readback, OTA, crash logs.
- Existing memory ceilings (30% PSRAM for details), six physically separate arenas, compact log retention user settings, existing Logs UI and its automatic live-pause while inspecting details.
- The 868 MHz packet receiver is unchanged. No new radio protocol decoder is claimed.

## Bench checks
Install the new Experimental build; enable Include noise for a 433 radio. Press a known transmitter 10 times. Verify both a `433 raw RMT capture` and (if valid) a `Raw pulses` forwarded event; inspect/export at least one JSON capture. Compare widths, repetitions, RMT symbol count, and any incomplete records. Confirm 868 and other radio buffers remain available. An export contains RF device identity information: do not share it publicly without consideration.
