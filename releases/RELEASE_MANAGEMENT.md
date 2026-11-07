# RFLink Universal — firmware release channels and safe recovery

## What users see in the Home Assistant Builder

GitHub commits and merged pull requests are **source-development events**, not firmware releases. The Builder displays only deliberately published GitHub **firmware** Releases under three channels:

| Channel | Example label in Builder | Immutable GitHub release tag | Intended use |
| --- | --- | --- | --- |
| Experimental | `v0.7.0 · Experimental · build 14` | `firmware/v0.7.0-exp.14` | New features on a separate test board |
| Pre-release | `v0.7.0 · Pre-release · RC 2` | `firmware/v0.7.0-rc.2` | Integration, bug fixes and candidate testing |
| Stable | `v0.7.0 · Stable` | `firmware/v0.7.0` | General use **only after verified real hardware tests** |

Version family v0.7 retains the codename **Radioactive**, including patches. Other version families must have their own radio-themed names. A subsequent Experimental build creates another tag (such as `exp.15`) **without changing, moving or deleting** `exp.14`. The Builder selects the latest build **within** a channel, and pins the exact source commit for reproducible local building. The GitHub hash remains in technical/provenance data, not in the everyday firmware picker.

**Stable is the default** when a published stable firmware release is available. The new-device wizard hides Experimental and Pre-release unless the user checks **Show test firmware**. The firmware history view still exposes the prior five stable versions and an expanded All releases history. If no release is published, the Builder offers only explicitly labelled, locally held development source; it does not invent a Stable version.

## Automated, PR-validated publication

The workflow is implemented in code, but its first end-to-end run and
device testing are pending. A GitHub PR is a **proposal**, not a release.

1. **Normal firmware PR:** edit firmware code, create a PR against main and
   pass the existing source checks, host tests and official ESP-IDF build.
   A new release-plan check reports that the next Experimental version is
   eligible. Documentation-only and Builder-only PRs do **not** create
   duplicate firmware versions.
2. **Merge and publish:** after that PR is merged, source checks run again
   on the exact main commit. Only when those checks **all succeed** does
   the separate auto-publish workflow run. It detects the merged PR,
   calculates the next unused Experimental sequence from immutable remote
   tags, recompiles the exact tagged source with official ESP-IDF, checks
   the test-only package, then publishes a source-only GitHub Release.
   With no previous Experimental release, the first one is exp.1.
3. **Promote to Pre-release:** create a separate PR that adds ONE JSON
   promotion request under [promotions](promotions/README.md), selecting
   a published Experimental tag. Validation checks the request and existing
   source tag. A passing merge builds and publishes the next RC from the
   **same source SHA**, not from the newer main branch.
4. **Promote to Stable:** use another one-shot promotion PR pointing to a
   published RC. Include the explicit stable approval phrase, actual
   recorded board-test evidence links covering RF, Wi-Fi, USB, crash
   persistence, TX/RX restoration, OTA A/B rollback and recovery. A human
   must merge the specific Stable PR to approve the evidence. Automated
   checks **cannot prove** the physical measurements are genuine. Protect
   main with required CI and human reviews before enabling this route.
5. **Recovery and version history:** published tags never move; a failed
   validation/build does not publish a release. Every completed tag is a
   separate history entry; the Builder shows friendly semantic versions,
   not GitHub merge hashes.

Technical implementation: [.github/workflows/auto_firmware_release.yml](../.github/workflows/auto_firmware_release.yml)
waits for a successful **RFLink source checks** workflow on a merged main
commit. The version planner is [release_auto.py](release_auto.py), with the
current firmware family/codename in [RELEASE_AUTOMATION.json](RELEASE_AUTOMATION.json).
A later firmware family needs a reviewed update to native CMake and package
version stamping before changing this configuration. The old
[manual publisher](../.github/workflows/publish_firmware_release.yml) remains
an emergency/operator-controlled fallback and must never reuse existing tags.

**Important:** automatic publication is **source-only**. It does not flash
an ESP32, automatically update Home Assistant, or establish hardware safety.
Temporary Experimental/RC CI artifacts lack per-device Wi-Fi and OTA
provisioning. Always build a matching device package in the Home Assistant
Builder and follow the [first Experimental bench guide](../docs/EXPERIMENTAL_BENCH_V07.md).

### Version identity and limitations

Three separate identities matter: **Builder app version** (e.g. `0.6.12` in `rflink_builder/config.yaml`), **published firmware version and channel** (e.g. `v0.7.0 Experimental build 14`), and **locally compiled binary** (source SHA, board profile hash and binary SHA-256). The Builder resolves tags to immutable Git commits and recompiles selected snapshots with the local pinout/SPI configuration. A release entry is therefore **not a ready-to-flash binary** and is not an OTA compatibility guarantee.

Builder v0.6.12 passes the immutable release tag as `RFLINK_RELEASE_TAG` into the isolated ESP-IDF build; CMake stamps the channel and build sequence into the ESP-IDF app description while retaining the N16R8 and board-profile hash used by the OTA identity guard. The package verifier rejects a mismatch between chosen tag and actual built version. RFLink `/api/status` exposes both a friendly label and the raw build identity. Local development builds retain a visibly separate Stage 20 technical identity. This software path is **not yet validated on physical hardware**. See [first Experimental bench guide](../docs/EXPERIMENTAL_BENCH_V07.md).

A downgrade is not the same as the bootloader's automatic OTA rollback. Data migrations, partition compatibility and anti-rollback eFuse restrictions can make an older firmware unsafe. Legacy ESPHome v0.6.6 is **not** supported by the native Radioactive OTA update API.

**Release policy:** Never move or reuse tags, replace existing stable binaries, or declare a build hardware-validated based only on CI. Do not overwrite a working v0.6.6 device during prototype testing.
