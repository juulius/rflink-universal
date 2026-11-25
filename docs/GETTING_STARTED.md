# Getting Started with RFLink Universal v0.7

Welcome! **RFLink Universal** is a do-it-yourself gateway that listens to and controls compatible wireless devices. It uses an ESP32-S3 board and three small CC1101 radio modules.

You do **not** need to understand electronics to use the finished RFLink web interface. This page explains what to expect without assuming technical experience.

> **Not ready for normal installation:** v0.7.0 *Radioactive* is still an experimental native firmware. Official ESP-IDF compilation has passed, but radio, OTA and rollback hardware tests are pending. Only use a separate test ESP32-S3, never the working v0.6.6 gateway. Read the [Experimental bench test guide](EXPERIMENTAL_BENCH_V07.md) before flashing.

## What should using RFLink look like?

**This is our intended workflow, not a tested installation guide.**

1. **Install RFLink Builder** in Home Assistant once, from the project's GitHub app repository.
2. **Select your board profile.** The default pinout and SPI speed are supplied; your personal changes stay local.
3. **Choose a firmware version** from GitHub Releases. Five previous stable releases are shown by default; choose **All releases** to see the full history.
4. Select **Build firmware**. The Builder will compile the selected native ESP-IDF source.
5. Choose **USB · This PC** to flash from the browser on your computer, **USB · HA host** for a board connected to the server, **Update over Wi-Fi (OTA)** or **Download firmware**. Options will only be enabled if the firmware and board pass their safety checks.
6. Open the RFLink web interface to learn and control supported wireless devices.

**Want to go back?** Select an older compatible release and rebuild or reuse a verified previous build. Downgrade safety and settings compatibility still require testing.

**Where are settings?** Use **Devices → your RFLink → Firmware** for everyday builds and installation. **Advanced → Hardware & Wi-Fi** allows editing the saved profile *before* installation only. After flashing or linking the physical board, hardware and embedded Wi-Fi settings are locked to prevent an unsafe mismatch. A different configuration currently requires a separate setup and deliberate USB installation; changing Home Assistant's **Defaults for new RFLinks** never changes installed firmware. **Source & release history** is for development and recovery, not ordinary firmware updates.

## Test the new 868 MHz receiver (Experimental firmware)

**New builds from main and new Experimental firmware releases** use the
fixed 868.30 MHz 2-FSK receiver with CC1101 **GDO0 DATA and GDO2 CLOCK**.
The Home Assistant Builder selects this automatically when building a newly
published Experimental tag. There is no UART/FIFO hopping and no need to
manually set a CMake option. A previously published Experimental tag is
immutable, so choose the new build rather than rebuilding an old tag.

1. Open **Devices → your RFLink → Firmware** in the Builder, select
   **Experimental**, press **Refresh**, and choose the newly published
   version. Ensure that its release/source date follows the enabling
   change on main. Select **Build firmware** for your saved board profile.
2. Keep a verified rollback firmware image available, verify the compiled
   GPIO pinout (reference: 868 MHz GDO0 GPIO16 / GDO2 GPIO17) and use USB
   recovery if Wi-Fi/OTA is unavailable. This firmware is intentionally
   **not yet hardware-validated**; do not depend on it for essential devices.
3. After installing, open the firmware console/diagnostics and check the
   868 synchronous receiver start and its clock-edge/drop counters. The
   protected 868 recording API can then capture real messages for analysis
   using [the 868 evidence guide](CC1101_868_SYNC_BIT_EVIDENCE.md).
4. On the device's **direct private-LAN web address**, open
   **Logs → Radio → 868.30 MHz → RAW bit capture**. Enter the local
   management key, select **Arm capture**, operate your 868 MHz remote,
   then select **Freeze** and **Analyze captured bits**. The native
   replay runs on the ESP32 in a separate low-priority task and displays
   candidate protocol matches in the log viewer.
5. Optionally **Export & inspect CSV** to keep the private raw sample and
   compare with the original PC-side
   `python firmware/tools/replay_sync_bits.py` offline analyzer.
   Neither path transmits nor changes the radio profile.

**What to expect:** 433.42 and 433.92 MHz keep their existing OOK/RMT
receivers. On 868 MHz the firmware currently **records raw bits**, but
does **not yet decode live Itho/RAMSES messages into devices**. The fixed
radio configuration, correct GDO clock edge and ~38,400 interrupts/second
still require measured hardware verification. A failed 868 initialization
must report a fault rather than silently switching back to FIFO.

