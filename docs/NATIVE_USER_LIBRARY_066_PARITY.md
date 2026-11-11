# Native user-decoder library (v0.6.6 parity, stage 1)

This change ports the **user library model and codebook** from the archived
working v0.6.6 source into the native ESP-IDF decoder-core. It depends on
the native bounded interpreter introduced in PR #138; it does not import
ESPHome source, preference keys, web endpoints, or runtime dependencies.

## What this stage implements

- Up to **8** user decoder recipes and **16** user profiles, separate from
  official decoder packs, saved physical devices and protocol transport
- Up to **12** labelled capture examples per decoder (minimum three
  examples **per action** before activation), with a 4-action codebook
  auto-generation limit
- Strict 433.42/433.92 MHz RAW versus 868.30 MHz PACKET routing, complete
  bounded capture data, unique identifiers/action names and duplicate checks
- A compact RX recipe generator from stable signal components, plus replay
  of all labelled samples before enablement; ambiguous action examples are
  rejected rather than silently accepted
- Optional *offline* TX preview programs for stable fixed-code patterns;
  changing captured bytes intentionally disable automatic TX preview
- Profile metadata, expected actions and optional controls are distinct
  from the decoder and **are not TX authorization**
- Immutable rules for active decoders until explicitly disabled; profiles
  cannot reference actions absent from their user decoder
- Strictly monotonic proposed per-device TX state, without any RF send
- A bounded v0.6.6-compatible v1/v2 binary snapshot codec, checksum and
  generation number, with transactional decode into a temporary library.
  The codec is an internal data format only; an actual persisted native
  A/B NVS store is **not** wired into a runtime path by this PR.

## What this stage deliberately does not claim

There is no live decoder registry, no web create/learn wizard, no persistent
NVS writes yet, no saved device CRUD, no automatic import from ESPHome
preferences, no MQTT discovery of user devices, and no user-program TX API.
Normal RF captures are not classified differently by this change. These
parts require separate review so that incomplete firmware cannot falsely
claim to have learned a device or to have transmitted a command.

## Review gates for the following stages

1. Persistent native NVS store with **two independent snapshot slots**,
   read both, validate checksum/shape, choose the newest valid generation,
   commit the *other* slot first, verify write and preserve the previous
   generation on power failure. Do not erase/overwrite v0.6.6 preference
   keys, and do not automatically bring back old TX approval/counters.
2. Thread-safe runtime user decoder registration in the RX worker (not
   in the ISR), with bounded copy of the pulse train / packet. Ensure user
   decoders cannot silently trump official protocol decoders or turn
   a raw RMT capture into a claimed authenticated command.
3. Read-only library API and **real** devices/profiles. The user-facing
   UI should show official versus user decoders and support searching,
   conflict review and RX-only default states.
4. Add-device wizard: choose 433.42/433.92/868.30, sample background,
   label actions and capture at least 3 examples each, replay all examples,
   show a clear warning for ambiguous/noisy/rolling-code signals, and
   explicitly save. Device identity and profile are separate decisions.
5. Explicit v0.6.6 export/import workflow, with schema preview, manual
   conflict resolution and no automatic TX permission reinstatement.
6. Protocol-specific TX and pairing only after confirmed counters,
   board-tested timing, explicit approval and RX restore evidence.

Limits are enforced by the code, not only the UI. The existing six
isolated detailed-log PSRAM rings and their 30% quota are untouched.
All tests in this stage run on the real native core via C++ host builds;
a successful host/ESP-IDF compile is **not** a physical RF test.
