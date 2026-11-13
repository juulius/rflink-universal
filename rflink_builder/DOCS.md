# RFLink Universal Builder — Home Assistant

## Builder 0.6.39 — 868 MHz FIFO unknown packet isolation

Update Builder to 0.6.39 after merging the corresponding firmware changes. The Logs buffer overview adds **Unknown 868.30 FIFO**, showing read-only counts, overwrite events and isolated detail use. The six existing configurable log rings stay untouched. The unknown 868 packet *bytes* are stored separately from recognized 868 and shared 433 noise, at the expense of a smaller shared-noise detail arena (448 → 192 KiB); total detail storage remains within the 30% PSRAM ceiling. RF data is not persisted to flash. No OTA or CC1101 radio-setting changes are part of this update.


## Builder v0.6.38 — separate RAW 433 metadata buffers

After installing Builder v0.6.38 and compatible experimental RFLink firmware, the Logs buffer overview includes the new **RAW 433.42 captures** and **RAW 433.92 captures** rings. Each has fixed retention of 250 compact metadata records. These are separate from recognized signal logs, so noise-driven captures cannot overwrite valid device-message metadata. RAW durations are still in separately bounded PSRAM details, and no extra user setting or new URL endpoint has been added.


## Builder v0.6.37 — 433 MHz capture-stage diagnostics

The secure RFLink web tab now labels 433 MHz **RMT buffer saturation** separately from actual decoder/protocol results. It identifies that long RAW candidates are logged into their own radio's compact signal ring while tiny captures still use the shared noise ring. The native firmware adds non-saturated, saturated, short-only, raw and post-filter pulse measurements. These diagnostic fields require the corresponding experimental native firmware; Builder v0.6.37 only updates the trusted Ingress UI copy. Neither Builder nor this milestone establishes successful reception of the physical 433.92 MHz doorbell.


## Builder v0.6.36 — native attachment filename via Ingress

In the RFLink web Logs tab, **Export full CSV** now uses the authenticated server attachment directly, preserving `RFLink_Diagnostics_Full.csv` rather than creating a random UUID name in embedded browsers. The Builder reports approximate progress of the *same* stream from safe numeric counters. After completion the 100% indicator remains five seconds. The RFLink device firmware need not be changed to correct the Ingress filename, but the Builder add-on must be updated to **0.6.36**.


## Builder v0.6.35 — latest RFLink web interface in Ingress

The **RFLink web** tab serves locally packaged trusted files from the **Builder add-on image** rather than running arbitrary JavaScript downloaded from an ESP32. To obtain the current CSV progress bar, error feedback and improved NTP/uptime display, install Builder **v0.6.35** in Home Assistant, then close/reopen the RFLink web tab. Refresh the Home Assistant add-on repository if v0.6.35 is not offered. Merely restarting v0.6.34 is not enough.

If the page opened directly using the ESP32 LAN IP already shows the updates, the RFLink firmware itself is working and **no new OTA flash** is necessary for this issue. Home Assistant Ingress retains identity validation, fixed route allowlists and same-origin trusted assets.


## Builder v0.6.34 — complete RFLink log export over Ingress

Install Builder v0.6.34 to use the extended Logs CSV through Home Assistant. Native firmware **v0.7.0-exp.40 or later** is also required for the richer file contents. The single **Export full CSV** command sends all retained radio, noise, system and web history, preserved RF binary/timing detail, per-buffer usage/evictions and live system diagnostics. This is never scoped to visible log filters. The Builder's identity-verified route now streams large CSV downloads in bounded chunks (96 MiB ceiling), rather than failing at its previous 6 MiB buffered API limit. This does not add a separate logger, persistent storage, or internet access to the ESP32.


## Builder v0.6.33 — remote native device UI via Ingress (development)

In Devices choose an online, physically identified RFLink and open **RFLink web**. The Builder shows its familiar native v0.7 interface as a tab or full page over authenticated Home Assistant Ingress. Remote access therefore works anywhere the Home Assistant session is reachable, without publishing an ESP32 HTTP endpoint. Connection requires HA host → saved private ESP32 IP (TCP/80).

