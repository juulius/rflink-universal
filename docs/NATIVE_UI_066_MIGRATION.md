# Native v0.7 UI migration from Radio Gaga v0.6.6

**Purpose:** Restore the complete, familiar RFLink *device* web experience on the
native ESP-IDF firmware, while incorporating the clearer 0.7 radio diagnosis,
OTA/boot history and compact dark layout. This is a **migration plan**, not
a claim that all 0.6.6 features work in 0.7.

## Reference and user-facing goal

The comparison uses the provided, working v0.6.6 source archive
(SHA-256 `d91dde4f579a92e5313b13e6499de45ff4758ad149045817f6014b444a519a8f`),
particularly `rflink_universal/ui/index.html` (approximately 307 kB),
`ui/locales.bundle.json` (approximately 269 kB) and the ESPHome-backed
`rflink_universal.cpp` HTTP handlers. The source contains five main screens:
**Status, Devices, Logs, Add device, Settings**. Profile library and advanced
diagnostics are embedded in their workflows rather than top-level pages.

The native 0.7 firmware must remain **independent of ESPHome and Home Assistant**.
The Builder compiles and deploys; the ESP32 hosts its own day-to-day web UI.

The intended everyday journey is: open RFLink, see the current health, select
a device, control or learn it with a short clear path. Diagnostic complexity
belongs behind an advanced control. Unsupported operations must be visibly
described as unavailable, not presented as live buttons.

## Important URL mismatch: not a CC1101 driver error

The old v0.6.6 JavaScript **actively requests**
`GET /rflink/api/status` in its status, radio-selection and log-refresh
logic. Its ESPHome HTTP handler intentionally served that route.

The native v0.7 app serves **`GET /api/status`**, and its current HTML and
Home Assistant Builder device probe use that native path. An ESP-IDF warning
`URI '/rflink/api/status' not found` means a client requested the *legacy*
URL. It does **not** establish that the native app generated the request.
An older page/browser tab or legacy integration is a plausible cause; identify
the request origin before claiming it is fixed. Avoid adding an unexamined
legacy alias: the rest of the v0.6.6 API is not native-compatible.

Canonical paths:

- Direct ESP32 web UI, opened at `http://device-ip/`: use root-relative
  `/api/...` URLs and the exact native endpoint names.
- Home Assistant Builder ingress: use same-origin **relative**
  `api/...` paths to the Builder, then its identity-checked proxy to
  `http://device-ip/api/...`. Never forward `/rflink/api/...`.
- Never expose any management credentials in query strings, client log output,
  or an unauthenticated endpoint.

To locate a remaining legacy caller, close old RFLink tabs and compare the
ESP32's HTTP warning count after loading only the native root page; then
check any remaining integrations or bookmarked old dashboards. A 404 is
not an RF initialization failure.

## Function and API parity inventory

| v0.6.6 experience | Native v0.7 currently | Migration requirement |
| --- | --- | --- |
| Status + radio summary | `/api/status`, `/api/events` and compact test cards exist | Rebuild the compact dark Status layout using **native** presence/listening/health, per-radio activity, current version and warnings; never infer successful RF packets from SPI presence |
| Devices, saved identities and detail/control cards | **No device CRUD/control HTTP API**; shared radio manager and partial decoder counters exist | Add persistent native device store and detail API with explicit radio/profile identity; defer live buttons until authorized TX and RX restore can be tested |
| Logs (All / Radio / System / Web), noise, export | `/api/logs`, `/api/logs.csv`, `/api/logs/config`, native console and crash API exist | Port filter/selection and compact cards; preserve three signal rings, shared noise, System/Web, NVS size settings and saved boot history; do not claim ZIP/history parity yet |
| Add device (baseline, listening, candidate, confirmation) | No native candidate/learning wizard API | Port the staged wizard using the universal RX/candidate pipeline; no ESPHome `/developer` calls or phantom pairing success |
| Profile selection, library and user decoders | Partial native Itho/RAMSES decoder path, no full user library API | Native manifests/catalog, validated profile assignment, safe persistent user data and migration/import policy before enabling UI |
| Device TX, pairing, rolling codes | Native UART/RMT/FIFO transports + trace infrastructure, **no authorized UI/API TX trigger** | Add secure capability-gated command/pairing endpoints, preserve exact RX-to-TX-to-RX recovery and trace outcome without claiming on-air success |
| Settings: logging, MQTT, Wi-Fi, language, LED, build | Logging and MQTT endpoints exist; Wi-Fi/OTA live in native provisioning; other controls lack parity | Compact independent-height cards, save feedback, version/channel, language and LED only once their native backing is implemented |
| NL/EN/FR/DE localization | Native embedded test page is not multilingual | Port translation values with a native-safe asset strategy, avoid raw v0.6.6 API assumptions |
| Firmware update and boot/crash diagnosis | Native `/api/ota`, ESP-IDF OTA slots, boot checkpoints and crash summaries exist | Reuse 0.7 identity/rollback/health, make diagnostics easy to reach, no ESPHome OTA wiring |

