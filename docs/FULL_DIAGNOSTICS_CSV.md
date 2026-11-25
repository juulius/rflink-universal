# Complete unfiltered RFLink diagnostics export

The existing Logs page keeps its single export button, now named Export full CSV. No independent log store, RF capture task, new page or secondary export endpoint was added.

## Why
The earlier CSV contained only small log metadata records and was scoped to the selected view filters. It omitted actual RF pulse values, FIFO bytes, PSRAM detail retention and system context. This was not enough for post-mortem RF analysis.

## Output format
An RFC4180 CSV with a fixed 26-column schema. Every row has a record_type:
- context: JSON in snapshot_json. The header includes schema rflink-diagnostics-csv-v2; then device_status with firmware version, uptime, RAM and PSRAM, OTA status, active radios, SPI, RMT and decoder counters; log_buffer_usage with the six log ring capacities/usage/overwrites and the six PSRAM detail arena capacities/usage/rejections/evictions; sanitized_crash_history (not raw core dump); export_cutoff; export_summary.
- event: all original sequence, uptime, source, radio, code, severity, count, error and correlation fields from the six log rings; available RF or text detail in detail_json and a detail_type. RF detail is the actual unmodified stored signed GDO2 pulse timing array, 868 MHz FIFO hex bytes, and any RSSI/LQI/FREQEST the driver recorded. A pre-filter RMT capture is clearly distinguished from a decoder-input sample.

- sync868_bit_block (new in Experimental): the same frozen, explicitly armed DATA+CLOCK bit blocks that were previously exported only as a separate protected CSV. Each row keeps the radio index 2, a `sync_bits_msb_hex` detail and JSON containing `generation`, `index`, `epoch`, `bit_count`, `first_us`, `last_us`, `discontinuity_before`, `window_end`, and `hex_msb`. The normal event rows and CSV columns are unchanged.
- context `sync868_capture`: reports whether a frozen RAW capture exists, its recorded-bit count and whether RAW rows were actually embedded. `export_summary.sync868_raw_embedded` explicitly reports inclusion.

**Using one file:** On the direct private-LAN RFLink Logs page, open **868.30 MHz · RAW bit capture**, enter the management key, arm a recording, operate the remote promptly and freeze. Then select the **normal Export full CSV button**. The browser verifies the key and appends `?sync868_raw=1` with the `X-RFLink-Update-Key` header only when a frozen capture is present. The server holds an immutable snapshot lease and streams one 2,048-bit block at a time to the same CSV, without changing RX mode or duplicating an entire capture in RAM.

**Boundaries:** A keyless/general `/api/logs.csv` request still exports all ordinary retained logs, but **not** protected synchronous RAW bytes. The general CSV records a visible `sync868_capture.raw_embedded=false` warning state, so absence is never mistaken for a clean capture. Remote Home Assistant Builder/Ingress exports do not automatically receive RAW, because they do not forward a private key. Only a *frozen and retained* ~13.7-second capture can be included; RFLink cannot reconstruct the previous hour of continuous 38.4 ksymbol/s bitstream from counters after the fact. Regular Radio events only include classified candidates.

No RF protocol or checksum is invented; absent detail is labelled no_capture_link, not_retained_or_overwritten, no_detail_or_overwritten, invalid_detail_length or detail_snapshot_unavailable. These are reasons, not proof that no signal was received.

## Always complete relative to retention
The export does not use browser tabs, radio frequency selection, Include noise or pagination. It visits all six buffers up to a fixed maximum sequence, using blocks of 16 records, and outputs all records that are STILL retained.

The device may overwrite an older event while exporting; this is a volatile, best-effort snapshot, not an atomic or lossless archival log. Context rows include overwritten and detail-evicted counts, and final summary reports detected sequence gaps. Neither flash memory nor the native radio/RMT ISR is used for export buffering. A reboot clears these RAM logs.

## HA Builder Ingress
The RFLink Builder now treats GET api/logs.csv as a special identity-verified, LAN-only HTTP stream. The regular 6 MiB buffered API response limit remains for other endpoints. The Builder forwards 32 KiB pieces with HTTP/1.1 chunk framing, and enforces a 96 MiB ceiling rather than silently delivering a truncated success. A download that loses the upstream connection is aborted, not marked complete. The new stream route is not a general-purpose LAN URL proxy.

