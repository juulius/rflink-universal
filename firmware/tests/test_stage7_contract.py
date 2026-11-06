from pathlib import Path
r=Path(__file__).resolve().parents[1]
a=(r/'main/main.cpp').read_text()
p=(r/'components/rflink_radio_hal/rflink_packet_rx.cpp').read_text()
m=(r/'components/rflink_radio_hal/rflink_radio_manager.cpp').read_text()
assert 'RadioManager manager(transport)' in a
assert 'manager.start_receive' in a
assert 'struct Reg' in p
assert 'esp_err_t PacketRx868::stop()' in p
assert 'ManagerError::unsupported_transport' in m
assert 'ManagerError::missing_profile' in m
assert 'RadioOperation::switching' in m
print('Stage 7 architecture and compilation fixes OK')
