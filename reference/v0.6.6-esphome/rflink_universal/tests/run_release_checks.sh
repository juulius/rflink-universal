#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

python "$ROOT/tools/check_decoder_packs.py"
python "$ROOT/tests/test_decoder_catalog_v2.py"
python -m py_compile \
  "$ROOT/components/rflink_universal/__init__.py" \
  "$ROOT/tools/new_decoder.py" \
  "$ROOT/tools/check_decoder_packs.py" \
  "$ROOT/tools/embed_web_ui.py" \
  "$ROOT/tests/test_board_config.py" \
  "$ROOT/tests/test_led_integration.py" \
  "$ROOT/tests/test_packaging.py" \
  "$ROOT/tests/test_somfy_ui_browser.py" \
  "$ROOT/tests/test_itho_ui_browser.py"

python "$ROOT/tests/test_esphome_compilefix.py"
python "$ROOT/tests/test_ha_config.py"
python "$ROOT/tests/test_central_settings.py"
python "$ROOT/tests/test_root_favicon.py"

bash "$ROOT/tests/run_tests.sh"

python - "$ROOT" <<'PY'
from pathlib import Path
import sys,yaml
root=Path(sys.argv[1])
class L(yaml.SafeLoader): pass
def anytag(loader,tag_suffix,node):
    if isinstance(node,yaml.ScalarNode): return loader.construct_scalar(node)
    if isinstance(node,yaml.SequenceNode): return loader.construct_sequence(node)
    return loader.construct_mapping(node)
L.add_multi_constructor('!',anytag)
for n in [root.parent/'rflink-universal.yaml', root/'secrets.example.yaml']:
    yaml.load(n.read_text(), Loader=L)
text=(root.parent/'rflink-universal.yaml').read_text()
assert 'decoder_directory: rflink_universal/decoders' in text, 'decoder directory incorrect'
assert 'path: rflink_universal/components' in text, 'external components path incorrect'
assert text.count('non_blocking: true') == 2, 'radio TX defaults not explicit'
for n in ('rflink__encryption_key','rflink__ota_password','wifi_ssid_iot','wifi_password_iot','rflink_fallback_password'):
    assert '!secret '+n in text, f'secret reference missing: {n}'
for n in ('itho_id1:', 'itho_id2:', 'itho_id3:', 'itho_device_type:', 'id: itho_counter', 'name: \"Itho Pair\"'):
    assert n not in text, f'legacy global Itho ESPHome control still present: {n}'
assert 'packet_transmitter: cc1101_868' in text, 'native Itho packet transmitter not configured'
h=(root/'components/rflink_universal/web_ui.h').read_text()
a=h.index('R"RFHTML(')+len('R"RFHTML('); b=h.index(')RFHTML";',a)
assert h[a:b] == (root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/', (root/'ui/locales.bundle.json').read_text()), 'embedded UI differs from HTML/locales'
print('YAML + embedded UI integrity: PASS')
PY

python - "$ROOT" <<'PY'
from pathlib import Path
import sys
root=Path(sys.argv[1]); html=(root/'ui/index.html').read_text().replace('/*__RFLINK_LOCALES__*/', (root/'ui/locales.bundle.json').read_text());
js=html.split('<script>',1)[1].split('</script>',1)[0]
Path('/tmp/rflink_release_ui.js').write_text(js)
PY
node --check /tmp/rflink_release_ui.js
echo "Frontend JS syntax: PASS"
node "$ROOT/tests/test_led_heartbeat_ui.js"
node "$ROOT/tests/test_user_controls_ui.js"

# The beginner wizard must create a complete pack without changing central files.
TEST_PACK="$ROOT/decoders/zz_release_wizard_test"
rm -rf "$TEST_PACK"
python "$ROOT/tools/new_decoder.py" zz_release_wizard_test "Release Wizard Test" >/tmp/rflink_new_decoder_test.txt
python "$ROOT/tools/generate_catalog.py" >/tmp/rflink_catalog_wizard_test.txt
python "$ROOT/tools/check_decoder_packs.py" >/tmp/rflink_pack_recheck.txt
rm -rf "$TEST_PACK" "$ROOT/../docs/source/rflink_universal/decoders/zz_release_wizard_test"
python "$ROOT/tools/generate_catalog.py" >/tmp/rflink_catalog_wizard_cleanup.txt
python "$ROOT/tools/check_decoder_packs.py" >/dev/null
echo "New-decoder wizard smoke: PASS"

python "$ROOT/tests/test_hotfix.py"

python "$ROOT/tests/test_board_config.py"
python "$ROOT/tests/test_release_modes.py"
python "$ROOT/tests/test_packfix1_release.py"
python "$ROOT/tests/test_release_rev2_guard.py"
python "$ROOT/tests/test_release_update_regression.py"
python "$ROOT/tests/test_release_radio_guard.py"
python "$ROOT/tests/test_baseline_catalog.py"
python "$ROOT/tests/test_legacy_hf3_profile.py"
python "$ROOT/tests/test_migration_conflicts.py"
python "$ROOT/tests/test_led_integration.py"
python "$ROOT/tests/test_led_ota_radio_regression.py"
python "$ROOT/tests/test_packaging.py"
python "$ROOT/tests/test_radio_optional.py"
python "$ROOT/tests/test_boot_rmt_regression.py"
python "$ROOT/tests/test_rmt_tx_budget.py"
python "$ROOT/tests/test_radio_monitor_config.py"
python "$ROOT/tests/test_decoder_linker.py"
# Chromium regression is a developer-only optional test; never used by Windows
# 00_RFLink_START.bat and never included in the ESPHome bundle.
if [ "${RFLINK_SKIP_BROWSER:-0}" != '1' ] && python -c 'import playwright' >/dev/null 2>&1 && command -v chromium >/dev/null 2>&1; then
  python "$ROOT/tests/test_radio_monitor_browser.py"
  python "$ROOT/tests/test_ux_browser.py"
  python "$ROOT/tests/test_decoder_library_browser.py"
  python "$ROOT/tests/test_library_ux_browser.py"
  python "$ROOT/tests/test_localization_browser.py"
  python "$ROOT/tests/test_somfy_ui_browser.py"
  python "$ROOT/tests/test_itho_ui_browser.py"
else
  echo 'Radio monitor Chromium UI test: SKIPPED (Playwright/Chromium unavailable)'
fi
