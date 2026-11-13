# RFLink Universal Builder (Home Assistant app)

## Builder 0.6.36 — correct Ingress CSV filename and download progress

The RFLink web tab now downloads the diagnostic CSV through the Builder's real HTTP attachment, preserving the filename `RFLink_Diagnostics_Full.csv` instead of an Ingress/WebView-generated UUID. The separate, small numeric progress endpoint reflects that same stream and removes a successful 100% status after five seconds. Install Builder **0.6.36** (update the add-on/repository metadata) to use the fix; no separate ESP32 firmware flash is required for the filename repair.


## Builder 0.6.35 — updated RFLink web UI over Home Assistant Ingress

The RFLink web page inside the Builder is **a trusted copy included in the Builder image**, not the HTML/JS fetched from the ESP32. If a newly flashed RFLink has a newer page at its LAN IP but Home Assistant still displays the old page, update the **Builder add-on itself** to v0.6.35 (refresh the add-on repository metadata if needed), then reopen RFLink web. Simply restarting an existing v0.6.34 instance does not replace bundled assets.

This update includes the latest CSV download-progress interface and NTP/uptime improvements in the Builder mirror. The native firmware already contains those updates, so the RFLink board does **not** need to be reflashed just for this Builder fix. The secured proxy and same-origin asset policy stay in place.


## Builder 0.6.34 — complete RF diagnostics CSV over Home Assistant Ingress

After updating the Builder to v0.6.34, open a managed RFLink using **RFLink web → Logs → Export full CSV**. The download is available through secure Home Assistant Ingress, can exceed the previous 6 MiB proxy limit, and includes all retained log buffers, actual preserved RF pulse/FIFO details, firmware, memory, radio and buffer-usage diagnostics regardless of visible filters. For the extended CSV file, also install native firmware **v0.7.0-exp.40** or newer on that ESP32. Earlier firmware will still export its old, shorter log format. Raw RF captures may expose device identifiers/rolling-code material; keep exports private. The new export uses volatile device memory and cannot recover samples already overwritten or lost at reboot.


## Open RFLink remotely through Home Assistant (Builder v0.6.33 preview)

1. Open Home Assistant (locally or using your normal secured remote Home Assistant address) and select **RFLink Builder**.
2. Choose a managed **online** RFLink with a verified physical device ID, then select **RFLink web**. The **Connection → Open RFLink web in Builder** button opens the same view. **Open full page** uses the authenticated Ingress URL rather than a direct LAN URL.
3. Use the familiar Status, Logs, Diagnosis and Settings screens. The Builder forwards only selected native v0.7 API requests to the saved RFLink IP; no port-forwarding for the ESP32 is necessary. The ESP32 must remain reachable on the local network **from the Home Assistant host**, even if the user's phone is away from home.

The embedded interface uses versioned, trusted files bundled with the Builder; it does **not** run HTML or JavaScript delivered by a device at an arbitrary address. API responses may differ for older native firmware. Devices with unverified identity, legacy IP-only pairing or a mismatched board are blocked. Mutating requests require the Builder CSRF token, and native web OTA still requires the password entered by the user.

Home Assistant remote access should use HTTPS and appropriate account security. The hop from the Home Assistant host to the ESP32 is currently plain HTTP on the trusted LAN. Never forward TCP port 80 from the ESP32 to the internet. This is a development preview: real Home Assistant Ingress, browser/mobile rendering, LED/Settings writes, log export and OTA require hands-on validation before a stable release.


## Compiled firmware disk retention (Builder 0.6.30)

The Builder no longer needs unlimited saved compilations. Go to **Settings → Compiled firmware · Disk storage** to inspect package count, disk use, and the *estimated* reclaimable bytes. **Clean up old packages** uses a confirmation and protects the selected active build. Each package contains the OTA binary, USB boot files, checksum manifest, ZIP, and sometimes device-specific private Wi-Fi/OTA provisioning NVS; those copies add up.

**Automatic policy:** An unassigned (advanced) build keeps one latest package. After a physical **OTA-verified** installation (exact firmware/board identity, reboot into a different slot, and app health VALID), a device keeps **one installed package**. During a subsequent build/install, that verified package is protected alongside a newer candidate, temporarily permitting **two**. On failed OTA or incomplete USB flash, the prior verified package remains.

