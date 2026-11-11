#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C="$ROOT/components/rflink_universal"
OUT="${TMPDIR:-/tmp}/rflink_universal_tests"
INC="${TMPDIR:-/tmp}/rflink_universal_host_include"
rm -rf "$INC"
mkdir -p "$INC/esphome/components"
ln -s "$C" "$INC/esphome/components/rflink_universal"

# Match ESPHome: generated catalog headers live in src/rflink_decoders.
python3 "$ROOT/tests/test_esphome_generated_include_paths.py"

mapfile -t DECODER_SOURCES < <(python - "$ROOT" <<'PY'
import json,sys
from pathlib import Path
root=Path(sys.argv[1])
for folder in sorted((root/'decoders').iterdir()):
    if not folder.is_dir() or folder.name.startswith('_'): continue
    m=json.loads((folder/'manifest.json').read_text())
    if m.get('enabled', True) and m.get('host_test', False):
        
        for source in sorted(folder.glob('*.cpp')):
            if source.name != 'tx_encoder.cpp': print(source)
PY
)

mapfile -t TX_ENCODER_SOURCES < <(python - "$ROOT" <<'PY2'
import json,sys
from pathlib import Path
root=Path(sys.argv[1])
for folder in sorted((root/'decoders').iterdir()):
    if not folder.is_dir() or folder.name.startswith('_'): continue
    m=json.loads((folder/'manifest.json').read_text())
    tx=folder/'tx_encoder.cpp'
    if m.get('enabled', True) and tx.exists(): print(tx)
PY2
)

# RAMSES-II encoder depends on both the wire-format codec and the protocol codec.
# Unlike DECODER_SOURCES, TX_ENCODER_SOURCES only lists tx_encoder.cpp files.
RAMSES_LINK_SOURCES=("$ROOT/decoders/ramses_ii/codec.cpp" "$ROOT/decoders/ramses_ii/tx_codec.cpp")

# Radio Engine V2 universal manifest + route-aware dispatcher and 100-decoder benchmark.
python3 "$ROOT/tests/test_radio_engine_v2_manifests.py"
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_radio_engine_v2.cpp" -o "${OUT}_radio_v2"
"${OUT}_radio_v2"
# Decoder-declared SPI/UART transport selection (no GPIO hardcoding).
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_radio_transport_policy.cpp" -o "${OUT}_transport_policy"
"${OUT}_transport_policy"
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_ramses_uart_stream.cpp" "$C/ramses_uart_stream.cpp" \
  "$ROOT/decoders/ramses_ii/codec.cpp" "$ROOT/decoders/ramses_ii/tx_codec.cpp" \
  -o "${OUT}_ramses_uart"
"${OUT}_ramses_uart"
# RAMSES4 SPI-only FIFO engine (physical adapter disabled unless lab-opted-in).
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_ramses_spi_fifo.cpp" "$C/ramses_spi_fifo.cpp" \
  "$ROOT/decoders/ramses_ii/codec.cpp" "$ROOT/decoders/ramses_ii/tx_codec.cpp" \
  -o "${OUT}_ramses_spi"
"${OUT}_ramses_spi"
# RAMSES3 pre-hardware arbiter: safe exclusive access and guaranteed restore.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_radio_mode_manager.cpp" -o "${OUT}_radio_mode"
"${OUT}_radio_mode"
# RAMSES-II/Orcon TX codec and pairing protocol remain offline until a
# validated physical CC1101 RAMSES-II transport is commissioned.
g++ -std=c++17 -Wall -Wextra -Werror -O2 \
  "$ROOT/tests/test_ramses_ii_tx.cpp" "$ROOT/decoders/ramses_ii/codec.cpp" \
  "$ROOT/decoders/ramses_ii/tx_codec.cpp" -o "${OUT}_ramses_tx"
"${OUT}_ramses_tx"
# RAMSES-II/Orcon integration: real RFLink decoder adapter + encoded capture fixtures.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" -I"$INC" \
  "$ROOT/tests/test_ramses_ii.cpp" "$C/decoder_registry.cpp" \
  "$ROOT/decoders/ramses_ii/decoder.cpp" "$ROOT/decoders/ramses_ii/codec.cpp" \
  "$ROOT/decoders/itho_rft/decoder.cpp" "$C/itho_codec.cpp" \
  -o "${OUT}_ramses_ii"
"${OUT}_ramses_ii"
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" -I"$INC" \
  "$ROOT/tests/test_core.cpp" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" \
  "${DECODER_SOURCES[@]}" \
  -o "$OUT"
"$OUT"
# Compile and execute real, registered weather decoders against deterministic RF pulses.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" -I"$INC" \
  "$ROOT/tests/test_weather_decoders.cpp" "$C/decoder_registry.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" \
  "${DECODER_SOURCES[@]}" -o "${OUT}_weather"
