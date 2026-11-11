# RFLink Universal v0.7 Radioactive — implementation status

This page distinguishes **code present**, **host-tested**, **built with official ESP-IDF**, and **verified on hardware**. Native v0.7 remains a development prototype and is **not** verified for production hardware.

| Area | v0.7 state |
| --- | --- |
| Native ESP-IDF project | **Official ESP-IDF v5.5.6 build passed** in GitHub Actions [run #10](https://github.com/juulius/rflink-universal/actions/runs/42224090381) on 2026-11-06; no hardware test yet |
| Three CC1101 radios / shared SPI | **Development branch, not hardware-validated:** cold-boot PARTNUM/VERSION plus boot-only two-pattern CHANNR readback/restore. A manager-serialized read-only identity check samples each active receiver around every 5 seconds; three consecutive failures release RX and mark that radio unavailable. Radio-specific System events, raw identity samples and a compact Diagnostics panel report the evidence. No changes to the radio GPIO profile, SPI clock or OTA safety checks. Full ESP-IDF CI and powered/unpowered hardware acceptance pending. A radio may take up to approximately 15 seconds after loss of SPI response to show unavailable; this is not a rail-voltage measurement. |
| RX 433.42 and 433.92 MHz via RMT | Experimental, Stage 21 manager watchdog restarts stopped RMT captures; host regression tests, hardware verification pending |
| RX 868.30 MHz packet FIFO | Experimental, Stage 21 lost-IDLE recovery and 3-consecutive-SPI-error fail-closed handling with host simulation; hardware verification pending |
| Universal UART, RMT, FIFO transmit | Stage 22 TX preflight permits already-receiving radios; RX pause failures are distinct from physical TX failures, prior RX is restored after recoverable UART/RMT/FIFO TX. Host regression on all three radios pending CI; web TX and RF on-air verification pending |
| Itho and RAMSES-II decoding | Both use the existing shared 868.30 MHz 63-byte packet RX; RAMSES-II does not need a separate RX profile. Host-tested, but overlapping classification (some Itho frames also parse as RAMSES) and end-to-end RF verification remain pending |
| Web status and firmware upload | Prototype; security and hardware tests pending |
| Crash reporting and boot history | Stage 23 merged: native ELF flash core dumps, bounded NVS boot history, redacted crash summary and compact prototype cards. Official ESP-IDF compile passed; real panic/watchdog/OTA verification pending |
| Native logbooks | **New development PR (CI/hardware pending)**: three isolated 250-entry signal rings, one shared 2,000-entry noise ring, **separate 512-entry System and Web rings**, bounded 64-entry pages across the complete retained history, matching filtered CSV export, per-ring usage/overwrite counters, four All/Radio/System/Web views with one optional Include noise filter and detailed client/server Web API audits. The 1.5 MiB RAM cap and NVS limits for radio/noise remain; no RF payload or passwords are stored. Each full ring overwrites only its own oldest entries. Web and radio operation require actual hardware validation. |
| Universal TX diagnostics and CSV | Stage 25 merged and official ESP-IDF build passed: every real transmit_encoded attempt is tagged with a correlation ID and records preflight rejection or start/completion, selected UART/RMT/FIFO, driver outcome and exact RX restore/failure stage. All six rings support filtered chronological CSV export. No web TX button exists, and driver success **does not** prove on-air RF transmission |
| Crash build identity | Stage 26 merged: show the crashing ELF hash from the checksum-verified core dump, the running firmware version and ELF hash, and whether hashes match fully, only by prefix, or differ. Distinguish missing from invalid/unreadable crash data. Download redacted metadata as JSON. Host/official IDF build passed in CI; hardware tests pending |
| Durable startup checkpoints (Stage 27 PR) | **New development code, unverified on hardware**: separate versioned 12-boot NVS phase journal for last reached stage (NVS, radio probing, RX start, network, OTA validation, app ready); included in sanitized crash API and Builder diagnosis. No persistent serial-text log, safe mode or claim of crash causality. Host regression and official ESP-IDF CI must be green before review; physical crash tests pending |
| Early diagnostic services (Stage 28 PR) | **New development code:** crash history, in-RAM logs, Wi-Fi and HTTP status/crash/logs/OTA begin before CC1101 SPI/RMT startup. Radio-status consumers use a release/acquire publication gate and report radio_initialized and startup_complete separately. Builder v0.6.23 defers MAC-based post-flash auto-link until full startup, while showing early diagnostics. Official ESP-IDF CI and hardware bootloop tests pending; no safe mode yet |
| OTA A/B rollback | Code present; end-to-end update and rollback tests pending |
| MQTT client / hub discovery | Code present; broker integration and hardware tests pending |
| Native device registry, pairing and controls | Not completely migrated |
| Browser USB first-install (Builder v0.6.13 PR) | Browser-PC Web Serial handoff implemented: device-verified private factory image (bootloader, partition table, NVS, OTA data and app), sent to official ESPHome Web HTTPS tab. Host tests and first actual Windows/ESP32-S3 USB install pending. Previous HA-host USB path retained |\n| Home Assistant Builder | v0.6.12 (new test-stage code, CI pending) stamps pinned release tags into the native ESP-IDF build and cross-checks its package identity. v0.6.11 release channel picker passed CI. v0.6.10 adds safe removal of disposable ESP-IDF build workspace after all firmware jobs, and distinguishes raw container RAM from an inactive-file-cache-adjusted working-set estimate. Earlier v0.6.9 limits discovery memory pressure using 12 full-scan / 4 health-check workers and opener reuse. Home Assistant post-build savings still need measurement; GHCR publication remains disabled |
| PR-driven release automation | Development PR adds a release-plan CI check, post-merge publication after successful main CI and official ESP-IDF rebuild, immutable incrementing Experimental tags, and one-shot PR requests for RC/Stable promotion. No end-to-end workflow execution or hardware validation yet |
| GitHub release selector | Stable, Pre-release and Experimental labels and pinned source selection implemented in Builder v0.6.11; the first Experimental release has not yet been published; the manual promotion workflow and install flow are not hardware-validated |

Crash reports are retained locally and do not depend on the Home Assistant Builder. The flash dump and NVS boot history must be tested through restarts and power loss before this stage is marked hardware-validated.

## Builder v0.6.31 idle memory release (development)

The Home Assistant Builder attempts a one-time Python garbage collection and glibc heap trim only after a firmware build, LAN scan or known-device health check returns to a ten-second idle window. The existing memory monitor reports the measured process RSS before/after; it does not purge the ESP-IDF toolchain, compiled firmware packages or Linux filesystem cache. This behavior is host-testable, but reduced RSS on Home Assistant OS has **not** yet been measured or validated. A stable plateau, not zero RAM use, is the acceptance goal after repeated builds and device scans.

## Builder v0.6.32 OTA progress and firmware checks (development)

The Builder tags each OTA/USB install with a unique deployment ID, so late HTTP replies from a previous installation cannot overwrite the current device's progress. OTA upload occupies 0–90% of an overall progress meter; acknowledgement and boot verification occupy the remaining portion, and 100% requires the new ESP32 to report healthy OTA status. Transfer bytes are shown exactly alongside the overall bar. The existing HA Builder device monitor also checks published releases hourly, and the device Firmware view refreshes the catalog every 15 minutes while visible. GitHub backoff and cache limits remain. Host tests and cross-platform Builder image checks are required; physical UI timing and OTA behavior must be verified on a test board. Native ESP-IDF firmware and CC1101 logic are unchanged.

## Unified Builder health and ESP-IDF console — Builder v0.6.24 (development)

The Builder now performs fast saved-IP identity-checked health probes on page
reload instead of automatically scanning entire LAN subnets. A full scan
remains a deliberate operator action or a bounded post-flash enrollment step.
A pending probe is shown as **Checking**, not prematurely **Offline**.

A small, volatile firmware ring mirrors normal application ESP-IDF log output
via `esp_log_set_vprintf` from the start of `app_main`. The
`/api/diagnostics/console` endpoint is explicitly management-key protected
and bounded; the Home Assistant Builder first checks board signature and
physical RFLink identity before passing that key to the correct device.
Wi-Fi Diagnosis and OTA post-flash console can display this same application
log alongside the existing structured logs and persistent boot/crash history.

This does **not** make the ROM bootloader, a pre-application crash or a
pre-network failure visible as live Wi-Fi serial output. Such failures still
need USB or the existing saved reset/crash metadata. Older images do not
have the protected endpoint and use the previous structured log fallback.
The rolling log may overwrite lines and filtering may suppress sensitive
entries. Privacy, exact board identity, official ESP-IDF build, flash/boot
stability, OTA restarts and physical multi-radio operation still require CI
and separate hardware acceptance. **Not yet hardware validated.**

## ESPHome/native architecture feasibility — shadow-decoder experiment

**Hardware evidence (November 9, 2026):** On the same ESP32-S3 and three
CC1101 modules, the user re-flashed **v0.6.6 Radio Gaga** and all radios
came online. Native **v0.7 Experimental build 7** continued to report
`ManagerError::driver_failure` for both 433 MHz receivers during startup
and periodic recovery. Neither a successful GitHub ESP-IDF compile nor a
CC1101 SPI probe demonstrates functioning packet/pulse reception.

Because the existing native-first approach has not yet reached hardware
parity with this known-working system, a **separate, non-production
feasibility experiment** now lives under
[`experiments/esphome_hybrid/`](../experiments/esphome_hybrid/README.md).
It prepares a fresh copy of the locally supplied v0.6.6 source ZIP, with
ESPHome retaining exclusive ownership of CC1101, GPIO, SPI, RMT, OTA,
Wi-Fi and TX. The v0.7 pure C++ `NativeDecoder` receives a read-only
copy of the *existing* 868 MHz `on_packet` callback and produces only
metadata counters; the original legacy callback still runs first.
The 433 MHz paths are untouched.

**Hybrid source integration correction:** the v0.6.6 ESPHome decoder packs
already link their own `rflink_ramses_ii` codec, which has the same global
function names as the native v0.7 port. The isolated preparation script now
renames the **copied** native decoder's RAMSES symbols only, avoiding a
duplicate-symbol linker error without touching the radio or legacy codec.
The source generator can export a standalone ZIP and refuses to overwrite
an existing experiment. An actual provided v0.6.6 source ZIP was confirmed
with SHA-256 `d91dde4f579a92e5313b13e6499de45ff4758ad149045817f6014b444a519a8f`.
Local C++ smoke tests linked both RAMSES codecs in one process, and YAML
structure/config checks passed. Full ESPHome firmware compilation and
physical three-radio validation remain **outstanding**.

**Status:** host-tested (four new regression checks passed in GitHub Actions [run #42617448547](https://github.com/juulius/rflink-universal/actions/runs/42617448547)), not yet compiled with ESPHome,
not flashed, and **not hardware-validated**. This does **not** change the
native-only production rule in `AGENTS.md`, modify the native firmware,
switch the Builder, or release a new firmware binary. A hybrid production
pivot requires explicit decision after verified ESPHome compile, 3-radio
hardware reception and controls/OTA regression tests. Keep working v0.6.6
as the physical fallback.

## Native port of the working v0.6.6 radio stack — first implementation stage

**Architecture correction:** keep the standalone native ESP-IDF v0.7 firmware,
with the *complete operational behavior* of the working Radio Gaga v0.6.6
CC1101/SPI/RMT/RX/TX/recovery code as the verified reference. No ESPHome
runtime, YAML or firmware dependency in production. The previous ESPHome
hybrid decoder trial is not the selected production architecture.

The first native parity component, `rflink_native_boot_guard.cpp`, ports
v0.6.6's 200 ms high-impedance radio-pin delay, safe CSn/SCK/MOSI/MISO
setup, one-at-a-time 5 × 10 ms SO/CHIP_RDYn checks, and 25 ms quiet gap
before SPI. A missing radio is optional; GPIO configuration errors stop
radio startup. This runs once before CC1101 and network initialization
and does not assert radio reception or modify decoder logic.

**Status:** source implemented with simulated GPIO/ready-signal regression;
official ESP-IDF CI and physical test remain independent gates. This
does **not** fix the known 433 RMT startup failure until the remaining
radio/RMT/GDO/recovery paths are ported and hardware checked.

See [full v0.6.6-to-native fidelity matrix](NATIVE_RADIOGAGA_PARITY_PORT.md).

## Confirmed 433 MHz RMT start failure — Stage 34 (native fix awaiting bench)

**Hardware evidence (November 9, 2026, v0.7.0-exp.9):**
A fresh USB flash booted, all three CC1101 modules passed the Radio Gaga
pre-SPI boot guard and SPI identity probes, and Wi-Fi came online.
**Both 433 MHz receivers failed specifically at ESP-IDF RMT receive-arm,
not at CC1101 SPI/register programming.** Actual serial output:

```text
rmt_receive: signal_range_min_ns too big, should be less than 3187 ns
rflink.rmt: radio=0 GDO2=6 start stage=6, ESP_ERR_INVALID_ARG (0x102)
rflink.ook: radio0 RX start FAILED step=9, esp_err=258
rflink.rmt: radio=1 GDO2=14 start stage=6, ESP_ERR_INVALID_ARG (0x102)
```

**Root cause:** The native RMT RX driver passed Radio Gaga's
`filter: 30us` directly as `signal_range_min_ns=30000`. On ESP32-S3
and ESP-IDF 5.5.6 the actual RMT *source clock* (typically 80 MHz),
rather than the 1 MHz symbol resolution, limits the hardware filter
to 255 ticks (~3.187 µs). That invalid argument prevented both
`rmt_receive()` calls and caused repeated five-second watchdog retries.
The preceding high-level `ManagerError::driver_failure=7` did not
identify the low-level cause.

**Native implementation:** Read the RMT source clock using
`esp_clk_tree_src_get_freq_hz()` and clamp the desired 30,000 ns
hardware threshold below the 8-bit filter-register limit, as
ESPHome's receiver does. A bounded, allocation-free software filter
then coalesces sub-30 µs raw pulses before passing signed pulse data
to the native decoder. Preserve two non-DMA 96-symbol channels,
GDO2 GPIO6/14, 5 ms end-of-frame idle, and all radio GPIO/SPI/TX
configuration. Both initial receive arm and recovery re-arm share the
same validated threshold. Do not remove the source clock query:
the 1 MHz receive-symbol timebase **does not** define the hardware
glitch-filter counter's clock.

**Validation:** C++ host tests simulate the 80 MHz clock limit and
exercise software pulse filtering. The existing Stage 8 source contract
has also been corrected: it previously required the invalid 30,000 ns
hardware setting; it now enforces source-clock clamping and post-capture
filtering. GitHub's official ESP-IDF build is the next gate. The user's physical board has **not** run this new
fix yet. Only a bench test showing both 433 RX channels listening
**and receiving real pulses**, uninterrupted 868 RX, recovery and no
bootloop can establish hardware parity. Continue to keep v0.6.6 as
the fallback until those checks pass.

Additional unrelated warning: occasional `/rflink/api/status` 404
does not cause receiver initialization to fail; the existing native
API endpoint is `/api/status`.

## 433 MHz source-first failure diagnosis — Stage 33

**After PR #75:** The Stage 32 native Radio Gaga pre-SPI boot guard is already
merged into main. Stage 33 was rebased by merging that exact main commit into
this diagnosis branch; it deliberately preserves the boot guard in
`firmware/main/main.cpp`, where `prepare_radio_pins_before_spi()` runs
before network and shared SPI initialization. The only Stage 33 change to
that file selects the complete `OokPulseReceiver` providers for exact
CC1101 pre-RMT error reporting. The generated native source manifest and
Builder source ZIP must describe the combined tree, not either older PR
in isolation.

The real Radio Gaga v0.6.6 ESPHome stack is the behavioral reference for
**the fully native** ESP-IDF radio-port direction. Stage 33 does not change
the CC1101 modem profile, GPIO, SPI bitrate, 96-symbol RMT allocations or TX.
Instead it makes the first failing *pre-RMT* step observable before another
blind rewrite: `OokPulseReceiver::start()` reports stable error stage IDs,
the original `esp_err_t`, any offending register address, and the
expected/actual register readback.

Stable stages: **0** none; **1** RX mutex; **2** invalid profile;
**3** missing hardware; **4** CC1101 SIDLE; **5** frequency register write;
**6** modem register write; **7** modem register read;
**8** readback mismatch; **9** RMT channel/receiver start;
**10** CC1101 verified RX entry. A failure at stage 4–8 or 10 will
typically show `rx433xx_rmt_error_stage=0` even though the radio manager
reports `ManagerError::driver_failure=7`; that is expected and now
distinguishable.
Test interpretation: start stage **8** identifies an exact readback mismatch
(register/expected/observed), whereas start stage **9** directs attention to
the separate `rx433xx_rmt_error_stage/code` pair. Values are reset after a
confirmed successful startup, and repeated watchdog retries increment the
failure count without inventing packet reception. The native radio overview and identity-checked Builder
diagnosis Copy/Export include both the new CC1101 start stage and existing
RMT driver stage.

The working v0.6.6 YAML uses two independent 433 MHz OOK receivers, GDO2
GPIO6 and GPIO14 (30 μs filter, 5 ms idle, 96 RMT symbols non-DMA each).
The native v0.7 RF modem configuration remains an unverified partial
translation of ESPHome's CC1101 setup. Stage 33 deliberately **does not**
equate identical frequencies with full initialization/register/IRQ parity.
An actual build, flash and on-device register/RMT report are required
before implementing targeted register, RMT allocation or receiver recovery
changes. Keep v0.6.6 as the hardware baseline.

## Native CC1101 receive restoration — Stage 29 (development)

The v0.6.6 Radio Gaga **source archive** and the upstream ESPHome CC1101 driver were compared against the native Radioactive receive and transmit paths. The legacy adapter explicitly released UART, reset and reconfigured the radio, re-established packet receive mode, and checked the CC1101 RX MARCSTATE after transmitting. ESPHome also waits for the hardware state to settle and retries calibration if the PLL fails to lock.

This Stage 29 change makes **all three native receiver start/recovery paths** wait for a confirmed CC1101 IDLE (0x01), flush the RX FIFO only in IDLE, enter RX (0x0D), and check FSCAL1 PLL lock with a bounded retry. Native UART TX cleanup explicitly returns GDO0 to MCU input mode and logs the exact failing release step or register address. Recovery deliberately **does not send SRES during an active radio session**; the radio manager retains its fail-closed fault state if verification fails. The 868 MHz polling receiver's overflow/IDLE recovery now uses the same verified path.

**Status: source implemented, host regression/official ESP-IDF CI pending, NOT hardware-validated.** A real build-5 receiver-restore failure has not been reproduced or diagnosed from ESP32 logs. Record the frequency/radio, TX correlation ID (if any), previous RX/TX state, exact first `rflink.spi` / `rflink.uarttx` error, post-recovery MARCSTATE and receiver status during a controlled test. A passing state transition is **not** proof that RF packets are actually decoded; check reception after recovery. If this bounded approach still fails, design a *controlled* per-radio hard reset and full profile/GDO reinitialization under exclusive ownership, instead of silently resetting a live radio.

**Release gate:** Recheck the final PR commit after the automated source-manifest and Builder-ZIP regeneration. A green test for an earlier commit does not validate the final firmware source archive. Keep Stage 29 experimental until the official ESP-IDF build and bench RX/TX recovery tests have passed.

**Bench troubleshooting after a verified build:** On a separate test ESP32-S3, capture a USB serial trace beginning **before** the first receive start. Distinguish `RX restore IDLE wait failed`, `RX restore RX wait failed`, `RX PLL unlock`, `TX cleanup UART1 release failed`, `TX cleanup GDO0 ...`, and `TX restore register ... mismatch`. Record whether the issue occurs during initial boot, a 868 MHz FIFO overflow, an RMT receive restart, or an actual UART TX operation. Test that the previously listening radio resumes receiving real packets; do not label a CC1101 MARCSTATE transition as proof of radio reception.

## 433 MHz dual-RMT receiver configuration parity — Stage 30 (development)

A real Experimental build-5 device log shows `ManagerError::driver_failure` (numeric 7) for **both 433.42 and 433.92 MHz** at +5.396/+5.416 seconds, with repeated watchdog attempts every ~60 seconds. The error originates in `RadioManager::start_receive()` when the lower-level pulse RX driver returns failure; it is **not evidence of failed TX-to-RX restoration**. No 868 MHz receive failure appears in that log. Wi-Fi recovered before the 433 MHz failures, and the stored reset history shows a completed boot. The full device serial trace has not yet been captured.

Direct comparison with the supplied **v0.6.6 Radio Gaga source ZIP** (`rflink-universal.yaml`, `remote_receiver` blocks and early `RadioBootGuard`) found:

| Setting | Working v0.6.6 ESPHome | Native v0.7 before Stage 30 |
| --- | --- | --- |
| 433.42 MHz RX pin | GPIO6 (GDO2), input with pull-down | GPIO6 GDO2, RMT input without explicit pull-down |
| 433.92 MHz RX pin | GPIO14 (GDO2), input with pull-down | GPIO14 GDO2, RMT input without explicit pull-down |
| Non-DMA RMT memory per receiver | 96 symbols, explicitly shared across two RX channels | 64 symbols, no DMA |
| RX filter / idle | 30 µs / 5 ms | 30 µs / 5 ms |
| SPI rate | 200 kHz | 200 kHz |
| Modulation and RX filter | ASK/OOK, approximately 203 kHz | ASK/OOK, matching MDMCFG2/MDMCFG4 |
| GDO0 | Not used for reception; explicitly disabled as receiver output | IOCFG0=0x2E during RX |

Stage 30 aligns non-DMA RMT buffer allocation (96 symbols) and GDO2 input pull-down for each 433 MHz receiver. Importantly, every low-level RMT startup failure now retains a **stage** and raw ESP-IDF error code in RAM, exposed by `/api/status` as `rx43342_rmt_error_stage`, `rx43342_rmt_error_code` and their 43392 counterparts. The device's radio overview displays the same information. Stages: **1** queue, **2** channel allocation, **3** GPIO pull-down, **4** callbacks, **5** channel enable, **6** receive arm, **7** rearm.

**Hardware acceptance checklist for the next build:** After flashing, check the device Radio overview *before* sending anything; both 433 channels should show RX active, with no RMT error stage. Leave the device running for at least two watchdog cycles and verify no repeated `Receiver restore failed` events. Test actual 433 pulse reception on each channel. If either channel still fails, copy the `RMT fout` stage plus the numeric ESP-IDF error from the radio overview, or retrieve the corresponding fields from `/api/status` over the local network. Keep this report separate from the optional USB serial trace, which remains necessary for radio-register errors occurring before Wi-Fi connects.

This preserves the current fail-closed radio-manager behavior and does **not** add RF transmission, dynamically change board pinout, reset radios during normal RX/TX, or promise that parity alone fixes the hardware fault. It is a **development change**, requiring host regression, official ESP-IDF compile and real bench verification. If code 7 persists, capture the new stage and ESP-IDF error code; a separate high-level `ManagerError::driver_failure` is not enough to identify a failed underlying API.

## Builder settings navigation and locking (UI proposal)

The Builder's Advanced view now explains its existing safeguard: once a configured RFLink has been flashed or linked, pinout and embedded Wi-Fi configuration cannot be edited in place. Device-level **Firmware** remains the daily update workflow; **Defaults & diagnostics** only changes new-device defaults, while **Source & release history** is for developer-source comparison and manual recovery. This is a UI-only clarification on this feature branch: it does **not** implement in-place Wi-Fi or GPIO migration, automatically update firmware, or establish hardware validation.

## Builder v0.6.22 diagnostic recovery (development)

The Builder's device diagnosis now has a read-only USB serial recovery loop (up to 120 seconds after a port disappears), also reused by the direct-PC post-flash monitor. It uses only previously browser-approved ports, and accepts a re-enumerated VID/PID match only when one approved candidate is present. On Wi-Fi the Builder exposes the firmware's existing sanitized 12-entry NVS restart history and saved core-dump state, after rechecking the physical ESP32 identity. The main RFLink radio firmware, automatic USB enrollment and OTA paths are unchanged.

This is **code plus host tests only**, not a claim that Web Serial reconnect after an ESP32-S3 hardware watchdog or physical reset was observed. It does **not** persist full early-boot serial lines, decode a backtrace, add safe mode, or make Wi-Fi diagnostics available before the firmware can answer HTTP. ESP32 hardware verification and those firmware stages are separate follow-ups.

A trusted same-repository PR workflow also regenerates the native source manifest and a byte-for-byte deterministic Builder ZIP when firmware sources change.

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

## v0.6.6 device web UI migration inventory (planning only)

The source-backed [native UI parity and URL migration plan](NATIVE_UI_066_MIGRATION.md)
maps the five legacy device screens to native v0.7 API readiness.
The old ESPHome interface requests `/rflink/api/status`, while this firmware
implements `/api/status`. A 404 for the legacy path identifies an incoming
legacy client request, not an RF driver failure. The caller has not been
identified; old browser tabs or integrations remain possible.

**Implementation status:** analysis and offline URL source checks only.
No full device, learning, library or pairing UI has yet been migrated.
Existing real CC1101 probe/RMT start evidence is not proof of end-to-end
RF reception or on-air TX.

## Native web UI Stage 1 (pending validation)

The native ESP32 web interface now has a source-level first-stage migration
under `firmware/main/webui/`: standalone, offline dark/mobile UI with
Status, Logs, Diagnostics and settings backed by existing 0.7 API routes.
At `/`, configured boards load this new shell; `/advanced` retains the
original diagnostic/OTA recovery page and unconfigured boards still serve
the Wi-Fi setup form. Devices and Add device are explicitly unavailable
until native device storage, learn/pairing and authorized TX are implemented.

**Validation:** code integrated; offline tests and official ESP-IDF compile
must still pass. Not yet flashed, not tested for actual RF reception, and not
feature-complete with v0.6.6. The remaining legacy `/rflink/api/status` 404
origin is not identified; new native JS calls only `/api/...`.

The first UI build exposed an ESP-IDF embedded linker symbol mismatch; the implementation now generates an in-build C++ asset header using CMake `configure_file` rather than referencing linker-generated names. The source web assets remain separate and offline. Official IDF compilation is still the release gate.

## Experimental 12–15 USB boot-loop regression (November 10, 2026)

**Measured on ESP32-S3 N16R8, not yet resolved on hardware.** A browser-USB
flash of `0.7.0-exp.15` completed, but the application crashed immediately
after the persistent crash-history log, before early Wi-Fi diagnostics or
CC1101 initialization. Core 0 raised `LoadProhibited` with
`PC=0x4202423c` and `EXCVADDR=0x00060927`, repeatedly saving a flash
core dump. Build-15 symbolic decoding in GitHub Actions matched:

`app_main():main.cpp:255` → `esp_ota_get_state_partition()` →
`esp_ota_get_app_partition_count()` → `esp_partition_find_first()` →
`esp_partition_next():partition.c:337`.

The code path was introduced by PR #87 at Experimental 12. The exact invalid
partition-list pointer origin is **not yet proven**; a software-triggered
partition walk before networking is directly verified. This change defers the
pending-OTA status scan and `checking` report until *after* essential network,
diagnostic, and radio startup. OTA A/B validation, successful/failed health
reporting, and rollback decisions remain in place.

This is a **targeted boot-path mitigation**, not proof of a repaired partition
list. Before promotion, rebuild with official ESP-IDF, flash by USB on the
same board, capture serial boot through `app_ready`, verify normal Wi-Fi
discovery and all three receivers, then test an A/B OTA update and a deliberate
failed-health rollback. Also investigate the separate ROM bootloader
`SHA-256 comparison failed` warning seen during repeated reboot: the trace
shows the application loaded despite the warning, so it is not proven to
explain the `LoadProhibited` exception.


## Experimental 16: second boot-loop after PR #100 (November 10, 2026)

**Confirmed physical test:** The flash tool erased and wrote the ESP32-S3
successfully, and the device reported `0.7.0-exp.16-n16r8-b5e871b06` with
the PR #100 startup message. The radio boot guard reported
`CHIP_RDYn radios=1/1/1`; before Wi-Fi came up, the app crashed
repeatedly with `LoadProhibited`, `PC=0x42024292`,
`EXCVADDR=0x00060953`. A factory erase before the initial crash also
makes old persisted NVS contents an unlikely explanation.

GitHub Actions symbolized the **exp.16 release source** using the official
ESP-IDF v5.5.6 toolchain (the actual Builder-compiled board hash is
different, so function-level agreement is stronger evidence than an
exact binary/ELF identity comparison). The stack is:
`app_main()` → `crash_report_mark_boot_phase()` →
`nvs_set_blob()` → `esp_partition_write()` → `esp_flash_write()` →
`esp_partition_is_flash_region_writable()` →
`esp_partition_next()`. An iterator to the RAM partition list is invalid.
The first exp.12–15 crash happened at an earlier partition iterator walk;
PR #100 moved that call, but **did not establish the origin of the invalid
pointer**.

**Diagnostic mitigation under test, not hardware-proven:** Set
`CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192` rather than leaving ESP-IDF's
small default main-task stack. An overfull stack is plausible after the new
core dump, OTA health journal, log setup and repeated NVS operations, but
not yet proven. At three boundaries, emit `rflink.bootmem` logs with
minimum unused main stack, free internal heap and allocator-integrity status:
after crash history, after PSRAM log setup, and after radio GPIO guard just
before the suspect NVS phase write. A fourth log appears if that write returns.
This is intentionally observational: no NVS keys are erased, no SDK
partition safety check is bypassed, no OTA rollback is disabled, and radio
pin behavior is unchanged.

**Hardware acceptance required:** Flash on the same separate test board
and capture the complete USB trace, especially all `rflink.bootmem`
checkpoints. If the device still crashes, use the last checkpoint,
remaining stack margin and allocator-integrity status to isolate corruption
before proposing another fix. Then verify Wi-Fi, all three receivers,
boot-history persistence, OTA A/B validation and rollback. No Stable
promotion on host tests or ESP-IDF compile alone.


## Native read-only web UX, migration diagnostics and LED timing (November 10, 2026)

Experimental 18 successfully boots after the 8 KiB main-task stack change.
The user's hardware log shows three CC1101 probes, app_ready, Wi-Fi/DHCP
and device identity confirmation. Later requests to old ESPHome endpoints
`/rflink/api/status` and `/rflink/api/rx-log` returned HTTP 404. The
runtime source of native 0.7 and Home Assistant Builder uses root-relative
`/api/status`, `/api/logs` and other new paths; **the logs do not prove
which external tab/client made the obsolete requests**.

This change adds three narrow improvements, without modifying RF RX/TX:

- The **filtered in-RAM application console** at
  `GET /api/diagnostics/console?since=N` is read-only and no longer needs
  the firmware-update password. The Builder still verifies the board hash
  and physical ID before proxying its output. Raw core dumps, ROM text
  from before the app logger starts, and full USB serial remain outside
  this endpoint. Never forward this prototype's HTTP port to untrusted
  networks: its logs may still expose operational details.
- Local LED brightness/enable and log-retention forms no longer ask for a
  management password. Their POST handlers accept only JSON content type,
  enforce existing limits and reject changes during OTA. This is convenient
  on a **trusted LAN**, not user authentication or a network trust boundary;
  firmware OTA and MQTT credentials/remote configuration retain management
  key checks.
- An HTTP 404 callback records bounded, query-stripped path, referer and
  User-Agent for old `/rflink/` requests so the originating application
  can be identified in future logs. The native API deliberately does not
  implement old ESPHome paths. A test scans all shipped Builder/runtime
  JavaScript, HTML and Python for legacy API literals.

**Status LED, verified against uploaded v0.6.6 Radio Gaga source:** The
original release ZIP `RFLink_Universal_v0.6.6_RadioGaga_RELEASE_HASH_FIX_SOURCE (2)(1).zip`
was read from the active container. The implementation at
`rflink_universal/components/rflink_universal/status_led.h`, lines 117–122,
states exactly `phase=now%6000` and
`beat=(phase<130 || (phase>=235 && phase<365))`. When READY and
heartbeat is enabled, the green LED has **two 130-ms pulses every 6 seconds**
at the selected brightness, plus a continuous **8%-of-selected-brightness
green idle glow** between the pulses. Factory brightness is 20%. The
native READY pattern, initial default brightness and its source-contract
tests now reproduce this timing and behavior (integer WS2812 rounding
can differ from ESPHome's floating-point brightness scaling). Existing NVS
preferences still override defaults.

The source's `quiet` preference suppresses *only* the READY heartbeat.
Boot, Wi-Fi loss, missing radio, error and OTA keep showing; original
source priority begins with OTA, then Error. The native toggle now matches
that scope and ordering. Native v0.7's other status animations are not
claimed to be identical to every legacy v0.6.6 animation: the current
request was to match the **green ready heartbeat**. Normal RX operation,
safe OTA and CC1101 channel ownership are not changed.

The first PR check failed while compiling the new legacy 404 error handler
because ESP-IDF 5.5.6 compiles with `-Werror=format-truncation`; the
unbounded `snprintf(...,"%s",req->uri)` writes to a 96-byte array from a
possible 512-byte URI. The copy is now explicitly width-limited to 95
characters; old clients remain 404, with bounded sanitized context.

**Hardware acceptance still needed:** build/host checks cannot confirm
actual GPIO48 waveform, LED brightness, whether stale callers carry
revealing headers, or the security of an exposed network. Test the
functional UI, 404 diagnostic metadata and LED normal/disabled/alert
states on a controlled local test board before release.
