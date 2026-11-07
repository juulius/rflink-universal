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

## Manual publication and promotion

GitHub Actions workflow `.github/workflows/publish_firmware_release.yml` runs **only** from a deliberate manual action; pushing to main or merging a PR never publishes firmware. It performs source validation, host tests and an official ESP-IDF compilation before publishing a **source-only** GitHub Release. It does not publish flash-ready binaries, and it cannot validate RF reception, on-air transmission, Home Assistant OS installation or OTA without actual boards.

1. **Experimental:** select `experimental`, version `0.7.0`, codename `Radioactive`, and a new sequence number, such as `1`. It publishes the current main source as `firmware/v0.7.0-exp.1` if all checks pass. Multiple reviewed PRs may be merged before this action.
2. **Pre-release:** select `pre-release`, use a new RC sequence and set **promote_from** to the exact existing Experimental release tag, e.g. `firmware/v0.7.0-exp.3`. It copies that source commit into an RC tag. Later RCs may use a different reviewed Experimental source tag after fixes, but no existing tags move.
3. **Stable:** first document successful real-board RX/TX, reset/crash, Wi-Fi, USB, OTA A/B rollback, storage migration and recovery tests. Select `stable` and **promote_from** an existing RC release of the same version; type the explicit stable approval phrase in the workflow. This action is **not an automated proof** of hardware tests and must not be performed until those tests really passed.

After publication, reopen **Home Assistant → RFLink Builder → Firmware versions and recovery → Refresh**. Published releases will appear in the appropriate channel. The publication workflow never flashes a board or modifies personal pinout, Wi-Fi, OTA keys or user decoders.

### Version identity and limitations

Three separate identities matter: **Builder app version** (e.g. `0.6.12` in `rflink_builder/config.yaml`), **published firmware version and channel** (e.g. `v0.7.0 Experimental build 14`), and **locally compiled binary** (source SHA, board profile hash and binary SHA-256). The Builder resolves tags to immutable Git commits and recompiles selected snapshots with the local pinout/SPI configuration. A release entry is therefore **not a ready-to-flash binary** and is not an OTA compatibility guarantee.

Builder v0.6.12 passes the immutable release tag as `RFLINK_RELEASE_TAG` into the isolated ESP-IDF build; CMake stamps the channel and build sequence into the ESP-IDF app description while retaining the N16R8 and board-profile hash used by the OTA identity guard. The package verifier rejects a mismatch between chosen tag and actual built version. RFLink `/api/status` exposes both a friendly label and the raw build identity. Local development builds retain a visibly separate Stage 20 technical identity. This software path is **not yet validated on physical hardware**. See [first Experimental bench guide](../docs/EXPERIMENTAL_BENCH_V07.md).

A downgrade is not the same as the bootloader's automatic OTA rollback. Data migrations, partition compatibility and anti-rollback eFuse restrictions can make an older firmware unsafe. Legacy ESPHome v0.6.6 is **not** supported by the native Radioactive OTA update API.

**Release policy:** Never move or reuse tags, replace existing stable binaries, or declare a build hardware-validated based only on CI. Do not overwrite a working v0.6.6 device during prototype testing.
