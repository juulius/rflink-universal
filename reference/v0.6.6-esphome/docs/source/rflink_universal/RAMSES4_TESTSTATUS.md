# RAMSES4 SPI-only — validation status (2026-10-26)

## Passed here

- Standalone `g++ -std=c++17 -Wall -Wextra -Werror` SPI FIFO test: valid framed RX, corrupt frame rejection, 10-byte chunk streaming, RX timeout, FIFO overflow, simulated >64-byte TX FIFO refilling, I/O error.
- `test_ramses_ii.cpp`: two user-captured valid Orcon/RAMSES-II packets; checked identity, modes and Itho matcher precedence.
- `test_ramses_ii_tx.cpp`: send-codec actions, pairing telegram preparation, ACK guard and bit encoding (software only).
- `test_radio_mode_manager.cpp`: lease and fallback including error injection.
- `test_decoder_catalog_v2.py`, `test_boardsafe_cpp_compile.py`, `test_release_radio_guard.py`, `test_rx_log_integration.py`, `test_user_promotion.py`, `test_version_ui.py`, `test_packaging.py`, `test_radio_optional.py`.
- Partial `run_tests.sh`: core decoding, weather decoding, LED and RFLink monitor regression through `Core monitor: PASS`.
- YAML parse: `ramses_spi_lab: false` default.

## Not passed / not run here

- Full `run_tests.sh` did not finish inside the execution limit (no failure seen in completed stages). The remaining stages are **unverified**.
- Full ESPHome + ESP-IDF compile not available in this runtime (ESPHome not installed). API integration of ESP32 adapter remains to validate on target toolchain.
- No physical CC1101, spectrum/oscilloscope measurement, independent Orcon receiver, Itho fallback check, OTA or Home Assistant integration test.
- FIFO RX preamble/sync detection, on-air TX waveform fidelity and Orcon pairing are NOT validated. There is no exposed live TX/pairing control.

## Safety defaults

- No SPI register changes unless `ramses_spi_lab: true` and an explicit `request_ramses_spi_lab_rx(ms)` occurs.
- Max RX lease 30 s; example button uses 3 s. Itho driver register backup/reset/readback and sticky fail-close on restore failure.
- Physical Orcon TX not wired to a GUI, web API or MQTT. Never treat a software TX sample as an RF test.
