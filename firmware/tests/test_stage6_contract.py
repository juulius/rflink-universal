from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/'components/rflink_radio_hal/rflink_rf_profiles.cpp').read_text()
rx=(r/'components/rflink_radio_hal/rflink_packet_rx.cpp').read_text()
assert '{0x06,63}' in p and '{0x07,0x00}' in p and '{0x08,0x00}' in p
assert 'profile.modem_registers[j]' in rx
assert 'kIthoProfile' not in rx
assert 'nullptr,0,0' in p  # do not mistake a placeholder for active RX
print('Stage 6 profile contracts OK')
