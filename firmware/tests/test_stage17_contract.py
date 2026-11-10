from pathlib import Path
root=Path(__file__).resolve().parents[1]
web=(root/'main/rflink_web_ota.cpp').read_text()
assert '"/api/events"' in web and 'events_get(httpd_req_t* req)' in web
assert 'kRecentEventCapacity' in (root/'components/rflink_decoder_core/include/rflink_native_decoder.hpp').read_text()
assert '"/api/ota"' in web and '"/api/status"' in web
# Route capacity follows the actual registered handlers, including new
# diagnostics endpoints. Do not freeze it to a historical route count.
import re
capacity=re.search(r'cfg\.max_uri_handlers=(\d+)',web)
assert capacity, 'HTTP handler capacity is not configured'
registered=len(re.findall(r'httpd_register_uri_handler\(server,&',web))
assert int(capacity.group(1))>=registered,(capacity.group(1),registered)
assert 'led_get.uri="/api/led"' in web
assert 'led_post.uri="/api/led"' in web
assert 'root.handler=native_index_get' in web
assert 'advanced.handler=native_redirect_get' in web
assert 'styles.uri="/app.css"' in web
assert 'script.uri="/app.js"' in web
assert '"/api/tx"' not in web
assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in (root/'CMakeLists.txt').read_text()
print('PASS Stage17 web/event and build version contracts')
