#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  components/rflink_radio_hal/rflink_cc1101_spi.cpp
echo 'PASS SPI bus/burst/FIFO/PA C++ syntax against minimal simulated ESP-IDF headers (NOT real SDK)'
