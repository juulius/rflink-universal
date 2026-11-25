# Unified RFLink RX logging and detection roadmap

Agreed development plan (2026-11-25). The three CC1101 radios use the same logical RX categories and dashboard counters, while their actual receive transports differ. Keep the firmware local, use the same local and trusted Ingress UI, and never gate existing decoders on an unvalidated classification heuristic.

| Phase | Scope | Acceptance |
| --- | --- | --- |
| 1 | Consolidate reception logging; separate Noise, RAW and Signal; remove routine System summaries | One ordinary receive record per 433 RMT capture, original RF evidence retained once; real driver failures remain observable |
| 2 | Bounded retention | RAW rings 500 per radio; Signal rings 250 each by default; shared Noise 2,000; System and Web 512 each; better use the existing byte arenas and stay within 30% PSRAM for details |
| 3 | 868 RX activity | Investigate CC1101 RSSI/carrier sense; count distinct RF activity periods, not continuously clocked bits; measure sampling gaps, shared SPI cost and RX losses |
| 4 | Generic signal classification | Observe timing, structure, completeness, RSSI/carrier where available and repetition. Correlate real 868 bursts with RAW evidence and protocol outcomes; tune only against measured data |
| 5 | Offline replay | Analyze exact original 433 timings and automatic 868 bit windows; preserve provenance, bit boundaries and unavailable evidence markers. Group repeated captures without silently losing distinct payloads |
| 6 | Hardware validation | Passive baseline, controlled transmissions on each radio, missed captures and counter accuracy, queue losses, memory/CPU/Wi-Fi/OTA stability; compare to exp.65 |
| 7 | Decoder and device expansion | Migrate and improve v0.6.6 protocols, expand user decoder/device tooling and verify on real hardware |

## Interpretation

RX activity counts acquisition events (433 RMT completions; 868 RF burst detection is pending). RAW candidates are unknown or ambiguous captures worth investigation, not proven messages. RX messages are recognized decoder events. 868 preamble evidence and native decoder hits are currently independent; do not add their totals as a count of unique physical messages.

For phases 1–2, a complete unknown 433 capture with at least six original and six decoder-input durations is retained as a RAW candidate; ambiguous matches also qualify. Recognized messages take precedence even if short. Short, filtered or incomplete captures go to Noise, with compact metadata and limited original-detail samples. This is a provisional conservative policy, not a claim of reliable RF structure classification. No changes to the actual receiver or decoder inputs.

The ordinary diagnostic export includes still-retained RAW evidence and sampled Noise for all radios without a management key. Keep separate manual frozen bench controls and OTA security unchanged. No personal captured RF data belongs in GitHub.

**Status:** phases 1–2 implemented in source on the development PR; host/ESP-IDF CI and physical acceptance pending. Phase 3 is merged with green CI; phase 4a is merged with green CI. Phase 4b adds observational carrier context; classification tuning and phases 5–7 remain planned.

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
