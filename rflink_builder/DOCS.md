# RFLink Universal Builder — Home Assistant

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

## Compiler cache and build-phase timings (Builder v0.6.16)

RFLink Builder stages a fresh disposable native ESP-IDF workspace for every build, but now **optionally** reuses verified, compiler-keyed C/C++ compilation results across builds with official ESP-IDF `IDF_CCACHE_ENABLE=1`. This is not a partially reused build tree: the board profile, source manifest, full ESP-IDF linker output and release/package validation still execute for each job.

The ccache object files are compressed but not encrypted. The cache is stored locally in `/data/compiler_cache` with owner-only filesystem permissions and a **384 MiB on-disk size limit**. There are no plaintext per-device Wi-Fi/OTA NVS credentials in native compiled C/C++ files; the private NVS image is created and saved separately for each device. Do not publish or externally share the cache. If needed for diagnostics, set `RFLINK_DISABLE_CCACHE=1` in the app environment to force an uncached build; build validity is unchanged.

The build console now shows **actual** durations for source, preflight, compiler, packaging and cleanup steps, plus per-build cache hit/miss counters. The initial run may have 0% cache hits. Compare at least two successive builds of the **same source + GPIO profile + toolchain** to measure a meaningful speed improvement. Home Assistant memory diagnostics may still count reclaimable Linux filesystem cache; **384 MiB of persistent disk cache is not 384 MiB of reserved RAM**.
