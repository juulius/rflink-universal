# RFLink Universal Builder — Changes

## v0.6.8 (trace unexpectedly high RAM from startup)

- A fresh Builder restart still showed **601.3 MB container memory** and **603.5 MB Python RSS** with only **5.8 MB file cache**, before any firmware build. The high baseline is not explained by the compiler or file cache, and thread/FD counts were only 13/23. The exact allocation cause has **not** yet been found.
- Lightweight, read-only memory checkpoints now record Python imports, toolchain manager, persisted settings/devices, Builder initialization, HTTP socket setup, toolchain readiness and the initial network scan. A single short-lived watcher records the initial scan's completion.
- View these checkpoints under **Settings → Diagnostics · RAM → Startup memory timeline**, or copy them in the existing report. Each includes the overall container and Python RSS, and is stored in a bounded in-process list only.
- Do not change heap allocation, threads, compiler parallelism, saved credentials or RF firmware until these measurements identify the real trigger. No RAM saving is claimed by this diagnostic release.

## v0.6.7 (investigate and limit idle Python RAM)

- Home Assistant measured an idle app container at **827 MB**, with Python RSS **605 MB** and file cache **218 MB**. This version addresses one potentially unbounded source of Python threads and adds process breakdowns to identify the remaining cause.
- HTTP/1.1 Ingress keep-alive worker connections now close after **20 seconds of inactivity** instead of retaining a Python thread indefinitely. Active POST bodies may use a longer upload timeout; each next keep-alive request resets to the idle timeout.
- Expands Settings → Diagnostics · RAM to show **Python anonymous RSS** (heap/stacks), **file-backed RSS**, live **thread count** and **file descriptor count** in both the advanced view and copyable summary. This is to distinguish memory retained by Python from sockets, shared libraries and file cache.
- Adds tests to ensure idle HTTP connections close and process counters parse correctly. No new monitoring agents, credentials access, changed discovery cadence or changed ESP-IDF build behavior.
- **Not yet verified on Home Assistant OS:** the original 605 MB may also have other causes. Compare fresh idle memory after restart, following a build and with the browser closed. RAM savings are not guaranteed by CI.

## v0.6.6 (lightweight RAM diagnostics)

- Adds a compact **Settings → Diagnostics · RAM** view showing app-container memory, Python webserver resident memory, previous idle RAM, last firmware build's sampled peak and a stable reading 15 seconds after a build.
- Reads Linux cgroup memory counters (including child ESP-IDF processes and filesystem cache) and `/proc/self/status`; no psutil, shell processes or external monitoring agents. Sampling every 5 seconds runs independently of the browser.
- Displays file cache, cgroup peak since container start (when supported), observed peak since Builder start, explicit percent denominator and a local copyable summary without device credentials.
- Uses an in-memory, bounded 10-minute history only; no persistent log or change to stored Wi-Fi, pinouts, OTA keys or firmware.
- Measurements are observational; a high RAM reading cannot be attributed to the compiler or Python until the diagnostic numbers confirm it. Further performance tuning will be based on actual idle/build/post-build measurements.
- Hardware-flashing and Home Assistant OS memory comparison still require on-device checks.

## v0.6.5 (device-centred firmware building and installation)

- **Build firmware** opens a focused per-device modal with the actual ESP-IDF live log, status, copy/stop/close and post-build installation action.
- Closing the modal never stops the background build; reopen the same job's log without starting another build.
- **Add device → Build firmware** uses the same modal instead of sending the user to the old compiler page.
- Once a validated, compatible local build exists, **Install firmware** offers OTA, USB connected to the Home Assistant host, or a private ZIP for another computer, directly in the device card on desktop and mobile.
- Previous local releases are matched by device ID, radio pinout hash and saved configuration revision. Builds for another RFLink or changed settings cannot be installed by mistake.
- The new direct device download endpoint requires exact device, build job, board identity and configuration revision; a global build is not blindly downloaded for another device.
- The user must still enter **TESTBOARD** for OTA/USB, and all existing backend board, partition, integrity and device identity checks remain in place. Real RFLink hardware flashing remains **not yet validated**.
- Compiler source management and advanced tools remain accessible through the **Advanced** panel; the old Build page is no longer a normal daily workflow.
- No changes to the ESP32 firmware itself or stored RFLink settings.

## v0.6.4 (Home Assistant ingress stability fixes)

