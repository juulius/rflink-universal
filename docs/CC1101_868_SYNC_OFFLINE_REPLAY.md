# Offline Itho and RAMSES-II replay for 868-MHz DATA+CLOCK captures

**State:** This is the existing **PC-side** offline replay. A separate
experimental on-device frozen-capture replay now uses the same native
firmware codec logic and displays bounded results in **Logs → Radio →
868.30 MHz**. Neither replay is a live streaming decoder, authenticated
device event, proof of physical RF compatibility, Home Assistant service,
pairing mechanism or transmitted command.

## What the tool does

Once the separate experimental 868-MHz bench capture firmware has collected
a private CSV export, RFLink can examine its saved bits **without touching
the CC1101 or changing reception modes**.

The local replay tool:
1. Validates the exact `rflink-sync-bits-v1` CSV schema and finite 256-block
   budget with the existing `inspect_sync_bits.py` validator.
2. Builds contiguous bitstream segments **only** within the same receive
   epoch and without a known capture discontinuity or explicit window end.
   An arbitrary 2,048-bit block boundary by itself is not a packet boundary.
3. Compiles the repository's real native C++ Itho and RAMSES-II receive
   decoders on the user's PC with `g++` or `clang++`.
4. Scans each segment at arbitrary bit offsets, not just byte boundaries.
   RAMSES-II matches require framed UART 8N1 bytes, the expected sync,
   valid Manchester encoding and a valid native RAMSES checksum/length.
   Each candidate is first bounded to its own UART-framed terminator, so an
   unrelated later message cannot be credited to an earlier candidate sync.
   Itho candidates require the native 63-byte legacy packet pattern
   and structural decoder acceptance.
5. Prints a bounded JSON report with offsets, relative segment index,
   protocol, evidence grade and simple command/speed summary. **No raw RF
   payload, device address, controller ID or captured hex is printed.**

**Important integrity distinction:** `rflink_ramses_codec.cpp` validates
RAMSES-II logical checksums; the current native
`rflink_itho_codec.cpp` verifies expected headers and message fields
but does **not** independently validate an entire Itho checksum.
Therefore every Itho result is labelled `structure_only`, **never**
`checksum_valid`. Treat it as an investigational candidate rather
than a verified device command.

The native decoder scans ordinary polarity by default. The optional
inverted-polarity pass is a **diagnostic hypothesis**, not proof of how the
real CC1101 is wired.

## Running locally

Prerequisites: Python 3.10+ and a local C++17 compiler (`g++` or
`clang++`), plus the full matching repository source checkout. Do not
put actual 868 captures, output reports or encoded IDs in the repository.

1. Export the CSV **privately** from the separately opted-in bench firmware
   as documented in [private 868 bit evidence](CC1101_868_SYNC_BIT_EVIDENCE.md).
   New main/Experimental firmware supplies the protected capture-export
   API. The offline tool itself does not connect to any device.
2. On your PC, in the RFLink repository folder, run:

   ```bash
   python firmware/tools/replay_sync_bits.py RFLink_868_SyncBits_Bench.csv
   ```

3. If no match appears and a logic analyzer suggests inverted bit polarity,
   run the **additional** diagnostic pass:

   ```bash
   python firmware/tools/replay_sync_bits.py RFLink_868_SyncBits_Bench.csv --try-inverted
   ```

4. To save the small match report (never overwrites an existing file):

   ```bash
   python firmware/tools/replay_sync_bits.py RFLink_868_SyncBits_Bench.csv --report-json private_replay_report.json
   ```

The compiler makes a temporary local executable and deletes it on exit.
There is no remote download, API call, network access, transmission or
automatic installation. The tool does not need the Home Assistant Builder
or any change to the active CC1101 configuration.

## Report fields

A match includes:
- `segment`, `epoch`, `bit_offset`, `span_bits`: where it came from. For
  RAMSES-II, the bit offset starts at the verified `33 55 53` UART sync
  suffix; it deliberately excludes any earlier `FF 00` or repeated `FF`
  preamble bytes that might be incomplete in a real recording.
