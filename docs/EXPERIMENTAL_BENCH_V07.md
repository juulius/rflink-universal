# RFLink v0.7 Radioactive — first Experimental test on ESP32-S3

**State: test plan, not a hardware test report.** Native ESP-IDF compilation and host simulations are required before first flashing. The results below must be collected on the actual test PCB; none are pre-verified.

## What you are testing

The first published source tag will be `firmware/v0.7.0-exp.1`, shown in the Home Assistant RFLink Builder as **v0.7.0 · Experimental · build 1**. The Builder needs **v0.6.12** or later to stamp that identity into the native ESP32-S3 firmware. Multiple reviewed GitHub merges may be included before the tagged Experimental release. The source tag never moves.

This test is for **a dedicated ESP32-S3-WROOM-1 N16R8** (16 MB flash, 8 MB PSRAM), with the three CC1101 radios wired according to the board profile displayed in the Builder. It is **not** a migration from the working v0.6.6 Radio Gaga firmware. The native partition layout is different; do not flash it over the working gateway.

## What GitHub Actions provides

When an Experimental source release is published deliberately, the workflow
compiles and verifies it with official ESP-IDF, then saves the resulting
`firmware/release/` directory as the GitHub Actions artifact
**RFLink-Experimental-TESTBOARD-ONLY** for 14 days. The artifact contains the
app image, bootloader, partition table, manifest and installation notes,
but **no device-specific Wi-Fi configuration or OTA key**. It is not a
production release and must not be flashed as a complete, provisioned device
package. Stable and Pre-release runs do not produce this test artifact.

For a first USB installation, use **Home Assistant RFLink Builder v0.6.12+**
to rebuild the pinned Experimental source with the **correct device-specific
pinout and provisioning NVS**. If an Actions artifact is inspected for
diagnostics, compare the release tag, version, hashes and board profile to
the local Builder package; do not assume they are interchangeable.

## Browser USB connected to your own PC\n\nBuilder v0.6.13 adds **USB · This PC**, allowing you to flash using a COM port on the computer running your browser, via the official secure ESPHome Web tab. It needs a device-specific merged factory image containing the private NVS credentials. The previous **USB · HA host** mode remains for boards actually plugged into Home Assistant. See the [browser USB guide](BROWSER_USB_FLASHING.md) for data-loss, credential, browser and recovery details. This path still needs real hardware validation.\n\n## Safe first USB flash

1. Confirm the experiment is on a **spare board**, connected with a reliable USB data cable, and that the selected board profile exactly matches its actual GPIO wiring. Use the Builder's conservative default 200 kHz SPI clock until the real PCB has been tested. Do not connect external loads that may be driven by incorrect pin choices.
2. Confirm that a GitHub Release marked **Experimental**, such as `firmware/v0.7.0-exp.1`, is actually published. GitHub merges are not releases. Update the RFLink Builder to v0.6.12 or later. In **Add device → New device**, select **Show test firmware**, choose **v0.7.0 Experimental build 1**, and configure this test device's Wi-Fi and unique OTA password.
3. Select **Build firmware**. Wait for the genuine official ESP-IDF compile and SHA-256 package verification. Inspect the release manifest. It must report `release_tag: firmware/v0.7.0-exp.1`, `release_channel: experimental`, `release_build: 1`, `test_only: true`, the expected board hash, and a nonzero SHA-256 for each binary.
4. Choose **Install firmware → USB · This PC** (Builder v0.6.13+) to flash via your computer's browser, or **USB · HA host** only when connected to the Home Assistant machine. Enter **TESTBOARD** and select the correct board. A new blank board requires its unique Wi-Fi/OTA provisioning NVS image alongside the bootloader, partitions, initial OTA data and application. **Never share the private USB ZIP**: NVS contains credentials.
5. Open the test device's local IP address. Expect **v0.7.0 · Experimental · build 1** on the RFLink page. The technical build details should include an ESP-IDF app version such as `0.7.0-exp.1-n16r8-b9d3226e6` (board hash is an example, not a promise). Read `/api/status`: `firmware_display`, `release_channel` and `release_build` must agree, while `board_hash` and `board_id` must match the selected profile.

## First hardware validation — record pass/fail and evidence

| Test | Pass condition | Notes |
| --- | --- | --- |
| Boot and Wi-Fi | No panic/reset loop; web interface reachable; version and channel correct | Record reset reason |
| CC1101 discovery | Expected radios detected; no repeated SPI identity timeouts | Record each radio and its part/version identity |
| Shared 868.30 MHz RX | Itho and RAMSES-II are recognized using the same shared RX path; no dedicated RAMSES RX mode | Record RSSI/counts and any overlap |
| 433.42 / 433.92 RX | Incoming signals cause receptions without watchdog reset loops | Test radios individually and concurrently |
| Recovery | A missing radio reappears after reconnection without disturbing an active TX (if tested) | Do not hot-wire powered RF pins without an appropriate test setup |
| Logging | Separate radio logs and shared per-radio-filterable noise log; test retention changes, export and limits | Expect 250 signals per radio and 2,000 shared noise events by default |
| Crash reporting | Restart and crash history persist across reboot; stored crash build identity is explained clearly | Do not share raw core dumps or secrets |
| OTA / rollback | **Only after a successful first USB boot:** build a newer Experimental release with the same board profile; observe OTA candidate checks, A/B rollback and restart health | A new source release is needed, e.g. exp.2 |
| TX diagnostics | Trace a controlled, authorized TX test if a supported test entry point exists | No general web TX/pairing button is implemented; driver success is not over-the-air proof |

**Not implemented/validated yet:** complete native device pairing and control, comprehensive v0.6 logging parity, authenticated TLS provisioning, and physical RF on-air TX confirmation. An ESP-IDF build alone never marks these as passed.

## Troubleshooting

- **No Experimental version in the Builder:** check that the GitHub Release was intentionally published; reload the release selector and enable **Show test firmware** in the wizard. Merely merging code is not enough.
- **Firmware version shows Stage 20:** the build came from local development source, an older Builder, or code that did not receive `RFLINK_RELEASE_TAG`. Rebuild from the pinned Experimental release in Builder v0.6.12 or later and check the manifest; do not rename binaries to fake a release.
- **No Wi-Fi after USB flash:** check device-specific provisioning, NVS offsets and router credentials using only private logs. Do not paste passwords or OTA keys into issues.
- **OTA refuses an image:** check the ESP-IDF native project, exact board profile and `-b<hash>` suffix. A different GPIO or SPI profile requires a controlled USB procedure, not a forced OTA.
- **Boot rollback:** preserve serial logs and reset reason. Check at least one correctly wired CC1101, RX worker health and both OTA partitions. Never disable safety checks just to mark a test green.

After completing the table, record which checks are **host-tested**, **official ESP-IDF-built**, and **physically measured** separately. Promote to Pre-release only with the required results. Do not publish Stable from an untested device.
