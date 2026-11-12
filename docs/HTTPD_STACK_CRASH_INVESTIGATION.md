# exp.40 repeated httpd software panics — safer status and export stack

Reported crash history on the real board: boots 23–29 reset with task `httpd` (PC mainly 0x40380a4d), boot 30 with task `wifi` and PC 0x0a2d0005. All recorded phases were `app_ready`. The user confirmed OTA had completed correctly; **no crash timestamp** and no full stack trace is present in the sanitized crash report. Thus a status/export stack overflow is a plausible hazard, **not a proven root cause**.

## Verified source-level hazard
The live `/api/status` callback used a 4096-byte local char array for a giant `snprintf` format, yet `httpd` had only 8192 bytes of total stack. This callback is polled repeatedly by the Builder, and the full CSV explicitly invokes the same function to embed system metadata. Other stack frames, FreeRTOS/HTTP TLS and formatting spill could exceed remaining headroom. `snprintf` could silently truncate the detailed JSON. In addition, the CSV handler keeps 16 small log records on the HTTP stack.

## Change
- Move the status formatting buffer off the task stack into a per-call **8 KiB PSRAM allocation**, with bounded internal heap fallback for early diagnosis. Never use a static shared buffer. Check the actual `snprintf` length, reject truncated JSON and release the scratch allocation after copying into the result string.
- Increase the single HTTP server task stack from 8 KiB to 16 KiB (small additional internal RAM cost), leaving RX driver and ISR stacks untouched.
- Expose `httpd_stack_min_free_bytes` in `/api/status`. This is the low watermark **of the current HTTP task** at the time of measurement. It is also included by the existing full CSV's device_status context; it is not a historical crash timestamp.
- Keep the same status API, complete unfiltered CSV endpoint, six compact/detail PSRAM arenas, radio protocols, OTA contract and crash storage.

## Verification
Run host regression, native IDF build and preflight, then test on a separate ESP32-S3 N16R8 board. Before testing full CSV, confirm regular status polling for 15 minutes does not reset the device. Export the CSV first with few retained records and then full buffers; monitor `httpd_stack_min_free_bytes`, internal heap low-water mark and PSRAM use; repeat over direct LAN and authenticated Builder Ingress. Avoid continuous repeat tests if it crashes; preserve the crash JSON and original matching ELF for symbolization. OTA is not implicated solely by boot numbering, and the existing journal records no timestamps for older boots.
