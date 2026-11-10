# Getting Started with RFLink Universal v0.7

Welcome! **RFLink Universal** is a do-it-yourself gateway that listens to and controls compatible wireless devices. It uses an ESP32-S3 board and three small CC1101 radio modules.

You do **not** need to understand electronics to use the finished RFLink web interface. This page explains what to expect without assuming technical experience.

> **Not ready for normal installation:** v0.7.0 *Radioactive* is still an experimental native firmware. Official ESP-IDF compilation has passed, but radio, OTA and rollback hardware tests are pending. Only use a separate test ESP32-S3, never the working v0.6.6 gateway. Read the [Experimental bench test guide](EXPERIMENTAL_BENCH_V07.md) before flashing.

## What should using RFLink look like?

**This is our intended workflow, not a tested installation guide.**

1. **Install RFLink Builder** in Home Assistant once, from the project's GitHub app repository.
2. **Select your board profile.** The default pinout and SPI speed are supplied; your personal changes stay local.
3. **Choose a firmware version** from GitHub Releases. Five previous stable releases are shown by default; choose **All releases** to see the full history.
4. Select **Build firmware**. The Builder will compile the selected native ESP-IDF source.
5. Choose **USB · This PC** to flash from the browser on your computer, **USB · HA host** for a board connected to the server, **Update over Wi-Fi (OTA)** or **Download firmware**. Options will only be enabled if the firmware and board pass their safety checks.
6. Open the RFLink web interface to learn and control supported wireless devices.

**Want to go back?** Select an older compatible release and rebuild or reuse a verified previous build. Downgrade safety and settings compatibility still require testing.

**Where are settings?** Use **Devices → your RFLink → Firmware** for everyday builds and installation. **Advanced → Hardware & Wi-Fi** allows editing the saved profile *before* installation only. After flashing or linking the physical board, hardware and embedded Wi-Fi settings are locked to prevent an unsafe mismatch. A different configuration currently requires a separate setup and deliberate USB installation; changing Home Assistant's **Defaults for new RFLinks** never changes installed firmware. **Source & release history** is for development and recovery, not ordinary firmware updates.

## Updating source code without replacing files one by one

The planned Builder compares a downloaded or uploaded source package with the existing project and shows what is **new**, **changed** or **unchanged**. It preserves unchanged files, keeps local overrides separate and offers a backup before replacing changed files. It must not silently delete files.

## What is stored where?

| Location | Information |
| --- | --- |
| Public GitHub repository | RFLink source, documentation, example/default pinout and SPI speed, published release history |
| Home Assistant Builder (local only) | Your optional GPIO and SPI speed overrides, Wi-Fi/MQTT/OTA secrets, build preferences and approved firmware builds |
| RFLink device | The configuration the firmware actually stores on your ESP32; backup and restore behavior still needs verification |

Never upload passwords, device-pairing secrets, private keys or personal diagnostic captures to a public repository.

## Diagnose an ESP32 that restarts before Wi-Fi

In Builder v0.6.22, open your device and select **Diagnosis**. While the device is online, **Wi-Fi** shows live in-memory events together with up to 12 previous reset reasons and a saved crash summary from the firmware. For a bootloop, connect the ESP32-S3 to the computer by a USB data cable, choose **USB** and **Connect**, and select its serial port in Chrome/Edge. Press the ESP32 reset button **after** the monitor connects so early startup text can be recorded. The read-only monitor attempts to reconnect if the USB port disappears during a reset.

If the ESP32 never boots, Wi-Fi cannot retrieve a new crash report; the USB console is the immediate diagnostic source. Crash history is intentionally separate from volatile Radio/System/Web events. Always review a copied USB log for secrets before sharing it. Neither a confirmed flash write nor a last-known reset entry guarantees working radio hardware.

## CC1101 radio detection (development; hardware testing required)

The **Diagnostics → CC1101 power & SPI checks** card shows three separate radios. A
working SPI controller is not proof that a CC1101 is physically responding.
At a cold boot, RFLink reads the chip identity and tests a writable channel
register, then restores its original value before starting reception. While
receiving, the firmware only reads identity registers; after three consecutive
failed checks it reports the radio unavailable and retries later. **Readback
does not measure the actual 3.3 V rail**. The result still needs verification
against the physical board.

To compare the powered and unpowered conditions:
1. Switch the **entire board off** before changing wiring or 3.3 V.
2. With the CC1101 3.3 V supply disconnected, power the ESP32 and open
   **Diagnostics** (or capture the USB boot log). All three modules should show
   **Not detected**. Check the recorded PARTNUM/VERSION and probe errors.
3. Switch everything off again. Reconnect 3.3 V and restart. The modules
   should pass the boot readback test and enter receive mode.
4. Check **Logs → System** (or **Logs → Radio** for radio-specific events)
   and expand a CC1101 identity probe row to see the raw sample.

Never hot-plug the radios or remove their supply while the ESP32 is running.
An SPI identity/readback test is not proof of successful over-the-air RX.

## Early diagnostics (Stage 28 development)

The device now attempts to start logging, Wi-Fi and the local HTTP diagnostic service **before** configuring the three CC1101 radios. When you open **Diagnosis** from the Home Assistant Builder, you can see **Radio initialization pending** while the device is still preparing its RF subsystem; the startup checkpoint remains saved in flash for later inspection. The firmware also offers its existing authenticated OTA upload handler earlier, but a very fast reboot loop may end the connection before an upload completes.

