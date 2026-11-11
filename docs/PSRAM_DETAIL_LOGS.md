# v0.7 isolated PSRAM details (experimental)

## Contract

This feature is **optional, volatile evidence storage** behind the six existing
compact log rings. It does not change NVS log settings, retention limits,
main LogStore FIFO semantics, CSV output, or the radio modem.

**N16R8 PSRAM ceiling:** 8,388,608 bytes physical; detailed logs use at most
2,432 KiB (29.69%); index/object allowance <=64 KiB and six arena allocations
sum to 2,368 KiB. The existing metadata rings (~147 KiB by default) and
ordinary firmware memory are **separate** from that detail budget. The old
~98 KiB/24-capture history is disabled when the new detail store is available,
and remains an allocation fallback if the new store cannot initialize.

| Ring | Max detail slots | Detail byte arena |
| --- | ---: | ---: |
| 433.42 MHz signal | 250 | 512 KiB |
| 433.92 MHz signal | 250 | 512 KiB |
| 868.30 MHz signal | 250 | 256 KiB |
| Shared Noise (all three radios) | 2,000 | 448 KiB |
| System | 512 | 384 KiB |
| Web | 512 | 256 KiB |

**Important:** raw 433 waveforms and unclassified 868 packets currently enter
the **Noise** compact log stream, so their detailed payloads belong to the
**Noise** detail arena. Decoded 868 events enter the appropriate radio signal
arena. Future 433 decoders can emit to their per-radio signal arenas.

For each entry, data is stored with its actual length, not padded to 4 KiB.
Up to 1,024 signed pulse widths or 63 CC1101 FIFO bytes are stored in full.
Entries evict the **oldest details in their own ring** until the next entry
fits (FIFO). Noise never evicts a System/Web/per-radio signal detail. Oversized
details are rejected without evicting existing entries. Ordinary log lines
remain viewable after their extras have been evicted.

At boot the allocation attempts 100% of the fixed detail budget, then 50%
if allocation fails. In either case, at least **4 MiB of currently free PSRAM**
must remain for other modules. If the reserve cannot be met, the optional detail
store is disabled and radio, boot history, compact logs, web and OTA continue
normally. The log reports remaining free PSRAM and largest contiguous block. Never allocate a detail buffer in internal SRAM or
perform per-event malloc/flash writes on the RX worker.

## Correlation and APIs

The original `/api/logs`, `/api/logs.csv` and `/api/logs/config` fields
remain. `/api/logs/config` additionally reports the detail-store availability,
allocated bytes, used bytes, FIFO evictions, and rejected counters per ring.

`/api/radio/capture?radio=0..2&seq=<decoder-event-sequence>` now reads the
isolated detail ring when available (legacy 8-frame fallback otherwise).
The existing JSON format is preserved for both exact signed timings and
unverified FIFO packet bytes. A decoder-event sequence is namespaced away from
the global LogStore sequence, so TX details cannot collide with RX captures.

`/api/logs/detail?bucket=0..5&radio=0..2|255&seq=<log-sequence>` retrieves
vetted bounded text for TX, System and Web. Indexes and byte arenas are
protected by a short mutex during copies; HTTP/network writes happen after the
lock is released. The web UI fetches only when a user expands a log entry.

RF bytes, raw pulse trains, rolling-code material, API query/body/headers,
Wi-Fi credentials, encryption keys and other secrets must not enter generic
System/Web text details, the ordinary CSV, NVS or support bundles. RAW RF
evidence is only read on demand; use a trusted LAN.

## Validation

The host test `firmware/tests/test_log_details.cpp` exercises FIFO eviction
without cross-ring interference, variable-size RF evidence, exact 63-byte FIFO
capture, short text details, oversize rejection and wrong-radio protection.
Firmware host-stub syntax and native ESP-IDF CI are separate checks; successful
compilation is **not** on-device RX, OTA or crash/PSRAM fragmentation validation.

Before merging as stable, verify on physical N16R8: memory after allocation,
smallest contiguous PSRAM block, long-running 433 noise, loss/recovery of CC1101
power, log scrolling while expanded, bootloop/crash history, and OTA rollback.

## CI synchronization

GitHub's source-asset workflow updates the firmware manifest and Builder source ZIP
on the PR branch after firmware edits. Those bot commits do not trigger ordinary
pull-request checks. The refresh workflow therefore explicitly dispatches the
source and ESP-IDF check workflow against the synchronized branch head.
A documentation-only follow-up commit can also trigger the regular PR checks
without changing the verified firmware source fingerprint.