Pre-release (RC) and Stable firmware deliberately retain the old 868
63-byte FIFO receiver until the new approach has been validated.

For the private recording steps, keep your management key and raw RF
captured data on a trusted local network only: RFLink's management HTTP
endpoint is not encrypted. Do not export RF captures over remote Ingress.

## Updating source code without replacing files one by one

The planned Builder compares a downloaded or uploaded source package with the existing project and shows what is **new**, **changed** or **unchanged**. It preserves unchanged files, keeps local overrides separate and offers a backup before replacing changed files. It must not silently delete files.

## What is stored where?

| Location | Information |
| --- | --- |
| Public GitHub repository | RFLink source, documentation, example/default pinout and SPI speed, published release history |
| Home Assistant Builder (local only) | Your optional GPIO and SPI speed overrides, Wi-Fi/MQTT/OTA secrets, build preferences and approved firmware builds |
| RFLink device | The configuration the firmware actually stores on your ESP32; backup and restore behavior still needs verification |

Never upload passwords, device-pairing secrets, private keys or personal diagnostic captures to a public repository.

## Diagnose an ESP32 that restarts before Wi-Fi

In Builder v0.6.22, open your device and select **Diagnosis**. While the device is online, **Wi-Fi** shows live in-memory events together with up to 12 previous reset reasons and a saved crash summary from the firmware. For a bootloop, connect the ESP32-S3 to the computer by a USB data cable, choose **USB** and **Connect**, and select its serial port in Chrome/Edge. Press the ESP32 reset button **after** the monitor connects so early startup text can be recorded. The read-only monitor attempts to reconnect if the USB port disappears during a reset.

If the ESP32 never boots, Wi-Fi cannot retrieve a new crash report; the USB console is the immediate diagnostic source. Crash history is intentionally separate from volatile Radio/System/Web events. Always review a copied USB log for secrets before sharing it. Neither a confirmed flash write nor a last-known reset entry guarantees working radio hardware.

## CC1101 radio detection (development; hardware testing required)

The **Diagnostics → CC1101 power & SPI checks** card shows three separate radios. A
working SPI controller is not proof that a CC1101 is physically responding.
At a cold boot, RFLink reads the chip identity and tests a writable channel
register, then restores its original value before starting reception. While
receiving, the firmware only reads identity registers; after three consecutive
failed checks it reports the radio unavailable and retries later. **Readback
does not measure the actual 3.3 V rail**. The result still needs verification
against the physical board.

To compare the powered and unpowered conditions:
1. Switch the **entire board off** before changing wiring or 3.3 V.
2. With the CC1101 3.3 V supply disconnected, power the ESP32 and open
   **Diagnostics** (or capture the USB boot log). All three modules should show
   **Not detected**. Check the recorded PARTNUM/VERSION and probe errors.
3. Switch everything off again. Reconnect 3.3 V and restart. The modules
   should pass the boot readback test and enter receive mode.
4. Check **Logs → System** (or **Logs → Radio** for radio-specific events)
   and expand a CC1101 identity probe row to see the raw sample.

Never hot-plug the radios or remove their supply while the ESP32 is running.
An SPI identity/readback test is not proof of successful over-the-air RX.

## Early diagnostics (Stage 28 development)

The device now attempts to start logging, Wi-Fi and the local HTTP diagnostic service **before** configuring the three CC1101 radios. When you open **Diagnosis** from the Home Assistant Builder, you can see **Radio initialization pending** while the device is still preparing its RF subsystem; the startup checkpoint remains saved in flash for later inspection. The firmware also offers its existing authenticated OTA upload handler earlier, but a very fast reboot loop may end the connection before an upload completes.

The Builder does not confuse **diagnostics reachable** with **fully booted**: it waits for the correct physical MAC, board/profile and firmware, plus the new startup_complete=true signal, before completing automatic post-flash enrollment. For older firmware that does not expose that signal, the original rules still apply.

If early diagnostics are not reachable, connect the USB serial monitor *before pressing reset* on the ESP32-S3. A last reached checkpoint identifies where progress stopped, not the underlying cause. These changes require bench validation and must not replace a known-good RFLink v0.6.6.

## Boot checkpoints (Stage 27 development)

When the ESP32 starts, it stores a few **last reached** startup milestones in flash: NVS ready, CC1101 radio probing, receiver startup, network initialization, optional OTA verification, and application ready. Open **Diagnosis** in the Builder or **Crashes and restarts** on the RFLink's own web page to see the checkpoint next to recent reset reasons. These records survive normal restarts and power loss when NVS writes succeed; the current firmware does not keep full serial output in flash.

