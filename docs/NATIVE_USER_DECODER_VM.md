# Native v0.7 user decoder VM — parity foundation

## Purpose and scope

This stage ports the *bounded interpreter* used by user-created decoder
recipes from the archived, working v0.6.6 ESPHome reference
(`reference/v0.6.6-esphome/rflink_universal/components/rflink_universal/user_vm.*`)
to the native ESP-IDF decoder-core component. This is a **core-only**
migration: no UI, API, runtime recipe registration, device persistence,
on-air TX or pairing capability is enabled here.

The VM accepts RX frames and computes normalised device identity,
actions and fields. TX recipe interpretation is available for safe
offline preview and parity testing, **not** as an authorized transmit
path. No ESPHome headers, tasks or services are imported.

## Preserved limits and capabilities

- Recipe source: 4096 bytes; up to 192 instructions
- Execution budget: 2048 steps; stack: 32 values
- Raw pulse TX preview: up to 512 signed durations, 50–1,000,000 µs
- Packet TX preview: up to 64 bytes; repeat count 1–10
- RX pulse match tolerance: 25%, at least 100 µs
- RX identity, action and up to 16 fields; TX next-state map
- No file, GPIO, SPI, web, filesystem, network or unrestricted C++ access
- No claim that a rolling-code algorithm can be created from recordings

The native API is deliberately minimal: `rflink_user_vm.hpp` provides
`RFFrame`, `DecodeResult`, `Program`, `compile`, `receive`, and
`transmit`. A caller supplies copied, bounded input and must validate
the radio/profile route. TX state in `next_state` is a **proposal**:
higher layers must atomically commit monotonic counters *before*
authorizing RF TX. This stage never sends or persists it.

## Next functional migration layers (separate review gates)

1. **Manifest/catalog:** migrate and verify each v0.6.6 decoder pack
   against native `RxFrameSink`, keeping radio profiles and protocol
   decoding separate. Do not claim that current raw 433 pulse counters
   are already decoded protocols.
2. **User library:** port the 0.6.6 codebook/sample replay and conflict
   validation, with versioned NVS A/B snapshots and checksum/generation.
   A failed save must preserve the last valid generation. Budget the
   library separately from the six PSRAM log buffers.
3. **Device store and profiles:** port stable RF identities, candidate
   threshold, user-approved actions, device records, and optional
   product profiles, including Orcon-specific profiles. An official
   profile must never silently replace a local edit.
4. **Web UX:** restore `Add device` and `Create decoder` in the
   previously approved dark mockup. For simple fixed-code controls:
   choose radio, record each action three times, replay/test, name,
   activate, then optionally associate with an existing device. A
   decoder must work without an optional model profile. Make advanced
   interpreter text optional; only ask for technical rules when the
   automatic codebook cannot validate the examples.
5. **Migration/export:** import explicit user-authorized v0.6.6 JSON
   exports after schema validation; preview counts/conflicts first.
   Never assume old ESPHome preference keys are compatible with
   native NVS or that devices can be reconstructed from recordings.
   Never restore TX authorization automatically, roll counters back,
   infer Orcon/Itho model from RAMSES-II alone or reveal keys in logs.
6. **TX and pairing:** require capability-specific encoders, consent,
   hardware timing tests, driver result/recovery traces and real
   on-air verification. Failed TX must restore the exact RX mode
   safely. Do not expose a generic `send arbitrary RF` web endpoint.

## Verification boundaries

The host regression builds the **actual native .cpp** with GCC and
executes a fixed-code RX example, rejected variable pulses, packet
matching, instruction/loop limits and TX preview including a monotonic
counter. The native ESP-IDF compile is a separate CI gate.

A green host/IDF build does not establish RF capture parity or device
control. Preserve v0.6.6 as the working hardware fallback while native
radio and decoder tests continue.
