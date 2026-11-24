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