A last reached checkpoint is **not proof** that the corresponding component caused the restart. For example, a watchdog during radio initialization can also be caused by earlier setup or an unrelated task. An app_ready milestone means the HTTP application reached the end of startup, **not** that CC1101 RF reception/transmission is functional. If firmware never reaches its own web service, connect the Builder's **USB diagnosis** at 115200 baud and press the physical reset button after connecting. Very early bootloader failures cannot be captured by this in-app journal.

This code remains experimental until deliberately tested through resets, crashes and power loss on the separate ESP32-S3 board. Automatic safe-mode recovery and full decoded backtraces are not yet implemented.

## Crash and firmware identity preview

The development page shows three short cards: the last restart reason,
the status of the saved crash dump, and which firmware's ELF hash the
saved dump identifies. Expand **Boot history and build identification** to
inspect the exact build hashes or save a redacted JSON diagnostic report.
A matching **prefix** is not a verified full SHA-256 match; after an OTA,
the saved crash may be from the previous firmware. No task stacks or
passwords are included in the download. Physical panic and watchdog
tests on the separate board are still pending.

## Full retained logbook (development; hardware testing required)

Open **Logs** on your RFLink web interface. Choose **All, Radio, System or Web**. To view the shared noise ring, enable **Include noise** under All or Radio; there is no separate Noise tab. Use the radio dropdown to select a frequency, then tap any event to expand technical details. Select **Older** repeatedly to inspect older retained events (up to 64 rows per page); use **Newer** or **Latest events** to return. The newest page updates live; older pages maintain their original sequence boundary for consistent browsing. **Export CSV** includes all retained rows matching the selected source, radio and noise filters, not only the visible page.

The separate buffers hold **250 events for each of three radios**, **2,000 shared noise events**, **512 system events** and **512 web events** by default. Under **Buffer usage**, the numbers show how full each buffer is and whether its *own* oldest records have been overwritten. On phones the six-buffer overview is collapsed by default; tap Buffer usage to expand it. **A full noise buffer cannot overwrite any radio or web events.** Logs live in RAM and disappear after a reboot; they are not an unlimited permanent audit history. The three radio and shared-noise limits are configurable under Settings subject to the memory cap.

The **Web** tab records browser navigation and other frontend actions, failed HTTP API requests and successful changes made via POST. If the ESP32 itself is offline, it cannot record an unsuccessful request that never reached its server; inspect the browser or Builder console for those transport failures. This is deliberately more selective for successful read-only API polling to keep the log useful. No passwords, RF keys, request bodies or URL query parameters are retained. The **Radio** tab includes radio-specific system diagnostics (startup, driver errors and recovery). On mobile, the desktop table becomes compact, expandable cards: the event title, precise uptime, stream, radio and a short description appear without sideways scrolling. Tap a card for every available technical field.

## Earlier read-only logbook development preview

On a separate Radioactive test board, the Stage 24 prototype adds a compact
logbook with filters for the three frequencies, system events and web actions.
The shared noise log is hidden by default. The Stage 25
prototype also has a filtered CSV log export, and records each **actual**
transmit attempt when the RadioManager API is invoked: request rejected,
driver action, error stage and whether the previous receiver came back.
This is not a user-facing Send control and driver success is not on-air proof. Log sizes can be changed with the
device management key; the Builder does not store radio event history for you.
Crashes and boot reasons use a separate flash/NVS history. Until hardware
validation, do not replace a working Radio Gaga v0.6.6 board.

## What can I do today?

Read the project status and follow development. The firmware **has compiled with official ESP-IDF** in GitHub Actions, but reliable live radio operation, wireless transmission, pairing and OTA rollback still need dedicated ESP32-S3/CC1101 hardware testing. Do not install the native prototype over a working device.

Stage 22 improves the internal transmission safety checks. It does **not** add a transmit button yet: while a receiver is listening, RFLink can verify whether a matching transmit profile is supported without interrupting reception. A future authorized send will pause and restore that same receiver. See the [TX readiness and RX restoration guide](TX_STAGE22.md).

## Common terms

- **Firmware:** Software that runs on the ESP32.
- **OTA (Over-the-Air):** Updating firmware using the local network instead of a USB cable.
- **USB:** A wired connection that can be used to initially install or recover firmware.
- **BOM (Bill of Materials):** The list of parts needed to build a board.
- **SPI (Serial Peripheral Interface):** The data connection between the ESP32 and the radio modules.
- **GitHub Release:** A saved version with notes and, after validation, downloadable firmware.