Security limits: only allowlisted RFLink API routes are relayed; the browser runs Builder-bundled trusted native web assets, not remote-supplied scripts. Request methods, response sizes, URLs, headers and POST body sizes are limited. GET identity checks are cached for at most five seconds; every POST re-verifies the physical ID, board and hash. The existing Builder anti-CSRF header is mandatory for remote writes. OTA requires the device password entered on the remote page. IP-only legacy RFLink entries are not eligible. Home Assistant ingress is administrator-restricted by the existing app settings.

The local HA-to-RFLink HTTP hop is not encrypted. Keep RFLink on a trusted LAN; do not forward port 80 publicly. When native firmware changes web APIs, the trusted web asset mirror must be deliberately updated in a Builder release. This is a Builder-only change, **not** an ESP32 firmware release or hardware-validated remote-access implementation.


## Device settings versus Builder tools (UI clarification)

1. Open **Devices → your RFLink → Advanced** to inspect its hardware identity and saved board profile. Before the first installation, **Edit pinout & Wi-Fi** changes the saved build configuration.
2. After a device is flashed or linked, editing embedded GPIO and Wi-Fi configuration is **locked**. This is deliberate: a changed board hash or Wi-Fi secret in Builder would not reconfigure the firmware already running on the ESP32. For a different board profile or Wi-Fi provisioning, create a separate RFLink configuration and perform a carefully verified USB installation on the intended board. In-place migration is **not implemented**.
3. Open **Defaults & diagnostics** to edit *new-device defaults*, access discovery and inspect Builder RAM. These changes do not change installed RFLinks.
4. Open **Source & release history** only for developer-source comparisons, previous published releases or recovery. Routine updates remain under **Devices → RFLink → Firmware**. Importing GitHub source is not the same as selecting a published firmware release.


## Builder v0.6.23 — early boot diagnosis versus full boot verification

1. After installing a new RFLink on a separate test ESP32-S3, open **Diagnosis**. If the network is already up, the System log and saved boot-checkpoint cards can appear while SPI/CC1101 radio setup is still in progress.
2. A **Diagnosis online** status only confirms the HTTP endpoint is available. **Radio initialization pending** does not indicate a disconnected CC1101. **Finishing startup / OTA checks** means drivers are initialized but the application/rollback gate is not finished.
3. The Builder automatically links the known hardware MAC only when the expected firmware and board identity match **and** the new firmware reports startup_complete=true. It keeps the previous firmware's behavior when this flag does not exist. No extra manual linking is required.
4. If the ESP32 resets before connecting to Wi-Fi, use the existing USB serial monitor and review the persisted last-reached checkpoint on the next successful boot. Neither the native web server nor the OTA API can survive a device-wide reset or guarantee enough uptime to upload a replacement image.

The early diagnostic HTTP/OTA service continues using the current development firmware's plaintext HTTP and per-device OTA key. Keep it on a trusted local network only. Safe mode and authenticated remote diagnostics are separate work.



## Builder v0.6.21 — deterministic device enrollment

- Config IDs (`cfg-...`) identify saved Builder configurations. Physical IDs (`rf-...`) come from the ESP32's efuse/Wi-Fi base MAC and remain stable across flash and IP changes. Keep both, never automatically equate pinouts or IPs to a physical board.
- Direct Web Serial flashing reads the MAC via esptool-js before any erase. HA-host USB obtains it through a separate `esptool read-mac` preflight. After a successful per-device firmware write, store the expected identity and compiled application version with the original configuration. The background discovery finds that physical device and links only on matching MAC, board ID, board hash and running app version.
- Until verified, expose **Waiting for ESP32**, not a fabricated Online/success claim. A failed network match does not delete the saved Wi-Fi/OTA credentials; enrollment survives Builder restarts, while active scans are bounded. For devices on separate IoT VLANs, the Home Assistant host must reach the RFLink's TCP/80 and discovery must cover its configured /24 or known IP.
- For OTA firmware where the starting native image lacks a hardware ID (Experimental 2), a verified post-reboot response from the next version can migrate a previously confirmed legacy IP-bound device to its stable ID. Do not change this binding on IP/pinout alone. The original /api/ota write and basic ESP-IDF boot health check remain unchanged.
- Legacy browser/external flash fallback has no authenticated physical-port MAC read; a manual link is still possible in Advanced. Do not claim universal zero-click identity association for those fallbacks.


