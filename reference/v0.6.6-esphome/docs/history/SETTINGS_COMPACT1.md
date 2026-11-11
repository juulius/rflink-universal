# SETTINGS_COMPACT1 – compact settings dashboard

This update changes the **layout and navigation of the existing RFLink Universal settings page**. It is based on RXLOG1 CLEAN SOURCE (25 Oct 2026). The working YAML and hardware/reboot configuration are unchanged.

## Overview (visible immediately)

- **General & status light:** language selector, RGB brightness, heartbeat, LED status, individual save button.
- **Logging:** three independent signal limits (250 per radio by default), one shared noise limit (2000 by default), logger usage and individual save button.
- **Home Assistant & system:** MQTT broker status, device-management action, optional MQTT explanation, firmware version/revision and optional build details.
- **More settings & tools:** always-visible quick selectors for Library & updates, Decoder library, Custom protocols and profiles, and Diagnostics. Exactly one expanded panel at a time. Clicking again closes it.

On a large desktop all three primary groups share one row; on laptops two primary groups share a row and the shorter Home Assistant/system group spans the next row; on smaller displays the layout stacks vertically without horizontal scrolling. Detailed technical sections open only when requested.

## Compatibility

- Existing DOM element IDs and API handlers are retained, including `rxLogLimit0`–`2`, `rxLogNoiseLimit`, `rxLogSave`, `ledSave`, `haMqttStatus`, `firmwareVersion`, `firmwareRevision`, `firmwareBuilt` and library/decoder control IDs.
- Existing runtime logging, NVS persistence, LED and language logic are unchanged.
- Existing four UI languages receive new navigation/group labels; new labels use the same keys in all four locales.
- `rflink_universal/ui/index.html` is editable source; `components/rflink_universal/web_ui.h` and `ui/locales.bundle.json` have been regenerated via `tools/embed_web_ui.py`.
- No firmware binary or actual ESP32-S3 hardware/OTA verification is included.

## Important UI detail

The quick-selector panels are collapsed by default to avoid a very long settings page. This is **not** deletion of functionality. Decoder library, custom profiles, library updates and diagnostics are still available behind their matching visible selector. Links to library review or custom profile details expose the relevant panel automatically.

## Test status

- Syntax, translation parity, and HTML IDs: PASS (`test_user_ui_static.py`).
- RX logging source integration contract: PASS (`test_rx_log_integration.py`).
- Settings/YAML configuration contract: PASS (`test_central_settings.py`).
- C++ web integration compile with test stubs and RFLinkWebHandler vtable: PASS (`run_web_smoke.sh`).
- Static Chromium visual checks at 1680px, 1240px and 390px: PASS for layout/overflow; no horizontal overflow at 1240px and 390px.
- Existing Node VM mock tests require a TextEncoder polyfill in their sandbox; this is unrelated to the HTML layout change.
- Full ESPHome build and ESP32 hardware test: NOT EXECUTED.
