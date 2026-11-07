from pathlib import Path
root=Path(__file__).resolve().parents[1]
web=(root/'main/rflink_web_ota.cpp').read_text()
assert '"/api/events"' in web and 'events_get(httpd_req_t* req)' in web
assert 'kRecentEventCapacity' in (root/'components/rflink_decoder_core/include/rflink_native_decoder.hpp').read_text()
assert '"/api/ota"' in web and '"/api/status"' in web
assert 'max_uri_handlers=13' in web
assert '"/api/tx"' not in web
assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in (root/'CMakeLists.txt').read_text()
print('PASS Stage17 web/event and build version contracts')
