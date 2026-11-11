"""Check actual production event wiring; a policy-only unit test cannot catch missing callbacks."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
source = (root / 'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
header = (root / 'rflink_universal/components/rflink_universal/rflink_universal.h').read_text()
ui = (root / 'rflink_universal/ui/index.html').read_text()
led = (root / 'rflink_universal/components/rflink_universal/status_led.h').read_text()

# Check that the *production implementation*, not a copy of the test, calls
# each event in its successful and failure paths.
checks = {
    'boot in setup': r'void RFLinkUniversal::setup\(\)\s*\{\s*led_\.boot\(millis\(\)\)',
    'startup fault': r'if \(channels_\[i\]\.receiver == nullptr\)\s*\{[^}]*led_\.fault\(true\)',
    'baseline starts LED': r'void RFLinkUniversal::led_start_learning\([^)]*\)\s*\{[^}]*led_\.learn\(true\)',
    'baseline ends LED': r'void RFLinkUniversal::led_finish_baseline\(\)\s*\{[^}]*led_\.learn\(false\)',
    'listen stop LED': r'void RFLinkUniversal::led_stop_learning\(\)\s*\{[^}]*led_\.learn\(false\)',
    'capture on selected channel': r'led_\.learning\(\)\s*&&\s*!developer_\.baseline_mode\(\)\s*&&\s*frame\.channel == developer_\.channel\(\)\)\s*\{\s*led_\.capture',
    'confirm success after durable save': r'if \(!save_devices_\(\)\).*?led_\.learned\(millis\(\)\)',
    'link success after durable save': r'if \(!devices_\.link_candidate\(identity, uid\) \|\| !save_devices_\(\)\).*?led_\.learned\(millis\(\)\)',
    'link failure': r'if \(!devices_\.link_candidate\(identity, uid\) \|\| !save_devices_\(\)\)\s*\{.*?led_\.error\(millis\(\)\)',
    'save preferences': r'led_pref_\.save\(&stored\)',
    'read preferences': r'led_pref_\.load\(&stored\)',
    'web settings POST': r'void RFLinkWebHandler::handle_led_settings_\([^)]*\)\s*\{\s*if \(request->method\(\) != HTTP_POST\)',
    'web settings callback': r'parent_->set_led_settings\(static_cast<uint8_t>\(brightness\), quiet == "1"\)',
    'web status': r'StatusLed::name\(led\.mode\(now\)\)',
}
for label, pattern in checks.items():
    assert re.search(pattern, source, re.S), f'Missing real LED wiring: {label}'
for name in ('led_wifi', 'led_ota', 'led_pair_start', 'led_pair_end', 'led_tx'):
    assert name in header, name
for control in ('ledBrightness', 'ledHeartbeat', 'ledSave', 'stopListenBtn'):
    assert f'id="{control}"' in ui and f"$('#{control}')" in ui, control
assert "action=reset" in ui, 'Stop learning must call real API'
assert 'if (ota_) return Mode::OTA' in led and 'if (fault_' in led
assert 'if (radio_missing_) return Mode::RADIO_MISSING' in led
assert 'led_.radio_missing(all_checked && any_unavailable)' in source
assert '"radio_ontbreekt"' in led
assert "$('#ledHeartbeat').checked=!d.led.quiet" in ui, 'saved backend quiet state must be inverted for positive checkbox'
assert "($('#ledHeartbeat').checked?'0':'1')" in ui, 'positive checkbox must save inverse backend quiet value'
assert 'Groene hartslag uit bij gereed' not in ui, 'old confusing label must be removed'
print('Production LED event hooks, web settings, learning stop and UI controls: PASS')