## Builder v0.6.20 — verify OTA instead of trusting HTTP 200

After the ESP32 replies that it wrote the image, keep the OTA installation running through reconnection and ESP-IDF boot health validation. The Builder checks the saved device identity, `board_id`/`board_hash`, actual application version, switched `ota_slot` and `ota_state=2` (`ESP_OTA_IMG_VALID`). Failed recovery, rollback or mismatch must **not** display OTA as successful. If the device cannot be reached during the bounded check, display **not verified**, not a fabricated 100% progress. The running firmware checks SPI/web/RX during its first 10 seconds and may itself trigger an automatic rollback; this OTA verification only reports what the native status API can prove.


## Builder v0.6.22 — diagnose reset loops and historical crashes

1. On an RFLink device, open **Diagnosis**. For a running RFLink choose **Wi-Fi**. The Builder retrieves recent in-RAM events and, separately, the ESP-IDF persistent reset history and sanitized core-dump summary. Up to 12 boot reasons are stored on the ESP32 itself, independently of Home Assistant.
2. If the ESP32 does **not** boot far enough to answer HTTP, plug its USB data cable into the computer running a Web Serial-enabled browser, choose **USB · This PC**, then click **Connect** and select the COM port. Press the ESP32's physical reset button *after* the monitor is attached to capture the earliest bootloader output. Use Copy/Export only after checking the log for private content.
3. If the ESP32 resets or its USB device temporarily disappears, the read-only serial monitor retries for up to 120 seconds. It may reclaim the same previously approved port; with a changed port it only switches automatically when exactly one approved device has the same vendor/product identifiers. Otherwise choose Connect again. No flash, chip reset, serial writes or network provisioning takes place.
4. The post-flash USB monitor uses the same retry behavior, but may miss the very first boot lines because flashing itself triggers a reset. Do not interpret a successful write as a successful boot.

**Limitations:** Wi-Fi cannot recover a brand-new crash from an ESP32 that never reconnects. Radio, Web and System ring events are volatile and are *not* a flash journal. The single crash dump can predate the most recent reboot; it is not proof of a new crash. Full ELF-symbolicated backtraces, hardware-validated recovery safe mode and persistent early-boot journaling remain separate work. The Builder proxy checks the exact saved board and physical ID before returning a bounded, field-whitelisted crash summary; no raw core dump or credentials are exposed.

## Builder v0.6.18 — independent Flash and Diagnosis

- Flashing remains release-verified. The **USB · This PC** method requests the port from the browser and writes the validated full factory image directly using the Espressif `esptool-js@0.7.1` same-origin ESM bundle, without ESP Web Tools confirmation overlays. The full erase is covered by the existing explicit Builder checkbox. The browser's native USB permission picker cannot be removed.
- The shared dark console shows direct-USB write percentage, Home Assistant USB/OTA command output and useful post-flash ESP32 serial logs. Server-side USB and OTA cannot report a meaningful percentage without additional device support; display a non-numeric busy state. An accepted OTA request must not be labelled as verified hardware success.
- Diagnosis is a **separate read-only action** on the RFLink Overview, with USB serial at 115200 baud and bounded Wi-Fi log events through `/api/diagnostics/device`. The Wi-Fi proxy only requests stored RFLink LAN IPs and checks board ID, hash and physical identity, enforcing redirects off, size caps and no credentials. Wi-Fi does not carry early boot output.
- USB serial may be temporarily unavailable after resetting or flashing (USB port reenumeration). The native console attempts reconnect and offers separate Diagnosis after failure. The direct flash and diagnosis functions never intentionally share a simultaneously open port.
- The older ESP Web Tools installer and external HTTPS ESPHome fallback remain available if native Web Serial is blocked by Home Assistant Ingress. Keep the existing working production RFLink firmware safe until separate-board test is complete.


