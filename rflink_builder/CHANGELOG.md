# RFLink Universal Builder — Changes

## v0.6.36 (correct filename and auto-dismiss for Ingress CSV export)

- Ingress's embedded browser sometimes saves the RFLink diagnostics CSV Blob as a random UUID filename. Use the **original authenticated HTTP attachment response** with the explicit `Content-Disposition: attachment; filename="RFLink_Diagnostics_Full.csv"` rather than a temporary Blob URL.
- Retain a live transfer indicator without downloading the file twice: the Builder counts bytes and CSV event rows while forwarding the same 32-KiB stream. A new restricted progress endpoint reports only numeric metadata keyed by a one-time request token, not RF contents.
- Mark the transfer as complete only after the final HTTP chunk has been sent; errors never show 100%. Keep successful progress at 100% for five seconds, then hide it automatically.
- The direct ESP32 web interface keeps its proven Blob-based download but also hides the completed progress after five seconds. No changes to radio decoding, SPI, RMT, user settings or OTA.
- The Builder must be updated to v0.6.36 to get this Ingress-only filename fix; rebooting an older Builder does not replace the mirrored files.


## v0.6.35 (sync trusted RFLink web mirror with current firmware)

- Publish the current, trusted RFLink web HTML/JS/CSS as a new Home Assistant Builder version so existing v0.6.34 installations receive an update notification; direct LAN firmware UI and Ingress had different bundled asset versions after the native changes were merged.
- Includes the native UI's SNTP clock/start-time information and **Export full CSV** transfer feedback (progress, bytes received, estimated event rows and failure handling). The CSV transfer runs in the device's independent HTTP worker from firmware PR #165; the Builder streams its own authenticated Ingress connection and retains the 96 MiB ceiling.
- Keeps the Builder's proven **trusted static mirror**: it never serves arbitrary device-origin HTML/JS, forwards stored OTA credentials or weakens identity verification/CSRF checks.
- All Builder source assets are part of the versioned Docker image. Opening/restarting an already installed 0.6.34 container is not sufficient; **Home Assistant must install the new Builder 0.6.35 image** after the repository metadata is refreshed. Source CI checks alone do not publish that image or update a running add-on.
- Device firmware is **not** changed by this Builder release. If native features are added to the mirrored web UI later, a matching Builder version bump is required.


## v0.6.34 (complete RFLink diagnostic CSV via Home Assistant Ingress)

- Publish the previously merged full diagnostic CSV export through authenticated RFLink web in the Builder. The Logs **Export full CSV** action ignores the currently selected tab, radio and noise filter, and includes available retained GDO2 pulse durations, 868 MHz FIFO bytes and diagnostic detail text alongside firmware, radio, SPI, RAM/PSRAM, crash summaries and buffer capacity/eviction counters. Firmware v0.7.0 Experimental build 40 or later is required on the device for this file format.
- Route `/api/logs.csv` through the existing identity-verified LAN proxy using bounded 32 KiB HTTP chunks instead of reading the entire response into the Builder's 6 MiB API reply buffer. Other API replies keep their normal size limits; streams abort rather than silently reporting a truncated success. Keep the existing 96 MiB export safety ceiling.
- Update the trusted native RFLink UI mirror included with the Builder so its single Logs export button matches the firmware. No new external pages, RF buffers or persistent log storage are introduced. On-air RX and OTA behavior are unchanged by this Builder-only release.
- CI covers version consistency, retained export integration and an actual HTTP Ingress download greater than the previous 6 MiB limit. Home Assistant device-side tests are still needed before calling the feature production-proven.


## v0.6.33 (RFLink web UI via Home Assistant Ingress)

- A new **RFLink web** device tab embeds the native v0.7 web interface within the Builder, and a full-page button opens it over the same authenticated Home Assistant Ingress URL. The browser no longer connects directly to the ESP32 LAN IP, so the interface can also be used when Home Assistant is reached remotely.
- Bundle trusted native HTML/JS/CSS in the Builder rather than executing scripts served from an ESP32. Only explicitly permitted native RFLink API routes are relayed to the saved private LAN address. Before any write and periodically during reads, recheck the physical ESP32 identity, board ID and board hash; device mismatches fail closed. Changes require the Builder CSRF header. Do not forward Home Assistant sessions, stored OTA keys or arbitrary HTTP headers to the device.
- Native web OTA is limited to 4 MiB and requires the OTA password entered by the user; all LAN forwarding remains HTTP on the local network, while the browser-to-HA connection can be HTTPS. Existing Builder OTA remains available independently.
- Firmware RF code, ESP-IDF runtime and CC1101 drivers are unchanged. Host tests and end-to-end Home Assistant remote access/ESP32 hardware acceptance are required before this is considered verified.


