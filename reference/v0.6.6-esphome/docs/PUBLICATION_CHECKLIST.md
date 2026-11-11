# Publication checklist — RFLink Universal

This source tree is a **review draft**, not a released GitHub repository or a certified hardware product. Creator credit: **Juulius** (GitHub handle only).

- [x] English-first README, features, getting started, roadmap, FAQ, contribution/security templates and transparent human/AI credits.
- [x] English, Dutch, French and German JSON catalogs for the static web UI, with an on-device language setting, English default, NVS preference and a failed-write rollback.
- [x] No real-world name found in the text source at the time this archive was prepared. Review future Git author/committer metadata, PCB file authors, images and EXIF before publishing.
- [x] Existing configuration, updater and source folder layout retained; four locale catalogs and generated bundle included in packaging checks.
- [x] Browser test for all four language choices and persistence API mock; existing selected regression tests pass.
- [ ] Finish translating dynamically generated UI messages, dialogs, decoder descriptions, low-level diagnostic text and any separate ESPHome pages. Do not translate user-defined device names or raw protocol data.
- [ ] Inspect protocol-source provenance, licenses, bundled dependencies and hardware design ownership; add the **actual** finalized software/hardware/documentation license texts and notices before declaring this open-source or granting resale permissions. See `docs/LICENSING.md`.
- [ ] Add finalized KiCad board design, verified schematics, bill of materials, assembly instructions and hardware photos if you want to release hardware source. Enclosure files are not part of this draft.
- [ ] Perform ESPHome Validate/Compile, device boot tests with zero/one/all radios, live radio RX/TX tests, language NVS reboot/OTA verification and Windows BAT/PowerShell update/release runs.
- [ ] Re-run the complete test suite: it was not completed during preparation of this draft. Only separately listed smoke/regression tests were verified.
- [ ] Review firmware network authentication and release-related security before wider exposure; keep the web server on a trusted network for now.
- [ ] If physical units will be sold, separately investigate applicable manufacturer duties and conformity; an open-source permission is **not** a CE, radio or safety approval.
- [ ] Create/connect a GitHub repository, configure visibility, branch protection, issue labels and releases; import this source only after the preceding public-sharing/rights checks.
