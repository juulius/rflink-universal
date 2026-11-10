# v0.7 native logging and compact diagnostics — acceptance contract

This is the migration contract for **Radioactive v0.7**, not an obsolete
ESPHome v0.6 manual. Do not call a feature complete merely because the source
builds; confirm it on the three-radio hardware board.

## What the user should see

- **Status**: one compact health strip (radio status, RX/TX activity,
  firmware version and current warning). No full technical log on the homepage.
- **Logs**: one view with `All / Radio / Noise / System / Web` filtering. Radio
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

## Stage 24 host implementation and limits

- Original Stage 24 shipped six independent bounded ring buffers: three radio signal rings (250 records each), one **shared** noise ring (2,000), System (128) and Web (128). The later full-history/audit change increases System and Web to **512 each**. The noise log has a radio attribute and filters without triplicating memory. A record is fixed-size and contains only numeric event metadata; it does **not** expose recorded RF bytes or private device identity.
- Log buffers are allocated in external PSRAM and cannot exceed the 1.5 MiB combined budget. A reconfiguration preserves the newest records; a memory allocation failure leaves the old configuration untouched. Once full, the oldest entries are overwritten and reported as a counter.
- The original Stage 24 `GET /api/logs` showed the latest 48 filtered events (All/Signal/Noise/System/Web and per radio), with noise hidden by default. `GET /api/logs/config` reports current limits, usage and overwrites. `POST /api/logs/config` requires the existing local management key and explicitly saves the new limits in NVS. The prototype page has an independent-height, compact retention card with visible save feedback.
- Boot and radio restart/recovery events are wired to the System ring; decoded Itho and RAMSES to the 868 signal ring. Unclassified 868 packets and raw 433 pulses go to the shared Noise ring until the relevant 433 protocol decoders are migrated. Limited Wi-Fi and settings actions reach the System/Web rings. **There is still no authorized web TX workflow**, so no real send traces yet.
- Uptime milliseconds are labelled as uptime: real clock time, source/RSSI enrichment, full replayable captures, correlation IDs for asynchronous command chains and paged/CSV/ZIP export still require implementation. Core dump and reset history are preserved separately from volatile event logs.
- All access is LAN-only plaintext HTTP in this prototype. **Do not expose this firmware UI to the Internet.** A management-key-protected POST is not equivalent to TLS transport.

## Complete retained history and Web audit (new v0.7 development)

- **Visibility, not infinite retention:** The Logs viewer now requests up to **64 rows per page**, with Latest / Newer / Older controls and a stable sequence snapshot while navigating. No 48-row ceiling remains. Every record still held in the three 250-entry signal buffers, shared 2,000-entry noise buffer and separate 512-entry System and 512-entry Web rings is accessible via the UI or the matching full CSV export.
- **Snapshot caveat:** A fixed sequence cursor keeps pages from duplicating newly arrived events. Because these rings are volatile and bounded, older records can still disappear if their *own* ring fills while browsing. Download CSV promptly when forensic history matters; complete lossless archives would require dedicated persistent storage or an external collector.
- **Strict isolation:** Six ring allocations, heads, counts and overwrite counters are independent. A full or resized noise ring cannot evict Radio, System or Web entries. Within any full ring, *that ring's own oldest* entries will still be overwritten: this is bounded volatile PSRAM, not a lossless archive. Neither page navigation nor exports clear rings. Overwrite counts are shown per ring; reboot clears volatile events.
- **Sources:** Radio combines per-frequency signal events with related System radio health/start/stop/failure messages and optionally Noise. Noise explicitly shows the shared noise ring; All, System and Web are independent choices. The CSV endpoint implements the same source/noise/frequency filter as the UI.
- **Web records:** Frontend page navigation, source/noise/radio filter changes, expanded log details, CSV exports, pagination, console actions, refresh and Save/OTA actions are sent as bounded allowlisted numeric events to the firmware. The HTTP server records failed API responses (including 404/403/400/500) from *any caller* and successful state-changing POST operations, but not every successful five-second GET polling request. Browser-reported HTTP/network errors are rate-limited. Endpoint IDs, HTTP method/status, page/action IDs and uptime may be expanded in the log. The firmware does **not** store full URLs, query parameters, passwords, body content, captured RF payloads or keys in these records.
- **Radio lifecycle:** Initial receiver start/driver failure, watchdog stop/start/recovery and CC1101 probe/loss are recorded in the System ring, and surfaced in the Radio view. The page displays uptime down to milliseconds. Driver acceptance still does not prove over-the-air radio communication.
- **Test gate:** C++ host tests verify six separate full buffers, noise-only overwrite, radio/system filtering and equivalence between cursor pages and CSV paging. The web/HTTP regression suite checks that route capacity covers every registered endpoint, including the frontend-only audit collector. GitHub native ESP-IDF compilation and bench tests must succeed before these changes are considered hardware-ready.

