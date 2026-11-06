#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-but-set-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/main.cpp
echo 'PASS main.cpp native startup and OTA health syntax (IDF stubs, not real SDK)'