- `protocol`: `itho` or `ramses_ii`.
- `evidence`: `structure_only` (Itho) or `checksum_valid` (RAMSES-II).
- `polarity`: `normal` or `inverted`.
- `summary`: limited type/speed/command, without addresses or raw data.
- `hardware_verified: false`: all offline matches remain unverified on-air.

The summary counts distinguish `candidate_itho_structure_only` from
`checksum_valid_ramses_ii`. **Important:** synthetic native Itho speed
packets also contain a *nested checksum-valid RAMSES-II logical telegram*.
The report therefore explicitly separates
`ramses_ii_embedded_in_itho` from `ramses_ii_outside_itho`, and records
`overlapping_protocols` for each interpretation. These are NOT two
independent radio/device events. If the outer Itho header is damaged,
the inner checksum-valid logical RAMSES packet may remain readable;
checksum-validity alone does not verify the physical RF family.
`ramses_ii_outside_itho` means **outside a recognized Itho candidate**,
not proof the on-air sender wasn't an Itho device. The replay reports only accepted candidates; rejected UART framing,
Manchester patterns, invalid logical checksums and incomplete frames are
not counted as valid messages. This distinction prevents a corrupted
transmission from being silently promoted into a recognized device event.

A zero count is not proof that a protocol
is absent: the fixed 2-FSK profile, GDO clock edge, polarity, sampling
interrupt losses or message truncation may be incorrect.

## What will be tested

Synthetic fixtures in `firmware/tests/fixture_sync_replay.cpp` are
built with the same native Itho and RAMSES encoders used by RFLink.
The Python regressions ensure:
- Both decoder families match after arbitrary leading bit shifts.
- A RAMSES frame **can** cross normal contiguous 2,048-bit chunks.
- The same frame is **not reconstructed** across a window end, dropped
  edge marker or changed receive epoch (including after TX).
- Bad RAMSES bit patterns and corrupted Itho headers do not produce
  confident successful matches.
- Inverted polarity requires explicit opt-in and is reported as such.
- A malformed CSV is rejected before the native decoder is launched.

The replay is intentionally not an embedded streaming framer.
**Further gated stages** are: validate on real on-air Itho + RAMSES
captures using *one fixed CC1101 RF profile*, measure ISR loss under
Wi-Fi/433 load, strengthen Itho integrity beyond structural matching,
then consider bounded streaming candidate extraction on-device.

## Privacy, safety and troubleshooting

The capture CSV itself may contain RF device IDs and remote-control
commands. Keep it on a trusted computer and do not upload it to GitHub,
paste it into public issues or distribute it as a fixture. The small
report avoids raw IDs, but still keep it private.

If the tool reports an invalid CSV, rerun the existing inspector; don't
remove discontinuity markers or adjust bit counts to force a match.
If a compiler is missing, install a host C++17 compiler locally. If
RAMSES matches but Itho does not, that is not proof one modem profile
supports both: the lack of full Itho checksum and the absence of
physical demodulator validation remain explicit limitations.

## Replay directly from the full diagnostic CSV (Experimental)

After entering the management key in the local device's 868 RAW capture panel, arm and freeze a recording, then use the usual **Export full CSV** button. When that download embeds `sync868_bit_block` rows, the same offline command now accepts it directly:

```bash
python firmware/tools/replay_sync_bits.py RFLink_Diagnostics_Full.csv --try-inverted
```

The full diagnostic CSV's `sync868_capture` context must explicitly confirm `raw_embedded=true`. The parser independently validates capture generation, block order, bit length, MSB-first hex, padding and discontinuity/window markers; keyless/Ingress CSVs without the protected RAW blocks report an actionable error rather than falsely claiming there was no 868 RF traffic. Neither the analyzer nor the inspector uploads data or changes radio settings. Physical RF reception and valid decoder matches still require testing.
