# Localization — English, Dutch, French and German

**Default:** English (`en`). Available selections: Nederlands (`nl`), Français (`fr`) and Deutsch (`de`). The Settings selector calls `GET /rflink/api/language` and `POST /rflink/api/language?language=xx`; the ESP32 stores the selected locale under its own NVS key. GPIO, RF identities, learned devices and radio settings are not migrated or overwritten when the language changes. A browser-local copy is only a display fallback when the device cannot be reached; the ESP32 value is authoritative when online.

## What is localized

The RFLink web application uses one keyed catalog for all four languages. This includes static navigation and help text, dynamic radio/status messages, device-management actions, warnings, prompts, learning-wizard states, decoder-library labels and Home Assistant-facing explanatory text shown in the RFLink web UI. Decoder onboarding text is translated through locale overlays keyed by protocol, so the original protocol identifiers and learned data are never rewritten.

Protocol names, RF payloads, device names entered by the user, learned identities and raw diagnostic values are deliberately left untouched. Native ESPHome pages, ESPHome logs and lower-level component log messages are also outside the RFLink web-interface locale layer.

## Runtime behaviour

Changing language does not require a reboot. Existing dynamic panels are re-rendered immediately in the selected language. Missing translation keys fall back to English rather than to Dutch. The device preference is intended to survive normal restarts and firmware updates that preserve NVS; a full flash erase or factory-style NVS erase can reset it. Browser storage failures do not override a valid language returned by the ESP32.

## Developer workflow

Edit `ui/locales/en.json`, `nl.json`, `fr.json` and `de.json` together and keep their key sets identical. English is the source/default language for new UI copy. After editing `ui/index.html` or a locale file, regenerate the embedded firmware header with:

```text
python rflink_universal/tools/embed_web_ui.py
```

The generator produces `ui/locales.bundle.json` and `components/rflink_universal/web_ui.h`. Windows release/update scripts do not require Python; their release checks compare the editable HTML plus bundled locale bytes with the embedded header and reject drift. Do not edit only the generated C++ header.

## Regression coverage

`tests/test_localization_browser.py` switches between all four languages and verifies both saved-language behaviour and dynamic states for disconnected radios, Learn, Monitor, Devices and the device-name placeholder. The other Chromium UX tests intentionally use Dutch in some fixtures as an independent non-default-language regression. These are browser/API mocks; physical NVS persistence and native ESPHome pages remain separate hardware/integration checks.
