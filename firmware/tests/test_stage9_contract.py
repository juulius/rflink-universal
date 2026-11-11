from pathlib import Path
root=Path(__file__).resolve().parents[1]
a=(root/'main/main.cpp').read_text()
b=(root/'components/rflink_radio_hal/rflink_ook_rx.cpp').read_text()
p=(root/'components/rflink_radio_hal/rflink_rf_profiles.cpp').read_text()
w=(root/'main/rflink_web_ota.cpp').read_text()
assert 'ook_a(bus,rmt_a' in a and 'ook_b(bus,rmt_b' in a
assert 'manager.start_receive(rflink::RadioId::rf43342' in a
assert 'manager.start_receive(rflink::RadioId::rf43392' in a
# Stage 21: the RX task must survive an initial failure on *all* radios,
# otherwise there is no worker left to discover a recovered CC1101.
assert 'rx868=&packet_rx' in a and 'ook433a=&ook_a' in a and 'ook433b=&ook_b' in a
assert 'if(spi_ok)' in a and 'rx_worker_started=xTaskCreate' in a
assert 'check_rx_recovery' in a and 'rx_watchdog_action' in a
watchdog=(root/'components/rflink_radio_hal/include/rflink_rx_watchdog.hpp').read_text()
assert 'RadioOperation::transmitting:' in watchdog
assert 'RadioOperation::switching:' in watchdog
assert 'RxWatchdogAction::none' in watchdog
# The 433 MHz RMT capture is armed before a centrally verified
# IDLE/FIFO-flush/RX handover; SRX alone is no longer considered healthy RX.
spi=(root/'components/rflink_radio_hal/rflink_cc1101_spi.cpp').read_text()
assert 'capture_.start(radio_)' in b and 'bus_.enter_rx_verified(radio_)' in b
assert 'strobe(id,kSrx)' in spi and 'wait_for(kRxState,&last)' in spi
# v0.6.6 modem parity: ~5 kbps ASK/OOK, 153kHz IF and verified AGC.
assert 'kOok433' in p and '{0x08,0x30}' in p and '{0x12,0x30}' in p
for setting in ('{0x0b,0x06}','{0x10,0x87}','{0x11,0x93}', '{0x1b,0xc7}','{0x1c,0x00}','{0x1d,0xb1}'):
    assert setting in p, setting
assert 'rx43342_frames' in w and 'rx43392_frames' in w
print('Stage9/21 native three-radio RX and watchdog task lifecycle BENCH contract OK')