**Existing USB/legacy devices:** A USB flash command is not a healthy-boot verification, so the Builder does **not automatically delete** any older packages if no device package has been OTA-verified under this policy. The explicit cleanup button retains the **two newest** packages for each such device; on older devices, the physically installed firmware might be older than either retained image, so review carefully. A subsequent verified OTA allows automatic pruning to the single current package. GitHub release source history is independent and earlier versions can be recompiled.

Old compiled binaries are excluded from **Home Assistant backups** (`releases/**`) to prevent needless backup growth and storing duplicated private provisioning partitions. The selected firmware source and saved device settings, including Wi-Fi/OTA credentials, remain backed up as before. The ESP-IDF toolchain stays installed across normal app upgrades but was already excluded from Home Assistant backups; it is reinstalled when necessary after a restore. After restoring a HA backup, compile a new firmware package if needed. The disk report covers valid saved build directories only; damaged or unknown directories are ignored rather than deleted automatically. Never use these checks as proof of RF transmit/receive health.



## Automatic linking after USB flashing (Builder 0.6.21)

On **USB · This PC** and **USB · Home Assistant** the Builder now reads the real ESP32-S3 MAC before writing a validated image. It then records the physical ID expected from native RFLink firmware and watches for that same board on the LAN, without requiring an extra manual **Link** action. The saved device card shows *Waiting for ESP32* while reconnecting, then becomes online only if the stable hardware ID, board profile and exact compiled firmware version agree. Settings and OTA credentials remain in the original RFLink configuration. A correct USB flash is not the same as an operational ESP32; use **Diagnosis** for boot failures.

For previously installed Experimental 2 firmware (which lacks the hardware-ID API), an authenticated OTA to firmware that reports an ID can upgrade the saved legacy IP association after the new firmware's boot-health validation. The legacy ESP Web Tools/external fallback cannot read the hardware MAC on the selected local port, so its first installation may still need an explicit one-time link. Network discovery depends on reachability across the configured LAN/VLAN scan scope.


## OTA status that actually checks the ESP32 (v0.6.20)

**100% / Verified** for OTA now means the Builder observed the *same ESP32* reboot into a different OTA partition, report the exact selected firmware version and complete its native `ESP_OTA_IMG_VALID` boot health check. The OTA HTTP acknowledgement alone is **not** sufficient, and the installer displays an indeterminate progress bar while awaiting a reboot. The console records the old/target firmware, board identity, OTA slot, app image length and SHA prefix, the response and the recovery/health stages. A rolled-back or unreachable board is reported separately; inspect **Open diagnosis**. Boot validation does not guarantee RF reception or transmission.


## Flash and diagnosis consoles (Builder v0.6.18)

**Install:** Open **Devices → RFLink → Firmware**, choose and build your exact release, check the source/firmware identity, tick the overwrite confirmation and choose **USB · This PC**, **USB · HA host** or **OTA**. A single dark in-app console displays installation status. On a supported desktop HTTPS browser, USB This PC now uses the local Espressif `esptool-js` bundle: click *Choose COM port and flash automatically*, select the ESP32-S3 and it begins without the extra ESP Web Tools confirmation dialogues. USB permission is controlled by the browser and cannot be skipped. The former ESP Web Tools installer and secure external ESPHome Web handoff remain as explicit fallbacks.

**Diagnosis:** Open a saved RFLink device and choose **Open diagnosis** on Overview. Connect via **USB · this PC** for live serial/early boot/crash messages or **Wi-Fi** for native saved RFLink system/radio/web events. No successful firmware build or flashing action is needed. Wi-Fi logs are proxied through the authenticated Home Assistant ingress endpoint and must match the device's board hash and physical ID before reading; raw serial is not available over Wi-Fi. While an active USB diagnosis owns the COM port, disconnect it before flashing. Pause, follow, copy, clear and export are available.

**Limitations:** Chrome/Edge/Web Serial requires a secure top-level context and Ingress iframe permission; in some HA environments only the explicit secure external fallback works for PC USB. The post-flash serial monitor may have to be reconnected if the ESP32-S3 reenumerates. Firmware source, CC1101/RMT and radio operation have not changed. **CI cannot replace physical ESP32 flashing tests.**


## Finding a RFLink on an IoT VLAN (v0.6.17)

