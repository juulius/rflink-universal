# RFLink v0.7 — RX evidence and OTA diagnostics bench checklist

This development change is **not** an RF protocol implementation or a proven
fix for every 433 MHz signal. Keep the working v0.6.6 reference available. It
does not change Wi-Fi, OTA flash partitions, TX drivers, CC1101 frequency
profiles, non-DMA RMT hardware memory (96 symbols), 5 ms idle threshold or the
30 µs software pulse filter.

## 433.42 / 433.92 MHz: what changed

- **Before:** Two non-DMA receive channels each had `mem_block_symbols=96`,
  but their separate host-supplied RMT capture buffers could hold **256** symbols.
  When a capture reached 256 symbols, `complete=false` and the decoder did
  not see it. `full_frames` counted this event.
- **Now:** The host buffers hold **512** symbols. The temporary signed-pulse
  copy lives inside each static receiver rather than consuming ~4 KiB of the
  8 KiB FreeRTOS RX worker's stack. NativeDecoder accepts at most 1024 raw
  pulse durations (2 durations per RMT symbol).
- A capture that *still reaches* the new 512-symbol limit is **not forwarded**
  as a supposedly complete RF message. We explicitly count it as full.
  Do not increase hardware symbol memory or alter modem thresholds to hide
  this until the actual RF waveform can be measured.
- `/api/status` now provides `rx43342_capture_capacity`,
  `rx43342_last_symbols`, `rx43342_last_forwarded_pulses`,
  `rx43342_last_outcome` and equivalent 43392 fields.
  Last outcome codes: 0 forwarded, 1 too short, 2 buffer full, 3 filtered.
  The UI displays capacity, last sample and counts. Aggregated reasons stay
  in the **System** log for each radio every ~30 seconds.

**Physical test:** Power on all three CC1101s. Confirm RMT/RX starts with
three active radios and normal OTA boot health. At each 433 frequency,
transmit a legal known test signal repeatedly at controlled distance. Record
the number of RMT captures, full/forwarded counts, last symbols and pulse
count. Compare the same source on 0.6.6. If `full_frames` continues to grow,
capture the waveform externally and investigate RMT frame boundaries: no
arbitrary truncation/partial concatenation should be treated as decoded RF.
Check Wi-Fi availability, RX worker watchdog, LED and OTA during the run.

## 868.30 MHz: what is actually known

The current CC1101 RX profile reads **fixed 63-byte FIFO packets**. That
size is defined by the modem profile; it does not prove that a matching
over-the-air protocol transmission occurred. The profile currently has
PKTCTRL0=0x00, which does not enable packet CRC validation. The protocol
decoder requires the exact legacy five-byte Itho/RAMSES preamble. We now
report independent totals for:

- **Preamble missing** (`decode_unknown_prefix`): fixed 63-byte input does
  not begin with the supported preamble.
- **Preamble present, decoder rejected** (`decode_unknown_payload`): the
  preamble matches, but neither existing decoder accepted a message.
- **Recognized frames** (`decode_itho`, `decode_ramses`): actual supported
  decoder matched, not merely a FIFO read.

Unknown-packet log records store a small reason code: 1 (prefix absent) or
2 (decoder rejected); older version records may have 0 (reason unknown).
No RF payload, device identity, pairing key or rolling code is logged. Do not
report trustworthy RSSI/LQI/CRC metrics without implementing and verifying
them against the chosen CC1101 profile and receiving hardware.

**Physical test:** Leave radio 868.30MHz running in an RF-quiet interval and
compare preamble mismatch, overflow, decoder-rejected and decoded counters.
Then trigger a known Itho/RAMSES remote and compare results. This establishes
what the driver can and cannot decode before changing the modem profile.

## Web and OTA evidence

- Native `httpd_resp_send_err()` takes an **ESP-IDF enumeration**, not a raw
  HTTP numeric status. The Web ring now stores **400 / 403 / 404 / 500** for
  the corresponding errors, with proper warning/error severity. It still
  never stores arbitrary URL parameters, HTTP bodies or credentials.
- Native Diagnostics displays the *already persisted* NVS
  `rf_diagnostics/ota_health_v1` record: target firmware, boot sequence,
  final or in-progress verdict, count of active RX receivers and responsive
  SPI modules. The record survives ESP32 reset and OTA rollback when the
  fallback firmware understands this schema. It is the **latest report
  only**, not five full historic OTA installations. No new NVS writes or
  OTA safety/boot logic were introduced by the UI.
- Verify by completing an actual test-board OTA, letting the reboot health
  check finish, then opening Diagnostics and refreshing after a second
  power cycle. Ensure the report version and boot number are not mistaken
  for the *current* running firmware if a rollback occurred.

## Acceptance criteria

1. No new watchdog, stack overflow, failed RMT allocation or unexpected
   missing CC1101 after the buffer change.
2. Known-good 433 signals cause `forwarded_frames` to rise. If not, use
   the new capacity and outcome counters to identify capture loss; do not
   report success just because `captures` increased.
3. 868 unknown counts split between preamble mismatch and codec rejection,
   and actual decoded count still follows the original protocol validation.
4. HTTP 404 is correctly recorded as status **404**, not raw enum **6**.
5. OTA VALID stays reliable and its persisted NVS report is visible after
   reboot; changing log/receive handling must not relax OTA startup guards.
6. Full six-ring retention and CSV filters continue to function unchanged.
