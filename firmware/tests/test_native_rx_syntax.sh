#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for src in rflink_rmt_rx.cpp rflink_ook_rx.cpp rflink_packet_rx.cpp; do
  g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror \
    -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
    "components/rflink_radio_hal/$src"
done
echo 'PASS native GDO/RMT RX C++ syntax against minimal test stubs (not ESP-IDF build)'