The ESP32's **serial boot log** now prints `Wi-Fi READY: IP=… subnet=… gateway=…` and the web/API URLs. The native web status endpoint is `http://<device-ip>/api/status`, not `/rflink/api/status`. The Builder's Add existing RFLink dialog shows the **saved scan target**. If the device is on a different VLAN, set that network in advanced discovery settings or use **Add by IP**; Home Assistant must be allowed to reach the RFLink on **TCP port 80**. Manual IP tests distinguish unreachable/blocked, HTTP 404 and incompatible JSON.

New firmware also advertises an immutable per-board physical ID (derived from its STA MAC, not a password). The user confirms a link to an existing configured Builder profile; the Builder never assumes two RFLinks are the same based on network address or pinout. A configured RFLink shown offline may be awaiting this one-time link, even if its Wi-Fi is connected. The IP address and Wi-Fi disconnect reason also appear in the device's system log. On-board release/flash validation is still needed.


## Firmware identity and simpler daily workflow (v0.6.16)

Use **Devices → your RFLink → Firmware** for normal work: choose a published channel and exact release, build, then verify **Release → Source → Build → Install** before selecting USB or OTA. The confirmation shows the actual release tag, immutable source commit, compiled firmware version, board hash, and full application SHA-256 (under details). If an online RFLink reports a version, it is compared with the build; a matching version label is not proof of identical binary bytes. A published release with missing or mismatched source metadata requires a new build; old binaries are not silently used when switching releases.

**Current local development source** is a separate, unreleased option under Experimental. It is persisted under `/data/project` across Builder upgrades and does **not** automatically become the newest GitHub source. For repeatable testing, select a published Experimental release instead. Advanced pages remain for local source comparison, recovery, compiler diagnosis, global defaults and MQTT settings. This is a provenance check, not proof the RF hardware was tested.


Build native ESP-IDF firmware, compare GitHub changes, select published firmware releases and prepare safe USB/OTA testing on a separate board.

**Builder v0.6.16 (development preview)** — separate from the Radioactive v0.7 firmware. A native ESP-IDF firmware build has passed on GitHub Actions; installing this Builder app on Home Assistant OS and RF hardware operation have **not** been verified yet.

## OTA installation progress and automatic release checks

Each OTA/USB installation has its own server-generated deployment ID. The active installation screen ignores delayed status and log responses from a previous run (even for the same RFLink). OTA overall progress shows a maximum of 90% during upload; the bytes and upload percentage remain visible in the text. A 100% installation indication requires ESP32 reboot and valid OTA health, not simply a completed HTTP upload. The Builder checks GitHub Releases about once per hour in the existing device monitor, even if the firmware screen is closed. When a device's Firmware tab is visible, the release list refreshes every 15 minutes. GitHub caching, 120-second manual refresh cooldown and rate-limit backoff still apply. These checks never automatically flash a device or overwrite its selected release.

## RAM diagnostics

**Settings → Builder performance · RAM** shows current Home Assistant app-container RAM, Python webserver RSS, the sampled peak while compiling and memory after compilation has been idle for 15 seconds. It also separates Linux filesystem cache, which counts toward container memory, from the Python process itself. Sampling runs every 5 seconds with no additional agent and no credentials in the copied report. The advanced breakdown also shows the container memory excluding inactive file cache (an estimate of working-set memory, not a promise of immediately available RAM). After every successful, failed or canceled build the Builder deletes only its disposable `build_workspace` directory. Saved firmware releases, personal settings and the persistent ESP-IDF toolchain are untouched; a new build already rebuilds its workspace from scratch. This can reduce filesystem cache charged to the container, but toolchain file cache may remain until Linux reclaims it. After each completed firmware build, LAN scan or known-device health check, the existing five-second monitor waits ten seconds in an idle state, collects unreachable Python cycles and asks glibc to return unused heap pages. It does not delete active objects, firmware, source or toolchain files, and it never changes Linux file-cache policy. Settings also shows Python process RSS before/after the latest attempt, which may remain unchanged. The figures do not automatically equal Home Assistant's percentages; a real Home Assistant OS measurement is required to confirm actual savings.

## What happens on first launch?

