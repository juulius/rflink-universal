#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  components/rflink_radio_hal/rflink_uart_tx.cpp
echo 'PASS native UART transport C++ syntax against intentionally minimal ESP-IDF test stubs; NOT an ESP-IDF build'
