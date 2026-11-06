#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for src in rflink_tx_session.cpp rflink_rmt_tx.cpp rflink_packet_tx.cpp; do
 g++ -std=c++17 -fsyntax-only -Wall -Wextra -Werror \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  "components/rflink_radio_hal/$src"
done
echo 'PASS native RMT/FIFO/session C++ syntax against minimal simulated ESP-IDF headers (not a real IDF build)'