## v0.6.17 — diagnose Wi-Fi versus Builder discovery

The ESP32 explicitly reports association, successful DHCP IP/subnet/gateway, local HTTP/API links, and disconnect reason through serial logs. The system-log Wi-Fi events store the assigned IPv4 and the disconnect reason. The status API exposes `device_id` (stable RFLink physical MAC identity) and `ipv4` for discovery.

The Builder prints the **configured subnet** in the scan dialog; defaults `192.168.178.0/24` and `192.168.1.0/24` apply only when no custom network has been saved. For an IoT VLAN use the actual subnet, e.g. `192.168.52.0/24`, or enter the exact IP manually. A failed IP check distinguishes TCP/80 routing/firewall, a wrong status path (HTTP 404), and a non-native response. Existing configured profiles require an explicit link to a discovered physical RFLink: no silent credential transfer to a reused IP address.


## V0.6.16 — trace the installable image before flashing

Normal flow: **RFLink device → Firmware → choose channel/version → build → inspect release/source/build/board identity → installation method → install**. The install panel displays source commit or local source fingerprint, actual application version, board hash and firmware binary SHA-256. The same selected-release + exact Git commit comparison protects previous-build activation, OTA, HA-host USB, browser USB and device downloads; old released packages without embedded source SHA must be recompiled. The local development project under `/data/project` remains unchanged after app updates and must not be mistaken for the latest GitHub release.

The older standalone Build and Versions screens are **advanced/recovery tools**; ordinary firmware actions should occur on the device's Firmware tab. Global Wi-Fi and default OTA credentials in Settings apply to new RFLinks; existing device credentials belong on the device page. Compiler setup logs are collapsed until needed.


> **Development app:** A full Home Assistant OS installation and RF hardware flash/OTA test have not yet been verified. Do not replace a working v0.6.6 Radio Gaga gateway.

## Install

1. Open **Settings → Apps → Install app → ⋮ → Repositories** in Home Assistant OS.
2. Add `https://github.com/juulius/rflink-universal` once and refresh the app store.
3. Choose **RFLink Universal Builder** and select **Install**.
4. Start the app and open its web interface. You should see the **Devices** dashboard immediately, even while the compiler downloads.
5. Select **Add device → New device**. Enter a name, review your copied global Wi-Fi and pinout defaults, optionally change this device's hardware settings, and accept or regenerate its unique OTA password. Choose the source version and select **Create device**.
6. Select **Build firmware** for the saved RFLink; its board configuration is isolated from other RFLinks and the global defaults.
5. Select a device and use **Firmware → Build firmware** to reach the compiler setup page, then follow the live setup console. New test boards can also start a build through **Add device**. The official **ESP-IDF v5.5.6** source, ESP32-S3-specific tools and Python packages are downloaded and installed into persistent `/data/toolchain` storage. This can still need considerable space, time and internet access.
8. When Compiler setup shows **Ready**, check your local source and hardware profile, then choose **Build firmware**. Do not flash until you have a verified test-board plan.
9. The **Devices** dashboard discovers native RFLinks; use **Add device** to adopt a gateway, then open it. **Overview** holds name and its own OTA key (with an eye to reveal it), **Connection** shows IP and status, **Firmware** links to build/release/OTA, and **Advanced** holds infrequent actions. Shared advanced Builder settings are behind the gear icon. Source/GitHub history and the compiler remain separate tools, accessed from each device rather than displayed on the dashboard.

**Device discovery limitation:** Unlike ESPHome's modern Device Builder, current RFLink firmware does not announce a custom mDNS service. For now the Builder scans only private /24 LAN ranges (defaults: 192.168.178.0/24 and 192.168.1.0/24) and supports explicit IP entry for VLANs. Already managed devices are checked periodically and stay visible when offline. On legacy native firmware without a stable hardware device ID, saved passwords are tied to an IP address; a DHCP reservation is recommended to avoid address reuse. A future firmware change will introduce real identity-aware discovery.

