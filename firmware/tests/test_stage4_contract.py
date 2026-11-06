from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'components/rflink_radio_hal/include/rflink_cc1101_spi.hpp').read_text()
s=(r/'components/rflink_radio_hal/rflink_cc1101_spi.cpp').read_text()
m=(r/'main/main.cpp').read_text()
assert 'boot_reset_and_probe(RadioId id)' in h
assert 'read_runtime_status(RadioId id' in h
assert 'strobe(id, kSres)' in s
assert 'kMarcstate = 0x35' in s and 'kRxbytes = 0x3b' in s
assert 'read_status_register(id,kPartnum' in s
assert 'read_status_register(id,kVersion' in s
assert 'boot_reset_and_probe' in m
assert 'rflink_start_web_ota()' in m
assert '"rx_implemented\":false' not in m # RX not represented as active in entrypoint
assert 'esp_ota_mark_app_valid_cancel_rollback' in m
assert 'ESPHome' not in s
print('Stage 4 native boot diagnostic source contracts OK (not a hardware or IDF build test)')
