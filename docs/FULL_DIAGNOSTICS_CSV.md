# Complete unfiltered RFLink diagnostics export

The existing Logs page keeps its single export button, now named Export full CSV. No independent log store, RF capture task, new page or secondary export endpoint was added.

## Why
The earlier CSV contained only small log metadata records and was scoped to the selected view filters. It omitted actual RF pulse values, FIFO bytes, PSRAM detail retention and system context. This was not enough for post-mortem RF analysis.

## Output format
An RFC4180 CSV with a fixed 24-column schema. Every row has a record_type:
- context: JSON in snapshot_json. The header includes schema rflink-diagnostics-csv-v2; then device_status with firmware version, uptime, RAM and PSRAM, OTA status, active radios, SPI, RMT and decoder counters; log_buffer_usage with the six log ring capacities/usage/overwrites and the six PSRAM detail arena capacities/usage/rejections/evictions; sanitized_crash_history (not raw core dump); export_cutoff; export_summary.
- event: all original sequence, uptime, source, radio, code, severity, count, error and correlation fields from the six log rings; available RF or text detail in detail_json and a detail_type. RF detail is the actual unmodified stored signed GDO2 pulse timing array, 868 MHz FIFO hex bytes, and any RSSI/LQI/FREQEST the driver recorded. A pre-filter RMT capture is clearly distinguished from a decoder-input sample.

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
- Export 868.30 MHz records and check actual 63-byte FIFO hex (where still retained) and the frequency/quality metadata. A missing preamble is not automatically an Itho/RAMSES message.
- Compare direct-LAN and authenticated Builder Ingress downloads, particularly files above 6 MiB.
- Observe heap, PSRAM, queue-drop, RX rearm and uptime counters while exporting.