The dashboard's **New published release** indicator compares release versions only; it does **not** mean a compatible OTA image has been built, validated, or flashed. The OTA action remains behind the existing board/profile and TESTBOARD checks. The **Prepare OTA install** button only becomes enabled when a verified local firmware build matches the current RFLink's board ID and profile hash, it is online, and it has a saved OTA key. The RFLink must still be a test board. Board GPIO/SPI settings in this Builder are currently **shared**, not per-device build profiles; the device details screen makes this limitation explicit.

The official source repository `juulius/rflink-universal` is configured automatically. It is not necessary to save or re-enter it. If you intentionally use another public repo, expand **Use another repository (advanced)** in **Versions & GitHub**.

## Progress, errors and retries

- **App installation is still spinning:** inspect **Settings → System → Logs → Supervisor**. The app has not started yet, so its own setup console cannot report Docker installation errors.
- **App opens, compiler is downloading:** open **Build → Compiler setup → Live setup console**. The installer runs inside the started app rather than as part of the Home Assistant app image installation.
- **Compiler setup fails:** read the full console; use **Retry setup** once the network/storage issue is corrected. The toolchain never reports ready until the ESP-IDF version check passes.
- **Compiler is ready but Build fails:** open **Build console** for the individual compiler errors. Success here is not proof of RF or OTA functionality.
- **Restart/reinstall:** the toolchain cache is in the persistent `/data/toolchain` directory. The app uses a version marker and should reuse a successful matching installation. Restore only your local settings and build files from backups; downloading compiler files again is preferable to backing up gigabytes.

## Firmware releases

The firmware screen offers three channels: **Stable** (recommended), **Pre-release (RC)** and **Experimental**. Each channel displays the latest deliberately published version using a friendly label such as `v0.7.0 · Experimental · build 14`. The fallback local project is development source, not a published release. A new-device wizard defaults to a published Stable release if available, and requires opting in before it offers Pre-release or Experimental. The **Five previous stable versions** selector remains available for recovery, and **All releases** lists the full immutable history. Selected tags resolve to exact commit identities for isolated builds using your local board overrides. No source release guarantees an installable binary or safe OTA. The Builder's own version is separate from firmware versions.

## Local storage and safety

Personal GPIO settings, SPI clock, Wi-Fi/OTA credentials, MQTT credentials and build artifacts stay in the local Home Assistant app storage; do **not** commit them to public GitHub. These credentials are not automatically embedded in a firmware image.

Current OTA/mqtt provisioning experiments are over the trusted LAN and are **not** safe for direct internet exposure. Use a separate ESP32-S3 N16R8 test board for initial installation. Native v0.7 OTA is not a supported direct upgrade from v0.6.6.

The current Builder pipeline is a development prototype; full app startup, toolchain downloads on amd64/aarch64 Home Assistant hosts, and initial USB/OTA installations require practical verification.

## Initial installation with per-device settings

The device build generates a sensitive provisioning NVS image for the native firmware's `rf_net` namespace (`ssid`, `pass`, and `update_key`) at partition **0x9000**, size **0x10000**. The first installation via **USB connected to the Home Assistant host** writes bootloader, partition table, OTA metadata, application and the device-specific NVS image. The Builder checks that the selected device and its configuration revision match the build before installation, and retains the TESTBOARD confirmation.

You may instead **download the private USB ZIP** and flash the included components from another computer using ESP-IDF/esptool and the offsets in its release manifest. This ZIP contains plaintext Wi-Fi and OTA secrets via the NVS image. Do not store it in cloud shares, GitHub, or publicly accessible folders. The current firmware does not support hashed OTA-key storage or encrypted OTA transport; generating a high-entropy key is useful but does **not** fix those limitations.