## Privacy
Exports include sensitive RF payloads, which can expose device IDs and rolling-code material. They contain neither passwords, MQTT credentials, NVS secrets, OTA keys, unredacted logs nor raw core dumps. Treat this explicit CSV download as private diagnostic data. This feature does not change existing direct LAN access protections.

## Verification
- Test a known 433.92 MHz transmitter 10 times and export from the System tab with Include noise disabled. Radio, Noise, System and Web rows should all still be included, with linked raw timings and pre-filter RMT captures.
- On Experimental, arm/freeze a synchronous 868 RAW capture, enter the management key locally and use Export full CSV. Confirm `sync868_bit_block` rows reconstruct the same MSB-first bytes and boundaries as the protected stand-alone RAW export; verify keyless downloads omit those rows and label the omission. On RC/Stable the legacy FIFO hex remains available where still retained.
- Compare direct-LAN and authenticated Builder Ingress downloads, particularly files above 6 MiB.
- Observe heap, PSRAM, queue-drop, RX rearm and uptime counters while exporting.

## Analyze the combined CSV directly

The private offline tools accept either the stand-alone `RFLink_868_SyncBits_Bench.csv` or the new combined `RFLink_Diagnostics_Full.csv` containing authorized `sync868_bit_block` rows. No manual extraction required:

```bash
python firmware/tools/inspect_sync_bits.py RFLink_Diagnostics_Full.csv
python firmware/tools/replay_sync_bits.py RFLink_Diagnostics_Full.csv --try-inverted
```

This operation is local to your PC. Keep the CSV private: payloads may reveal RF identifiers, control commands or rolling-code content.

## Experimental 868 automatic RAW candidates and RX counters

Status: implemented in source; host regression and official ESP-IDF build gates are required. Physical RF validation remains pending.

The Status dashboard now counts recognized RX message events since boot per radio, not RMT captures, FIFO reads or continuous clock bits. RAW candidate totals are a separate counter; the same transmission may generate both a RAW candidate and a recognized message. Itho structural-only sync matches and nested RAMSES logical messages are not counted as recognized physical messages. On older firmware the new counters are unavailable, not zero.

For 433 MHz the existing RMT capture/filter pipeline retains longer pulse sequences (six or more durations, or saturation). For Experimental 868 MHz a separate low-priority consumer detects 32 alternating bits and retains an exact 504-bit MSB-first evidence window (63 packed bytes). The 8192-bit holdoff bounds capture writes. A partial window is discarded on sequence loss, epoch change, discontinuity or explicit window end; independent windows are never stitched. This is an experimental preamble heuristic, not universal packet framing, carrier/RSSI validation or a guarantee of complete messages. It may miss other preambles, short frames and transmissions during holdoff, and may capture interference. It does not change RF profiles, SPI, ISR, FIFO/Stable, decoders or TX.

The metadata uses the formerly inactive unknown FIFO ring (now 500 entries) in sync mode; payloads use its existing 256 KiB detail arena, separate from recognized 868 messages. The Logs viewer groups both buffers in each radio tile. The RAW record is code 55, not a FIFO packet; the exact retained payload is exported as `clocked_bits_hex` with a bit count. Open Logs → Radio → 868.30 MHz and expand a RAW candidate, export its RF JSON or use Export full CSV. Both the local interface and trusted Home Assistant Ingress use the same read-only logging access as 433 MHz, with no additional management key. The full CSV includes all retained automatic RAW details by default. Manual frozen bench capture remains independent and requires explicit authorization. Keep the RFLink HTTP interface on a trusted network.

Bench validation: record idle baseline, operate a known 868 control ten times with spaced presses, compare candidate and recognized counts, inspect exact windows and compare with a manual frozen capture. Repeat with both 433 receivers and verify Wi-Fi, queue drops, memory and retention. Do not interpret a preamble-window count as an RF message count. A future RF-energy/carrier-aware framing stage needs physical measurements first.

## Unified RX logging and retention (roadmap phases 1–2)

The 433 MHz receiver now finalizes a single ordinary receive log record after decoder dispatch. Recognized messages use Signal; unknown complete captures with at least six original and six decoder-input durations (including the synthetic idle tail), plus ambiguous matches, use RAW; short, filtered, saturated or otherwise incomplete captures use Noise. This conservative retention heuristic does not change the RMT filter, protocol decoders or receiver hardware. It is not yet a universal signal-quality classifier.

