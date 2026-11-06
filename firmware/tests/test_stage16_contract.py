from pathlib import Path
p=Path(__file__).resolve().parents[1]
web=(p/'main/rflink_web_ota.cpp').read_text()
assert 'same_board_profile_for_ota' in web
assert 'board_hash' in web and 'spi_pins' in web and 'radio_pins' in web
assert '<details><summary><strong>Hardware en SPI' in web
assert "document.getElementById('rx'+i)" in web
assert '"/api/tx"' not in web
assert 'Kies bestand en vul wachtwoord in' in web
assert 'boards/rflink_s3_n16r8.json' in web
cmake=(p/'CMakeLists.txt').read_text()
assert 'generate_board_header.py' in cmake and 'PROJECT_VER "0.7.0-n16r8-s20-b${RF_BOARD_HASH}"' in cmake
print('PASS Stage16 web OTA board pinout & staged UI contract')
