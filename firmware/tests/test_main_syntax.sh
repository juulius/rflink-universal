#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-but-set-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/main.cpp
# Compile the otherwise-inactive Experimental bench startup branch too.
# This is a host stub syntax check, NOT proof of CC1101/ESP32 hardware.
g++ -std=c++17 -DRFLINK_ENABLE_SYNC_868_BENCH=1 -fsyntax-only \
  -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-but-set-variable \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  -Icomponents/rflink_decoder_core/include -Imain main/main.cpp
echo 'PASS main.cpp normal FIFO and explicit sync bench compile-path syntax (IDF stubs)'

