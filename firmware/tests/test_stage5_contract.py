from pathlib import Path
r=Path(__file__).resolve().parents[1]
spi=(r/'components/rflink_radio_hal/rflink_cc1101_spi.cpp').read_text()
rx=(r/'components/rflink_radio_hal/rflink_packet_rx.cpp').read_text()
web=(r/'main/rflink_web_ota.cpp').read_text()
assert 'bus.max_transfer_sz = 64;' in spi
assert 'memcpy(out, rx + 1, length);' in spi
assert 'rxbytes&0x80' in rx and 'SFRX' in rx
assert 'rf_profile(RfProfileId::itho_868_packet)' in rx
assert 'rx868_packets' in web and 'rx868_fifo_overflows' in web
assert 'rx_implemented\\\":false' not in web or 'rx868_packet_diagnostic' in web
print('Stage 5 static checks OK')