## Continue reading

- [Current feature and test status](V0_7_STATUS.md)
- [Documentation overview](README.md)
- [PCB, assembly and BOM status](../hardware/README.md)
- [Logging, crash reports and compact web UI acceptance criteria](LOGGING_AND_UI_V07.md)

## Experimental 868 automatic RAW candidates and RX counters

Status: implemented in source; host regression and official ESP-IDF build gates are required. Physical RF validation remains pending.

The Status dashboard now counts recognized RX message events since boot per radio, not RMT captures, FIFO reads or continuous clock bits. RAW candidate totals are a separate counter; the same transmission may generate both a RAW candidate and a recognized message. Itho structural-only sync matches and nested RAMSES logical messages are not counted as recognized physical messages. On older firmware the new counters are unavailable, not zero.

For 433 MHz the existing RMT capture/filter pipeline retains longer pulse sequences (six or more durations, or saturation). For Experimental 868 MHz a separate low-priority consumer detects 32 alternating bits and retains an exact 504-bit MSB-first evidence window (63 packed bytes). The 8192-bit holdoff bounds capture writes. A partial window is discarded on sequence loss, epoch change, discontinuity or explicit window end; independent windows are never stitched. This is an experimental preamble heuristic, not universal packet framing, carrier/RSSI validation or a guarantee of complete messages. It may miss other preambles, short frames and transmissions during holdoff, and may capture interference. It does not change RF profiles, SPI, ISR, FIFO/Stable, decoders or TX.

The metadata uses the formerly inactive unknown FIFO ring (now 500 entries) in sync mode; payloads use its existing 256 KiB detail arena, separate from recognized 868 messages. The Logs viewer groups both buffers in each radio tile. The RAW record is code 55, not a FIFO packet; the exact retained payload is exported as `clocked_bits_hex` with a bit count. Open Logs → Radio → 868.30 MHz and expand a RAW candidate, export its RF JSON or use Export full CSV. Both the local interface and trusted Home Assistant Ingress use the same read-only logging access as 433 MHz, with no additional management key. The full CSV includes all retained automatic RAW details by default. Manual frozen bench capture remains independent and requires explicit authorization. Keep the RFLink HTTP interface on a trusted network.

Bench validation: record idle baseline, operate a known 868 control ten times with spaced presses, compare candidate and recognized counts, inspect exact windows and compare with a manual frozen capture. Repeat with both 433 receivers and verify Wi-Fi, queue drops, memory and retention. Do not interpret a preamble-window count as an RF message count. A future RF-energy/carrier-aware framing stage needs physical measurements first.

## Unified RX logging and retention (roadmap phases 1–2)

The 433 MHz receiver now finalizes a single ordinary receive log record after decoder dispatch. Recognized messages use Signal; unknown complete captures with at least six original and six decoder-input durations (including the synthetic idle tail), plus ambiguous matches, use RAW; short, filtered, saturated or otherwise incomplete captures use Noise. This conservative retention heuristic does not change the RMT filter, protocol decoders or receiver hardware. It is not yet a universal signal-quality classifier.

Original 433 timings are copied before filtering into a single optional 4 KiB PSRAM staging area owned by the existing RX worker. A finalized record retains the exact original waveform once; the decoder-input count, outcome and capture/decoder IDs are metadata. Noise metadata is bounded and tiny noise/detail sampling is limited per radio to 750 ms. Full CSV includes all still-retained original evidence without a management key. Ordinary forwarded/short/filtered counters are live telemetry; System retains saturation/queue-loss warnings and real driver errors. Decoder-specific diagnostic event counters remain available.

All three RAW metadata rings now retain 500 entries each (including legacy 868 unknown FIFO retention). The two 433 detail indexes each allow 750 entries; recognized 868 has 250, 868 RAW has 500, shared Noise has 2,000 and System/Web have 512 each. Byte arenas stay at 2,368 KiB with the same 5,274 detail-index entries; total including staging remains within the existing 30% PSRAM ceiling. Byte pressure and configured signal limits can still reduce actual detail retention. Known and unknown 433 share a band-specific byte arena; metadata rings remain independent.

Status now shows RX activity (RMT completion totals on 433, unavailable on 868 until RF burst detection), RAW candidates and RX messages. On 433 RAW candidates count unknown/ambiguous signals, not recognized messages; on 868 the existing heuristic still counts independent preamble windows. 868 automatic RAW and decoder hits are not yet correlated into one physical frame; this needs phase 3–4. See [the approved RX roadmap](RX_LOGGING_ROADMAP.md).

