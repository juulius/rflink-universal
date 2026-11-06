from pathlib import Path
root=Path(__file__).resolve().parents[1]
a=(root/'main/main.cpp').read_text()
b=(root/'components/rflink_radio_hal/rflink_ook_rx.cpp').read_text()
p=(root/'components/rflink_radio_hal/rflink_rf_profiles.cpp').read_text()
w=(root/'main/rflink_web_ota.cpp').read_text()
assert 'ook_a(bus,rmt_a' in a and 'ook_b(bus,rmt_b' in a
assert 'manager.start_receive(rflink::RadioId::rf43342' in a
assert 'manager.start_receive(rflink::RadioId::rf43392' in a
assert 'rx868=&packet_rx' in a and 'rx868 || ook433a || ook433b' in a
assert 'bus_.strobe(radio_,kSrx)' in b and 'capture_.start(radio_)' in b
assert 'kOok433' in p and '{0x08,0x30}' in p and '{0x12,0x30}' in p and '{0x10,0x8c}' in p
assert 'rx43342_frames' in w and 'rx43392_frames' in w
print('Stage9 native three-radio RX BENCH contract OK')
