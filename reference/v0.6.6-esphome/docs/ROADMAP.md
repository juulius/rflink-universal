# Roadmap (not a delivery promise)

## Release blockers
- Complete ESPHome Validate/Compile for the updated source and run Windows PowerShell/BAT packaging tests on Windows.
- Confirm ESP32-S3 boot both with and without all CC1101 modules.
- Keep all four web-language catalogs in sync when adding UI messages or decoder onboarding text; extend regression fixtures whenever a new dynamic screen state is added.
- Perform firmware/library source provenance and license review; include final license texts and third-party notices.
- Conduct real radio/antenna and power tests and document exactly which hardware is verified.
- Add web authentication and document a sensible LAN security model before considering network exposure.

## Later possibilities
- Decoder contribution examples with repeatable tests and a compatibility matrix.
- More guided learning flows and clearer diagnostics without misleading RF success claims.
- Verified upgrade-path migration tests, source archive provenance and CI checks.
- Publish the enclosure separately if and when Juulius chooses to release its design files.
