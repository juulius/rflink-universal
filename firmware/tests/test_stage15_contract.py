from pathlib import Path
p=Path(__file__).resolve().parents[1]
spi=(p/'components/rflink_radio_hal/rflink_cc1101_spi.cpp').read_text()
web=(p/'main/rflink_web_ota.cpp').read_text()
main=(p/'main/main.cpp').read_text()
assert 'cfg.spics_io_num = -1' in spi
assert 'gpio_get_level(static_cast<gpio_num_t>(kSpiPins.miso))' in spi
assert 'ready_timeouts_[i].fetch_add' in spi
assert 'const esp_err_t released=gpio_set_level(cs,1)' in spi
assert 'return transaction_burst(id,tx,nullptr,length+1)' in spi
assert 'const esp_err_t err=transaction_burst(id,tx,rx,length+1)' in spi
assert 'spi_ready_timeouts' in web and 'spi_transfer_errors' in web
assert 'rflink_set_spi_stats_provider(&bus)' in main
assert 'ota_busy.exchange(true)' in web
assert 'if(e!=ESP_OK){\n    ota_busy=false' in web
assert 'rflink_web_ota.cpp' in (p/'tests/test_web_syntax.sh').read_text()
assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in (p/'CMakeLists.txt').read_text()
print('PASS Stage15 contracts: CHIP_RDYn, CS release, shared FIFO guard, web/OTA exposure, one OTA at a time')