Original 433 timings are copied before filtering into a single optional 4 KiB PSRAM staging area owned by the existing RX worker. A finalized record retains the exact original waveform once; the decoder-input count, outcome and capture/decoder IDs are metadata. Noise metadata is bounded and tiny noise/detail sampling is limited per radio to 750 ms. Full CSV includes all still-retained original evidence without a management key. Ordinary forwarded/short/filtered counters are live telemetry; System retains saturation/queue-loss warnings and real driver errors. Decoder-specific diagnostic event counters remain available.

All three RAW metadata rings now retain 500 entries each (including legacy 868 unknown FIFO retention). The two 433 detail indexes each allow 750 entries; recognized 868 has 250, 868 RAW has 500, shared Noise has 2,000 and System/Web have 512 each. Byte arenas stay at 2,368 KiB with the same 5,274 detail-index entries; total including staging remains within the existing 30% PSRAM ceiling. Byte pressure and configured signal limits can still reduce actual detail retention. Known and unknown 433 share a band-specific byte arena; metadata rings remain independent.

Status now shows RX activity (RMT completion totals on 433, unavailable on 868 until RF burst detection), RAW candidates and RX messages. On 433 RAW candidates count unknown/ambiguous signals, not recognized messages; on 868 the existing heuristic still counts independent preamble windows. 868 automatic RAW and decoder hits are not yet correlated into one physical frame; this needs phase 3–4. See [the approved RX roadmap](RX_LOGGING_ROADMAP.md).

Validation: source changes and regression tests prepared; official ESP-IDF and physical board validation remain pending. Compare idle and controlled transmissions with exp.65, check exact original timings, counters, retention, queue drops, heap and CSV. No modem, SPI, TX, OTA or authentication changes.

## Phase 3: passive 868 RF activity telemetry (Experimental)

Status: source implementation; host, official ESP-IDF and physical validation required.

The sync RX worker reads CC1101 RSSI (0x34) and PKTSTATUS.CS (0x38 bit 6) after draining the existing DATA+CLOCK sampler, at most once per 10 ms. Existing modem registers are unchanged (MDMCFG2=0x00, no hardware sync-word detection). The shared SPI bus uses an immediate lock attempt and a 50 µs CHIP_RDYn budget per optional read; missed pairs are counted. No FIFO access, strobe, ISR change, decoder gating or new sampling task. The two register reads are not an atomic RF measurement. See TI CC1101 data sheet, sections 17.3 and 17.4 and status register descriptions: https://www.ti.com/lit/ds/symlink/cc1101.pdf.

Dashboard RX activity on 868 counts sampled carrier-high periods, not clock blocks or validated frames. One high sample starts a period; two low samples end it. A gap greater than 30 ms or RX restart breaks continuity; a first high after such a boundary is counted as a partial start, not a proven onset. Counts are boot-total. Actual sampling cadence depends on RX worker scheduling; short bursts may be missed or nearby bursts merged. Carrier sense uses the existing modem's AGC threshold; RSSI is an approximate dBm reading (signed register/2 minus nominal 74 dB offset), not calibrated signal power. RSSI is observational, not a new acceptance threshold.

Advanced RX diagnostics and the full CSV status context report sample count, carrier-high samples, missed pairs, partial starts, gap resets, maximum valid-sample gap, maximum read time, last sample uptime in microseconds, last/min/max RSSI (half-dBm units in the API), and carrier state. Missing current readings are null, and the dashboard shows unavailable until the first sample. No routine activity events are written to System, Noise or RAW. Existing RAW preamble capture and live decoders remain independent; correlation with real burst boundaries is phase 4. FIFO/RC/Stable RX is unchanged.

Board acceptance: compare exp.65/phase 1–2 baseline with passive listening and spaced known 868 transmissions. Check actual max gap, missed pairs, carrier-high proportion, RSSI range, 433 captures, sync queue losses and Wi-Fi. If carrier sense stays high at idle or misses known signals, diagnose before changing any thresholds or gating RAW. Do not interpret the new counter as a packet count or proof of complete reception.

## Phase 4a: descriptive RF evidence observations

Status: source implementation; host/ESP-IDF CI and hardware validation required. Phases 1–3 are merged; the phase 3 CI gates passed, but RF activity sampling still needs measured board evidence.

