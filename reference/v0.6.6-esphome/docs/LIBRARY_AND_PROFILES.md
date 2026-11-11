# RFLink Universal — Decoder & Device Profile Library

Status: USERLIB2 implementation plus explicitly marked future design goals. For everyday use, start with [USER_DECODER_GUIDE.md](USER_DECODER_GUIDE.md).

## Goal
RFLink must feel simple for a user who does not know RF protocols. Official firmware content and user-created content are separate. A firmware update may recommend an official replacement, but never silently destroys a user modification.

## Four layers
1. Decoder — turns RF frames into normalized actions/fields.
2. Device profile — optional recognition/mapping metadata for a product/family.
3. User device profile — locally created model variant.
4. Device instance — the user's physical device, name, RF identity and approved features.

A decoder MUST work without any device profiles installed.

## Built-in device profiles (PROFILES1)
The firmware now ships a small read-only product/family catalog **on top of** the decoder catalog. It is intentionally conservative: a product profile is added only when public documentation or upstream protocol implementations support the model/protocol association and the current RFLink decoder exposes compatible actions. This is metadata, not a new RF decoder and not a claim that every hardware revision has already been bench-tested on this board.

Current catalog (10 profiles):

- **AcuRite 609TXC** — 11112-609TXC / 00609A1TXR / 00609A2TX.
- **Alecto V1 weather** — WS3500, WS4500, Ventus W155/W044; current RFLink decoder intentionally does not claim wind fields.
- **Fine Offset WH2 / ClimeMET CM9088** — limited to the current WH2 decoder contract; WH2A/WH5 are not silently treated as WH2.
- **Itho Daalderop RFT (1,2,3,T) 536-0124** — low/medium/high and timer actions; newer RFT-N and the Auto variant are not inferred.
- **Byron SX21** — model family association is known, but button/chime command meanings are not guessed in the profile.
- **Nexa / Jula Anslut self-learning ON/OFF** — 408-063, 408-064 and LWST-605 family examples.
- **Dooya AC123-16 / Zemismart AM15** — family association only; button-number semantics remain to be confirmed by capture instead of being guessed.
- **Rubicson / TFA 30.3197** — current 36-bit decoder family only; longer PT-310 frames are excluded.
- **Somfy Telis 1 RTS** — Pure/Silver/Patio/Lounge model references.
- **Somfy Telis 4 RTS** — Pure/Silver/Patio/Lounge model references; multi-channel product profile, while RF protocol handling remains in `SomfyRTS56`.

Every source profile lives in `rflink_universal/profiles/<id>.json` and contains source links, four-language guidance and an explicit `hardware_validation` state. Until a matching physical product has been exercised on RFLink Universal, that state remains `pending_on_rflink_universal` and the UI says so. `tools/generate_profiles.py` validates protocol/action compatibility against the enabled decoder manifests and generates `profile_catalog_generated.h`. Do not edit the generated header by hand.

RFLink may auto-select a built-in profile only when **exactly one** profile matches the confirmed protocol. If multiple product profiles exist for the same protocol, the user chooses. Built-in profiles are read-only; locally created profiles remain removable/editable and can intentionally shadow the same stable ID.

## Sources
- `builtin`: shipped with firmware and canonical/recommended.
- `user`: created on this RFLink device and stored in two NVS snapshots.
- `firmware`: reviewed user recipe compiled into the firmware by `promote_user_library.py`.
- `imported` and `community`: future distribution sources. USERLIB2 does provide local JSON backup/restore, but does not yet install community content as a trusted source class.

User decoders/profiles have stable IDs and item versions; decoder fingerprints describe the RX/TX program. The current user collection and compiled recipes remain separate. At the same ID, the local user decoder takes precedence. A full fork ancestry or automated migration is not stored yet.

## Current update behavior
RFLink reports library conflicts on the dashboard and Settings page. Local recipes remain active at a matching ID, and firmware recipes never overwrite user NVS. There is no one-click official-replacement resolution or archive workflow in USERLIB2; local backup/restore is implemented separately and never grants TX permission.

## Future reconciliation design
These outcomes describe the intended review workflow, not implemented buttons:

- Exact duplicate: safe to resolve automatically to built-in; local duplicate can be archived.
- Newer built-in with unchanged local copy: recommend built-in; existing behavior remains active until user confirms when semantics could change.
- Local modification/fork: never overwrite. Offer official, keep mine, compare, and for profiles keep both under a new custom ID.
- Same version but different fingerprint: conflict; always require a decision.
- User version newer than firmware: keep current local version and flag for review.
- Two decoders with the same stable ID never execute simultaneously.

## Dashboard UX
Normal boot/update does not show a blocking popup. If attention is required, Dashboard shows one card:

`Library update — 2 items need your attention` → `Review changes`

No attention means no dashboard noise.

The review screen explains consequences in everyday language. Technical IDs/hashes live under Advanced.

## Future first-time decision behavior
The current working item remains active until the user confirms a replacement. The recommendation is visually marked, not pre-applied. Choosing the official version archives the old local copy rather than immediately deleting it. Undo remains possible until archive cleanup.

## Beginner rules
- RFLink decides safe/obvious cases itself.
- User input is asked only when behavior may change or information cannot be inferred.
- Required input uses amber outline + `Required`; red is reserved for actual errors.
- Every unfamiliar term can have a `?` help button with a short explanation and optional Learn more.
- Normal screens show outcomes, not implementation details.
- Advanced shows RF IDs, decoder/profile IDs, source, versions, fingerprints, raw fields, matching score and diagnostics.

## Persistent storage
Firmware/built-ins are immutable release content. User decoders, examples, profiles and counters use two preference snapshots in NVS. Saved devices use a compatible v3 snapshot with profile selection and separate user-TX permission. An ordinary OTA is intended to preserve NVS; changing the partition layout or erasing flash can remove this data. Export the JSON file first. USERLIB2 can restore that export explicitly from the web UI. Restore validates recipes again, never restores TX approval, advances counters only monotonically, and only reapplies a saved profile binding to an already known device when UID and protocol still match.

## Current export and restore
The web UI downloads a `rflink-user-library-export.json` containing user decoders, captured samples, profiles, protocol counters and a snapshot of saved devices. Firmware recipes may also appear in the listing; the promotion script only selects user entries. RF identifiers and counters are sensitive; store the file carefully. USERLIB2 restores user decoders, samples, profiles and monotonic protocol counters. It does not recreate unknown device records and does not restore per-device TX approval. A broader portable system backup with settings and conflict-preview remains a future goal.

## Firmware promotion
`promote_user_library.py --check` validates an export, compiles the generated C++ recipe header and replays samples. `--merge` adds reviewed recipes and profiles to firmware source; changed IDs require explicit `--replace` after review. These are compiled-in interpreter recipes. `--candidate-out` can also generate disabled RX-only normal decoder-pack candidates for review; they intentionally keep the bounded VM recipe until a maintainer rewrites/optimizes it. Full physical RF behavior and rolling-code pairing still require hardware testing.