"${OUT}_weather"
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_led_status.cpp" -o "${OUT}_led"
"${OUT}_led"
# The 47KB embedded HTML makes GCC's -O2 on this host-only integration test
# unnecessarily expensive; firmware compilation is a separate validation.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_radio_monitor_core.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_monitor"
"${OUT}_monitor"
# The large fixed-size NVS blobs make GCC -O2 spend minutes unrolling a
# host-only test; -O0 validates the same persistence implementation.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -I"$C" \
  "$ROOT/tests/test_device_persistence.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" \
  -o "${OUT}_persistence"
# Library reconciliation: official firmware wins only after safe comparison/user confirmation.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_library_reconciliation.cpp" -o "${OUT}_library"
"${OUT}_library"
"${OUT}_persistence"
# TX1 pure policy: supported decoder, opt-in identity and immutable TX snapshot.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_tx_policy.cpp" -o "${OUT}_tx_policy"
"${OUT}_tx_policy"
# Home Assistant MQTT Discovery: real encoder, core lifecycle, and optional-MQTT build guards.
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$C" \
  "$ROOT/tests/test_ha_discovery.cpp" -o "${OUT}_ha_discovery"
"${OUT}_ha_discovery"
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_ha_mqtt_core.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_ha_integration"
"${OUT}_ha_integration"
# TX1 integration: explicit opt-in, MQTT command routing and hardware safety.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_tx_mqtt_core.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_tx_mqtt"
"${OUT}_tx_mqtt"
# TX2: generic decoder-pack registry, Byron fixed 4-bit command and per-device opt-in.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_tx_encoder_registry.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_tx_registry"
"${OUT}_tx_registry"
# TX3: extended generic pack-based encoders, radio safety and opt-in tests.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_tx_remaining.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" \
  "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_tx_remaining"
# Somfy RTS native virtual remote: pairing, waveform, rolling-code reservation and persistence.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_somfy_tx.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_somfy_tx"
"${OUT}_somfy_tx"
# Somfy Home Assistant exposure: normal controls only after pairing/opt-in; never publish PROG.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_somfy_ha.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_somfy_ha"
"${OUT}_somfy_ha"
# Itho RFT native virtual remote: per-device identity, pair/unpair, packet speed/timer and monotonic state.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_ESP32 -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_itho_tx.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_itho_tx"
"${OUT}_itho_tx"
# Itho Home Assistant exposure: speed/timer buttons only after pairing and explicit TX opt-in.
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_ESP32 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_itho_ha.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" \
  "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" -o "${OUT}_itho_ha"
"${OUT}_itho_ha"
"${OUT}_tx_remaining"
# Compile translation units separately: -c -o ONE.o cannot accept multiple sources.
for file in rflink_universal rflink_log_store; do
  g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -DUSE_ESP32 -DUSE_RFLINK_UNIVERSAL_WEB \
    -I"$ROOT/tests/stubs" -I"$C" -c "$C/${file}.cpp" -o "${OUT}_ha_embedded_${file}.o"
done
echo 'HA MQTT optional C++ compilation (host ESPHome/MQTT stubs): PASS'
g++ -std=c++17 -Wall -Wextra -Werror -O0 -I"$C" \
  "$ROOT/tests/test_user_library.cpp" "$C/user_vm.cpp" "$C/user_library.cpp" -o "${OUT}_user_library"
"${OUT}_user_library"
g++ -std=c++17 -Wall -Wextra -Werror -O0 -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_user_flow.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" \
  -o "${OUT}_user_flow"
"${OUT}_user_flow"
g++ -std=c++17 -Wall -Wextra -Werror -O0 -DUSE_MQTT -I"$INC" -I"$ROOT/tests/stubs" -I"$C" \
  "$ROOT/tests/test_user_flow.cpp" "$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" "$C/tx_engine.cpp" "${TX_ENCODER_SOURCES[@]}" "${RAMSES_LINK_SOURCES[@]}" \
  "$C/decoder_registry.cpp" "$C/device_store.cpp" "$C/device_persistence.cpp" "$C/developer_session.cpp" \
  "$C/user_vm.cpp" "$C/user_library.cpp" "$C/somfy_codec.cpp" "$C/itho_codec.cpp" \
  -o "${OUT}_user_flow_mqtt"
"${OUT}_user_flow_mqtt"
python3 "$ROOT/tests/test_user_promotion.py"
python3 "$ROOT/tests/test_official_profiles.py"
python3 "$ROOT/tests/test_user_ui_static.py"
bash "$ROOT/tests/run_web_smoke.sh"