After the first USB installation, the RFLink should appear under **Found on your network** if reachable. Open the saved RFLink's Advanced panel and explicitly link the discovered device to its saved configuration. Device identity and board-profile hashes are checked. Later OTA requires a connected RFLink, saved key and compatible build; there is no wireless first install for an unflashed board. A full real Home Assistant OS install/flash remains unverified.


## Home Assistant update progress and ready-made app images

Home Assistant Supervisor (not the RFLink app) controls the **Settings → Apps → Update** progress bar. Until a prebuilt image is available, this repository **does not** set `image:` in `config.yaml`: Supervisor must build the Docker image on the Home Assistant host. The native updater may display "0%" or "Preparing" while building; an app cannot update that percentage itself.

A staged `Prepare RFLink Builder app images` GitHub Actions workflow now builds **amd64** and **aarch64** images without publishing them on pull requests. A maintainer can explicitly enable **publish** for a reviewed workflow run on the main branch. The image registry location is `ghcr.io/juulius/rflink-builder`; the tag must match the Builder's app version.

**Before switching users to a prebuilt image:**

1. Merge the workflow and verify both architecture builds and existing source checks complete successfully.
2. Run the workflow manually on a reviewed source revision with **publish = true** to upload its versioned images and multi-architecture manifest. This publishes only the Builder container, not a stable RFLink firmware release.
3. Make the new GitHub Container Registry (GHCR) package **public** in GitHub's package settings; GitHub packages may be private on first creation. Confirm an unauthenticated pull and check the manifest includes both `linux/amd64` and `linux/arm64`.
4. Verify that the image starts through Home Assistant ingress and that local credentials and device settings remain under persistent `/data`. A published image alone is not proof of successful Home Assistant installation.
5. Only after verification, bump the Builder version, publish **that exact version tag** first, and add `image: ghcr.io/juulius/rflink-builder` to `rflink_builder/config.yaml` in a separate PR. Never enable the image reference before the requested image tag exists, or updates will fail.

Once activated, Home Assistant downloads the ready-made container rather than locally compiling its Docker layers. Supervisor can display progress for the **image download**. The exact appearance and any subsequent "Preparing" phase are controlled by Home Assistant and may still differ by version; the app does not promise a continuous 0–100% progress percentage.

The much larger **ESP-IDF compiler** is installed *after the app starts* under persistent `/data/toolchain`. Its installation remains visible only under **Build → Compiler setup → Live setup console**, not in the Home Assistant app updater.


## Troubleshooting: add-device wizard or settings save failing

In Builder **v0.6.3 and earlier**, Home Assistant Ingress could forward a POST using `Transfer-Encoding: chunked`. The Builder web server ignored that body and could return HTTP 400 on settings saves (including **Hardware pinout**), then print `Bad request syntax ('23b')` or `('0')`. In addition, the new-device wizard JavaScript route was missing and returned `GET /wizard.js 404`. Both are corrected in **Builder v0.6.4**; previously saved device configurations and toolchain files in persistent `/data` remain intact.

After the fix is released and Home Assistant updates the Builder, verify that `GET /wizard.js` returns 200, **Add device → New device** opens, and changing a test setting produces one successful POST without any subsequent `Bad request syntax` line. If a post-fix request fails, copy the app log (without credentials) and the failing action to a GitHub issue. The fix requires no RF firmware flash.

## Device-first firmware workflow (Builder v0.6.5)

Open a saved RFLink from the **Devices** screen and click **Build firmware**. A dialog shows the real status and live ESP-IDF build log. You can close it without stopping the build, reopen the current device's log, or explicitly stop the build. No percent is invented for C++ compilation because the ESP-IDF pipeline has no stable percent estimate.

After successful compilation, **Install firmware** appears **inside the same device's Firmware card** with three choices: **OTA** for a linked online RFLink with its own stored password; **USB** via a serial port connected to the Home Assistant host; or **Other computer** to download a private ZIP. OTA and USB both require typing **TESTBOARD**; the server repeats all hardware identity, board-profile hash, configuration revision, partition-table, file integrity and target checks. USB packages created for a new device contain plaintext Wi-Fi and OTA credentials. Keep them private.

