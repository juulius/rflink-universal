# RFLink Universal — Offline Replay Engine v1

## A/B capture identity fix (Builder 0.6.45)

A physical event's boot, event sequence, uptime, radio, capture phase and exact RF payload do not change between two diagnostic exports. The CSV's `event_utc` and RF detail `timestamp_utc_ms` are reconstructed at export using the ESP32's current SNTP/boot epoch and can move by a small number of milliseconds. Because the former Replay capture ID hashed `event_utc`, identical frames in A/B sometimes had different IDs and appeared only in A or B.

The new `capture_id_scheme: stable-source-v2` hashes normalized RF data plus stable event fields; it intentionally ignores the reconstructed UTC labels. The report schema stays `rflink-offline-replay-v1`. A sanitized report produced with a legacy ID scheme does not carry enough RAW evidence to reconstruct the new ID; re-run Replay using the original private RAW capture bundle or CSV if available. UI warns if either A or B lacks the new scheme.

A single 433 pre/post-filter pair still represents **one** candidate physical frame. Singleton waveform buckets no longer advertise a group label on rows; `group_count >= 2` means at least two separate physical frame candidates had similar timing, **not** that a protocol or device was verified. No hardware or decoder behavior is changed.

## 433 timing similarity and input-stage correlation (Builder v0.6.44)

Replay emits an additive `rf433_insights` report object. It links original `pre_filter_rmt` and filtered `decoder_input` events only when radio, frequency, boot, uptime/UTC and consecutive log sequences match. It reports separate signed-duration counts and their numeric delta: this is **not** a loss-of-physical-pulses metric. For grouping it takes only the decoder-input representative of a matched pair; an unpaired original or decoder event counts as a separate candidate. At least 24 valid alternating signed durations are required before a conservative timing-similarity check; synthetic idle termination is stripped only where clearly distinguishable. Exact waveform lengths, levels, median timing and per-symbol jitter must agree. Search is bounded to 64 representative groups per compatible bucket.

A returned timing group is an *unverified repeat candidate*, not a protocol, device ID or a decoder recipe. Short/irregular RF samples are not grouped, but remain in their original replay rows. The original 433 captured pulses are not stored in sanitized reports; the optional private capture bundle retains raw samples only by explicit user request. Built-in 868 packet handling remains unchanged.

Roadmap: [#163](https://github.com/juulius/rflink-universal/issues/163). **Read-only offline milestone**: no radio transmission, CC1101 access, OTA, firmware changes or network connections.

## Run from the repository root

Requires Python 3.10+ and a C++17 compiler (`g++` or `clang++`) for the 868 MHz built-in decoders. No Python packages are required.

```bash
python3 tools/rf_replay.py RFLink_Diagnostics_Full.csv --report replay_report.json
```

The report contains a versioned JSON schema, input coverage, capture IDs, frequency/boot/time/sequence metadata, optional quality measurements and replay verdicts. It **omits raw 868 payload bytes, raw 433 pulse arrays and device identities**. A temporary C++ executable is compiled from the same RAMSES-II/Itho source files used by the native firmware, then removed.

If a C++ compiler is unavailable, use `--structural-only`. 433 timing checks still run, but 868 results explicitly say `native_decoder_not_run` and cannot be mistaken for protocol rejections.

```bash
python3 tools/rf_replay.py RFLink_Diagnostics_Full.csv --structural-only --report structural.json
```

Supported inputs: complete RFLink v2 CSV exports, standalone RF capture JSON from the Logs panel, and the versioned replay bundle produced by this tool.

## Preserve captured signals for future decoder comparisons

```bash
python3 tools/rf_replay.py RFLink_Diagnostics_Full.csv --report before.json --save-captures private_capture_bundle.json
python3 tools/rf_replay.py private_capture_bundle.json --report after.json
```

**Privacy:** `--save-captures` writes complete RF payloads and signed pulses, which can include device IDs or pairing/rolling data. Keep the bundle private, do not commit it to GitHub and do not share it unredacted. File permissions are restricted when possible, but content is not encrypted. No actual user's RF data is included in this repository.

## Decoder behavior

- **868.2999 MHz, 63-byte 2-FSK FIFO:** check the legacy `00 B3 2A AB 2A` prefix, then execute the repository's native C++ RAMSES-II UART/Manchester/checksum validation and Itho built-in decoder. Preserve firmware precedence when both pass. Results include `legacy_preamble_absent`, `built_in_decoder_rejected` or a built-in accepted protocol. Installed user recipes are not included, so a built-in rejection does not mean every possible decoder rejected the packet.
- **433.42 / 433.92 MHz signed RMT durations:** derive tentative pulse-width (PWM), pulse-distance (PPM) or Manchester timing/bit hypotheses where sufficient, stable alternating durations exist. Both Manchester polarities are returned as hypotheses. None of these candidates is a proven protocol, address or valid checksum. Short captures are explicitly `insufficient_signed_pulses`.
- **No invented samples:** a missing or evicted RF detail is counted, not synthesized. No sampling or additional RX/TX is attempted. RSSI/LQI/FREQEST remain estimated hardware status; the 868 profile has **CRC disabled**, so LQI is never labelled CRC verification.
- **Bounds:** input max 96 MiB, max 20,000 captures, 868 FIFO captures must be exactly 63 bytes, RMT pulse arrays 1–1024 signed durations. Bad data reports errors without echoing RF bytes. The report includes truncation counts if the capture limit is reached.

## Replay bundle schema

An optional private replay bundle uses `schema: rflink-replay-captures-v1` and `captures: [...]`. Each capture includes `radio`, `frequency_hz`, `format`, `capture_phase`, `count`, the exact `hex` or `pulses_us`, optional quality/UTC/uptime/boot/sequence, and a stable `capture_id`. Report IDs are SHA-256 content/metadata fingerprints, **not device identities**.

Example below is fabricated and contains no actual user/device capture:

```json
{
  "schema": "rflink-replay-captures-v1",
  "captures": [
    { "radio": 1, "frequency_hz": 433920000, "format": "signed_pulses_us",
      "capture_phase": "pre_filter_rmt", "boot_id": "35", "sequence": "42",
      "count": 4, "pulses_us": [330, -400, 680, -400] }
  ]
}
```

## Milestone boundaries / next work

- [x] Shared offline CSV and RF capture JSON ingestion for all three radios
- [x] Native RAMSES-II/Itho decoder reuse, deterministic synthetic protocol fixtures and negative controls
- [x] Timing-only 433 candidate decoding, with explicit unverified labels
- [x] Privacy-sensitive raw bundle is opt-in; default report sanitized
- [ ] Physical 433.92 MHz doorbell test (five captures) and 868 Orcon High/Auto confirmation
- [ ] More granular rejection traces, decoder version comparisons and optional on-Builder UI
- [ ] Optional 868 digital RAW/GDO mode only after separate hardware validation; do not disturb working FIFO profile

Source validation alone does not prove RF reception quality. Both physical Gate 1 checks from roadmap #163 remain open.