## v0.6.32 (accurate OTA installation progress and scheduled release checks)

- Fix the possibility that a delayed success response from an earlier OTA attempt marks a newly started installation complete before firmware bytes have been sent. Each Builder deployment now gets a unique ID; the device firmware panel and flash progress console ignore status and log responses from a different installation.
- The OTA progress meter is **overall installation progress**: uploading firmware accounts for up to 90%, acknowledgement and reconnect/boot validation for up to 98%, and 100% is shown only after the new ESP32 firmware reports OTA health `VALID`. The console still shows the precise byte count and upload percentage separately. Failed or rolled-back OTA never displays success.
- The existing low-frequency Builder device-monitor thread now checks the published GitHub Releases catalog approximately once per hour, even when the firmware screen is closed. While that screen is visible, its own catalog refresh remains every 15 minutes, subject to GitHub cache, rate limiting and backoff. The UI clarifies both intervals. No extra monitor threads are created.
- New host regression tests cover deployment identity guards, progress semantics and hourly/15-minute release-check timing. Firmware, CC1101 and OTA transport code on the ESP32 are unchanged. Real Home Assistant UI acceptance remains pending.


## v0.6.31 (release unused Python memory after idle workloads)

- After firmware builds, full LAN discovery or short known-device health checks, the existing five-second sampler waits for a ten-second quiet period and then runs one bounded cycle collection and a best-effort glibc allocator trim. Active builds, discovery, OTA and toolchain setup are never deliberately interrupted; no new threads or background processes are created.
- Settings → Builder performance · RAM now shows the measured Python webserver RSS before and after the last idle cleanup, alongside existing container/working-set figures. The copied diagnostic report includes this result. Linux filesystem cache is still managed by Linux and may legitimately keep the container RAM number high.
- Saved firmware, device configuration, Wi-Fi/OTA secrets, local source and the persistent ESP-IDF toolchain remain untouched. A trim only returns genuinely unused allocator pages where supported; it cannot remove live allocations or guarantee a lower RSS. Host regression coverage added, with Home Assistant OS measurements still required to establish actual savings.


## v0.6.30 (safe per-device firmware storage and disk cleanup)

- Prevent unbounded growth of compiled firmware under persistent Home Assistant `/data/releases/`. The Builder now tracks individual packages by device and keeps the last *OTA-verified* version safe while a replacement is pending. After the new firmware has rebooted, identified itself and passed ESP-IDF OTA health checks, automatic cleanup retains the newly verified build and removes older packages for that device. An unassigned advanced build retains only its latest package.
- USB flash completion alone is **not** proof that an ESP32 boots successfully. For legacy or USB-only devices with no recorded OTA-verified package, automatic cleanup does **not** guess which historical build is installed. Users can explicitly clean older packages under Settings; until a verified OTA is recorded, manual cleanup preserves the two newest packages and the currently selected build. A device may therefore temporarily have two packages during a safe update; it is **one** after confirmed installation.
- Add **Settings → Compiled firmware · Disk storage** with package count, measured bytes, reclaimable space, per-RFLink overview, and a confirm-before-delete action. The cleanup API validates build IDs, ownership and guarded paths; active builds/installations and other devices are never pruned. Failed build jobs also discard their own partially packaged output.
- Compiled firmware bundles (including duplicate private provisioning NVS images) are now excluded from Home Assistant backup archives via `releases/**`. Saved device settings, private credentials, the local source and ESP-IDF toolchain remain unaffected. A restored HA backup may require recompiling the selected firmware version.
- New regression tests exercise global and per-device isolation, OTA verification gates, USB safety, older build migration, explicit cleanup, path/symlink protection, disk accounting, backup exclusions and UI wiring. No CC1101 drivers, native RFLink firmware, RF packet handling or OTA partition layout are changed; physical Home Assistant and ESP32 verification remain outstanding.

## v0.6.29 (native WS2812 status LED and configurable hardware pin)