## Stage 25 TX diagnostics and CSV migration

- Every call to the **actual** universal RadioManager transmit_encoded() path creates a new correlation ID, including rejected attempts. A simple request_transmit() eligibility check is deliberately not a physical TX event.
- Where preflight succeeds, a started event and a completion event share that ID. Logs record the **radio**, **selected RF profile**, **transport** (UART, RMT or packet/FIFO), **number of transmitted data units**, **repeats**, **manager error**, **physical TX failure stage**, **whether the driver was called**, **whether it returned success**, and **whether the exact previous RX profile was restarted**. No raw TX buffer or private pairing/rolling code is logged.
- The driver returning success is labelled **driver accepted (RF not verified)**, never "transmitted on air." A failed RX pause records that no driver send was attempted. Missing RX restoration is a prominent separate failure.
- `GET /api/logs.csv` exports filtered numeric/enum metadata from all six in-memory rings as chronological CSV, including TX correlation and failure stages. It pages in **24-record** chunks without holding the RF mutex during HTTP network writes. A fixed highest sequence at download start prevents unbounded export if new RF events arrive continuously.
- At Stage 25, the regular log view displayed the newest 48 events and filtered by stream, radio and noise. The later full-history implementation replaces that fixed window with cursor pages. Export uses the exact same source/noise/radio filter. **This is not a stable forensic snapshot**: old volatile ring entries can be overwritten during a long download. Full RF captures, persisted event history, real timestamps, and authenticated raw crash-dump export remain future work.
- This stage does **not** introduce a web TX button or trigger real transmissions. The manager path must be invoked by an authorized, later-implemented pairing/device-control workflow before these traces appear during ordinary use. Tests confirm all three transport families, host compilation and CI; on-air RF and crash tests on the actual board are pending.

## Stage 26 crash image attribution (development)

- The native crash summary contains the **crashing application's ELF SHA-256**. The API now shows this separately from the **currently running firmware version and ELF SHA-256**. This avoids claiming that a retained dump from before an OTA belongs to the currently running image.
- Some ESP-IDF builds retain only a **prefix** of the ELF hash in the core dump; matching a prefix is explicitly labelled *matching_prefix*, not full-hash verification. Unknown, different, and fully matching identities have distinct states. Malformed/un-terminated SHA data is rejected.
- Show whether stored crash data passed the flash integrity check, is missing, or is invalid/unreadable. A saved crash can remain after an ordinary restart; the page must **not** imply it happened on the latest boot.
- The prototype UI keeps **Last restart / Stored crash report / Crash firmware** in small cards, with details in one expandable section. A JSON download provides only the sanitized reset/PC/task/build metadata, never stack memory, credentials, raw RF frames or private keys.
- Crash summaries and boot history still need deliberate recovery testing on a **separate** ESP32-S3. Hash comparison alone does not give a complete decoded backtrace; the exact historical ELF is required for reliable symbolication. No automatic upload, telemetry or cloud crash reporting is enabled.

## Stage 28 early management/diagnosis lifecycle (development)

