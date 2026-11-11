# RFLink Universal – source cleanup (2026-10-25)

The archive `RFLink_Archief_Vervallen_Hotfix_Bestanden.zip` contains the eight removed historical files listed below. Their original SHA-256 hashes are recorded in `cleanup_manifest.json`. They were one-time patch or patch-note files whose effects have already been merged into the source. Running a patch from that archive against this firmware is **not recommended**.

## Removed from main source root

- `APPLY_COMPILEFIX2.bat`: One-time migration script; legacy rflink_web.cpp is already absent and merged.
- `BOARDSAFE1_NOTES.md`: Historical board safe fix notes, already included in current firmware.
- `BOOTDIAG1_README.md`: Patch-specific notes, consolidated in release notes.
- `COMPILEFIX1_README.md`: Superseded one-time compile fix notes.
- `COMPILEFIX2_README.md`: Superseded compile fix documentation.
- `LINKFIX1_README.md`: Superseded linker hotfix documentation.
- `RELEASEFIX_REV2_README.md`: Superseded release build fix documentation.
- `VERSIONUI1_README.md`: Superseded UI patch notes.

## Deliberately retained

- `MAAK_RELEASE.bat`, `MAAK_UPLOAD.bat`, `UPDATE_RFLINK.bat` are active Windows workflows; `TEST_UPDATE_RELEASE.bat` verifies updater safety.
- `BOOTFIX_README.md`, `LEDFIX_README.md` and core installation documentation remain because the build/update scripts and existing tests explicitly reference them.
- `rflink_universal/maintenance/legacy_hotfix/` and baseline YAML files support backwards-compatibility/regression tests; not safe to remove solely by name.
- Decoder `.cpp`, manifests, profiles, UI locale files, generated catalogs and embedded `web_ui.h` may be loaded via ESPHome or build-time code generation despite having no obvious textual reference.
- No unused runtime C++ module was automatically deleted. The linker, decoder registry and ESPHome build need full verification before such deletion.

See `LOGGING_RUNTIME.md` for new API, memory design, and limitations.
