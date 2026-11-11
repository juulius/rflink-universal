"""Root redirect and favicon regressions: actual firmware routes + bundled asset parity."""
from pathlib import Path
import re
import struct

root=Path(__file__).resolve().parents[2]
base=root/'rflink_universal'
cpp=(base/'components/rflink_universal/rflink_universal.cpp').read_text()
html=(base/'ui/index.html').read_text()
header=(base/'components/rflink_universal/web_ui.h').read_text()
svg=(base/'ui/favicon.svg').read_text()
ico=(base/'ui/favicon.ico').read_bytes()
assert html.count('href="/rflink/favicon.svg"') == 1
assert html.count('href="/favicon.ico"') == 1
assert re.search(r'if \(url == "/"\).*?request->redirect\("/rflink"\)',cpp,re.S)
assert 'request->method() == HTTP_GET' in cpp
assert 'url == "/favicon.ico"' in cpp and 'url == "/rflink/favicon.svg"' in cpp
assert 'beginResponse(200, "image/x-icon", RFLINK_FAVICON_ICO, RFLINK_FAVICON_ICO_SIZE)' in cpp
assert 'beginResponse(200, "image/svg+xml", reinterpret_cast<const uint8_t *>(RFLINK_FAVICON_SVG), RFLINK_FAVICON_SVG_SIZE)' in cpp
bundle=(base/'ui/locales.bundle.json').read_text()
assert 'R"RFHTML('+html.replace('/*__RFLINK_LOCALES__*/',bundle)+')RFHTML"' in header
assert 'R"RFSVG('+svg+')RFSVG"' in header
m=re.search(r'static constexpr uint8_t RFLINK_FAVICON_ICO\[\] = \{(.*?)\};',header,re.S)
assert m
embedded=bytes(int(x,16) for x in re.findall(r'0x([A-Fa-f0-9]{2})',m.group(1)))
assert embedded == ico, 'ico header differs from editable icon'
assert ico[:4] == b'\x00\x00\x01\x00'
assert struct.unpack_from('<H',ico,4)[0] >= 2
release=(base/'tools/maak_release.ps1').read_text()
update=(base/'tools/update_rflink.ps1').read_text()
for path in ('rflink_universal/ui/favicon.svg','rflink_universal/ui/favicon.ico'):
    assert path in release and path in update, path
assert 'html|css|js|json|svg|ico' in release and 'html|css|js|json|svg|ico' in update
assert "'.svg','.ico'" in release
assert '${releaseVersion}_DEFAULT' in (base/'tools/test_update_release.ps1').read_text()
print('Root redirect, embedded HTML/SVG/ICO bytes, updater and release inventory: PASS')
