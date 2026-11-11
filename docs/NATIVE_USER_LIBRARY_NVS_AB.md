# Native user library — A/B NVS durability (post-#140)

## Purpose
The v0.6.6-compatible decoder and profile snapshot codec from #140
now has an **ESP-IDF NVS backend** with two independent keys. This is
separate from RFLink's existing settings, crash reports, OTA state,
logging buffers, saved physical devices and decoder packs.

This PR does **not** yet connect web edits or radio callbacks to the
backend. It provides a separately testable, fail-closed storage API for
the next runtime integration stage.

## Storage and recovery contract

- NVS namespace: `rf_user_library`; keys: `lib_a`, `lib_b`.
  This namespace is new and does not overlap v0.6.6 ESPHome preferences.
- Each bank contains a complete, bounded v0.6.6 binary snapshot:
  magic, schema version (reads versions 1 and 2), generation, length,
  checksum, user decoders, user profiles and proposed counter state.
- On startup, read and validate **both** banks (including actual decoder
  compilation and recorded-example replay). Prefer the highest valid
  nonzero generation. Reading neither bank on a fresh boot reports
  `empty` **without erasing or creating anything**.
- On save, re-read the current generation while holding a mutex. Require
  the caller's exact expected generation (optimistic concurrency control).
  Reject missing/decreased monotonic protocol counters.
- Encode and decode the proposed snapshot in memory *before* writing.
  Write **only the inactive** bank, commit through `nvs_commit()`,
  re-open read-only and verify the stored complete blob byte-for-byte.
  Do not touch the previous valid slot on failure.
- Corrupted/short/undecodable banks never become current. If one bank
  is damaged, the other can be **recovered for inspection** but further
  writes are blocked. The invalid bank might have held a newer rolling
  counter, so automatic recovery cannot safely assume it is allowed to
  resume transmitting or advancing counters.
- Reject two valid snapshots with the **same generation** (ambiguous
  history), as well as zero-generation or checksum-invalid snapshots.
  Generation `UINT32_MAX` cannot be incremented.
- A failed commit or readback does not count as success, even if a
  partially written bank might exist. On uncertainty, reload and
  inspect the latest durable generation before retrying.

## Capacity and memory

The two snapshot blobs are at most approximately **16 KiB each** and
are stored in the existing 64 KiB NVS partition. NVS has its own page
and garbage-collection overhead and also stores other RFLink settings:
**capacity is not guaranteed for two maximum-size snapshots**. A
full/corrupt partition produces a clear *save failure*, never an erase
or a silent truncation. Evaluate NVS free-page headroom and realistic
libraries on the physical ESP32-S3 before enabling web edits. Do not
change the OTA partition layout as a casual fix.

The 16 KiB blob is allocated temporarily on the **heap**, not the RX
task stack. Storage writes must be invoked outside ISR/RX critical
paths and must not trigger on raw RF noise. Six existing segregated
PSRAM detailed-log rings and their 30% budget are unchanged.

## Current code review boundaries

- `UserLibraryNvs::load`: `empty`, `ready`, `recovered`,
  `corrupt` or `io_error`; return the active generation and bank.
  Does not alter caller's library on missing/corrupted data.
- `UserLibraryNvs::save`: expected generation + proposed library;
  returns confirmed next generation only after verified commit.
- No NVS format auto-migration from the ESPHome version. User-approved
  export/import will require a separate compatibility preview.
- No saved-device management or automatic approval, RF TX or pairing.
  A profile is not an RF encoder or a permission to transmit.
- Tests compile and execute the **actual** C++ NVS backend using a host
  NVS implementation with staged/committed writes, checksum corruption,
  out-of-date edits, failed set, failed commit, and bad readback.
  ESP-IDF CI separately compiles the entire native firmware. Neither
  substitutes for power-cut and RF hardware tests.