- Fix the **Add device → New device** wizard: `wizard.js` is now served by the Builder, instead of returning HTTP 404.
- Properly decode bounded HTTP/1.1 **chunked POST bodies** forwarded by Home Assistant Ingress, before executing write actions; this fixes settings and pinout saves being rejected with HTTP 400 and stops stray `Bad request syntax ('23b')`, `('2')`, and `('0')` log errors.
- Reject ambiguous or invalid request framing and oversized bodies with explicit errors and a closed connection. Keep standard Content-Length requests, CSRF checks and ZIP upload size limits.
- Add real in-process HTTP tests for chunked JSON, empty-body actions, persistence over a shared connection, file serving and request-body limits.
- No RFLink firmware changes. This is a host/backend fix; a real Home Assistant update is still required to verify the installed app behavior.

## v0.6.3 (ESPHome-like create-device wizard)

- **Add device** now starts by creating a new RFLink configuration; adding an existing network device is a secondary choice.
- Copies the Builder's global Wi-Fi and hardware defaults into an independent local device profile; both can be edited before first flash.
- Uses per-device cryptographically random 192-bit OTA passwords with eye/copy/regenerate controls, stored only in the private Home Assistant app directory.
- Allows selecting current source or a pinned, published GitHub firmware release before building this device.
- Builds the native ESP-IDF source with this device's own validated radio GPIO/SPI profile.
- Generates an actual Espressif NVS initialization image with the device's Wi-Fi/OTA credentials; first USB flash also writes this partition at 0x9000.
- Offers Home Assistant host USB installation or a manual private ZIP download. Initial blank-board installs are *not* OTA; later OTA requires verified target linkage.
- Enforces build/device/config revision matching; changing credentials or pinout invalidates older builds.
- **Security limitation:** native v0.7 firmware still requires a plaintext OTA key stored in unencrypted NVS; the key is randomly generated, **not hashed at rest**. USB ZIPs contain sensitive credentials and must remain private. OTA currently uses HTTP on a trusted LAN. A future native firmware update must implement hashed/stronger authentication before claiming it.
- Home Assistant OS installation and physical RF hardware are **not yet validated**.

## v0.6.2 (device-first desktop and mobile redesign)

- Matched the approved RFLink UI design: a focused devices list and selected-device panel on desktop, with a separate back-navigable device screen on mobile.
- Per-device Overview, Connection, Firmware and Advanced views keep the main screen free of compiler and repository controls.
- Device name and its own OTA password can be edited, revealed and saved on the selected device, without visiting global settings.
- Firmware installation is enabled only when a verified local build matches the selected device's board ID and profile hash, the device is online and its key is saved; the existing TESTBOARD confirmation remains required.
- Published GitHub releases are informational, not automatically eligible OTA downloads. Shared board GPIO/SPI configuration is explicitly identified as **global** (per-device build profiles are not implemented).
- Existing advanced source, GitHub, USB, MQTT, hardware and build functions are retained behind deliberate navigation.
- App installation and live hardware operation are **still not tested on Home Assistant OS**.

## v0.6.1 (ESPHome-style device dashboard)

- Made Devices the homepage with compact cards, online/offline indicators, search and filters.
- Added discovered/unmanaged gateway cards and an Add device wizard with adoption and manual IP fallback.
- Added persistent management for names, saved devices and **independent OTA passwords** per RFLink, including explicit password visibility controls.
- Periodically rechecks known native RFLinks and preserves offline cards instead of deleting them.
- Shows newer **published releases** without falsely claiming that a compatible OTA binary exists.
- Current discovery is bounded local /24 scanning, not ESPHome mDNS; native firmware discovery and stable identity remain separate firmware tasks.
- **Home Assistant OS installation and hardware validation are not yet verified.**

## v0.6.0 (lightweight Home Assistant app + simplified interface)

- Replaced the large pre-provisioned ESP-IDF Docker base with a smaller Python/Debian app image. The web interface starts before the official ESP-IDF v5.5.6 toolchain is downloaded.
- Added persistent background ESP32-S3 toolchain setup, live installation console, clear failure status and retry control.
- Activated the downloaded toolchain for firmware builds and USB flashing only after setup verification.
- Reorganized the interface into Build, Versions & GitHub and Settings pages; kept existing advanced features without crowding the home screen.
- Selected `juulius/rflink-universal` automatically; changing repository is an Advanced action.
- Added a dedicated 128×128 Home Assistant icon and 250×100 logo with the approved RFLink artwork.
- **Home Assistant host install and hardware tests are still pending.**

## v0.5.1 (source migration candidate)

- Prepared a single GitHub repository for the Home Assistant app and native v0.7 firmware.
- Updated the bundled default source to Stage 20 native firmware.
- Retained the five-previous-releases selector, full release history, isolated version builds and local hardware/secret storage.

## v0.5.0

- GitHub Releases history with version-pinned builds, SHA-256 checks and board compatibility validation (prototype).
