# RFLink Universal — ESPHome + native decoder feasibility test

**Status: isolated adapter implemented; four new C++/ZIP host regression checks
passed in GitHub Actions run #42617448547. No ESPHome firmware compile
or hardware measurement yet. Not a production firmware release.**

## What this test does

This experiment preserves the complete *working* Radio Gaga v0.6.6 ESPHome
radio system. A tiny diagnostic observer forwards **copies of the same 868 MHz
packet bytes** from v0.6.6's existing `cc1101.on_packet` callback to the
native Radioactive v0.7 `NativeDecoder` (Itho/RAMSES family recognition).

The original `id(rflink_core).receive_packet(...)` remains the **first**
receiver of each packet. The native RAMSES-II copy is **namespace-isolated**
under `rflink_hybrid_ramses_ii`, because working v0.6.6 already links a
separate decoder that exports the same global `rflink_ramses_ii` symbols.
This avoids duplicate symbols without replacing the working decoder or
changing the native v0.7 library. The shadow decoder cannot transmit, reconfigure SPI,
switch profiles, modify devices, pair, set registers, control RMT, or change
Wi-Fi/OTA. Neither the native CC1101 HAL nor native radio manager is built.

The two 433 MHz receivers remain on their unchanged ESPHome
`remote_receiver` configurations (GDO2 GPIO6/14, 96 RMT symbols per
non-DMA channel). Native v0.7 has **not yet** gained a 433 pulse callback
in this test. That is a separate second-stage feasibility check.

This is deliberately separate from the existing v0.7 native-first
requirements in `AGENTS.md`. The requirements should change **only after**
a working hybrid test and an explicit architecture decision. No production
firmware, Builder, GitHub release configuration or saved data is changed.

## Generate an isolated test source tree

Prerequisites: the **actual v0.6.6 Radio Gaga RELEASE_HASH_FIX_SOURCE ZIP**
and this repository. Do not upload a ZIP containing real Wi-Fi/OTA credentials.

```bash
python3 experiments/esphome_hybrid/prepare_hybrid.py \
  --source /private/path/RFLink_Universal_v0.6.6_RadioGaga_RELEASE_HASH_FIX_SOURCE.zip \
  --output /tmp/RFLink_Hybrid_ESPHome_Trial
```

This creates a **new** local tree and refuses to overwrite an existing
directory. No secrets are copied. Keep your working v0.6.6 source and
ESP32 installation intact.

The generated `rflink-universal.yaml` adds only an ESPHome `includes`
entry and a read-only shadow call after the original 868 `on_packet` callback.
Pure decoder C++ sources are copied from the current repository, plus a
`HYBRID_PROBE_MANIFEST.json` recording exact file hashes and source ZIP hash.
The generated tree is **not** a firmware binary.

## Validation sequence

1. **Host gate:** run `python3 -m unittest discover -s tests_repo -p test_esphome_hybrid_spike.py -v`. This compiles and exercises the actual native Itho/RAMSES decoder behind the adapter on a PC. It does not compile ESPHome.
2. **ESPHome configuration/compile gate:** on a separate local workspace with the compatible ESPHome toolchain and private `secrets.yaml` already configured, run `esphome config /tmp/RFLink_Hybrid_ESPHome_Trial/rflink-universal.yaml`, then `esphome compile /tmp/RFLink_Hybrid_ESPHome_Trial/rflink-universal.yaml`. ESPHome's documented `includes` mechanism copies the adapter header and compiles the whole adapter source folder. **Do not flash yet.** A compile has not been executed for this experiment.
3. **Dedicated hardware gate:** only after a successful compile, use a **separate test ESP32-S3** with three connected CC1101s. Do not overwrite the confirmed-working Radio Gaga installation.
4. Compare the legacy v0.6.6 baseline and hybrid on the same pinout: all three radios online after restart, actual 433 RX on both bands, actual 868 packet reception, retained legacy device controls, Wi-Fi and ESPHome OTA, no unexpected resets, and heap/memory use. Leave each running through repeated watchdog intervals.
5. Trigger a valid 868 packet. Look for `rflink.hybrid` messages for the first three observed packets and then every 64 packets, including native Itho/RAMSES/unknown counts. The packet payload and identities are **not** logged. Check counts against the legacy diagnostic output; a classification mismatch is a test finding, not proof the 868 radio is down.

**Stop criteria:** any 433 receiver failing, loss of legacy RX/TX, bootloop, OTA
regression, unsafe additional SPI/GDO ownership, significant memory regression,
or native decoder causing latency in the original 868 callback. Uninstall this
test copy and keep using v0.6.6 if any criterion fails.

## What a successful result would mean

A passed *real ESPHome compile and physical test* would show that
**our decoder architecture can run on ESPHome-managed CC1101 hardware**.
It would not prove that the full native v0.7 web UI, persistent crash history,
OTA identity model, pairing, universal TX, or 433 decoders are already ported.

Only after that evidence should we decide whether to:
- build the next 433 MHz read-only decoder adapter;
- move pure device registry/profile/user-library code incrementally;
- adapt RFLink Builder to compile signed/versioned ESPHome-backed firmware;
- explicitly revise the native-only architecture requirement.

The existing native ESP-IDF branch and experimental releases remain archived
and available for comparison. No automatic firmware release is made by this
experiment.
