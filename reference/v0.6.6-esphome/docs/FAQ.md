# Frequently asked questions

## What is RFLink Universal?
An experimental ESP32-S3 project for using several CC1101 RF channels with modular protocol handling, a web UI and optional Home Assistant device discovery.

## Is it an RFID or NFC reader?
No. The project is about radio-frequency links around the frequencies configured in the firmware, not general-purpose RFID tags or NFC cards.

## Does everything work immediately?
**No.** This is a hobby project. Expect bugs, unsupported devices, firmware regressions and possibly hardware faults. A successful compile or host test does not guarantee an RF function works in the real world.

## Can I build one myself?
The project is intended for self-building after its relevant source, design files and licenses are published. This draft ZIP contains firmware and documentation; it does **not** contain finalized PCB manufacturing files or enclosure files.

## Must I be a programmer?
Not necessarily for a standard documented installation, but you need to understand your wiring, firmware configuration, ESPHome compile process and the risks of RF transmission. Adding decoders takes additional knowledge.

## Will it boot without all three CC1101 modules?
The firmware is designed to keep network/web services accessible when a radio is absent. That behavior has code checks but still needs to be confirmed on the actual ESP32-S3 hardware. A missing radio cannot handle RF traffic.

## Does it recognize every 433/868 MHz device?
No. Matching frequency is not enough; modulation, framing, encoding, decoder compatibility and legal radio limits all matter. Encrypted devices and rolling codes impose additional constraints.

## Will it add every received device automatically to Home Assistant?
No. A candidate must be recognized, reviewed and explicitly approved. MQTT Discovery is optional and off by default; the ESP32 itself continues to use ESPHome's API.

## Can it transmit?
Yes, built-in encoders and user TX recipes can send supported actions on a matching radio after explicit per-device approval. Stable short codes can receive an automatic previewable TX recipe; advanced programs can calculate frames and persist counters. Encryption, rolling-code algorithms and real target response are not inferred from recordings. A submitted TX command is not proof that a target responded.

## Can I make a new protocol directly on the device?
Yes. Record each action at least three times in the web interface; a new decoder can start receiving immediately after replay validation. For protocols with changing data, edit the bounded RX/TX rules under Advanced. Saved device profiles and JSON export are available. Read [USER_DECODER_GUIDE.md](../USER_DECODER_GUIDE.md). This is not arbitrary C++ installation through the web UI.

## How do I change the language?
Open Settings → Language & region. Choose English (default), Nederlands, Français or Deutsch. The preference uses its own ESP32 NVS key and is intended to survive reboot and firmware updates that preserve NVS. Static interface copy, generated status text, dialogs and decoder onboarding text follow the selected language. Native ESPHome pages and low-level logs are outside this RFLink web-locale layer. See [LOCALIZATION.md](LOCALIZATION.md).

## Is the firmware completely written by AI?
No. Juulius initiated and directed the project, designed the board, chose requirements and reviewed the work. ChatGPT produced a large share of software implementation and assisted with debugging, documentation and design review. The enclosure is being designed independently by Juulius.

## Did AI certify or fully test the PCB?
No. AI-assisted reviews are not independent physical measurement, formal sign-off or certification.

## Can I make a new decoder or contribute an existing one?
Contributions are welcome after publishing. Use the decoder template and manifest, explain the signal format and supply reproducible host tests and physical validation details. Do not submit copied code unless its license is compatible and attribution is retained.

## Can I sell boards or devices made from this project?
**Commercial use is intended, but this draft is NOT YET licensed for distribution.** Wait until the relevant final licenses and hardware design files are published. After that, comply with each applicable license, attribution and source-code obligations. Selling physical radio equipment also brings separate safety, radio, CE/RED, documentation and other applicable manufacturer obligations. Attribution alone is not enough. See [LICENSING.md](LICENSING.md).

## Can I modify it and sell it under my own brand?
That is an intended use once licensed, subject to the exact applicable licenses, original notices and third-party rights. Do not imply your edition is official or endorsed. A seller is responsible for their product's compliance and consumer obligations.

## Can I keep my software modifications private?
The answer depends on which code you use and how you distribute it. ESPHome C++/runtime code is GPLv3; an integrated distributed firmware requires its own licensing assessment. Private experiments and conveying firmware/products are not the same legal situation. Do not assume an attribution-only condition is sufficient.

## Is there a warranty, support guarantee or liability coverage?
There is no promised open-source hobby support or guaranteed response time. A software disclaimer is not a blanket waiver of mandatory rights if someone sells a physical device or provides a paid product.

## Can I use this as the only controller for an alarm or other safety-critical system?
No. It has not been qualified for safety-critical or high-availability applications.

## Can I expose the web interface to the public internet?
No. Authentication has not been implemented in this source snapshot. Restrict access to a trusted network and apply appropriate network segmentation/firewall rules.

## Where should I report a bug?
Once a repository is published, open a GitHub Issue using the bug template. Include the source version, board revision, relevant logs with secrets removed, reproduction steps and expected/actual results. Do not post full raw identifiers that compromise privacy.

## Why does the updater sometimes refuse a release?
It should fail closed if a base YAML is unrecognized or a merge could overwrite personal GPIOs/settings. Use the precise unmodified YAML of your installed baseline, review the change report and only then validate/compile.
