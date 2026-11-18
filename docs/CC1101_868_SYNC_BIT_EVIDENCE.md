# 868 MHz synchronous DATA+CLOCK evidence — experimental bench capture

**Status: source implemented; separate board proof pending.**
The ordinary RFLink firmware still uses its existing fixed 63-byte
868 MHz FIFO receiver. This feature is compiled and reachable ONLY
in a locally built firmware with `RFLINK_ENABLE_SYNC_868_BENCH=ON`.
It is **not** available in the normal Builder, release firmware, Home
Assistant Ingress, RFLink UI or publicly reachable RFLink diagnostics.

## Why capture raw bits?

The candidate synchronous 868 receiver provides DATA and CLOCK, not
packets. To verify one fixed 2-FSK radio configuration with both Itho
and RAMSES-II we must retain raw bit order, partial blocks, acquisition
epochs and known discontinuities. A 2,048-bit buffer boundary is not
a valid radio packet; a marker of lost clock edges is not a new packet.

### What this increment does

- A dedicated, preallocated **256-block PSRAM store**, each block up to
  2,048 MSB-first bits, with bit count, first/last sample timestamps,
  receiver epoch, discontinuity marker and a window-end indicator.
- **Explicit arm and freeze** actions. A capture never starts at boot.
  At nominal 38.4 ksymbols/s, full capacity holds ~13.7 seconds of
  bits (or less if partial blocks consume slots). A full store freezes
  automatically and preserves the oldest evidence. No silent overwrite.
- No modem/profile switching on arm, freeze or export. The radio keeps
  receiving on its already selected fixed synchronous profile.
- The existing GDO clock ISR and bit-buffer pipeline continue to run.
  Only the dedicated RX task writes evidence to PSRAM, not the ISR.
- A private management-key-protected local HTTP API provides recording
  status, arm/freeze, and CSV export. The network response sends one
  immutable block at a time, without holding a radio mutex during
  socket waits. Starting a new recording during export is refused.
- A PC-side `firmware/tools/inspect_sync_bits.py` checks payload lengths,
  MSB bit padding, timestamps, epochs, capture markers and declared
  totals. Optional bit-packed segments preserve loss boundaries for
  later offline replay. **It does not yet decode Itho or RAMSES.**
- Raw RF bytes are **never** added to System/Web console, routine
  diagnostics, Home Assistant, MQTT or GitHub. No new persistent NVS.

### How to use on a dedicated, explicitly enabled bench image

These steps are **for an isolated lab board only** after a separate
full ESP-IDF bench-on build and safe firmware rollback have been
verified. Do not enable on your running gateway.

1. Build an isolated ESP-IDF image with the local CMake option
   `RFLINK_ENABLE_SYNC_868_BENCH=ON` and use the correct generated board
   profile. Bench operation changes the actual 868 receiver to a **new,
   unvalidated fixed** GDO DATA+CLOCK profile.
2. From a trusted local computer on the same private LAN, set your
   device's management key in a shell environment variable
   `RFLINK_KEY` **without putting it in shared logs or scripts**.
   The key travels in an HTTP header over **unencrypted HTTP**.
   Do not expose the device to the internet or use remote cloud access.
3. Check receiver status (replace the documentation-only TEST-NET example IP with the actual device):

   ```bash
   curl -H "X-RFLink-Update-Key: $RFLINK_KEY" \
     http://192.0.2.10/api/diagnostics/868-sync
   ```

4. Arm a new recording, then operate an actual 868-MHz device promptly
   (for example, press known Itho/Orcon buttons several times). A new
   arm deliberately clears the **previous** captured blocks:

   ```bash
   curl -H "X-RFLink-Update-Key: $RFLINK_KEY" \
     -X POST --data-binary arm \
     http://192.0.2.10/api/diagnostics/868-sync
   ```

5. Freeze before exporting, or let capacity auto-freeze. A frozen
   capture remains in PSRAM until the next arm, firmware restart or
   power loss:

   ```bash
   curl -H "X-RFLink-Update-Key: $RFLINK_KEY" \
     -X POST --data-binary freeze \
     http://192.0.2.10/api/diagnostics/868-sync
   ```

6. Save the CSV **privately**, then verify it on your PC:

   ```bash
   curl -f -H "X-RFLink-Update-Key: $RFLINK_KEY" \
     -o RFLink_868_SyncBits_Bench.csv \
     http://192.0.2.10/api/diagnostics/868-sync.csv

   python firmware/tools/inspect_sync_bits.py RFLink_868_SyncBits_Bench.csv
   ```

   To prepare contiguous loss-aware chunks for later decoder replay,
   add `--segments-json private_segments.json`. That output contains
   demodulated RF bits: treat it as confidential device data.

In normal firmware the above paths do not exist (404), even if the
device's management key is known.

## CSV format (v1)

The CSV is immutable during a successful export. It begins with
`#schema,rflink-sync-bits-v1`, its mode, radio, generation, capacity and
recorded bit count. Each data row provides:

```text
index,epoch,bit_count,first_us,last_us,discontinuity_before,window_end,hex_msb
```

`hex_msb` is MSB-first packed with the low unused bits of the
terminal byte set to zero. A receiver epoch changes after a radio
stop/restart (including TX). `window_end=1` is an explicit capture
boundary, **not a packet delimiter**. `discontinuity_before=1`
indicates known missing/abandoned evidence before that block.
Offline replay must never bridge these boundaries. A timestamp
gap by itself is **not** proof of a missing clock edge.

## Limitations and required hardware gates

- The candidate ~38.4k interrupts/s GPIO ISR has **not** been
  validated under Wi-Fi reconnect, OTA, both 433 OOK/RMT receivers,
  heavy logging or flash cache-disabled periods. A peripheral-based
  sampler may be needed to eliminate uncounted missed GPIO edges.
- `ring_dropped_total` and `clock_missing_reported_total` are
  **cumulative since boot**, not isolated to a single capture.
  The store preserves any known per-block discontinuity markers.
  Unobserved physical GPIO interrupts cannot be inferred from the
  software-only drop counters.
- When the recorder reaches capacity, the sample pipeline continues
  but no more raw evidence is retained until a new manual arm.
  The store is bounded: **older blocks are never overwritten**.
- The fixed RF configuration has **not been proven compatible with
  real Itho AND RAMSES-II** signals. We must measure GDO0 DATA and
  GDO2 CLOCK polarity/phase with a logic analyzer and compare known
  repeated transmissions.
- The CLI's loss-aware bit segments are **replay input**, not protocol
recognition. Do not concatenate independent segments and call that
"one valid frame"; additional preamble search and Itho/RAMSES integrity
checks must come before any device command is accepted.

The CSV is a **raw capture, not an authenticated/CRC-checked device
  packet**, and the offline inspector never claims recognized messages.
  Protocol framing and native Itho/RAMSES replay are the next step.
- HTTP key authentication protects the endpoint but does not encrypt
  the contents. Captures can include device identities and control
  commands. Never upload them to a public repository or share
  them without reviewing the data.

See [the sync manager bench plan](CC1101_868_SYNC_MANAGER_BENCH.md),
[the GDO clock ISR assessment](CC1101_868_GDO_CLOCK_ISR_BENCH.md), and
[the unified RX architecture](CC1101_UNIVERSAL_RX_ARCHITECTURE.md).
