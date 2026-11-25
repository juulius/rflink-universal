# 868 MHz synchronous DATA+CLOCK evidence — experimental bench capture

> **Experimental live logging update (PR pending):** Completed synchronous bit blocks are now also handed off without blocking the RF worker to a low-priority native decoder task. It reuses the frozen RAW replay codecs and emits only classified **Itho structural candidates** and **RAMSES-II checksum-valid candidates** to the normal Radio signal log. Live 868 receiver counters in Logs → Radio → 868.30 MHz report bit activity **without claiming packets**; routine activity no longer consumes retained log entries. This does not replace or automatically arm the existing authenticated RAW evidence recorder. RC/Stable firmware continues using the existing FIFO transport, and real 868 devices still require hardware validation. If RX samples increase but no protocol candidates appear, the codec/polarity/RF profile is not yet proven.


**Status: source implemented and enabled for newly published Experimental
releases; physical reception still unverified.**
New main and Experimental builds now compile
`RFLINK_ENABLE_SYNC_868_BENCH=ON` automatically, including firmware
built through the Home Assistant Builder from a new Experimental tag.
RC/Stable builds keep the old 868 MHz FIFO receiver and do not
register the 868 sync recording endpoints.

The private management endpoints require the existing RFLink key.
They are HTTP, **not encrypted**, and must not be used through a
publicly accessible proxy/remote Ingress. Do not upload raw RF data
to GitHub or to a public service.

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
- A PC-side `firmware/tools/inspect_sync_bits.py` checks bit counts,
  timestamps, epochs and loss boundaries. For protocol analysis use
  `firmware/tools/replay_sync_bits.py`, which applies the native
  Itho/RAMSES-II decoders offline. Itho remains a structural candidate
  without a verified full checksum.
- Raw RF bytes are **never** added to System/Web console, routine
  diagnostics, Home Assistant, MQTT or GitHub. No new persistent NVS.

### How to use on the new Experimental build

These steps are for **deliberate hardware testing only**. Preserve a
working firmware/USB recovery path before replacing an existing image.

1. In the Home Assistant Builder select the **newly published Experimental**
   release from main, check the saved GPIO/profile hash, select **Build
   firmware**, then install on the ESP32-S3 N16R8. The CMake sync option
   is enabled automatically for these tags. Existing older Experimental
   tags are immutable and do not gain this mode. The 868 radio now uses
   the **unvalidated fixed** DATA+CLOCK profile, not FIFO.
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

These paths are available only in new main/Experimental firmware with
the receiver enabled. RC/Stable firmware returns 404; the RFLink
management key still must be supplied in Experimental builds.

## Browser controls inside the native Logs viewer (Experimental)

On a new Experimental build with the 868 synchronous receiver enabled,
open the ESP32's **direct private-LAN address** in your browser, select
**Logs → Radio → 868.30 MHz** and expand **868.30 MHz · RAW bit capture**.
You no longer need PowerShell/curl to start, freeze or export a bench capture.

1. Enter the same local management key as used for firmware OTA. It is
   held only in the current browser input, not in localStorage, NVS, URL,
   UI audit logs or the device's System/Web logs. This still travels over
   **unencrypted HTTP**; use it only on a trusted private LAN.
2. Select **Refresh status** to see receiver state, recording status,
   capacity, captured bits, known discontinuities and cumulative drop
   counters. The panel also polls status while open on the Logs page.
3. Select **Arm capture** (confirms that previous evidence will be erased),
   operate a known 868 MHz device, then select **Freeze**. The capture
   automatically stops when all 256 blocks are full.
4. Select **Export & inspect CSV**. The browser validates metadata, exact
   bit counts, per-block padding, receiver epochs and known loss boundaries,
   then downloads the private CSV with a stable filename. It displays
   a small summary of segments, bit balance and effective within-block
   sampling rate without exposing captured bits to routine logs.

5. Select **Analyze captured bits** to start a separate low-priority
   **native C++ replay task on the ESP32**. The panel reports progress,
   Itho **structure-only candidates**, RAMSES-II **checksum-valid**
   logical telegrams, and overlap between nested interpretations.
   **Also test inverted polarity** is an optional diagnostic hypothesis
   and does not alter the actual GDO sampling edge.

**Important boundaries:** The native on-device replay operates ONLY on
an explicitly frozen capture, never on a live RF receiver or as automatic
device control. It uses the same firmware Itho/RAMSES codecs as the PC-only
offline replay; the PC tool remains useful for comparison and deeper
diagnostics, but is no longer required for a basic protocol match report.
The new panel does not alter DATA/CLOCK sampling, GPIO interrupt choices,
RF profiles, existing PSRAM log allocations, or the offline replay tool. The ordinary **Export full CSV** can embed a frozen 868-bit capture when the management key is provided locally. Keyless and remote Ingress downloads only include capture status, not RAW bits.

The interface intentionally does not proxy the management key or RAW RF
samples through Home Assistant remote Ingress, even when the general
RFLink Logs viewer is reachable that way. RC/Stable builds return 404
for the private bench routes; the normal Logs viewer keeps working.

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