- Restore the onboard WS2812 RGB **status LED** in the native v0.7 firmware. It runs independently from ESPHome, using an optional low-priority RMT TX channel **only after** the two 433MHz RMT receivers have been allocated. LED-driver failure is nonfatal and does not bypass OTA boot checks.
- Native LED signals include booting (blue), missing Wi-Fi (double blue), missing/inactive receiver (double amber), all three active RX drivers (subtle green heartbeat), OTA transfer (white) and a short error warning (red). A green LED means driver-level readiness, **not** verified radio communication.
- The native device Settings page has a compact LED card for on/off and brightness (1–100%). Changes are management-key protected and persisted to NVS; an enabled LED defaults to 12% brightness.
- The Builder **global hardware pinout** and **per-device Create/Edit RFLink wizard** now include `Status LED (WS2812) GPIO` next to SPI and CC1101 pins, default **GPIO48**.
- LED GPIO is a **build-time** pin, checked against the board GPIO allowlist, strapping/flash pins and all SPI/CC1101 pins. A nondefault LED GPIO changes the board hash, requiring a new, matching firmware build and potentially USB installation. The original GPIO48 default preserves preexisting board hashes to avoid unnecessary OTA incompatibility.
- Older locally saved RFLink profiles missing the LED field are transparently treated as GPIO48 without overwriting the user's saved configuration. Older firmware sources/releases incapable of applying a custom LED GPIO are rejected rather than silently producing an incorrectly wired image.
- Host regressions cover GPIO collisions, stable legacy hashes, alternate pin constants, global/per-device Builder settings and LED behavior. A successful ESP-IDF build does **not** replace a physical GPIO48 LED or radio-reception bench test.

## v0.6.28 (OTA inactive-slot preflight and conflict diagnosis)

- **OTA installation popup console preflight:** before sending firmware bytes, show an identity-verified, sanitized live `/api/status` snapshot: running firmware/channel/board, Wi-Fi/MQTT/uptime/heap, OTA layout and boot state, SPI pinout and counters, CC1101 detection, each receiver's listening/error state, decoder counters and worker activity. Include safe saved reset/OTA-health history when reachable. Unsupported fields appear as `unknown` rather than invented values. Never echo management keys, Wi-Fi credentials, raw logs or RF payloads.
- The same console retains its scroll position when the user scrolls up to inspect preflight while transfer progress continues; it follows new lines when near the bottom. If the layout is unsafe the console already contains the evidence before the update is blocked.

- The native firmware now reports the **actual running flash partition** and independently discovered OTA slots in `/api/status` (slot count, running/target flash offsets, safe-to-update flag and a fixed layout reason). The two expected N16R8 app slots are `ota_0` at `0x20000` and `ota_1` at `0x420000`, each 4 MiB.
- The native HTTP OTA handler selects the **other verified physical slot**, rather than assuming `esp_ota_get_next_update_partition(nullptr)` has a distinct inactive slot. Missing/unexpected live partitions are rejected **before** calling `esp_ota_begin()` or reading the OTA image. It never writes the running app or erases NVS to repair a mismatch.
- The Builder preflight reads these fields on compatible future firmware, displays the live slot layout, and refuses the transfer immediately if no safe target exists. Older firmware such as Experimental 12 does not provide these fields; when it responds with `ESP_ERR_OTA_PARTITION_CONFLICT`, the Builder explains why 100% upload is not an installation and advises inspecting the actual partition table and performing a controlled USB recovery if necessary.
- This is a safety and diagnostic correction, **not proof of the cause of the user's specific board's conflict**. The installed flash partition table cannot be inferred solely from the firmware source or manifest. The next native image needs a controlled USB installation if the current image cannot accept OTA; back up/provision Wi-Fi and OTA settings first.
- No changes to CC1101, RMT, rollback thresholds or stored OTA keys. Host-only tests exercise safe slot selection and source contracts; full CI/real hardware validation must still pass.

## v0.6.27 (live OTA transfer progress and simpler firmware installation)

- The OTA installation popup shows measured firmware bytes sent to the ESP32 as an upload percentage, using a fixed HTTP Content-Length and 32 KiB transfer blocks. Upload percentage does not claim that the image was written or booted successfully.
- After upload, the Builder separately waits for the ESP32's OTA acceptance, reboot and firmware/boot-slot/health validation. Only completed verification confirms installation.
- **Installation options** in the build popup now opens the device's Firmware tab, consistent with **Install firmware**.
- The firmware version picker initially shows five choices, with five more available on demand. The selected development version remains accessible; full release history and the existing safe refresh behavior are retained.
- Includes host regression tests from PR #89. Real ESP32 OTA transfer and recovery still require on-board validation. This release updates the Home Assistant Builder, not the ESP32 firmware or its release channel.

## v0.6.26 (OTA failure reason survives rollback)