**Evidence boundary:** logs showing three CC1101 identity probes and
`RX start ... (0=OK)` prove boot-level probing and the driver returning
success. They do **not** yet establish stable real RF reception, profile
decoding, pairing, transmitter output or reliable OTA rollback. The v0.6.6
firmware remains the reference fallback until these board tests pass.

## Implementation sequence (small, testable PRs)

1. **UI shell and read-only parity.** Extract native web assets from the C++
   page into bounded HTML/CSS/JS resources, preserve offline/no-CDN behavior,
   port the familiar five-screen navigation and dark/mobile styling, and
   fully wire Status, Logs and read-only crashes/diagnosis. Do not copy
   `/rflink/api/` requests; only render real 0.7 data.
2. **Native state and profile APIs.** Build NVS-backed device identity and
   profile catalog with versioned schemas, checks, bounded payloads and
   preservation across OTA. Add Devices and detail panels in read-only mode.
3. **Learning and device controls.** Implement repeatable RX candidate capture
   and explicit association; add protocol-specific pairing or TX buttons
   *only* after each capability's authorization, transmit logging, radio
   restore and on-board regression gate.
4. **Settings, localization and export.** Complete NL/EN/FR/DE strings, LED,
   logging/MQTT settings, compact cards and any missing safe exports.
5. **Usability/hardware acceptance.** Test navigation/mobile overflow, empty
   states, failed/slow radio, multi-radio activity, learning, persisted
   devices across power loss, OTA rollback/retained settings and main UI
   response time on the physical ESP32-S3.

## Non-negotiable implementation checks

- No ESPHome runtime or old ESPHome REST routes in the native browser bundle.
- Every interactive action has a corresponding native endpoint with validated
  responses, authorization for state changes, clear error messages and an
  on-device test. Incomplete features are hidden/labelled, never simulated.
- No private management keys, rolling codes, Wi-Fi passwords, raw core dumps
  or unredacted credentials exposed through public read endpoints.
- A responsive UI is not proof of physical radio operation; distinguish
  hardware detected, RX listening, frames decoded, driver TX accepted and
  RF transmission verified.
- No backward-incompatible NVS or profile rewrites without a migration and
  rollback test. Keep working 0.6.6 as a tested fallback.
- Follow `docs/LOGGING_AND_UI_V07.md` for ring budgets, timestamps, filters
  and card proportions.

This document is an architecture and feature-parity checklist. Nothing here
automatically enables new device or TX features.

## Stage 1: native web shell and read-only wiring (development)

