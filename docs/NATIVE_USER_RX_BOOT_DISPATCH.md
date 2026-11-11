# Native v0.7: read-only user decoder RX runtime

## What is active in this stage

This stage connects the libraries merged in #138 (recipe interpreter),
#140 (bounded user library) and #142 (two NVS snapshots) to live RX
*without* exposing an edit, transmit or pairing interface.

On ESP32 startup (after NVS initialization, before any RX worker starts):

1. Read the new `rf_user_library` namespace via the A/B NVS backend.
   Missing storage is an empty library, **not a boot failure**.
2. If a snapshot is valid or recoverable, validate and replay each
   enabled decoder's labelled sample captures, then copy the bounded
   **RX recipe programs only** into the immutable runtime dispatcher.
   Disabled recipes are not installed. A failed install leaves no
   partial active set. The full sample archive and optional TX rules
   are *not* retained in the receive dispatcher.
3. Return to normal CC1101 startup and RX recovery even if the
   user-library bank is missing or invalid. No NVS writes are made
   while receiving. A recovered bank remains read-only.

## Runtime classification

- 433.42 and 433.92 MHz: only a complete pulse train with **2–512**
  signed durations at the exact configured frequency is eligible for
  user recipe classification. Hardware capture, 30-us filtering and
  periodic RMT overrun diagnostics are unchanged.
- 868.30 MHz: only 63-byte fixed-length CC1101 packets within
  868.20–868.40 MHz are considered. The existing built-in
  RAMSES-II / Itho decoders **always run first**; user programs see
  the frame only when neither official decoder accepted it.
- At most 8 enabled user recipes (each bounded by 192 instructions,
  2048 execution steps and 32 VM stack values) are tested in the
  normal RX worker, **never in the RMT ISR**. Each candidate has
  already passed replay validation. The dispatcher does no TX, SPI,
  filesystem, NVS or network calls and does not modify rolling state.
- If one user recipe accepts a frame, the event is labelled
  **User decoder RX match**. This is a recipe-level observation, **not**
  proof of physical device identity, action authorization, pairing
  or future transmit compatibility.
- If two or more recipes accept the same frame, the event is
  **Ambiguous user RX match**, severity warning. Do not choose a
  winner or expose a device action. Do not expose RF identities,
  recipe names or rolling-code values in HTTP status or log metadata.

## Diagnostics

Local `/api/status` adds only four numeric fields:

| Field | Meaning |
| --- | --- |
| `user_decoders_enabled` | Number of installed, enabled RX rules |
| `user_rx_matches` | Cumulative unambiguous recipe acceptances |
| `user_rx_ambiguous` | Cumulative collisions between user rules |
| `user_library_storage_state` | NVS read state: 0 empty, 1 valid, 2 recovered read-only, 3 corrupt, 4 I/O or recipe install error; 255 not queried |

The existing per-radio event and six isolated PSRAM log rings
remain intact. Signal log codes `5` and `6` carry only a
frequency, frame length, sequence number and classification.
The web Logs/Diagnostics view explains the difference between a
user decoder matching and an actual saved device.

## Follow-on work

1. Read-only user library catalog API (decoders, action names and
   profiles with no RF secrets), then the small, mobile-friendly
   Library/Devices layout from the approved mockup.
2. Separate user-approved write transactions from RX processing,
   with generation checks, snapshot validation and an explicit
   limited create/edit/delete wizard.
3. Device candidate observation and stable identities from
   multiple captures; no automatic MQTT exposure, device binding
   or generic TX endpoint.
4. Explicit v0.6.6 backup/import review, safe RF profile
   routing and eventual independent TX capability tests.

A green host and ESP-IDF build only confirms firmware compilation
and source behavior. **Hardware 433.92 MHz reception, CPU load
under RF noise, 868 overlaps, power loss and end-to-end web learning
still require bench validation.** Keep v0.6.6 as fallback.
