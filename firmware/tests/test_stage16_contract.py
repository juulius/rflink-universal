from pathlib import Path
p=Path(__file__).resolve().parents[1]
web=(p/'main/rflink_web_ota.cpp').read_text()
assert 'same_board_profile_for_ota' in web
assert 'board_hash' in web and 'spi_pins' in web and 'radio_pins' in web
# Board identity is served through /api/status; the duplicate Advanced UI is gone.
assert 'board_id' in web and 'board_hash' in web
assert 'status.uri="/api/status"' in web
assert 'advanced.handler=native_redirect_get' in web
assert '"/api/tx"' not in web
page=(p/'main/webui/index.html').read_text()
js=(p/'main/webui/app.js').read_text()
assert 'id="ota-form"' in page and 'id="ota-file"' in page
assert 'X-RFLink-Update-Key' in js and "request.open('POST','/api/ota')" in js
assert 'boards/rflink_s3_n16r8.json' in (p/'CMakeLists.txt').read_text() or (p/'boards/rflink_s3_n16r8.json').exists()
cmake=(p/'CMakeLists.txt').read_text()
assert 'generate_board_header.py' in cmake and 'PROJECT_VER "0.7.0-n16r8-s20-b${RF_BOARD_HASH}"' in cmake
print('PASS Stage16 web OTA board pinout & staged UI contract')
