from pathlib import Path
root=Path(__file__).resolve().parents[1]
manager=(root/'components/rflink_radio_hal/rflink_radio_manager.cpp').read_text()
uart=(root/'components/rflink_radio_hal/rflink_uart_tx.cpp').read_text()
main=(root/'main/main.cpp').read_text()
web=(root/'main/rflink_web_ota.cpp').read_text()
profiles=(root/'components/rflink_radio_hal/rflink_rf_profiles.cpp').read_text()
assert 'transmit_encoded' in manager and 'stop_packet_rx' in manager and 'start_packet_rx' in manager
assert 'restore_failure' in manager and 'tx_failure' in manager
assert 'transmit_uart' in main and 'static rflink::Cc1101GdoUartTx uart_tx(bus)' in main
assert 'uart_driver_delete' in uart and 'gpio_reset_pin' in uart
assert 'kChangedRegs' in uart and 'read_register' in uart and 'write_register' in uart
assert 'uart_wait_tx_done' in uart and 'uart_write_bytes' in uart
assert '0x40' in profiles and '(t->deviatn & 0x08u)' in profiles
assert '"tx_implemented\\":false' in web
assert '"tx_lab_driver_present\\":true' in web
assert 'httpd_uri_t tx' not in web and 'transmit_encoded' not in web
assert 'rflink_ramses_tx.cpp' in (root/'components/rflink_decoder_core/CMakeLists.txt').read_text()
print('Stage11 architecture, recovery, no accidental OTA/web TX exposure: PASS')
