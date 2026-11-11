# Validation RXLOG1 — 2026-10-25

## Passed (source/test environment)

- Existing project source and `rflink-universal.yaml` preserved; exact YAML bytes match uploaded baseline (SHA-256 recorded below).
- Embedded web UI generated from edited HTML + all four locale files; integrity check passed.
- `rflink_universal.cpp` compiled with `-Wall -Wextra -Werror` against the project's ESPHome host stubs (C++17, USE_ESP32 + WEB + MQTT).
- 7 isolated logging storage C++ tests passed (noise-first eviction, independent signal caps, pagination, large 868 noise flood).
- Browser regression verified runtime POST logging settings, category + radio filters, downloaded ZIP with eight validated members.
- Legacy browser tests passed (desktop/mobile, offline radios, RX/TX/noise UI).
- Static regression tests passed: root/favicon, updater, board pinout, decoder baselines, release modes, packaging, RX diagnostics and version UI.
- Conservative source inventory found no byte-identical file duplicates. Eight obsolete patch-root BAT/MD items archived separately and removed from active root.

## Not verified

- Full ESPHome/ESP-IDF compilation for physical ESP32-S3 N16R8 and target linker placement.
- OTA upload, CC1101 hardware reception, allocation from actual PSRAM or reboot/uptime observation.
- Running PowerShell `MAAK_RELEASE.bat` under Windows (PowerShell not present here).
- Full project host-test suite (partially executed; exceeded execution time during deeper tests).

## Safety properties / differences

- Uses 3 signal buffers (250 default each), 1 shared noise buffer (2000 default), global max 4096 events and PSRAM arena 1.5MB max with smaller fallback.
- Preserves the original YAML, reboot configuration, TX/decoders, old diagnostic activity feed and updater workflows.
- When runtime settings are saved, only log-count limits are committed to NVS.
- Receiver captures record original full RAW/packet display and quality metrics; events over 32KiB are rejected, not truncated. This is visible in rejection counters.
- Live browser ZIP export is not transactionally frozen while high-volume RF traffic is arriving.

Baseline YAML SHA256: `1d5bc36b0c34b1e921708ab2def6cd75698e1bf1ce02cb4dbf17e1a374f45f55`
