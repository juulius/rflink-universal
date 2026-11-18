#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror -Wno-unused-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/rflink_web_ota.cpp
# Compile the opt-in-only protected capture/export routes, too, without
# enabling them in ordinary firmware or claiming any hardware validation.
g++ -std=c++17 -DRFLINK_ENABLE_SYNC_868_BENCH=1 -fsyntax-only \
  -Wall -Wextra -Werror -Wno-unused-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/rflink_web_ota.cpp
echo 'PASS: normal and bench-only web+OTA source syntax (host IDF stubs, not real SDK)'
