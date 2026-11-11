# RFLink v0.7 raw RF monitor parity with RadioGaga v0.6.6

## Original behaviour, verified against the archived source

The archived `reference/v0.6.6-esphome/rflink_universal/components/rflink_universal/rflink_universal.cpp`
shows two distinct receive paths:

- **433.42/433.92 MHz:** `receive_raw` takes signed GDO2 pulse durations from
  ESPHome `remote_receiver`; `full_frame_detail_` writes **all durations in
  microseconds** and a width-bucketed FNV-1a hash. These are NOT bytes read
  from the CC1101 FIFO. RSSI is not available in that asynchronous OOK mode.
- **868.30 MHz:** `receive_packet` receives the original 63-byte packet
  vector and best-effort CC1101 RSSI/LQI/FREQEST; `full_frame_detail_` formats
  every byte in hexadecimal. The packet length alone is not proof of a
  recognized protocol. CRC is disabled in the current Itho profile.

The normal v0.6.6 activity list stayed compact and a separate **32-entry**
capture/export history held full details. Native v0.7 initially logged only
counts and protocol names, so a received frame could be counted but not
inspected.

## Native implementation (development)

- Raw capture ring is **PSRAM-only and volatile**; eight samples per radio.
  All three independent rings coexist with the six existing metadata log rings.
  The total native raw history memory is ~100 KiB plus C++ object metadata.
  PSRAM allocation failure is nonfatal: OTA/network/RX/log counters continue.
- Full **accepted** 433 pulse train (up to 1024 signed microsecond values),
  exact 868 63-byte FIFO input, frequency and the original v0.6.6 FNV bucket
  hash are copied in the RX worker, not in RMT ISR or HTTP task stack.
- 868 channel optionally reads CC1101 read-only status registers FREQEST
  `0x32`, LQI `0x33` and RSSI `0x34` immediately after FIFO input. Only
  if all three reads succeed are the snapshot values exposed; failure never
  changes packet delivery, RX state, modem configuration or OTA. RSSI uses
  signed register half-dB units offset by 74dB; FREQEST resolution is
  26MHz / 2^14; LQI is masked to 7 bits. No CRC-OK claim.
- Each compact Radio event stores a decoder **RX sequence** as
  `correlation_id`. The global LogStore `sequence` still tracks all six
  log buffers independently. The sequence link must not be confused with
  TX correlation IDs.
- `GET /api/radio/capture?radio=0..2&seq=N` returns only the selected
  short-lived capture. A rotated-out capture is reported as unavailable,
  not silently replaced by unrelated RF data. Regular log CSV export does
  not contain raw RF payloads.
- In the Radio tab, expand a recent pulse/unknown/recognized event to view
  the exact signed GDO2 timing sequence or the 868 FIFO hex bytes. Includes
  quality only when a hardware snapshot succeeded.
- Full/overflowed RMT captures are **not** decoded or forwarded by this
  change. The bounded host receive buffer was already enlarged in PR #124.
  Hardware validation of partial/long-frame handling remains separate.

## On-device acceptance checklist

1. Cold boot three CC1101 modules, with all radios online and no watchdog.
2. At 433.42 and 433.92 MHz, trigger repeatable legal RF signals and expand
   Radio noise events: signed timing array matches count, no fabricated bytes,
   and the same sample produces the same width-bucketed hash.
3. At 868.30 MHz, inspect a known remote transmission: full FIFO hex is
   shown and optional signal-quality fields follow hardware status; unknown
   FIFO data remains marked unknown rather than a learned device.
4. Trigger 9 different captures on one channel; the first expires, but the
   most recent sample on the other channel remains accessible.
5. Verify the ordinary log CSV is still compact; unrelated System/Web rings
   have not gained RF payload data. Confirm browser refresh and RX worker
   responsiveness while expanding a long timing capture.
6. Confirm OTA flash and its NVS health report remain reliable. Changing this
   monitor must not affect OTA slot logic, boot validation or RF modem profiles.

**Privacy:** Raw RF bytes/timings may expose identifiers or rolling-code state.
The read-only raw endpoint is intended for a trusted LAN, not the public
Internet; do not expose unencrypted HTTP via port forwarding. Do not persist
captured RF payloads to NVS or ship them in standard support CSV bundles.
