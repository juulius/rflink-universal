from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'components/rflink_radio_hal/include/rflink_rmt_rx.hpp').read_text()
c=(r/'components/rflink_radio_hal/rflink_rmt_rx.cpp').read_text()
a=(r/'main/main.cpp').read_text()
m=(r/'components/rflink_radio_hal/rflink_radio_manager.cpp').read_text()
assert 'kMaxSymbols=512' in h
# Stage 34: The former literal 30,000 ns hardware RMT filter was
# rejected on ESP32-S3 as > 3,187 ns. The desired Radio Gaga 30us RF filter
# must be software; the hardware value must be clock-tree clamped.
assert 'kRequestedFilterUs=30' in c and 'kIdleNs=5000000' in c
assert 'esp_clk_tree_src_get_freq_hz(' in c
assert 'std::min<uint64_t>(kRequestedFilterNs,max_ns-1u)' in c
assert 'cfg.signal_range_min_ns=hardware_filter_ns_' in c
assert 'filter_short_pulses_inplace(' in c
assert 'rmt_receive(channel_' in c and 'xQueueSendFromISR' in c
assert 'radio!=RadioId::rf43342 && radio!=RadioId::rf43392' in c
assert 'start_pulse_rx' in m and 'stop_pulse_rx' in m
assert 'rmt_receive(channel_' in c
assert 'ook433a' in a and 'ook433b' in a
print('Stage 8 RMT transport and safety gating contract OK')
