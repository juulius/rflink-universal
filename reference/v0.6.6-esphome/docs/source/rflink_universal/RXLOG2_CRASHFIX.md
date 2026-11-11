# RXLOG2 – Crash / noise viewer hotfix (2026-10-25)

## Symptoms and verified cause

- ESPHome 2026.10.0 ESP32-S3 rebooted with **Abort** in `__cxa_allocate_exception` / `std::basic_stringbuf::overflow` from `RFLinkWebHandler::rx_log_json_()` (previous line 2285).
- Original RXLOG1 assembled 40–100 full RX records into a contiguous `std::ostringstream` and then passed the result into an additional HTTP response string. Large temporary internal heap allocations could fail even while overall **free RAM including PSRAM** was around 7 MB.
- `record_activity_()` also emitted malformed JSON: missing `"` after `summary` before `detail`, making received RX records impossible to parse even without a reboot.

## Changes

- **ESP-IDF only:** `/rflink/api/rx-log` uses `httpd_resp_send_chunk` with at most 1024-byte network segments, instead of `std::ostringstream` or ESPHome's `beginResponseStream()` (which also concatenates into a single string).
- Response retrieval first reads lightweight metadata only; event bodies are copied one by one into a caller-owned 32 KiB **PSRAM** scratch buffer, protected by the existing RX log mutex for the copy only. Network writes happen **without** holding the mutex. Scratch memory is released at the end.
- HTTP response is paginated (default 20 events, maximum 50). A one-row lookahead sets `has_more` correctly; `next_cursor` is based on last delivered sequence. No RAW-event truncation is introduced. If a row was evicted during transfer, it is represented as `record:null, expired:true` rather than breaking response JSON.
- UI reads 20-event pages and honors `has_more`; background polling and changing filters cannot overwrite results from a newer request. Export reads 50-event pages, traverses full 4096-event maximum, and skips records evicted during export.
- UI shows API failures explicitly rather than silently showing an empty activity list.
- Corrected the missing JSON quotation mark between `summary` and `detail` in the RX record writer.
- **Unchanged:** YAML, GPIO, radio/CC1101 setup, decoder registry, signal/noise retention limits, NVS settings, reboot mitigation, LED, MQTT, device library.

## Tested

- Host C++ logging tests: 30 KB RAW event exact copy from fixed arena to temporary buffer; 3500 simulated noise frames; preserved signal history, pagination and noise eviction priority.
- C++ web handler compile and vtable regression with provided ESPHome host stubs, C++ logging core and Python static integration tests.
- Playwright browser smoke: noise / radio filtering, NVS settings, ZIP export; compact settings DOM and four locale keys.
- **NOT YET VERIFIED:** full ESPHome 2026.10.0 / ESP-IDF 5.5.6 toolchain build, OTA upload, real RAM fragmentation behavior and uptime on the user's ESP32-S3. This package is a **source** hotfix, not a verified precompiled binary.

## Installation and validation

1. Back up the currently working YAML and project folder. **Retain your locally working reboot/YAML changes** if different from the YAML shipped in this package.
2. Replace the matching `rflink_universal/` folder from this ZIP; regenerate the embedded web UI with the normal release script if your build flow requires it (the package already includes refreshed `web_ui.h`).
3. Compile in ESPHome, inspect errors, then OTA flash only if compile succeeded. Do not update NVS manually.
4. Open the monitoring page and select `Ruis` with `Alle radio's`, and then each radio. Verify records can be expanded and downloaded and do not show a blank screen.
5. Leave it running for at least 12–24 hours; test sustained 868-MHz noise, Wi-Fi reconnects and the ZIP export while logging continues. Watch boot count, reset reason, **internal free heap / largest internal free block**, PSRAM free, and log evictions.
6. If a problem returns, stop frequent opening of Monitor (the former crash path), use the previously stable firmware as fallback and share the new backtrace and diagnostics.

These changes fix the two code-level causes identified from the submitted backtrace; an on-device stability result cannot be guaranteed until physical testing.