The Builder keeps validated historical firmware locally, and the device card offers the latest matching build only. A custom pinout, changed device password, or other RFLink's firmware will not be installed accidentally. The legacy build/compilation/source management screen stays available behind **Advanced → Compiler and build console**, rather than interrupting everyday device tasks. First-flash validation on physical Home Assistant OS + test ESP32-S3 hardware is still needed.

## RAM diagnostics and Builder memory usage (v0.6.6)

Open **Settings → Diagnostics · RAM**. RFLink samples its own Home Assistant container once every five seconds without an additional monitoring service. The app displays the total container's cgroup memory consumption, Python webserver RSS (which does **not** include compiler child processes), sampled build peak, last idle sample, and a reading after 15 seconds of idle time following the previous build. An expandable breakdown provides cgroup file cache, cgroup peak since container start and a copyable text summary with no passwords.

**Interpreting the numbers:** Linux `memory.current` includes filesystem cache, kernel-accounted memory and child processes. Python VmRSS is only the webserver process. When a cgroup memory limit is configured, its percentage is displayed explicitly; otherwise, the interface estimates a percentage of host RAM when available. Neither is guaranteed to use the same denominator or accounting method as Home Assistant's app percentage. During builds, compiler workers legitimately increase RAM; after builds, filesystem cache can remain counted even though it is reclaimable. Five-second samples can miss short spikes, so the kernel's cgroup peak since start is shown separately when supported.

These diagnostics do not change compilation parallelism, scanning, saved credentials or any firmware settings. First collect actual **idle → building → idle** readings before tuning resource consumption, so a slower device scanner or compiler is not introduced without evidence. Values are held only in the running server process, and reset when the Builder container restarts. Toolchain files and saved device settings in persistent `/data` are unaffected.

## High idle RAM investigation (Builder v0.6.7)

A real Home Assistant RFLink Builder memory sample showed **827.2 MB container RAM**, **605.3 MB Python process RSS**, **218.5 MB file cache**, **602.1 MB before build** and **827.6 MB 15 seconds after build**. The ~225 MB difference after the build is comparable to the observed file cache, so this single measurement does **not** prove a Python memory leak from the compiler. The 605 MB Python process RSS is high even when the container is idle and requires separate investigation.

**Settings → Diagnostics · RAM → Memory breakdown** now includes process anonymous RSS, file-backed RSS, Python threads and file descriptors. Copy the diagnostic summary (no credentials) once after restarting the Builder when idle, once with the browser open, and again after building. A bounded HTTP/1.1 keep-alive idle timeout was added because the previous Ingress handler could retain a request thread per inactive connection indefinitely. This may reduce retained request threads, but memory savings must be measured on the actual Home Assistant OS host; if thread counts are low while anonymous RSS stays high, the cause is elsewhere (such as Python/native allocations).

**Interpretation:** cgroup file cache is reclaimable; don't sum process RSS and cache as two exact non-overlapping portions, since their accounting is not strictly additive. The cgroup peak is historical since container start. The private saved RFLink configurations and installed ESP-IDF toolchain stay in /data, unchanged.

## Identify 600 MB Python RSS on a fresh restart (Builder v0.6.8)

A fresh Home Assistant Builder restart measured **601.3 MB** cgroup memory, **603.5 MB** Python RSS (of which **591.5 MB** anonymous), only **5.8 MB** filesystem cache, **13 threads**, **23 file descriptors** and **no firmware build**. This indicates the high Python RSS exists before compilation. At this stage it is not proven whether it comes from Python objects, native allocation arenas, or the first background tasks.

**Settings → Diagnostics · RAM → Startup memory timeline** now records checkpoints after Python imports, manager initialization, saved device configuration load, Builder initialization, HTTP listening socket setup, ESP-IDF readiness check, and the start and completion of the first network scan. These checkpoints use Linux's existing cgroup/process counters; no large Python-object heap snapshots or external memory profiler is activated. Each point includes container memory and Python RSS. They are held in memory until the Builder restarts, without device names, passwords or Wi-Fi credentials. The copyable RAM summary also includes the startup timeline. Use a fresh Builder restart, allow the initial scan to finish, and copy the report to identify the first large jump. This release provides evidence rather than claiming a RAM fix.

