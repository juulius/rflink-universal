from pathlib import Path
p=Path(__file__).resolve().parents[1]
manager=(p/'components/rflink_radio_hal/rflink_radio_manager.cpp').read_text()
spi=(p/'components/rflink_radio_hal/rflink_cc1101_spi.cpp').read_text()
session=(p/'components/rflink_radio_hal/rflink_tx_session.cpp').read_text()
rmt=(p/'components/rflink_radio_hal/rflink_rmt_tx.cpp').read_text()
fifo=(p/'components/rflink_radio_hal/rflink_packet_tx.cpp').read_text()
main=(p/'main/main.cpp').read_text()
web=(p/'main/rflink_web_ota.cpp').read_text()
assert 'transmit_rmt' in manager and 'transmit_packet' in manager
assert 'frame->pulse_count>256' in manager and 'frame->length!=p.packet_length' in manager
assert 'if(!outcome.hardware_restored)' in manager
assert 'read_pa_table' in spi and 'write_pa_table' in spi
assert '0x7f' in spi and 'length>63' in spi
assert 'set_checked(0x22,0x11)' in session and 'set_checked(0x22,0x10)' in session
assert 'snapshot' in session and 'if(pa_saved_)' in session and 'readback!=old_pa_' in session
assert 'rmt_tx_wait_all_done' in rmt and 'rmt_disable' in rmt and 'session.restore()' in rmt
assert 'seen_tx && mode==0x01 && (bytes&0x7f)==0' in fifo
assert 'if(!session.restore())' in fifo
assert 'Cc1101GdoRmtTx' in main and 'Cc1101PacketTx' in main
assert '"tx_implemented\\":false' in web
assert 'build\\rflink_universal.bin' in (p/'BUILD_OFFLINE.bat').read_text() or 'tools\\package_release.py' in (p/'BUILD_OFFLINE.bat').read_text()
assert any(v in (p/'CMakeLists.txt').read_text() for v in ('0.7.0-stage12','0.7.0-stage13','0.7.0-n16r8-s15','0.7.0-n16r8-s20-b${RF_BOARD_HASH}'))
print('Stage12 TX transport, PA/FIFO safety, manager routing, build script and no web TX: PASS')
