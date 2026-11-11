#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C="$ROOT/components/rflink_universal"
ST="$ROOT/tests/stubs"
FLAGS=(-std=c++17 -Wall -Wextra -Werror -DUSE_RFLINK_UNIVERSAL_WEB -DUSE_ESP32 -I"$ST" -I"$C")
g++ "${FLAGS[@]}" -c "$C/rflink_universal.cpp" -o "${TMPDIR:-/tmp}/rflink_core_web.o"
# Catch the *exact* firmware linker regression: the main TU must DEFINITELY
# export the RFLinkWebHandler vtable, not merely compile in isolation.
nm -C "${TMPDIR:-/tmp}/rflink_core_web.o" | grep -E '[[:space:]][VD][[:space:]]vtable for esphome::rflink_universal::RFLinkWebHandler$' >/dev/null
if test -f "$C/rflink_web.cpp"; then
  echo "Separate rflink_web.cpp would risk duplicate symbols" >&2
  exit 1
fi
echo "RFLink web/core compile + vtable symbol regression: PASS"