The Builder does not confuse **diagnostics reachable** with **fully booted**: it waits for the correct physical MAC, board/profile and firmware, plus the new startup_complete=true signal, before completing automatic post-flash enrollment. For older firmware that does not expose that signal, the original rules still apply.

If early diagnostics are not reachable, connect the USB serial monitor *before pressing reset* on the ESP32-S3. A last reached checkpoint identifies where progress stopped, not the underlying cause. These changes require bench validation and must not replace a known-good RFLink v0.6.6.

## Boot checkpoints (Stage 27 development)

When the ESP32 starts, it stores a few **last reached** startup milestones in flash: NVS ready, CC1101 radio probing, receiver startup, network initialization, optional OTA verification, and application ready. Open **Diagnosis** in the Builder or **Crashes and restarts** on the RFLink's own web page to see the checkpoint next to recent reset reasons. These records survive normal restarts and power loss when NVS writes succeed; the current firmware does not keep full serial output in flash.

A last reached checkpoint is **not proof** that the corresponding component caused the restart. For example, a watchdog during radio initialization can also be caused by earlier setup or an unrelated task. An app_ready milestone means the HTTP application reached the end of startup, **not** that CC1101 RF reception/transmission is functional. If firmware never reaches its own web service, connect the Builder's **USB diagnosis** at 115200 baud and press the physical reset button after connecting. Very early bootloader failures cannot be captured by this in-app journal.

This code remains experimental until deliberately tested through resets, crashes and power loss on the separate ESP32-S3 board. Automatic safe-mode recovery and full decoded backtraces are not yet implemented.

## Crash and firmware identity preview

The development page shows three short cards: the last restart reason,
the status of the saved crash dump, and which firmware's ELF hash the
saved dump identifies. Expand **Boot history and build identification** to
inspect the exact build hashes or save a redacted JSON diagnostic report.
A matching **prefix** is not a verified full SHA-256 match; after an OTA,
the saved crash may be from the previous firmware. No task stacks or
passwords are included in the download. Physical panic and watchdog
tests on the separate board are still pending.

## Full retained logbook (development; hardware testing required)

Open **Logs** on your RFLink web interface. Choose All, Radio, Noise, System or Web. Use the radio dropdown and Include noise filter where relevant, then click any event to expand its technical details. Select **Older** repeatedly to inspect older retained events (up to 64 rows per page); use **Newer** or **Latest events** to return. **Export CSV** includes all retained rows matching the selected source, radio and noise filters, not only the visible page.

The separate buffers hold **250 events for each of three radios**, **2,000 shared noise events**, **512 system events** and **512 web events** by default. The numbers on the page show how full each buffer is and whether its *own* oldest records have been overwritten. **A full noise buffer cannot overwrite any radio or web events.** Logs live in RAM and disappear after a reboot; they are not an unlimited permanent audit history. The three radio and shared-noise limits are configurable under Settings subject to the memory cap.

The **Web** tab records browser navigation and other frontend actions, failed HTTP API requests and successful changes made via POST. This is deliberately more selective for successful read-only API polling to keep the log useful. No passwords, RF keys, request bodies or URL query parameters are retained. The **Radio** tab includes radio-specific system diagnostics (startup, driver errors and recovery), and the separate **Noise** tab lets you examine all raw/noise metadata without hiding it behind the All filter.

## Earlier read-only logbook development preview

On a separate Radioactive test board, the Stage 24 prototype adds a compact
logbook with filters for the three frequencies, system events and web actions.
The shared noise log is hidden by default. The Stage 25
prototype also has a filtered CSV log export, and records each **actual**
transmit attempt when the RadioManager API is invoked: request rejected,
driver action, error stage and whether the previous receiver came back.
This is not a user-facing Send control and driver success is not on-air proof. Log sizes can be changed with the
device management key; the Builder does not store radio event history for you.
Crashes and boot reasons use a separate flash/NVS history. Until hardware
validation, do not replace a working Radio Gaga v0.6.6 board.

## What can I do today?

Read the project status and follow development. The firmware **has compiled with official ESP-IDF** in GitHub Actions, but reliable live radio operation, wireless transmission, pairing and OTA rollback still need dedicated ESP32-S3/CC1101 hardware testing. Do not install the native prototype over a working device.

Stage 22 improves the internal transmission safety checks. It does **not** add a transmit button yet: while a receiver is listening, RFLink can verify whether a matching transmit profile is supported without interrupting reception. A future authorized send will pause and restore that same receiver. See the [TX readiness and RX restoration guide](TX_STAGE22.md).

## Common terms

- **Firmware:** Software that runs on the ESP32.
- **OTA (Over-the-Air):** Updating firmware using the local network instead of a USB cable.
- **USB:** A wired connection that can be used to initially install or recover firmware.
- **BOM (Bill of Materials):** The list of parts needed to build a board.
- **SPI (Serial Peripheral Interface):** The data connection between the ESP32 and the radio modules.
- **GitHub Release:** A saved version with notes and, after validation, downloadable firmware.

## Continue reading

- [Current feature and test status](V0_7_STATUS.md)
- [Documentation overview](README.md)
- [PCB, assembly and BOM status](../hardware/README.md)
- [Logging, crash reports and compact web UI acceptance criteria](LOGGING_AND_UI_V07.md)
