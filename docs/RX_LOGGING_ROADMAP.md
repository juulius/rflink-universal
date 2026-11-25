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

**Status:** phases 1–2 implemented in source on the development PR; host/ESP-IDF CI and physical acceptance pending. Phases 3–7 remain planned.
