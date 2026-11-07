# RFLink Universal v0.7 Radioactive — implementation status

This page distinguishes **code present**, **host-tested**, **built with official ESP-IDF**, and **verified on hardware**. Native v0.7 remains a development prototype and is **not** verified for production hardware.

| Area | v0.7 state |
| --- | --- |
| Native ESP-IDF project | **Official ESP-IDF v5.5.6 build passed** in GitHub Actions [run #10](https://github.com/juulius/rflink-universal/actions/runs/42224090381) on 2026-11-06; no hardware test yet |
| Three CC1101 radios / shared SPI | Code and host checks present; Stage 21 periodic read-only re-probe of missing radios, no resets during active TX; hardware verification pending |
| RX 433.42 and 433.92 MHz via RMT | Experimental, Stage 21 manager watchdog restarts stopped RMT captures; host regression tests, hardware verification pending |
| RX 868.30 MHz packet FIFO | Experimental, Stage 21 lost-IDLE recovery and 3-consecutive-SPI-error fail-closed handling with host simulation; hardware verification pending |
| Universal UART, RMT, FIFO transmit | Stage 22 TX preflight permits already-receiving radios; RX pause failures are distinct from physical TX failures, prior RX is restored after recoverable UART/RMT/FIFO TX. Host regression on all three radios pending CI; web TX and RF on-air verification pending |
| Itho and RAMSES-II decoding | Both use the existing shared 868.30 MHz 63-byte packet RX; RAMSES-II does not need a separate RX profile. Host-tested, but overlapping classification (some Itho frames also parse as RAMSES) and end-to-end RF verification remain pending |
| Web status and firmware upload | Prototype; security and hardware tests pending |
| Crash reporting and boot history | Stage 23 merged: native ELF flash core dumps, bounded NVS boot history, redacted crash summary and compact prototype cards. Official ESP-IDF compile passed; real panic/watchdog/OTA verification pending |
| Native logbooks | Stage 24 merged and official ESP-IDF build passed: three independent 250-entry signal rings, one shared 2,000-entry noise ring, 128-entry System and Web logs, a 1.5 MiB PSRAM memory cap and authenticated NVS size controls. Hardware test and full v0.6.6 parity pending |
| Universal TX diagnostics and CSV | Stage 25 merged and official ESP-IDF build passed: every real transmit_encoded attempt is tagged with a correlation ID and records preflight rejection or start/completion, selected UART/RMT/FIFO, driver outcome and exact RX restore/failure stage. All six rings support filtered chronological CSV export. No web TX button exists, and driver success **does not** prove on-air RF transmission |
| Crash build identity | Stage 26 merged: show the crashing ELF hash from the checksum-verified core dump, the running firmware version and ELF hash, and whether hashes match fully, only by prefix, or differ. Distinguish missing from invalid/unreadable crash data. Download redacted metadata as JSON. Host/official IDF build passed in CI; hardware tests pending |
| OTA A/B rollback | Code present; end-to-end update and rollback tests pending |
| MQTT client / hub discovery | Code present; broker integration and hardware tests pending |
| Native device registry, pairing and controls | Not completely migrated |
| Home Assistant Builder | v0.6.12 (new test-stage code, CI pending) stamps pinned release tags into the native ESP-IDF build and cross-checks its package identity. v0.6.11 release channel picker passed CI. v0.6.10 adds safe removal of disposable ESP-IDF build workspace after all firmware jobs, and distinguishes raw container RAM from an inactive-file-cache-adjusted working-set estimate. Earlier v0.6.9 limits discovery memory pressure using 12 full-scan / 4 health-check workers and opener reuse. Home Assistant post-build savings still need measurement; GHCR publication remains disabled |
| PR-driven release automation | Development PR adds a release-plan CI check, post-merge publication after successful main CI and official ESP-IDF rebuild, immutable incrementing Experimental tags, and one-shot PR requests for RC/Stable promotion. No end-to-end workflow execution or hardware validation yet |
| GitHub release selector | Stable, Pre-release and Experimental labels and pinned source selection implemented in Builder v0.6.11; the first Experimental release has not yet been published; the manual promotion workflow and install flow are not hardware-validated |

Crash reports are retained locally and do not depend on the Home Assistant Builder. The flash dump and NVS boot history must be tested through restarts and power loss before this stage is marked hardware-validated.

## Mandatory v0.6 logging migration

The [native v0.7 logging and compact UI contract](LOGGING_AND_UI_V07.md) is a release gate. Stage 23 implemented the crash-report foundation. Stage 24 added the bounded signal/noise/system/web ring engine and live NVS retention controls. Stage 25 added structured TX lifecycle diagnostics and volatile CSV export. Stage 26 adds crash-build provenance and clearer corrupt/stale report states. Full RF payload capture, permanent non-crash event storage and complete v0.6 UI parity remain outstanding. The working v0.6.6 experience should be restored before calling v0.7 a functional replacement.

## Architectural contract

**RFLink**, rather than ESPHome, exclusively owns all CC1101 SPI and GPIO access, RMT/UART peripherals and radio profile state transitions.

- Three radios use a shared SPI bus and separate chip-select and GDO pins.
- The decoder specifies RF profile and TX/RX transport, not hardware pin manipulation.
- The radio manager tracks module presence independently of its active receive or transmit mode.
- Personal pinout and SPI speed overrides belong to the Builder's persistent local settings; GitHub contains only defaults.
- Source versions and binaries are managed through versioned GitHub Releases. Compiler success and hardware validation must be tracked separately.

## Documentation requirements before first use

The full compiler build has passed for prior native revisions. A [first experimental bench guide](EXPERIMENTAL_BENCH_V07.md) now documents a test-board-only procedure and **unverified** acceptance checklist. After first device use, write **hardware-tested** guides for initial USB installation, network setup, website OTA updates, recovery, radio diagnosis, MQTT setup and rollback. Do not represent the previous ESPHome v0.6 procedures as valid for v0.7.

## Shared 868 MHz reception contract

The current `itho_868_packet` identifier names the **common 868.30 MHz hardware RX path**, not an Itho-only receiver. `NativeDecoder` attempts Itho and RAMSES-II recognition on the same 63-byte frame. The RAMSES-II-specific RF profile is TX-only in the current native implementation; it must not replace the shared RX profile. After RAMSES TX the radio manager restores the **previous** receive profile. The regression contract and current overlap caveat are documented in [Shared 868 MHz RX](SHARED_868_RX.md). This behavior is not yet validated on real CC1101 hardware.

## Stage 22 TX readiness and failure phases

The `RadioManager::request_transmit` check now accepts an already-receiving radio as *eligible*, with **no hardware side effect**. The subsequent `transmit_encoded` call still performs a serialized stop-RX → encoder-selected transport → restore exact previous RX profile. A failure to pause RX is a distinct `rx_pause` stage; the driver is not invoked after that error. A rejected invalid payload clears any stale physical TX-stage value, leaving the precise cause in `ManagerError`. The existing numeric codes for other TX stages do not change.

These are native driver and test changes only. **There is no web/API pairing or send button yet**, no transmission initiated automatically, and no on-air proof. For detail see [Stage 22 transmit guardrails](TX_STAGE22.md).