- When a **new native firmware** boots as ESP-IDF `PENDING_VERIFY`, it writes a small, versioned, checksummed "checking" report to the shared NVS diagnostics partition **before** touching radio hardware. This can distinguish a crash before validation from a failed 10-second health check.
- At OTA health completion it saves the actual boot number, built firmware version and **SPI initialized, HTTP started, RX worker started, RX cycles, active receivers and responding CC1101 count**; includes valid, health-failed and ESP-IDF validity-confirmation-error outcomes. On health failure it commits the report **before** calling ESP-IDF rollback/reboot.
- The current and later firmware can expose that report in the existing safe `/api/diagnostics/crashes` response; the identity-verified Builder proxy only forwards fixed-type, bounded, allowlisted fields. It never exposes credentials, RF payloads or raw core dumps.
- The OTA flash popup fetches durable boot history **independently** of live ESP-IDF console. It shows a matching boot report from the **actual running ESP32**, with measured health verdict and explicit no-report messaging when unavailable. An old stored report for another target is never labelled as the current failure.
- **Upgrade caveat:** Existing Experimental 9/11 firmware cannot decode this *new* NVS report after rollback; a newer firmware must be successfully installed at least once so the fallback partition also understands the report. In the initial transition, USB 115200 or a live network capture may still be required. If NVS is unwritable or the device crashes before NVS initialization, the report is not guaranteed.
- This change does **not** relax OTA validation, change GPIO/CC1101 behavior, disable rollback, auto-flash devices or claim successful RF operation. Validation still requires GitHub CI and a controlled spare-board physical test.

## v0.6.25 (rate-safe published firmware refresh)

- Firmware versions are now sorted by numeric version and Experimental/RC build number. GitHub's release creation order can list exp.9 ahead of exp.11; the newest build now appears first.
- The firmware tab adds a manual **Refresh** button with last-check and rate-limit feedback. When the firmware tab is open and visible, the Builder checks for updates every 15 minutes; hidden/closed tabs do not poll.
- The server shares a 15-minute GitHub Releases cache across browser tabs and enforces a two-minute minimum between manually forced remote checks. The selected firmware never changes automatically.
- Cached page lookups send ETag/If-None-Match conditional GET requests to reduce payload, while respecting that unauthenticated 304 responses are not a free rate-limit exemption.
- GitHub HTTP 403/429 responses respect Retry-After and X-RateLimit-Reset; repeated secondary limits use increasing backoff. Temporary errors retain the last available firmware list and suppress aggressive retries.
- GitHub's unauthenticated REST API allows 60 requests per hour **per public IP**, not per Builder. Automatic checks issue at most four page-one requests/hour per active Builder before other GitHub operations. Shared-host/large-scale deployment may need authenticated requests or a centralized release feed.

## v0.6.24 (verified live device health and unified diagnosis)

- Reloading the Builder now probes **saved RFLink IPs** first instead of starting two full /24 LAN scans. The dashboard shows **Checking** during a pending health probe, not a premature Offline verdict. A manual full network scan remains available. Refresh/health checks use the saved board+immutable RFLink identity; a different ESP32 at the previous IP cannot inherit OTA credentials. A true failed check still reports Offline with the last known contact.
- A per-boot console identifier resets Wi-Fi log cursors after OTA reboot, even if the new boot has already produced more lines than the previous boot. Temporary Wi-Fi interruptions are retried rather than permanently disabling the console.
- USB serial remains full 115200-baud ESP32 output. A **new, bounded 16 KiB in-memory ESP-IDF application console** now mirrors normal firmware logging without changing the USB writer. The firmware provides cursor-paged, key-protected /api/diagnostics/console. Builder rechecks board and physical device identity and forwards the saved device management key strictly in a request header, not logs or URLs.
- The **Wi-Fi Diagnosis** popup now includes the same captured ESP-IDF application messages as USB, together with persistent crash/reset checkpoints, per-radio error stages and structured event logs. The **OTA flash** popup can follow the authenticated application console and saved boot checkpoints after reboot, without its installation progress being overwritten by polling.
- **Limitations:** the ROM/bootloader output before app_main, plain printf bypassing the ESP-IDF logging callback, and a crash before the device joins the network are not retroactively available via Wi-Fi. Persistent reset reasons and a sanitized core-dump summary still help after an early crash. Old firmware without this new endpoint falls back to existing structured events. The console is volatile, truncated if overrun, and redacts log calls containing credential-like words.
- This is a source/host-test change, **not yet compiled with official ESP-IDF or tested over actual Wi-Fi/USB/OTA**. Keep v0.6.6 as fallback and validate firmware on a separate test ESP32. Build and test gates are required before merging or installing.

## v0.6.23 (diagnostics available before radio initialization)