The first implementation is in `firmware/main/webui/` (standalone HTML, CSS
and JavaScript compiled into the ESP-IDF application through a CMake-generated C++ header).
The device root `/` serves the compact dark navigation. The retired
`/advanced` page (and old `/rflink` and `/rflink/` bookmarks) now redirects
to `/` instead of polling legacy RFLink APIs. The browser OTA upload is in
**Settings → Firmware update** and retains the native authenticated `/api/ota`
partition/board checks. Internal RAM/PSRAM metrics are visible in **Diagnostics**
(used/total, free, minimum free since boot, largest free block). Browser logbook
and uptime durations use **DD.HH.MM.SS** based on elapsed boot time; raw
millisecond timestamps remain in APIs and CSV, and the low-level ESP-IDF console keeps its original timestamps for crash analysis. A fresh, unconfigured board still
serves the Wi-Fi and OTA-key provisioning form at `/` until it is configured.
The Home Assistant Builder is unchanged.

Implemented native-backed pages: Status (three separate radio probes/RX states,
activity, boot and build summary), Logs (source/radio/noise filters, CSV export),
Diagnostics (sanitized reset/boot history and management-key-protected
ESP-IDF application console), and Settings (NVS-backed log retention plus
read-only Wi-Fi/MQTT and firmware identity). Devices and Add device are clear
non-actionable placeholders until a persistent native device store, learning
workflow, profile library and authorized TX are implemented. The old v0.6.6
REST APIs are **not** copied or aliased.

**Scope and limitations:** source-only implementation pending host tests,
official ESP-IDF build and real board/mobile-browser tests. Status means the
native API is reachable; a responsive receiver or a successful SPI probe does
not establish RF on-air reception. Browser language switching, full native
MQTT editing, device controls, advanced TX/learning and profile management
remain future stages. The combined Radio + Noise filter is displayed
client-side; the current native CSV endpoint cannot express that exact
combination and the UI warns the user when selected.


## Native status LED migration (WS2812, GPIO48 default — bench pending)

The onboard addressable RGB LED on the specified ESP32-S3 N16R8 test board
uses **GPIO48 by default**, not the SPI/GDO radio pins. The Builder offers
one validated build-time GPIO override in both the global pinout and each
device's Create/Edit wizard. Changing that pin requires a rebuild and may
require USB reinstallation because the board profile hash changes. A legacy
saved profile without an LED field still uses GPIO48 with its original hash.
This feature does **not** use
ESPHome `light`, `remote_transmitter`, or the RF radio TX driver.

A low-priority FreeRTOS task operates one separate ESP-IDF RMT TX channel
at 10 MHz (24 GRB bits, 48 RMT symbols). It initializes **after** both
433-MHz RX drivers were started; failure to allocate the LED channel is
nonfatal and visible in `/api/led`. It must not block RF receive work or OTA
boot-health validation. It has not yet been measured on the physical board.

| Observable state | Default low-brightness signal |
| --- | --- |
| Starting / OTA health pending | Blue blink |
| Wi-Fi not connected after app is ready | Double blue blink |
| Any CC1101 absent or a receiver not listening | Double amber blink |
| Three detected CC1101s and all three drivers listening, Wi-Fi ready | Brief green heartbeat every 2 seconds |
| OTA active | White blink |
| OTA write/validation error (10 seconds) | Red blink |
| Disabled via Settings | Off |

The green heartbeat is a **driver-level** readiness indicator only, not
proof of decoded RF reception, device pairing or RF transmit success.
The priority order is off > recent error > OTA > boot > Wi-Fi > radio.
The LED does not allocate or transfer RMT frames from the radio threads.

`GET /api/led` returns public bounded state (mode, GPIO, driver available,
brightness, transmit/error counters). `POST /api/led` requires the same
`X-RFLink-Update-Key` authorization as the other native settings, accepts
only `enabled` (boolean) and `brightness` (integer 1-100), and commits a
versioned `rf_led/settings` NVS record **before** updating the running LED.
The separate Settings card displays success or a rejected/unsaved update.
The GPIO itself is **not** a runtime setting; only brightness and on/off are.
An older immutable release which lacks the LED pin generator must reject a
nondefault LED GPIO rather than silently building a firmware for GPIO48.
Credentials are not stored or echoed in the UI or logs.

This is a **development PR**, not a successful LED/RMT hardware test; keep
v0.6.6 as fallback and do not assume a merge alone installs the LED firmware.

