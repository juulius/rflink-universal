"""QA1 static regression guards; no optional Python packages."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
html=(root/'ui/index.html').read_text(encoding='utf-8')
embedded=(root/'components/rflink_universal/web_ui.h').read_text(encoding='utf-8')
assert '<label for="deviceName" data-i18n="static.061">Device name</label>' in html
assert 'id="snippet" class="codebox" readonly aria-label="Code suggestion" data-i18n-aria-label="static.090"' in html
for lang in ('en','nl','fr','de'):
    locale=json.loads((root/'ui/locales'/f'{lang}.json').read_text(encoding='utf-8'))
    assert locale['static.061'] and locale['static.090']
assert '<label for="deviceName" data-i18n="static.061">' in embedded
script=(root/'tests/run_tests.sh').read_text()
assert 'for file in rflink_universal rflink_log_store; do' in script
assert '"${OUT}_ha_embedded_${file}.o"' in script
assert '"$C/rflink_universal.cpp" "$C/rflink_log_store.cpp" -o' not in script
flow=(root/'tests/test_user_flow.cpp').read_text()
assert 'rflink_host_millis += 120;' in flow
print('QA1: localized accessible labels, deterministic embedded HTML, separate host objects, real-time learning fixture PASS')