- The native firmware starts persistent crash-history capture, system log buffers, Wi-Fi and the read-only HTTP diagnostics endpoints **before** CC1101 SPI, GPIO/RMT and the three receiver drivers. Where Wi-Fi connects quickly enough, startup problems can be observed live; if firmware crashes first, saved reset reasons/checkpoints and USB are the recovery path.
- The firmware explicitly reports radio_initialized and startup_complete. The first means RF driver initialization has been attempted (not that RX is healthy). The second means application startup ended normally, including successful ESP-IDF pending-OTA validation; neither is proof of on-air RF.
- The Builder's diagnosis status distinguishes early radio startup and OTA/startup validation. It periodically refreshes saved boot checkpoints while startup is incomplete without duplicating unchanged console history.
- **Safety:** an early HTTP response no longer completes the post-flash automatic hardware binding. The original eFuse/MAC, board ID/hash and firmware checks remain necessary, and new firmware additionally requires startup_complete=true. Older firmware without that field retains the original validation. Device passwords and pinned builds are unchanged.
- The existing password-checked OTA HTTP endpoint is available earlier, but fast bootloops might not leave time for a full upload. Dedicated safe mode and live bootloader logs without USB are **not** implemented here. On-device crash, power cycle, network, OTA rollback and flash verification are still required.



## v0.6.22 (bootloop-aware USB and persistent crash diagnosis)

- In **Open diagnosis**, USB reads the selected ESP32 at 115200 baud and retries for up to 120 seconds when its serial port disappears during a reset or bootloop. If USB re-enumerates, only one already-authorized port with matching USB VID/PID may be selected automatically; ambiguous or unauthorized ports require a new user choice. Nothing is reflashed and no boot-control lines are changed.
- The post-flash USB console uses the same recovery logic. To capture all bootloader output, attach the monitor and press the ESP32's physical reset button; first startup lines may be missed during USB reset/re-enumeration.
- The Wi-Fi diagnosis now retrieves the device's **existing sanitized crash metadata and last 12 reset reasons**, with compact Last known restart / Stored crash cards and history in the log. The Builder's Home Assistant Ingress proxy rechecks the saved physical identity and board profile for each request. Stack memory, raw core dumps, credentials, and RF payloads are excluded.
- Existing volatile System/Radio/Web event buffers still disappear on reset; historical reset reasons and the single stored crash dump are the durable sources. Wi-Fi cannot inspect a device before it boots far enough to respond; use USB. Full backtrace symbolication, early-boot flash journaling and recovery safe mode are separate firmware stages, **not** claimed by this Builder-only change.
- USB enrollment, device identity matching, firmware selection, OTA, CC1101 radio paths and v0.7 ESP-IDF source remain unchanged. Python/JS regression and GitHub CI checks are required; real USB reconnect and controlled crash still need hardware verification.


## v0.6.21 (automatically recognize and link the ESP32 you just flashed)

- **USB · This PC** reads the ESP32-S3 efuse MAC through the already-selected Web Serial connection **before** writing. **USB · Home Assistant** runs an explicit `esptool read-mac` preflight before writing. Neither method guesses identity from IP address, board profile or firmware label. A different, already-linked ESP32 is blocked before erasing.
- After the exact per-device validated image is written, the Builder persists the expected hardware ID (the same `rf-<MAC>` reported by native firmware), target app version and saved configuration. It starts bounded network discovery and continues to recognize that same board on future scans/restarts. It marks the RFLink **online** only after the physical ID, board/profile and **running firmware version** all match; no separate **Link RFLink** click is needed.
- A saved RFLink card displays **Waiting for ESP32** while the written device has not reported itself. Wrong device ID or firmware is never silently matched; the old unclaimed discovery card is not offered as a second device during pending automatic enrollment.
- Preserves RFLink name, GPIO pinout, Wi-Fi settings, OTA password, firmware release choice and saved build identity. Reflashing the same board requires a fresh network response rather than trusting a previously cached online status.
- Native OTA now promotes pre-ID v0.7.0-exp.2 installations from legacy IP-bound identity to permanent hardware identity automatically **after** the OTA health check verifies a new slot and the selected app version. It does not claim that an old Experimental 2 can itself report an ID.
- Old ESP Web Tools/external fallback installers cannot expose the physical COM-port MAC to the Builder; they remain usable but may require a manual link until firmware-side provisioning identity support is implemented. LAN discovery must be able to reach the device's IPv4/subnet, including across VLANs.
- Regression suite covers MAC parsing, anti-collision, wrong target version, wrong board, same-IP different hardware, reflash, persistence and exp.2→exp.3 migration. No changes to RF firmware, CC1101 operation or built-in decoders. Actual serial hardware still needs testing.


