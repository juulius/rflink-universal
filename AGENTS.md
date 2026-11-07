# RFLink Universal — Repository Instructions for AI Agents

Read this file and [docs/V0_7_STATUS.md](docs/V0_7_STATUS.md) **before proposing or changing code**. These instructions apply to the entire repository. When a claim conflicts with the source code or measured results, investigate and describe the discrepancy; do not invent success.

## Mission and audience

Build a reliable, self-contained RF gateway based on ESP32-S3 N16R8 and **three CC1101 radios** at 433.42, 433.92 and 868.30 MHz. Both the Home Assistant Builder and the RFLink web interface must be understandable to a nontechnical user.

- Write **all repository content in English**: UI copy, documentation, README, comments, logs, release notes and agent instructions.
- Write plain-language guidance first: what the feature does, how to use it, what the user will see. Put registers, timing and raw diagnostics in clearly labeled Advanced sections.
- Everyday actions should generally require **at most three clicks**; a guided setup wizard may take more.
- Minimize scrolling and redundant controls. Use a clean, consistent, compact dark UI. Group related settings.
- Never describe a planned feature as available. Label **implemented**, **host-tested**, **ESP-IDF-built** and **hardware-validated** separately.

## Firmware architecture (non-negotiable)

1. **Native ESP-IDF**, not ESPHome. ESPHome must not own RFLink SPI, GPIO, UART, RMT, radio scheduling, web server or firmware lifecycle. No ESPHome runtime or YAML configuration in v0.7 firmware.
2. One **protocol-agnostic RX/TX radio engine**, shared by all three CC1101 modules and all built-in/user decoders. No decoder-specific hardware driver.
3. RF decoder/profile manifests specify frequency, modulation, registers, receive and transmit encoding, timing and transport (packet/FIFO, GDO/RMT or GDO/UART). The radio manager owns hardware operations.
4. Exactly one component owns shared SPI and hardware mode changes. Serialize access and guarantee deliberate RX-to-TX-to-RX transitions, recovery and observable error reasons.
5. Track **hardware presence** separately from **busy/mode/health**. Normal protocol switching is not "offline".
6. Both received and transmitted actions require traceable diagnostic events; a successful write to SPI/FIFO alone is not proof of an on-air transmission. Never claim on-air success without measurement.
7. Preserve compatibility with future first-party and user-created decoders and device profiles; maintain a path for promoting user decoders into tested firmware decoders.
8. Avoid regressions in the working v0.6.6 reference; it is a comparison source, **not the current firmware or documentation specification**.

## Hardware and configuration

- Default board profile and default SPI speed belong in version control. Board-specific personal GPIO/SPI overrides belong to the Builder's persistent **local** storage, never in GitHub.
- Current documented reference GPIOs: shared SCLK 12, MOSI 11, MISO 13; 433.42 CS/GDO0/GDO2 4/5/6; 433.92 9/8/14; 868.30 15/16/17. **200 kHz is the current conservative default SPI clock, not a hardware-validated maximum.** Confirm any change against the actual board and tests.
- Pin changes are build-time/profile operations, not ordinary runtime web toggles. SPI speed changes must never interrupt active radio work without a controlled stop and recovery path.
- Keep Wi-Fi credentials, MQTT users/passwords, OTA credentials, keys, paired identifiers and captures with private data out of Git. Never log secrets.
- Firmware OTA updates must check board/profile compatibility and support the ESP-IDF A/B rollback path; a downgrade may also require storage-migration safeguards.
- PCB revisions, schematic, PCB layout, verified BOM and manufacturing artifacts must be mutually consistent. Do not invent manufacturer part numbers, pricing, validation or assembly status.

## Native logging and visual density — mandatory migration requirements

- v0.7 must inherit the v0.6 logging capabilities as **standard built-in functionality**: three independent signal logs, one shared noise log filterable by radio without duplication, bounded memory, persistent runtime limits, and System/Web diagnostics. See [v0.7 logging acceptance contract](docs/LOGGING_AND_UI_V07.md).
- Persistent crash/reset reporting is a release blocker, not an optional experimental debug feature. Keep a flash core dump, last failure summary, and historical reset reasons, including after power cycles. Do not disclose raw stack memory or credentials via an unauthenticated web endpoint.
- Design web layouts for readability **and** proportion. An oversized Logging settings card must not stretch neighbouring Language, Status LED, or Build cards. Keep independent-height cards, concise labels, visible save feedback and an immediately discoverable crash report.
- Never claim a log exporter, an on-air TX log, or persisted crash history is hardware-validated without tests on the actual board.

## Home Assistant Builder and releases

- Home Assistant is a **build/management integration**; RFLink must keep operating independently of Home Assistant.
- The Builder must support source ZIP import with a **diff preview**, keep unchanged files intact, back up replacements, protect local edits/secrets and never silently delete files.
- Make firmware locally/build-host compilable using official ESP-IDF; do not require Docker or a Windows compiler on the user's PC for the Home Assistant workflow.
- Support both USB flashing and Wi-Fi OTA, plus downloading exact build artifacts, after appropriate compatibility/identity checks.
- GitHub Releases are the source of version history; show the previous **five** stable firmware releases by default, with an **All releases** view for older/newer releases and prereleases.
- Allow rebuilding a selected tagged release with local board settings in an isolated workspace; keep its original commit identity and a hash-verified build history.
- Keep **Builder version**, **firmware source version** and **published firmware version** distinct. Tags and immutable commit IDs matter.
- Use short semantic firmware versions and radio-themed release names (v0.6 Radio Gaga, v0.7 Radioactive); patch updates keep the release family name. Keep human-readable notes and a short build/source hash.

## Test, release and safety gates

- Before declaring work complete, run all available tests, compile with the **real** ESP-IDF toolchain when available and report the exact results. Host syntax checks do **not** prove a full firmware build.
- Test RF hardware, TX/RX restoration, reboot, Wi-Fi, web OTA and rollback **on a board** before recommending a general release.
- Do not publish a flash-ready or stable binary based only on mock tests or stub headers. Do not overwrite a known-working v0.6.6 device during early v0.7 tests.
- Show actionable errors and avoid silently accepting unsupported modem registers, missing profiles or TX transports.
- Keep source/license attribution and third-party decoder provenance under review before publicly publishing imported code. Do not assume prior-source permission merely because the repository is public.

## Documentation rules

- Documentation describes **v0.7 only** unless a previous version is explicitly being compared.
- Do **not** bulk-copy the v0.6 manuals. Migrate *concepts* only after verifying the v0.7 implementation; omit obsolete features, screenshots and instructions.
- When changing behavior, update the beginner guide, relevant technical guide and [implementation status](docs/V0_7_STATUS.md).
- Write numbered, actionable setup steps; expand acronyms at first use; label examples as examples; include troubleshooting and expected outcomes.
- Keep board design files and the BOM under [hardware/](hardware/README.md) when verified. No fabricated quantities, pinouts or build instructions.

## Starting a new task

1. Read this file, [README.md](README.md), [docs/V0_7_STATUS.md](docs/V0_7_STATUS.md) and the relevant source.
2. Identify whether the request concerns **firmware**, **Builder**, **hardware/BOM** or **documentation**, and check the current implementation.
3. Make focused changes and tests; keep unrelated or working features intact.
4. Report changed files, executed tests, limitations and whether the result is safe to build/flash.
5. Do not commit personal data. Do not automatically publish stable releases or flash hardware without explicit user approval.