Validation: source changes and regression tests prepared; official ESP-IDF and physical board validation remain pending. Compare idle and controlled transmissions with exp.65, check exact original timings, counters, retention, queue drops, heap and CSV. No modem, SPI, TX, OTA or authentication changes.

## Phase 3: passive 868 RF activity telemetry (Experimental)

Status: source implementation; host, official ESP-IDF and physical validation required.

The sync RX worker reads CC1101 RSSI (0x34) and PKTSTATUS.CS (0x38 bit 6) after draining the existing DATA+CLOCK sampler, at most once per 10 ms. Existing modem registers are unchanged (MDMCFG2=0x00, no hardware sync-word detection). The shared SPI bus uses an immediate lock attempt and a 50 µs CHIP_RDYn budget per optional read; missed pairs are counted. No FIFO access, strobe, ISR change, decoder gating or new sampling task. The two register reads are not an atomic RF measurement. See TI CC1101 data sheet, sections 17.3 and 17.4 and status register descriptions: https://www.ti.com/lit/ds/symlink/cc1101.pdf.

Dashboard RX activity on 868 counts sampled carrier-high periods, not clock blocks or validated frames. One high sample starts a period; two low samples end it. A gap greater than 30 ms or RX restart breaks continuity; a first high after such a boundary is counted as a partial start, not a proven onset. Counts are boot-total. Actual sampling cadence depends on RX worker scheduling; short bursts may be missed or nearby bursts merged. Carrier sense uses the existing modem's AGC threshold; RSSI is an approximate dBm reading (signed register/2 minus nominal 74 dB offset), not calibrated signal power. RSSI is observational, not a new acceptance threshold.

Advanced RX diagnostics and the full CSV status context report sample count, carrier-high samples, missed pairs, partial starts, gap resets, maximum valid-sample gap, maximum read time, last sample uptime in microseconds, last/min/max RSSI (half-dBm units in the API), and carrier state. Missing current readings are null, and the dashboard shows unavailable until the first sample. No routine activity events are written to System, Noise or RAW. Existing RAW preamble capture and live decoders remain independent; correlation with real burst boundaries is phase 4. FIFO/RC/Stable RX is unchanged.

Board acceptance: compare exp.65/phase 1–2 baseline with passive listening and spaced known 868 transmissions. Check actual max gap, missed pairs, carrier-high proportion, RSSI range, 433 captures, sync queue losses and Wi-Fi. If carrier sense stays high at idle or misses known signals, diagnose before changing any thresholds or gating RAW. Do not interpret the new counter as a packet count or proof of complete reception.

## Phase 4a: descriptive RF evidence observations

Status: source implementation; host/ESP-IDF CI and hardware validation required. Phases 1–3 are merged; the phase 3 CI gates passed, but RF activity sampling still needs measured board evidence.

Open an existing retained RF row in Logs. The expanded RAW details display a short descriptive observation above the exact waveform; Export RF capture JSON and Export full CSV include the same versioned observation object (`rflink-signal-observation-v1`) when the original detail is available. No new log events, receiver task, retention change, hardware reads, decoder input change or management-key requirement. The firmware calculates features only on a copied detail snapshot in the existing HTTP/CSV read paths. Old firmware and the legacy small capture fallback may omit observations.

- **433 MHz pulse timing:** measured sum/min/max of absolute signed durations, high/low/zero counts, level changes and sub-30-us durations. Nonzero widths of at least 30 us are the timing denominator. Rounded 50-us width bins are bounded to 0–63 per polarity; widths outside those bins remain in the denominator. The two most frequent bins per polarity provide a descriptive support count. Fewer than eight timing samples or fewer than two high/low durations are insufficient; otherwise at least 75% support is labeled concentrated timing, less is varied timing. This describes timing concentration, not a calibrated signal-quality estimate, protocol recognition, repetition across frames, or a reason to discard a waveform. Decoder-input evidence can include a synthetic idle tail; original pre-filter captures remain unchanged.
- **868 MHz automatic 504-bit synchronous windows:** number of ones, transitions, alternating prefix length and longest constant run, plus independent statistics for the 472 bits following the first 32 trigger bits. The latter are labeled long constant run if a run covers at least half the tail, predominantly alternating if at least 90% of adjacent tail pairs differ, or mixed bits otherwise. These are descriptive patterns, not validation or complete frame boundaries. Legacy FIFO packets are not treated as clocked windows. Capture-specific RSSI/carrier correlation is not fabricated from a live or later status reading.