The Home Assistant app itself uses a relatively small Debian/Python image instead of the multi-gigabyte ESP-IDF Docker image. The web interface becomes available first. It then starts downloading and preparing the **official ESP-IDF v5.5.6 toolchain for ESP32-S3** in the background. You can watch the live build log by opening a device and clicking **Build firmware**; it opens a popup without leaving the device. Closing the popup does not stop the build, and you can reopen the log. Validated builds expose **Install firmware** within the same device, with OTA, browser-based USB on the user's PC, Home Assistant host USB, or a private ZIP download. Compiler setup and older build tools remain under Advanced. Build actions remain disabled until setup passes a version check.

Compiler files live under the persistent Home Assistant `/data/toolchain` directory. They should not be downloaded again after an ordinary app restart. A failed download is visible and can be retried from the web interface. The large download and disk requirements still exist; they simply no longer block the app's initial web interface. We have not timed or validated the full Home Assistant setup in practice.

When you build a deliberately published firmware Release, the Builder now passes its release tag through to the native ESP-IDF app version and checks the resulting package manifest. The ESP32's web status displays the same Experimental/RC/Stable label; its exact board hash remains in technical build details. Read the [separate-board Experimental bench guide](../docs/EXPERIMENTAL_BENCH_V07.md) before attempting a USB installation.

Firmware versions are shown as **v0.7.0 · Experimental · build 14**, **v0.7.0 · Pre-release · RC 2**, or **v0.7.0 · Stable** when those releases actually exist. GitHub merge hashes are retained only as advanced source identities. By default, the new-device wizard selects the latest published Stable release; previews require an explicit opt-in. There are currently no published firmware releases, so the separate test-board development source remains the fallback. Release publication is source-only: firmware PRs generate Experimental tags after validated main merges; promotion PRs require stronger checks. No published source release is automatically declared safe to flash.

The default source repository is **`juulius/rflink-universal`**. It is selected automatically and does not need to be entered. A different repository can be configured under **Versions & GitHub → Use another repository (advanced)**.

**Add device** first creates a new RFLink configuration, like ESPHome. The wizard starts from your global default pinout/SPI and Wi-Fi settings, offers per-device changes, generates a strong random 192-bit OTA key and lets you choose current source or a pinned published firmware release. Each device keeps its own settings. First USB installation generates a separate NVS binary containing that device's Wi-Fi/OTA credentials. Browser USB uses a private factory image handed to the official HTTPS ESPHome Web flasher tab; host USB remains an advanced fallback. Neither path is yet hardware-tested. A manual ZIP can be downloaded for another computer. Existing native RFLinks can still be discovered and adopted separately.

**Security limitation:** the current Radioactive firmware keeps the OTA key and Wi-Fi credentials unencrypted in NVS. The generated password is **random, not hashed**; storing only its hash would break the current firmware's OTA authentication. Keep all USB ZIP downloads private. TLS, stronger device authentication and hashed-key storage are future native firmware features.

The default screen is now a quiet **Devices** list. On desktop you select a device to see its settings alongside the list; on mobile you open its dedicated page and use Back to return. It shows managed gateways, online/offline state, published release indicators, and newly discovered native RFLinks. **Add device** adopts a detected RFLink or accepts a known private IP; for a new unflashed board it links to the USB firmware builder. Names and per-device OTA passwords are configured directly inside **Overview**, with **Connection**, **Firmware**, and **Advanced** tabs for that RFLink. Shared compiler, board pinout, source history and MQTT settings remain accessible from the device page or the small settings button. The prototype currently uses limited private LAN probing and manual IP as fallback; unlike ESPHome it does not yet have native RFLink mDNS announcements. Only published release comparisons, not guaranteed installable updates, are shown. Details for firmware installation, MQTT credentials and board GPIOs remain available when you need them.

Follow the [Home Assistant guide](DOCS.md). Do not flash this prototype over an existing working v0.6.6 Radio Gaga device.


## Choose firmware per device (Builder v0.6.15)

Open a configured RFLink → **Firmware**. Select **Stable**, **Early adopter** or **Experimental** using the single channel-card row. Select an actual published version, then **Build firmware**, choose **USB · This PC / USB · HA host / OTA / Download ZIP**, and install. Your selected release is saved for that device and revalidated against its immutable GitHub Release ID and source SHA. An old build never silently becomes the new firmware after changing selection. Local development source remains explicitly marked unreleased. A configuration update does not overwrite existing GPIO, saved Wi-Fi or OTA credentials.
