#!/usr/bin/env python3
"""Generate embedded UI and favicon headers from editable files (build time only)."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / "ui" / "index.html").read_text(encoding="utf-8")
locales = {code: json.loads((ROOT / "ui" / "locales" / f"{code}.json").read_text(encoding="utf-8"))
           for code in ("en", "nl", "fr", "de")}
assert all(set(locales["en"]) == set(catalog) for catalog in locales.values()), "Locale keys must match"
marker = "/*__RFLINK_LOCALES__*/"
assert html.count(marker) == 1, "Missing or duplicated locale marker"
# JSON does not interpolate user input and cannot terminate the enclosing script.
catalog = json.dumps(locales, ensure_ascii=False, separators=(",", ":")).replace("<", "\\u003c")
# Persist the exact bytes so the Python-free Windows release builder can verify
# that the embedded UI matches the editable HTML plus bundled translations.
(ROOT / "ui" / "locales.bundle.json").write_text(catalog, encoding="utf-8")
html = html.replace(marker, catalog)
svg = (ROOT / "ui" / "favicon.svg").read_text(encoding="utf-8")
ico = (ROOT / "ui" / "favicon.ico").read_bytes()
if ")RFHTML\"" in html or ")RFSVG\"" in svg:
    raise SystemExit("UI/favicon contains reserved raw-string delimiter")
if not ico.startswith(b"\x00\x00\x01\x00"):
    raise SystemExit("Missing or invalid favicon.ico")
icons = ",\n    ".join(", ".join("0x%02X" % b for b in ico[i:i+16]) for i in range(0, len(ico), 16))
out = ROOT / "components" / "rflink_universal" / "web_ui.h"
out.write_text(
    '#pragma once\n\n#include <cstddef>\n#include <cstdint>\n\nnamespace esphome {\nnamespace rflink_universal {\n\n'
    'static constexpr char RFLINK_UI_HTML[] = R"RFHTML(' + html + ')RFHTML";\n'
    'static constexpr size_t RFLINK_UI_HTML_SIZE = sizeof(RFLINK_UI_HTML) - 1;\n\n'
    'static constexpr char RFLINK_FAVICON_SVG[] = R"RFSVG(' + svg + ')RFSVG";\n'
    'static constexpr size_t RFLINK_FAVICON_SVG_SIZE = sizeof(RFLINK_FAVICON_SVG) - 1;\n\n'
    'static constexpr uint8_t RFLINK_FAVICON_ICO[] = {\n    ' + icons + '\n};\n'
    'static constexpr size_t RFLINK_FAVICON_ICO_SIZE = sizeof(RFLINK_FAVICON_ICO);\n\n'
    '}  // namespace rflink_universal\n}  // namespace esphome\n',
    encoding='utf-8'
)
print(f"Embedded UI: {len(html.encode('utf-8'))} bytes, SVG: {len(svg.encode('utf-8'))} bytes, ICO: {len(ico)} bytes")
