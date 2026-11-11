# RFLink Universal RXLOG1 – runtime logging

## Defaults

| Store | Entries |
|---|---:|
| 433.42 MHz signals | 250 |
| 433.92 MHz signals | 250 |
| 868.30 MHz signals | 250 |
| All-radio noise pool | 2,000 |

One noise queue shared by all radios, but the Monitor can filter noise per radio. All log events are held in RAM; reboot clears the events. Limits survive reboot in ESP-IDF NVS. The oldest noise samples are sacrificed first under memory pressure (a soft 50-event radio noise preference is used). Signals are only discarded when required to meet their own cap or the hard byte budget. A very large event is rejected whole rather than silently truncated.

## Website

Open `/rflink`: Monitor → select Signal/Noise/All and frequency, view newest first and load older events. The ZIP export retrieves currently retained data over paginated read-only requests and builds files in the browser. Settings → Logging: choose max signals for each radio and the one shared noise cap; save without reflashing/reboot. Counts cannot exceed 4,096 combined. The log byte budget is compile-time; it is not an NVS runtime setting.

## Hardware / memory considerations

The firmware tries to allocate a fixed arena solely in PSRAM: 1536 KiB, then 1024 KiB, then 512 KiB if required. Logs are disabled if PSRAM allocation fails, but other functions continue running. The allocation is fixed after boot and only events inside the arena are evicted. The legacy 32-entry activity ring remains available to existing status/TX integrations, but when the new RX logger is running its duplicate full RX exports are reduced to short previews.

Two counters matter: `evicted_limit` (configured count cap) and `evicted_memory` (hard arena limit); `rejected` counts oversized events, while `dropped_busy` counts events skipped during contention with the HTTP reader. There is no persistent SD/flash logging. The `uptime_ms` and `seq` fields are per-boot. RTC timestamps are not asserted unless actual device wall-clock synchronization becomes available. Browser ZIP export is not an atomic snapshot: during high-volume live traffic old samples can expire between pages.

## HTTP read API

- `GET /rflink/api/rx-log-config` current limits, used capacities, drops and free-memory diagnostics
- `POST /rflink/api/rx-log-config` form values `signal_0`, `signal_1`, `signal_2`, `noise`, `write_token` (and optional `noise_floor`)
- `GET /rflink/api/rx-log?order=desc&limit=80&radio=0&kind=1` returns recent noise on 433.42 MHz. `radio` and `kind` are optional. `cursor` is a per-boot sequence number; `order=asc` useful for export.

The write token is device-local and should only be sent to the device over a trusted LAN/TLS reverse proxy.

## Status

This is a source release for integration testing. The full ESPHome ESP-IDF firmware build, actual OTA, CC1101 receive flood, free internal heap and long-run reboot stability are still to be verified on a physical ESP32-S3 N16R8.
