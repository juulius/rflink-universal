#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="$PWD"
inc=("-I$root/components/rflink_radio_hal/include" "-I$root/components/rflink_decoder_core/include" "-I$root/tests")
for x in test_contract test_pin_contract test_radio_manager test_rf_profiles test_tx_manager test_rx_watchdog; do
  g++ -std=c++17 -O2 -Wall -Wextra -Werror "${inc[@]}" "tests/$x.cpp" components/rflink_radio_hal/rflink_rf_profiles.cpp components/rflink_radio_hal/rflink_radio_manager.cpp -o "/tmp/rflink_${x}_s10"
  "/tmp/rflink_${x}_s10"
done
g++ -std=c++17 -O2 -Wall -Wextra -Werror "${inc[@]}" \
  components/rflink_decoder_core/src/rflink_native_decoder.cpp \
  components/rflink_decoder_core/src/rflink_itho_codec.cpp \
  components/rflink_decoder_core/src/rflink_ramses_codec.cpp \
  components/rflink_decoder_core/src/rflink_ramses_tx.cpp \
  tests/test_native_decode.cpp -o /tmp/rflink_native_decode_s10
/tmp/rflink_native_decode_s10
g++ -std=c++17 -O2 -Wall -Wextra -Werror "${inc[@]}" \
  components/rflink_decoder_core/src/rflink_ramses_codec.cpp \
  components/rflink_decoder_core/src/rflink_ramses_tx.cpp \
  tests/test_native_tx_codec.cpp -o /tmp/rflink_native_tx_codec_s11
/tmp/rflink_native_tx_codec_s11
g++ -std=c++17 -O2 -Wall -Wextra -Werror "${inc[@]}" \
  components/rflink_radio_hal/rflink_rf_profiles.cpp \
  components/rflink_radio_hal/rflink_radio_manager.cpp \
  tests/test_tx_transports.cpp -o /tmp/rflink_tx_transports_s12
/tmp/rflink_tx_transports_s12
bash tests/test_native_uart_syntax.sh
bash tests/test_native_tx_syntax.sh
bash tests/test_native_spi_syntax.sh
bash tests/test_native_rx_syntax.sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -Icomponents/rflink_radio_hal/include \
  tests/test_native_rmt_filter.cpp -o /tmp/rflink_rmt_filter_s34
/tmp/rflink_rmt_filter_s34
# Native Radio Gaga 0.6.6 electrical guard, with real GPIO/CHIP_RDYn mocks.
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  components/rflink_radio_hal/rflink_native_boot_guard.cpp \
  tests/test_radiogaga_boot_guard.cpp \
  -o /tmp/rflink_radiogaga_boot_guard
/tmp/rflink_radiogaga_boot_guard
bash tests/test_main_syntax.sh
# The six ring buffers must remain independent and fully pageable.
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread -Imain \
  tests/test_log_pagination.cpp -o /tmp/rflink_log_pagination
/tmp/rflink_log_pagination
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Imain tests/test_native_safety.cpp -o /tmp/rflink_native_safety_s14
/tmp/rflink_native_safety_s14
for x in tests/test_stage*_contract.py tests/test_web_contract.py; do python3 "$x"; done
python3 tests/test_stage13_regression.py
python3 tests/test_stage14_contract.py
echo 'PASS legacy: 9 C++ hosttests, Python contracts Stage3-14, UART/RMT/FIFO/session syntax. Full ESP-IDF build NOT executed.' 
# Stage15: actual simulated SPI transactions + full native web translation-unit syntax.
bash tests/test_web_syntax.sh
g++ -std=c++17 -O2 -pthread -Wall -Wextra -Werror \
  -Itests/host_idf_stubs -Icomponents/rflink_radio_hal/include \
  components/rflink_radio_hal/rflink_cc1101_spi.cpp \
  components/rflink_radio_hal/rflink_packet_rx.cpp \
  components/rflink_radio_hal/rflink_rf_profiles.cpp \
  tests/test_spi_ready_behavior.cpp -o /tmp/rflink_spi_ready_stage15
/tmp/rflink_spi_ready_stage15
python3 tests/test_stage15_contract.py

python3 tests/test_board_profile.py
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Imain tests/test_board_ota.cpp -o /tmp/rflink_board_ota_s16
/tmp/rflink_board_ota_s16
python3 tests/test_stage16_contract.py

# Stage17: real decoder ring behavior, all source translation units, CMake declarations.
g++ -std=c++17 -O2 -pthread -Wall -Wextra -Werror "${inc[@]}" \
 components/rflink_decoder_core/src/rflink_native_decoder.cpp \
 components/rflink_decoder_core/src/rflink_itho_codec.cpp \
 components/rflink_decoder_core/src/rflink_ramses_codec.cpp \
 components/rflink_decoder_core/src/rflink_ramses_tx.cpp \
 tests/test_recent_events.cpp -o /tmp/rflink_recent_events_s17
/tmp/rflink_recent_events_s17
python3 tools/audit_all_sources.py
python3 tests/test_stage17_contract.py
python3 tools/check_embedded_js.py

python3 tests/test_stage18_build_gate.py

python3 tests/test_oneclick_build.py