Use these observations to compare known transmissions with the passive baseline; assess actual carrier sampling gaps, RSSI range, candidate count, and queue losses from phase 3 before choosing any filtering threshold. Phase 4b will need bounded timestamp/epoch-aware carrier correlation and measured classification evidence; cross-capture repetition analysis and offline replay belong to phase 5. Keep RAW/Noise/Signal routing and all decoders unchanged until validated.

## Phase 4b: correlate automatic 868 RAW evidence with sampled RF activity

Status: source implementation, CI and physical validation pending. Phase 4a is merged with green CI.

Expand an automatic 868 RAW candidate in Logs to inspect the original bits, descriptive bit pattern and carrier context. The same optional `carrier_context` object is included in the per-capture API, RF JSON export and full diagnostics CSV. It records the RX epoch, estimated start/end in boot microseconds, valid sample count, carrier-high sample count, missed read pairs, first/last included read interval boundaries, approximate min/max RSSI in half-dBm units, first/last observed carrier activity IDs and whether the retained history starts late.

The clocked block records only its first and last measured edge times. Candidate boundaries are estimated by interpolation of bit positions between those endpoints, including a preamble split across adjacent blocks. Invalid endpoint timing is explicitly unavailable. The existing decoder task queries at most 64 already recorded RSSI/CS read pairs using an immediate attempt at the receiver mutex. It includes only pairs whose entire read interval falls within the estimated window and whose epoch matches. No extra SPI read, new task, ISR change or blocking history query. Delayed processing, lock contention or sparse sampling can leave no matching samples; this is not evidence of no RF. Missing RSSI is null, never zero or a current status reading. The two register reads are not atomic, and activity IDs describe sampled carrier periods, not verified RF frames.

Store a bounded trailer of at most 64 bytes with the existing 63-byte synchronous RAW payload in its current detail arena. Original bytes and hash remain unchanged. No extra retention ring, log event, buffer capacity change or authorization. The receiver retains a small fixed history (at most 2,304 bytes); no heap allocation in the query. Legacy FIFO/pulse storage is unchanged. Older payload-only sync evidence reports not recorded. The legacy `quality_state` still refers to packet quality metadata, not this separate approximate carrier context.

The labels describe observations only. No RAW-to-Noise routing, RF threshold, decoder gate or recognition change has been made. Correlation of individual protocol hits with captures and tuning classification still require measured board data. Validate with a passive baseline and known spaced 868 transmissions; compare valid/missed samples, timing estimates, activity IDs, RSSI range, queue losses and heap. A low or absent carrier sample never justifies rejecting a candidate by itself.

## Phase 5: offline automatic 868 RAW replay (Builder 0.6.58)

Open the existing RF Replay page in Builder and analyze the device snapshot or upload a full diagnostics CSV or RF capture JSON. Automatic 868 `clocked_bits_hex` records (exactly 504 bits / 63 packed bytes) now enter the same replay engine as legacy FIFO bytes and 433 signed durations. The CLI and Builder use identical Python and C++ sources. The image builds the existing firmware synchronous scanner together with the legacy codecs. Each automatic window is scanned independently in both polarities; no concatenation, invented edges, carrier-based gating or hardware access. Manual frozen recording block rows are not stitched by this engine.

Itho structural matches, RAMSES checksum matches and embedded RAMSES matches remain separate. Only a non-embedded RAMSES checksum match in a non-truncated scan is reported as a protocol verdict for a clocked window. A missing match does not rule out a protocol because a fixed window can end before a complete message. `--structural-only` skips native decoding.

Reports include descriptive bit statistics, whitelisted existing signal observations and carrier context where present. Exact bit-window groups require equal full payload and frequency and at least two distinct capture IDs. All result rows remain; groups do not prove repeat transmissions or identify devices. No approximate merge of differing payloads. Consolidated 433 metadata exposes original/decoder-input duration counts without inventing a second waveform; legacy paired records remain supported. Existing 433 timing hypotheses and clustering thresholds are unchanged.

The existing compact diagnosis adds only aggregate 868 counts, without raw payloads. The private replay bundle retains exact samples for later decoder comparisons. Builder processes uploads locally and temporarily; no upload to external services. Existing size and capture limits remain. This is a Builder/tools change, not an ESP32 receiver change. CI and actual hardware data validation are required before stronger classification or decoder migration.
