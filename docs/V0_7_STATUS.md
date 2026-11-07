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
| OTA A/B rollback | Code present; end-to-end update and rollback tests pending |
| MQTT client / hub discovery | Code present; broker integration and hardware tests pending |
| Native device registry, pairing and controls | Not completely migrated |
| Home Assistant Builder | v0.6.9 targets measured initial discovery RAM spike (33 MB → 466 MB Python RSS; later 880 MB idle) using 12 full-scan / 4 health-check workers, per-worker HTTP opener reuse and glibc arena cap. All /24 network and VLAN discovery coverage preserved, tests added; HA OS memory reduction and physical device testing pending; GHCR publication remains disabled |
| GitHub release selector | Builder code present; release and install flow not yet validated |

## Architectural contract

**RFLink**, rather than ESPHome, exclusively owns all CC1101 SPI and GPIO access, RMT/UART peripherals and radio profile state transitions.

- Three radios use a shared SPI bus and separate chip-select and GDO pins.
- The decoder specifies RF profile and TX/RX transport, not hardware pin manipulation.
- The radio manager tracks module presence independently of its active receive or transmit mode.
- Personal pinout and SPI speed overrides belong to the Builder's persistent local settings; GitHub contains only defaults.
- Source versions and binaries are managed through versioned GitHub Releases. Compiler success and hardware validation must be tracked separately.

## Documentation requirements before first use

The full compiler build has passed. Before first device use, write **hardware-tested** guides for initial USB installation, network setup, website OTA updates, recovery, radio diagnosis, MQTT setup and rollback. Do not represent the previous ESPHome v0.6 procedures as valid for v0.7.

## Shared 868 MHz reception contract

The current `itho_868_packet` identifier names the **common 868.30 MHz hardware RX path**, not an Itho-only receiver. `NativeDecoder` attempts Itho and RAMSES-II recognition on the same 63-byte frame. The RAMSES-II-specific RF profile is TX-only in the current native implementation; it must not replace the shared RX profile. After RAMSES TX the radio manager restores the **previous** receive profile. The regression contract and current overlap caveat are documented in [Shared 868 MHz RX](SHARED_868_RX.md). This behavior is not yet validated on real CC1101 hardware.

## Stage 22 TX readiness and failure phases

The `RadioManager::request_transmit` check now accepts an already-receiving radio as *eligible*, with **no hardware side effect**. The subsequent `transmit_encoded` call still performs a serialized stop-RX → encoder-selected transport → restore exact previous RX profile. A failure to pause RX is a distinct `rx_pause` stage; the driver is not invoked after that error. A rejected invalid payload clears any stale physical TX-stage value, leaving the precise cause in `ManagerError`. The existing numeric codes for other TX stages do not change.

These are native driver and test changes only. **There is no web/API pairing or send button yet**, no transmission initiated automatically, and no on-air proof. For detail see [Stage 22 transmit guardrails](TX_STAGE22.md).