Open an existing retained RF row in Logs. The expanded RAW details display a short descriptive observation above the exact waveform; Export RF capture JSON and Export full CSV include the same versioned observation object (`rflink-signal-observation-v1`) when the original detail is available. No new log events, receiver task, retention change, hardware reads, decoder input change or management-key requirement. The firmware calculates features only on a copied detail snapshot in the existing HTTP/CSV read paths. Old firmware and the legacy small capture fallback may omit observations.

- **433 MHz pulse timing:** measured sum/min/max of absolute signed durations, high/low/zero counts, level changes and sub-30-us durations. Nonzero widths of at least 30 us are the timing denominator. Rounded 50-us width bins are bounded to 0–63 per polarity; widths outside those bins remain in the denominator. The two most frequent bins per polarity provide a descriptive support count. Fewer than eight timing samples or fewer than two high/low durations are insufficient; otherwise at least 75% support is labeled concentrated timing, less is varied timing. This describes timing concentration, not a calibrated signal-quality estimate, protocol recognition, repetition across frames, or a reason to discard a waveform. Decoder-input evidence can include a synthetic idle tail; original pre-filter captures remain unchanged.
- **868 MHz automatic 504-bit synchronous windows:** number of ones, transitions, alternating prefix length and longest constant run, plus independent statistics for the 472 bits following the first 32 trigger bits. The latter are labeled long constant run if a run covers at least half the tail, predominantly alternating if at least 90% of adjacent tail pairs differ, or mixed bits otherwise. These are descriptive patterns, not validation or complete frame boundaries. Legacy FIFO packets are not treated as clocked windows. Capture-specific RSSI/carrier correlation is not fabricated from a live or later status reading.

Use these observations to compare known transmissions with the passive baseline; assess actual carrier sampling gaps, RSSI range, candidate count, and queue losses from phase 3 before choosing any filtering threshold. Phase 4b will need bounded timestamp/epoch-aware carrier correlation and measured classification evidence; cross-capture repetition analysis and offline replay belong to phase 5. Keep RAW/Noise/Signal routing and all decoders unchanged until validated.

## Phase 4b: correlate automatic 868 RAW evidence with sampled RF activity

Status: source implementation, CI and physical validation pending. Phase 4a is merged with green CI.

Expand an automatic 868 RAW candidate in Logs to inspect the original bits, descriptive bit pattern and carrier context. The same optional `carrier_context` object is included in the per-capture API, RF JSON export and full diagnostics CSV. It records the RX epoch, estimated start/end in boot microseconds, valid sample count, carrier-high sample count, missed read pairs, first/last included read interval boundaries, approximate min/max RSSI in half-dBm units, first/last observed carrier activity IDs and whether the retained history starts late.

The clocked block records only its first and last measured edge times. Candidate boundaries are estimated by interpolation of bit positions between those endpoints, including a preamble split across adjacent blocks. Invalid endpoint timing is explicitly unavailable. The existing decoder task queries at most 64 already recorded RSSI/CS read pairs using an immediate attempt at the receiver mutex. It includes only pairs whose entire read interval falls within the estimated window and whose epoch matches. No extra SPI read, new task, ISR change or blocking history query. Delayed processing, lock contention or sparse sampling can leave no matching samples; this is not evidence of no RF. Missing RSSI is null, never zero or a current status reading. The two register reads are not atomic, and activity IDs describe sampled carrier periods, not verified RF frames.

Store a bounded trailer of at most 64 bytes with the existing 63-byte synchronous RAW payload in its current detail arena. Original bytes and hash remain unchanged. No extra retention ring, log event, buffer capacity change or authorization. The receiver retains a small fixed history (at most 2,304 bytes); no heap allocation in the query. Legacy FIFO/pulse storage is unchanged. Older payload-only sync evidence reports not recorded. The legacy `quality_state` still refers to packet quality metadata, not this separate approximate carrier context.

The labels describe observations only. No RAW-to-Noise routing, RF threshold, decoder gate or recognition change has been made. Correlation of individual protocol hits with captures and tuning classification still require measured board data. Validate with a passive baseline and known spaced 868 transmissions; compare valid/missed samples, timing estimates, activity IDs, RSSI range, queue losses and heap. A low or absent carrier sample never justifies rejecting a candidate by itself.
