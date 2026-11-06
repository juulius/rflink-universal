#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export IDF_COMPONENT_MANAGER=0
command -v idf.py >/dev/null || { echo "ESP-IDF 5.5 environment missing" >&2; exit 1; }
python3 tools/generate_board_header.py
python3 tools/preflight.py
python3 tools/source_manifest.py
idf.py build
python3 tools/package_release.py
