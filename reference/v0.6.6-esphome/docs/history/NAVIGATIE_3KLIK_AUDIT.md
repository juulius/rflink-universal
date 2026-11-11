# RFLink Universal – NAV3 usability/navigation audit

Date: 2026-10-25. Starting source: RXLOG2_ACTIVITYFIX1.

## Design contract
- Target: a named destination or everyday control is visible within **at most three navigation clicks**, starting at Status, on desktop and mobile.
- Do not count typing a value or the separate, intentional steps needed to pair or learn RF devices as navigation clicks. A wizard may require more than three actions to finish safely.
- A button naming a tool must reveal the tool's actual content, not stop at a second, collapsed heading.
- Simple daily tasks stay compact. Advanced technical detail stays behind an explicit expander **only when it really adds information**, not to hide the primary content.
- Preserve live UI state: a changing background log must not close open details or move the reader unexpectedly (ACTIVITYFIX1 remains in place).

## Audited journeys (clicks from Status)

| Destination/goal | Clicks to visible destination | Path | Result |
|---|---:|---|---|
| Status / radio health | 0 | Initial dashboard | Already visible |
| Activity / raw signals / noise | 1 | Activity nav | Working; filtering is direct |
| Activity for a particular radio | 1 | Radio card → View activity | Existing contextual shortcut |
| Add device wizard | 1 | Status quick action → Add device | Device wizard visible |
| Manage devices | 1 | Status quick action → Manage devices | Device list visible |
| Diagnose from Status | 1 | View diagnostics | Fixed: diagnostics content opened immediately |
| Diagnose from Settings | 2 | Settings → Diagnostics | Fixed: inner accordion automatically opens |
| Recent starts | 3 | Settings → Diagnostics → Recent starts | The third click opens actual secondary detail |
| Library and updates | 2 | Settings → Library and updates | Opens visible content directly |
| Decoder library | 2 | Settings → Decoder library | Opens visible content directly |
| Own decoders/profiles | 2 | Settings → Custom protocols and profiles | Fixed: inner accordion automatically opens |
| Create a new decoder | 3 | Settings → Decoder library/Custom → Create decoder | Added direct route to the advanced decoder editor |
| Home Assistant device controls | 2 | Settings → Manage HA devices | Fixed: per-device controls expanded at target |
| Logging limits | 1 | Settings | All four limits immediately visible |
| LED & language | 1 | Settings | Direct controls on one compact card |
| Radio boot log export | 3 | Status → View diagnostics → Recent starts → Export | Note: fourth click if entering via Settings; export is a **separate action** after reaching the boot log |

Click counts describe **reaching** a screen/control. Changing a setting and pressing Save is a workflow action, not a new navigation level.

## Changes introduced by NAV3
1. Automatically open `diagnosticsDetails` and `userLibraryDetails` on their named settings shortcuts, while retaining expandable detailed subsections.
2. Add Back to settings overview at the top of each settings tool, without hiding other settings behind extra tabs.
3. Add Create decoder in the decoder library and own protocols section; both jump straight to the existing decoder editor in Add device.
4. Have Manage HA devices open the device's Home Assistant controls. Normal Devices navigation stays compact.
5. Keep all controls/element IDs and API contracts unchanged; include new translation in English, Dutch, French and German.
6. Update older browser tests which still expected all settings tool contents to be visible concurrently from a former version of the settings page.

## Remaining considerations / scope limits
- The 3-click rule targets **navigation and findability**. Device pairing, calibration, RF decoding and destructive actions should require deliberate confirmation when appropriate, even when that takes more than 3 actions.
- MQTT broker configuration is currently via YAML/secrets; the web UI only gives instructions. This was not converted to runtime editing.
- Browser history/deep URLs are not implemented by NAV3; the interface is a one-document app and uses internal navigation. This can be considered separately, with proper state restoration.
- Long device lists may require scrolling. The three-click rule cannot eliminate vertical scrolling for an arbitrary number of devices; a search field or quick filter is a possible future enhancement.
- After any OTA update, spot-check the status → diagnostics shortcut, Settings → own protocols, and Settings → HA devices on the actual ESP32.

## Validation
- Chromium browser NAV3 tests: 1680px desktop, 1280px laptop and 390px mobile: PASS.
- Updated legacy browser tests for compact settings, library review, simple UI, wide UI: PASS.
- Boot diagnostics browser test (including export): PASS.
- RXLOG1 browser test (radio/noise filter/settings/export): PASS.
- Core UI contract tests (settings, localization bindings, embedded web UI, RXLOG2 source): PASS.
- Not run here: full ESPHome 2026.10 compilation or ESP32-S3 hardware/OTA soak test.
