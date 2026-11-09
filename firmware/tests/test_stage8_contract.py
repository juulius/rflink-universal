from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'components/rflink_radio_hal/include/rflink_rmt_rx.hpp').read_text()
c=(r/'components/rflink_radio_hal/rflink_rmt_rx.cpp').read_text()
a=(r/'main/main.cpp').read_text()
m=(r/'components/rflink_radio_hal/rflink_radio_manager.cpp').read_text()
assert 'kMaxSymbols=256' in h
# Stage 8 originally required a fixed 30,000 ns hardware RMT filter.
# Physical exp.9 on ESP32-S3 proved this exceeds the 255 source-clock tick
# limit (3187 ns at 80 MHz) and prevents both 433 receivers from starting.
# Require the proven ESPHome clock-bound behavior instead of the regression.
filter_header=(r/'components/rflink_radio_hal/include/rflink_rmt_filter.hpp').read_text()
assert 'kRmtRequestedFilterNs = 30000' in filter_header
assert 'kRmtIdleTimeoutNs = 5000000' in filter_header
assert 'kRmtGlitchFilterMaxTicks = 255' in filter_header
assert 'rmt_effective_filter_ns(' in filter_header
assert 'esp_clk_tree_src_get_freq_hz(' in c
assert 'cfg.signal_range_min_ns=effective_filter_ns_' in c
assert 'cfg.signal_range_max_ns=kRmtIdleTimeoutNs' in c
assert 'effective_filter_ns_=rmt_effective_filter_ns(kRmtRequestedFilterNs,source_hz)' in c
assert 'rmt_receive(channel_' in c and 'xQueueSendFromISR' in c
assert 'radio!=RadioId::rf43342 && radio!=RadioId::rf43392' in c
assert 'start_pulse_rx' in m and 'stop_pulse_rx' in m
assert 'rmt_receive(channel_' in c
assert 'ook433a' in a and 'ook433b' in a
print('Stage 8 RMT transport and safety gating contract OK')
