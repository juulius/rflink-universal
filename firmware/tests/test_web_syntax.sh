#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror -Wno-unused-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/rflink_web_ota.cpp
echo 'PASS: full web+OTA translation unit C++ syntax with IDF host stubs (NOT official SDK)'