- Native firmware boot order is **NVS and crash history → bounded logs → Wi-Fi and HTTP diagnosis/OTA → CC1101 SPI probe → RX startup → MQTT and pending OTA health verification → app-ready marker**. Network connectivity is asynchronous. HTTP may already answer before the radio initialization completes, but Wi-Fi and the watchdog may still race a fatal crash.
- For HTTP handlers, all radio-status pointers and the initial radio snapshot are published as a single release/acquire boundary. Before publication, radio_initialized=false; radio status and event counts are not presented as valid hardware observations, but /api/status, /api/diagnostics/crashes and /api/logs remain available if TCP/IP is up.
- startup_complete becomes true only after reaching the end of normal boot or validating a pending OTA slot; it is not proof of successful RF reception, transmission or WAN reachability. The Builder uses that explicit signal to avoid prematurely auto-linking an ESP32 that only reached early diagnostics.
- The Stage 27 NVS enum values remain unchanged for backward compatibility; an explicit progress rank permits starting networking before radio probing without changing stored historic phase labels. Phase journaling is bounded to a few startup writes and never captures private credentials or arbitrary serial lines.
- The OTA HTTP route still requires the device-specific key, RFLink/board identity checks and ESP-IDF rollback. Short repeated panics may not leave enough time to perform a network OTA; persistent safe mode, watchdog escalation, crash-stack symbolication and a real serial capture remain future independent work.
- **Verification gate:** real ESP32-S3 bootloop/panic tests; HTTP/crash/logs accessibility during intentionally blocked SPI, USB capture during a power cycle, precise post-flash auto-enrollment after boot health, OTA rollback and preservation of prior NVS settings. GitHub host/IDF build status must be reported separately.

## Stage 27 persistent boot checkpoints (development)

- A separate versioned 12-entry NVS ring records the last **entered** startup stage for each boot sequence: NVS ready, SPI/CC1101 probing, RX startup, HTTP/network initialization, optional OTA verification and app startup completion. This does not change the existing 12-entry reset history NVS layout or crash core-dump partition.
- Checkpoints are **not** the raw serial log, and do not prove crash causation. RF signal events, recovery loops, Wi-Fi reconnects and HTTP requests must never cause phase-journal writes. Each checkpoint is saved only during startup, with an explicit phase_history_saved state for write errors.
- The sanitized existing crash diagnostic API includes boot_phases with sequence, enum and completion flag. The Home Assistant Builder applies a strict allowlist and verifies hardware identity before presenting any stage; no passwords, NVS contents or task stacks are sent.
- OTA validation and rollback semantics are untouched. The app_ready state is written only after normal startup or successful pending OTA verification; it does not assert radio or Wi-Fi health.
- **Release gate:** on-device cold boot, warm reset, repeated watchdogs, interrupted stage writes, power-loss preservation, flash wear and rollback tests. USB remains necessary if firmware cannot reach HTTP. Persistent early serial text, an automatic safe mode and decoded ELF stack traces are separate follow-up features.

## Current implementation and remaining gates

- **Stage 23 initial implementation**: ESP-IDF flash ELF core dumps enabled
  by default on dedicated v0.7 test builds; bounded boot/reset history
  recorded once on startup; crash task and PC exposed through a sanitized
  local diagnostic endpoint; a small crash/restart card is shown in the
  prototype web page. Raw crash stacks are deliberately never exposed by
  that endpoint. Host tests and official IDF compile are required; physical
  panic/watchdog/OTA tests are not yet passed.
- **Not yet migrated**: rich per-radio capture payloads, full System and Web
  instrumentation, persistent non-crash event history, full crash backtrace
  export with authentication, full cross-system correlation and permanent
  log retention, ZIP export, and the final compact multilingual dark UI.
- **Required before general release**: cold/warm reboot and power-loss
  preservation, deliberate controlled panic on a sacrificial test board,
  crash capture/summary with real firmware ELF, multiple watchdog failures,
  brownout classification, OTA rollback with preserved crash history and
  measurement of PSRAM, flash wear, NVS and heap. Never intentionally
  trigger a panic on a working reference device.

Core-dump availability is best-effort: a failed flash write, exhausted panic
stack or destroyed flash metadata may leave only a reset reason. The UI must
show that uncertainty rather than pretending a crash report exists.
