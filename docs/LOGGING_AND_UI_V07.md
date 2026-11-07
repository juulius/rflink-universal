# v0.7 native logging and compact diagnostics — acceptance contract

This is the migration contract for **Radioactive v0.7**, not an obsolete
ESPHome v0.6 manual. Do not call a feature complete merely because the source
builds; confirm it on the three-radio hardware board.

## What the user should see

- **Status**: one compact health strip (radio status, RX/TX activity,
  firmware version and current warning). No full technical log on the homepage.
- **Logs**: one view with `All / Radio / System / Web` filtering. Radio
  events can be filtered by 433.42, 433.92 or 868.30 MHz. Noise can be
  hidden with one toggle. A selected event shows correlation ID, precise
  timestamp where available, action, reason and raw technical details.
  A transmit action is not labelled `sent on air` until independently verified.
- **System → Crashes and restarts**: prominently show the most recent reset
  reason, availability of the newest core dump, failing task and PC when
  available. Older boot reasons must survive ordinary restarts. Display
  `No crash recorded` when there is no evidence, never invent a stack trace.
  Detailed boot history can expand in place; it should not fill the page.
- **Settings**: simple, even visual density, with related controls grouped.
  The Logging card is **not** permitted to set the height of the adjacent
  Language, Status LED or Build information cards. Use independent card
  heights / top alignment or separate grid rows. On mobile, each card stacks
  cleanly without sideways scrolling.

## Storage and operational design

1. Three **separate** per-radio *signal* ring buffers, initially 250 entries
   each: 433.42, 433.92 and 868.30 MHz. Do not accidentally combine them.
2. One **shared physical noise** ring buffer, initially 2,000 entries across
   all radios. The viewer must still filter noise by radio **without copying**
   stored records into three duplicate noise buffers.
3. A default in-RAM signal/noise budget target of **1.5 MB** with a firm
   maximum defined and verified before enabling variable-size RF payloads.
   Shorten noise retention first when approaching the cap, then signal
   retention as a last resort. Show eviction/drop counts.
4. Adjust limits per radio and shared noise in the web UI without rebuilding.
   Validate against the memory cap and store settings in NVS. Do not write
   NVS for every incoming frame; settings only change on explicit save.
5. System events (boot, radio recovery, SPI failures, OTA, panic, watchdog,
   heap warnings) and web-management events (requests, user actions and
   failures) require bounded, independent logs. Never log secrets.
6. Preserve a bounded recent boot history in NVS and the most recent ELF
   **core dump in flash**. A panic or watchdog reset should be recognized
   on the next startup even with Home Assistant offline. Record the exact
   firmware/build identity needed to decode a backtrace. Persist summaries
   separately from the core dump; subsequent normal restarts must not be
   mislabelled as repeat crashes.
7. Flash core dumps can include stack data and credentials. Raw core dump
   downloads must be authenticated and never silently uploaded to cloud
   services; offer a redacted diagnostic export first.
8. Support chronological signal/system/web export with timestamps, radio,
   severity, type and correlation ID. Real-time viewers should be bounded
   and should not endlessly scroll as new entries arrive.

## Settings layout — compact by design

| Card | Main content | Advanced content |
| --- | --- | --- |
| General | Language, LED mode | Localization and LED diagnostics |
| Logging | Signal limit for 3 radios, shared noise limit, Save | Per-source filtering, drops, memory cost |
| System | Last restart, last crash status, View report | Boot history and flash summary |
| Build info | Short version, codename, short source hash | Compile metadata, change notes |

**Rules**: related settings in a single card; no empty jumbo cards;
independent rows or intrinsic-height cards; save confirmation adjacent to
the control; at most three clicks to common functions. Keep System crashes
visible without opening Settings. Destructive actions require confirmation.

## Current implementation and remaining gates

- **Stage 23 initial implementation**: ESP-IDF flash ELF core dumps enabled
  by default on dedicated v0.7 test builds; bounded boot/reset history
  recorded once on startup; crash task and PC exposed through a sanitized
  local diagnostic endpoint; a small crash/restart card is shown in the
  prototype web page. Raw crash stacks are deliberately never exposed by
  that endpoint. Host tests and official IDF compile are required; physical
  panic/watchdog/OTA tests are not yet passed.
- **Not yet migrated**: full per-radio signal/noise log engine, limits and
  NVS controls, separate System and Web event streams, raw crash export
  with authentication, correlation and chronological ZIP export, and the
  final compact multilingual dark UI.
- **Required before general release**: cold/warm reboot and power-loss
  preservation, deliberate controlled panic on a sacrificial test board,
  crash capture/summary with real firmware ELF, multiple watchdog failures,
  brownout classification, OTA rollback with preserved crash history and
  measurement of PSRAM, flash wear, NVS and heap. Never intentionally
  trigger a panic on a working reference device.
