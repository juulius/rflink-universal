> **Nieuwste ontwikkeling:** RFLink Universal **v0.6.1 Radio Gaga**. Zie [V061_RELEASE_NL.md](releases/V061_RELEASE_NL.md) voor de GDO/UART TX-diagnostiek, veilig radioherstel, de vernieuwde apparaatdetails en het hardwaretestplan.

> **RXLOG1 (2026-10-25):** The Monitor now stores individual signal histories for three radios and one shared noise history, with runtime log limits and an all-radio ZIP export. Read [LOGGING_RUNTIME.md](LOGGING_RUNTIME.md) for setup and memory behaviour. These changes are source-tested; a full ESPHome build and hardware verification are still required.

# RFLink Universal

**One board. Multiple RF bands. A project you can build on.**

> **Experimental hobby project / release candidate — not a finished consumer product.**
> RF compatibility, the PCB, transmit paths and the full firmware have **not** been comprehensively tested on physical hardware. Do not use this as a sole safety or security system. Review your configuration and compile before flashing.

RFLink Universal is an ESP32-S3-based multi-radio project combining three configurable CC1101 radio paths, modular RF protocol decoders, ESPHome, and a dedicated web interface. It is intended to make experimenting with 433 MHz and 868 MHz RF devices more approachable without hiding the underlying limitations.

The name describes radio-frequency links; **this is not a general-purpose RFID or NFC tag reader**.

## What it can do

| Area | Current implementation / qualification |
| --- | --- |
| Multi-radio | Three CC1101 paths in the supplied YAML: 433.42, 433.92, and approximately 868.30 MHz. Confirm your own module, region and antenna configuration. |
| Web interface | Status, radio monitor, guided device learning, saved device management, diagnostics and decoder library. Trusted LAN only: web authentication is not implemented yet. |
| Protocols | Modular decoder packs, examples and manifests. Availability and compatibility vary; documented software checks are not over-the-air hardware verification. |
| Known device profiles | 10 source-backed built-in product/family profiles are shown on top of matching decoders, with model aliases, learning guidance, sources and an explicit hardware-validation status. See [library architecture](LIBRARY_AND_PROFILES.md). |
| Your own protocols | Capture at least three examples per action, create a bounded RX rule in the web UI and, for stable short frames, generate or edit a TX rule. Save model profiles and promote reviewed recipes into firmware with the supplied Python tool. See [user decoder guide](USER_DECODER_GUIDE.md). |
| Home Assistant | ESPHome native API for the ESP32; optional MQTT Discovery for individually approved RF devices and capabilities. MQTT is OFF by default. |
| Transmit | Built-in TX is available by default but never sends automatically. Somfy RTS and Itho RFT now use native per-device virtual-remote/pairing paths with persistent state; physical motor/ventilation-unit validation is pending. Home Assistant TX still needs MQTT and explicit per-device approval. User recipes have separate per-device web approval. A TX request does not prove remote execution. |
| Updates | Separate Windows PowerShell/BAT release and update helpers, designed to preserve your original configuration. Do not blindly replace a personalized YAML. |
| Languages | English-first web UI with selectable English, Dutch, French and German. Static copy, dynamic status/actions/dialogs and decoder onboarding text follow the selected language. See [localization status](LOCALIZATION.md). |

**Hardware in this source package:** ESP32-S3 N16R8 profile, two RAW-oriented 433 MHz channels and one packet-oriented 868 MHz channel. The firmware is intended to stay accessible without connected radio modules; actual boot and radio behavior still require physical verification. Your board, antenna and local radio rules determine what is appropriate to use.

## Getting started

1. Read [Installation and updating](GETTING_STARTED.md) and inspect `rflink-universal.yaml` before changing anything.
2. Copy **only the example secret keys** into your existing ESPHome `secrets.yaml`; never commit the real file.
3. Verify the configured GPIOs against **your exact PCB**, power input and module pinouts. Do not infer pins from another board revision.
4. Perform ESPHome **Validate and Compile** first. Upload only after reviewing the release limitations and your wiring.
5. On your trusted local network, open `http://<device-ip>/rflink`. The site redirects from `/`.
6. Optional: configure your existing MQTT broker and explicitly approve devices and functions for Home Assistant.
7. To learn your own protocol and inspect a TX preview, follow [User decoders and device profiles](USER_DECODER_GUIDE.md).

No preinstalled Python is required for the existing Windows release/update BAT workflow or for everyday learning in the web UI. Promoting a user recipe into firmware and developer-only tests require Python 3. For an existing installation, read the updater documentation before attempting a firmware update.

## Guides

- [Getting started](GETTING_STARTED.md) — safe beginner workflow.
- [Features and limitations](FEATURES.md) — what is implemented versus unverified.
- [FAQ](FAQ.md) — self-build, AI, sale, support and compatibility.
- [Language and translation status](LOCALIZATION.md) — four languages and how to contribute.
- [Development story and credits](CREDITS.md) — human-driven and AI-assisted work.
- [Licensing and commercial-use checklist](LICENSING.md) — **publication clearance still required**.
- [Roadmap](ROADMAP.md) — current next steps.
- [User decoders and device profiles](USER_DECODER_GUIDE.md) — learn, receive, preview/send, export and firmware promotion.
- [Library architecture](LIBRARY_AND_PROFILES.md) — current update behavior and future reconciliation design.
- [UX principles](UX_PRINCIPLES.md) — simple-by-default rules, contextual help and Advanced behavior.
- [Contributing](CONTRIBUTING.md) and [Security](SECURITY.md).
- [Publication checklist](PUBLICATION_CHECKLIST.md) — required work before public release.

Historical Dutch release notes, maintenance baselines and updater source are deliberately retained without moving paths: altering their paths can break the existing merge/update workflow. A future layout cleanup requires migration tests first.

## Releases and support

The supplied package is a **v0.3.3 RC4/HF4 USERLIB2-PROFILES1-UI4-SOMFY1-ITHO1 source snapshot** with host-tested user RX/TX recipes, profiles, native Somfy RTS TX and native Itho RFT packet TX. It is **not** a hardware-validated firmware release. Read `RELEASE_NOTES.md` and `TEST_RESULTS.txt` for test results and limits. No guarantee is made that every device, decoder, release tool or board works immediately. Use GitHub Issues for reproducible reports once a repository is published.

## Credits

**Juulius** — concept, requirements, board/hardware architecture, PCB layout, design decisions and enclosure development. **ChatGPT by OpenAI** — substantial AI-generated firmware implementation, technical design review, debugging, tests and documentation based on Juulius's guidance and feedback. A design review is not a substitute for electrical measurement, independent verification or product certification. The enclosure is being designed separately by Juulius without AI design assistance. This is an independent project and is **not affiliated with or endorsed by OpenAI, ESPHome or the referenced protocol projects**.

## License / commercial use

**Important: this is a private, pre-publication draft, not yet an open-source release.** The intended licensing split is GPLv3-compatible software, CERN-OHL-P-2.0 hardware designs, and CC BY 4.0 documentation, subject to rights and dependency clearance. No final license or design files are supplied here. See [licensing](LICENSING.md) before publishing or selling anything. Once the final licenses have been applied, commercial use is intended **subject to all applicable attribution, source-code and product compliance duties**, not attribution alone.
