# Licensing, attribution and commercial use — publication gate

**No final license is granted by this draft. Do not publish it as a licensed open-source release or offer it for sale until provenance and licensing have been checked and final license files added.** This is a working proposal, not a substitute for legal advice.

## Intended split, subject to clearance

- **Firmware:** GPLv3-compatible licensing for distributed compiled firmware and compatible terms for project-authored source. ESPHome's C++/runtime is GPLv3 and its Python/other code is MIT; separate components and dependencies must retain their notices. Exact project-source license and version remain to be selected after provenance review.
- **Hardware design files (once provided):** proposed **CERN-OHL-P-2.0** permissive hardware license, with required notices and source-file availability terms checked.
- **Original project documentation:** proposed **CC BY 4.0** with attribution to **Juulius**, a license link and an indication of changes.
- **Enclosure:** not included or licensed at present; independently created and still under development.
- **Third-party decoders, code, libraries and logos:** remain governed by their respective licenses and trademarks. Protocol-source references in `rflink_universal/PROTOCOL_SOURCES.md` must be individually checked for code copying versus independent implementation.

## Credit example (not a substitute for the actual license notices)

> Based on RFLink Universal by Juulius. Modified by [your name]. Original project: [the repository URL after publication]. See the accompanying license and third-party notices.

Credits must not disclose the creator's legal name or suggest endorsement by Juulius or OpenAI.

## Selling physical devices

A software/hardware license does **not** grant product regulatory approval. A manufacturer or seller must determine all applicable obligations, including relevant EU Radio Equipment Directive (RED), EMC, safety, CE marking, technical documentation, EU Declaration of Conformity, cybersecurity obligations where applicable, consumer rights and local radio/frequency restrictions. No declaration of conformity or certification is included in this project.

Official references:
- ESPHome licensing: https://github.com/esphome/esphome/blob/dev/LICENSE
- CERN-OHL: https://cern-ohl.web.cern.ch/home
- CC BY 4.0: https://creativecommons.org/licenses/by/4.0/
- Netherlands RDI manufacturer/importer duties: https://www.rdi.nl/onderwerpen/draadloze-apparatuur/handel-en-apparatuur/producenten-en-importeurs
- European Commission CE manufacturer obligations: https://single-market-economy.ec.europa.eu/single-market/goods/ce-marking/manufacturers_en

## Release checklist

- [ ] Audit the provenance and licenses of every source file and dependency.
- [ ] Confirm rights to license the independently created PCB, firmware changes and text.
- [ ] Choose and include the **full verbatim license texts**, per-directory license declarations and third-party notices.
- [ ] Provide fabrication files only after verifying they are the intended and safe board revision.
- [ ] Scrub original file metadata, PCB properties, screenshots, Git author/email and archives for personal data and credentials.
- [ ] Clearly separate hardware/software verification from regulatory assessment before any commercial offer.