## RFLink discovery was the RAM trigger (Builder v0.6.9)

Real Home Assistant startup checkpoints showed 29 MB container / 31.6 MB Python RSS before initialization; 33.2 MB at initial scan start; then **464.5 MB container and 466 MB Python RSS immediately after the scan**, later climbing to **880 MB idle** without a firmware build. The initial scanner used a thread pool of 48 workers across two private /24 networks (up to 508 IP addresses) and built a complete `urllib.request.OpenerDirector` for every host. This makes LAN discovery, not ESP-IDF, the strongest demonstrated cause of the startup memory surge.

The improved scanner limits full scans to **12 concurrent requests**, device health checks to **4**, and reuses an HTTP opener per worker. The Home Assistant Builder image sets **MALLOC_ARENA_MAX=2** at process startup, limiting glibc allocator arenas. All existing private-network checks, per-device identity checks, saved VLAN selection, exact IPs, board hashes and OTA credentials remain in place. The initial two-subnet scan can now take longer than before, especially on unreachable networks; no devices are silently omitted. Scan progress and startup RAM checkpoints remain accessible in the diagnostics UI.

**Verification after updating:** restart the Builder, allow `Initial network scan completed`, copy the RAM diagnostics once. Compare post-scan Python RSS with the old 466 MB and subsequent idle RSS with the old 880 MB; confirm your RFLink devices are still found. Then build a test firmware image once and verify the ESP-IDF build still succeeds. Do not assume the 48→12 worker change will reduce memory by an exact factor: Python/glibc can retain memory differently across platforms. No native RF firmware is modified.

## USB connected to the browser computer (Builder v0.6.13 preview)

After a successful device build, choose **Install firmware → USB · This PC**. The Builder checks the exact device configuration and makes a temporary private ESP32-S3 first-install image. It then opens the official `https://web.esphome.io/` HTTPS Web Serial flasher tab. Select the correct COM port and authorize the flash there. Chrome/Edge and a working USB data cable are recommended. The Home Assistant host no longer needs the USB cable; **USB · HA host** stays available separately. The first native v0.7 installation replaces the existing flash layout and requires a deliberate USB recovery path to return to v0.6.6. See [browser USB guide](../docs/BROWSER_USB_FLASHING.md). The actual Windows/ESP32-S3 workflow has not been hardware-validated yet.

## Same-page USB flashing and build console (Builder v0.6.14)

**USB · This PC** now opens a popup inside the Builder. When the Builder
is loaded over HTTPS/localhost and Home Assistant grants Web Serial,
the locally bundled official ESP Web Tools flasher requests the ESP32-S3
COM port directly inside this browser window. The firmware contains the
board's private Wi-Fi/OTA NVS image and must be kept private.

If your Home Assistant Ingress session is under plain HTTP or the enclosing
iframe blocks Web Serial, the popup explains the limitation and offers
**Open official ESPHome Web installer** as an explicit fallback. This is
necessary because browsers do not expose serial ports to an insecure or
policy-blocked document. The fallback uses the same-origin validated
firmware image and sends it via the existing one-time, origin-checked
browser-to-browser transfer. No new tab opens merely by clicking
**USB · This PC**.

First installation still requires acknowledging that the device's flash
contents may be replaced. The old manually typed `TESTBOARD` phrase is
replaced with one checkbox. The Builder continues to require the correct
device ID, matching build ID, board/pinout hash, config revision, firmware
SHA-256 and private NVS; no device is marked provisioned without an actual
hardware boot.

The build popup has only one scrollable area, its live compiler log; title,
status and buttons remain visible while it compiles. Closing this popup
does not stop the compiler. The older compiler/advanced tabs remain
available. The browser USB route has **not** been verified with a real
Windows COM port, Home Assistant Ingress policy or ESP32-S3 board yet.