## v0.6.20 (OTA is only successful after verified ESP32 boot)

- Correct the misleading OTA `100%` in the shared flash console. The old Builder reported success immediately after the OTA HTTP server answered "Update geschreven", without observing the subsequent ESP32 reboot or OTA rollback.
- Four real diagnostic phases: **preflight** (current firmware, RFLink identity, board hash and active OTA slot), **write** (target version, image bytes and SHA-256 prefix), **reconnect** (new OTA boot slot), and **validation** (ESP-IDF's native approximately 10-second health validation).
- The Builder polls the native `/api/status` on the original private IPv4 after the upload. It only shows green/100% for OTA when the same physical RFLink is running the selected firmware from a *different* OTA slot and the firmware reports OTA state `VALID`. No artificial percentage is shown during HTTP transfer or boot checks.
- Distinguish **unverified** (could not confirm new firmware) from **rolled back** (evidence of return to the previous slot), from outright write failure. The log explains the missing proof and suggests opening USB/Wi-Fi diagnosis rather than silently declaring success.
- The verification flow remains inside the existing dark flash popup, retaining all methods and credentials. No source/CC1101 firmware changes; the OTA client uses only the saved trusted local IP.
- Offline regressions cover same-version reflashing, new slot and pending/valid states, temporary Wi-Fi loss, wrong device/board, old version, rollback, and missing verification.


## v0.6.19 (link a previously USB-flashed RFLink without reflashing)

- Fix a recovery gap after **USB · This PC** flashing: the Builder correctly refuses to claim a flash was hardware-verified, but this left the saved configuration labelled *Not installed* and hid **Link RFLink** because it expected `provisioned=true` first.
- A detected, unclaimed RFLink whose **board ID and board profile hash match** a saved unlinked configuration now offers **Link to [saved name]**. If multiple configurations share that board profile, the operator must choose the correct one. The Advanced device tab also offers this link when not provisioned.
- Linking requires explicit user confirmation with IP, current firmware label and warning that a matching pinout alone cannot authenticate OTA credentials. No matching by IP alone, no silent auto-link, and no implicit flash, board edit or firmware update.
- Existing device name, Wi-Fi settings, OTA key, board config, config revision and selected firmware release are preserved. The discovered duplicate disappears once linked, and the actual running firmware version is displayed.
- Change misleading *Not installed* to **Not linked** until physical hardware is associated. Reject accidental rebinding of an already linked device.
- No changes to the ESP32 image or its radio drivers; host tests cover link recovery, duplicate prevention and secret preservation. The actual RFLink board must be checked before confirming any link.


## v0.6.18 (native USB flash + independent device diagnosis)

- **Flash** and **Diagnosis** are separate actions on each RFLink. Diagnosis does not require building firmware, uploading a private image or knowing the OTA password.
- **USB · This PC** uses Espressif's local, pinned `esptool-js` browser bundle as the primary direct flasher. The browser still requests USB/COM permission, after which the existing verified, per-device factory image flashes without a second installer or erase popup. ESP Web Tools and external ESPHome Web remain explicit **fallbacks**.
- One in-app, dark, bounded console presents a USB progress bar, live ESP32 output after flashing if its serial port reconnects, and OTA/Home Assistant USB build logs. OTA progress is indeterminate unless firmware provides actual percentage; command completion is not proof of successful boot.
- **Open diagnosis** reads 115200-baud serial output via this PC or saved native RFLink logbooks through an identity-validated, bounded Home Assistant LAN proxy. Features include pause/resume display, follow, copy and text export. The LAN proxy does not fetch arbitrary IPs or expose passwords and blocks identity mismatches.
- The browser uses the same-origin bundle and retains strict CSP. HA Ingress/browser Web Serial restrictions can still necessitate fallback or connecting from a compatible HTTPS browser. Real ESP32-S3 and Home Assistant USB/OTA operation must be verified on a separate board.


## v0.6.17 (Wi-Fi diagnostics and transparent RFLink discovery)

- Shows the exact network(s) being scanned instead of suggesting the two defaults are always used. Previously saved custom IoT /24 subnets remain in force.
- A manual-IP connection now distinguishes HTTP 404, connection/firewall failure (TCP/80), and a status response that is not compatible with the RFLink API.
- A saved but unlinked physical RFLink is no longer mistaken for a failed Wi-Fi connection: the Builder reports that the identity needs linking. Newly discovered devices can be linked to one uniquely matching configured RFLink with explicit user confirmation. Never auto-transfer passwords based on IP or identical board pinout.
- Native RFLink Wi-Fi diagnostics include AP association, DHCP/IPv4 address, subnet, gateway, API URL and disconnection reason in serial logs; the system log records the assigned IP and reason without passwords. The native status API now includes stable physical ID and IPv4.
- Dedicated test-board hardware validation is still required. These changes do **not** correct the known 433 MHz RMT receive configuration error.


## v0.6.16 (verified source → build → install; consistent device-first UI)

- The RFLink Firmware tab remains the normal path. Its install panel now shows the selected release, precise source commit (or clearly labelled local snapshot), compiled app version, board profile and binary SHA-256 before choosing USB or OTA. It also compares the live device's reported version label when online, without claiming binary read-back.
- Every newly built, published-release package records the verified immutable source commit in its installable manifest. A matching version or GPIO hash is no longer sufficient: reactivating stored packages, host USB, OTA, browser USB and device ZIP downloads must match the currently saved release **and commit**. Pre-provenance released packages require a rebuild.
- Both published tag and embedded version format are verified; existing local development snapshots remain explicitly unversioned and are never passed off as official releases. Source SHA-256 and firmware-image SHA-256 are distinct identifiers.
- The remaining Build, Versions/GitHub, recovery and global settings screens are consistently labelled Advanced, reuse compact Firmware-style cards, and avoid implying they are a second recommended firmware path. Source restoration is a deliberate local-only action; the oversized compiler setup log starts collapsed.
- Only Builder code and UI changed. No firmware source or CC1101 drivers were modified. Actual USB and OTA hardware installation still requires a dedicated test board.


## v0.6.15 (select release, build, install — one firmware tab)

- Each configured RFLink now has a proper **Firmware** tab with one row of selectable **Stable / Early adopter / Experimental** cards, a paginated list from actual published GitHub Releases, and details for the selected immutable tag. No duplicated release-channel dropdown.
- Previously created devices can now **change their firmware version**. Every selection is reverified against an actual published GitHub Release ID and source commit, saved per device, and survives Builder restarts without modifying board GPIO, Wi-Fi, OTA credentials or config revision.
- The device install workflow has a clear order: **choose channel → choose version → build → choose installation method → install**, retaining browser-PC USB, Home Assistant host USB, OTA and ZIP options.
- A saved build from a different release cannot accidentally be activated, downloaded or flashed after changing selection, even when it has the same GPIO hash and config revision. Previously built packages remain available when reselecting their version.
- The local unversioned development source is explicitly labelled as such in Experimental; Stable and Early adopter cannot show pretend versions. Normal published releases are never misrepresented as hardware-validated.
- HTTP and UI regression tests cover release switching, private credential preservation, CSRF, active-build rejection and channel/version workflow. All hardware flashing still requires an explicit user action.

## v0.6.14 (in-app USB installer and compact build console)

- **USB · This PC** opens an in-app popup. On a secure browser context with permitted Web Serial, official ESP Web Tools v10.4.0 is bundled locally to select the COM port and flash without opening a second browser tab.
- When HTTPS / Home Assistant Ingress iframe permissions prevent Web Serial, the same popup explains why and offers an explicit button to the already supported official secure ESPHome Web fallback. No unannounced navigation. Existing HA-host USB, OTA and download routes remain.
- No external JavaScript CDN is necessary; all vendored browser flasher assets ship with the Builder, including its Apache-2.0 license. Sensitive per-device factory image bytes are served only through the existing CSRF-protected, device-bound backend.
- Build log popup no longer grows a second outer scrollbar: only the bounded log field scrolls, with buttons and status kept visible even on smaller screens.
- Replace manually typing `TESTBOARD` in the selected device's install panel with one explicit overwrite checkbox. Backend device/hash/provisioning and internal test-board authorization checks stay in place.
- Browser Web Serial and first real ESP32-S3 installation still need on-device verification; CI cannot confirm that Home Assistant Ingress exposes the USB permission.

## v0.6.13 (browser USB flashing on your PC)

- A new **USB · This PC** installation method opens an official secure ESPHome Web Serial flasher tab on the user's own computer (Chrome/Edge). This works when Home Assistant itself is not plugged into the ESP32; the HA host USB method remains available as an advanced alternative.
- The Builder verifies the exact device, build ID, GPIO/board hash, configuration revision and every firmware SHA-256 before merging official ESP-IDF bootloader, partitions, per-device Wi-Fi/OTA NVS, OTA data and app into a private 16MB-compatible factory image via Espressif esptool.
- Transfer the private image directly between browser tabs using ESPHome's Web Serial v1 handoff protocol. A new boot firmware layout requires a full first install, which erases existing flash. The browser flasher is trusted with the embedded plaintext NVS credentials; the image is not uploaded to a release server.
- No ESP32 is flashed automatically or marked provisioned from a mere UI success event. Web Serial, Windows COM selection and hardware operation need real user/device testing.

## v0.6.12 (stamp the selected Experimental version into ESP32-S3 firmware)

- Pass the exact SHA-pinned GitHub firmware Release tag into the isolated ESP-IDF build. Previously, all tags still compiled as the Stage 20 firmware identity.
- Require the verified binary package's embedded release tag to match the selected GitHub Release; prevent an inherited environment value from silently marking a local development build as released.
- Native v0.7 status/web UI and package manifest now identify Experimental build sequences and release candidates while retaining the technical board-profile hash.
- The release publishing CI compiles and packages the selected exact version before publishing a source-only release. No ready-to-flash binary is auto-published and no device is flashed.
- **Test-board hardware validation is still pending.** The previous v0.6.11 release picker and diagnostics remain unchanged.


## v0.6.11 (human-readable firmware release channels)

- The Home Assistant Builder now groups published source releases into Stable, Pre-release (RC) and Experimental channels; users see semantic firmware versions and build/RC sequences rather than commit IDs.
- Experimental builds and release candidates get separate immutable tags. A new experimental build can supersede an earlier build without moving either tag; the Builder resolves selected releases to pinned source commits.
- The new-device wizard defaults to the latest published Stable release when one exists. Test releases are visible only after an explicit opt-in. Unreleased local source is clearly marked as development.
- A manual GitHub Actions publication workflow requires host regression tests and a real official ESP-IDF compile before any channel release; only a deliberate action can promote an experimental build to RC and then to Stable. Stable also requires explicit hardware-test approval.
- No firmware binary is published or flashed by this change. Real hardware, OTA, rollback and production readiness remain unverified.


## v0.6.10 (release disposable build cache after firmware jobs)

- Observed on Home Assistant: after firmware compilation the container showed **303.2 MB**, the Python webserver only **50.1 MB RSS**, and Linux file cache **250.7 MB**. These measurements do not indicate a Python heap leak.
- Always remove the disposable ESP-IDF `build_workspace` after a successful, failed or canceled firmware job. Release artifacts have already been copied to the permanent `releases` folder, and the persistent ESP-IDF `toolchain` is never removed. The workspace was previously recreated from scratch before every build anyway.
- Keep build state `running` until workspace cleanup completes, avoiding a race in which a new build starts while the previous job removes its files. Cleanup errors are logged but do not turn an otherwise successful firmware build into a failed one.
- The RAM diagnostics now show `inactive_file` and a *working-set estimate* (`memory.current - inactive_file`), alongside total cgroup memory and file cache. This estimate is not a guarantee of immediately reclaimable pages or a Python-only allocation metric.
- Added host regression tests for safe cleanup and the working-set calculation. **Home Assistant OS post-build RAM reduction and any effect on follow-up build speed remain to be measured.** No firmware or radio code was changed.

## v0.6.9 (reduce RFLink discovery memory pressure)

- Real Home Assistant diagnostics isolated the problem to network discovery: Python RSS jumped from **33.2 MB at scan start** to **466 MB when the initial scan finished**, then reached **880.5 MB at idle** without a firmware build. The network scan is a proven major contributor to high idle memory.
- Initial private LAN discovery now uses a bounded **12 workers instead of 48**, while its recurring known-device health checks use **4 instead of 16**. This lowers the number of concurrent network clients and native allocator arenas.
- Reuses a no-proxy, no-redirect HTTP opener for each short-lived worker rather than constructing a complete opener per IP checked. Identity validation, 8 KB response limit, 0.6-second discovery timeout and 1-second known-device timeout are unchanged.
- Sets `MALLOC_ARENA_MAX=2` in the Builder image *before Python starts* to reduce glibc heap fragmentation from short-lived scan threads. The change also affects subprocess allocator behavior; validate build speed and RAM on Home Assistant hardware.
- Device scanning still covers both original fallback /24 private networks, or a selected VLAN/subnet plus known device addresses. Existing managed names, OTA credentials and board profiles remain persistent. Manual add-by-IP works as before.
- Tests cover subnet completeness, worker caps, custom VLAN plus known out-of-range IPs, existing OTA credentials, per-worker client reuse and Docker memory settings. A scan with lower parallelism may take longer; no precise RAM saving is claimed until tested on the original Home Assistant host.

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
