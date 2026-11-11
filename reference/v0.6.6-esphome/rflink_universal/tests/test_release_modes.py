"""Static safety contract for Windows release/update scripts; run 00_RFLink_START.bat and choose developer tests on Windows too."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
bat = (ROOT/'00_RFLink_START.bat').read_text()
upbat = bat
release = (ROOT/'rflink_universal/tools/maak_release.ps1').read_text()
updater = (ROOT/'rflink_universal/tools/update_rflink.ps1').read_text()
yaml = (ROOT/'rflink-universal.yaml').read_text()
assert 'MAAK_RELEASE.bat update' not in bat
assert 'rflink_universal\\tools\\maak_release.ps1' in bat and 'rflink_universal\\tools\\update_rflink.ps1' in bat
assert 'rflink_universal\\tools\\test_update_release.ps1' in bat
assert [x.name for x in ROOT.glob('*.bat')] == ['00_RFLink_START.bat']
assert '-UseUpdaterOutput' not in bat
assert 'update_output ignored' in release and "'_DEFAULT'" in release and "'_CUSTOM'" in release
assert 'if ($UseUpdaterOutput) {' in release and 'if (-not $selectedUpdate) { Fail' in release
assert '$m.schema -ne 2' in release
assert 'Rev1 board label' in release
assert 'Rev2 GPIO14 board label' in release
assert "'eigen_rfid_board_rev2_gpio14'" in release
assert "'GPIO9','GPIO8','GPIO14','GPIO15','GPIO16','GPIO17','GPIO48'" in release
assert "$expectedBoardRevision = 'eigen_rfid_board_rev2_gpio14'" in updater
assert 'hardware_source' in updater and 'board_revision' in updater
assert 'Choose-Hardware $parsed[1] $parsed[2]' in updater
assert "Read-YN 'GPIO-pinout uit nieuwe release gebruiken (zelfde PCB-revisie)?' $true" in updater
assert "Read-YN 'Past de PCB die je echt gebruikt bij de pinout van deze release?' $null" in updater
assert "if ($Mode -eq 'Automatic' -or $NonInteractive)" in updater
assert 'Assert-PinTable $old' in updater and 'Assert-PinTable $new' in updater
assert "if ($k -eq 'board_revision')" in updater
assert 'PRESERVE_PERSONAL' in updater
assert 'Resolve-KnownBaseline' in updater and 'verified release-specific baseline' in updater
assert 'Executable-Lines' in updater and 'KNOWN_LEGACY_TX' in updater
assert 'EXPLICIT_FIRMWARE_OVERRIDE' in updater and 'INPUT SHA256:' in updater
assert 'Afwijkende firmware in ${label} vervangen' in updater
assert "if ($baseRelease -cne $oldRelease)" in updater
assert 'Baseline selection:' in updater
assert "& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $builder -ProjectRoot $root -UseUpdaterOutput" in updater
assert 'rflink_universal/options/ha_mqtt.yaml' in release and 'rflink_universal/components/rflink_universal/ha_discovery.h' in release
assert 'rflink_universal/options/ha_mqtt.yaml' in updater and 'rflink_universal/components/rflink_universal/ha_discovery.h' in updater
assert 'DECODER_REVIEW' in updater and 'custom decoder' in updater
assert 'Automatic' in upbat and 'Advanced' in upbat
assert re.search(r'^  board_revision: eigen_rfid_board_rev2_gpio14$', yaml, re.M)
expected={'radio1_csn':'GPIO4','radio1_gdo0':'GPIO5','radio1_gdo2':'GPIO6',
          'radio2_csn':'GPIO9','radio2_gdo0':'GPIO8','radio2_gdo2':'GPIO14',
          'radio3_csn':'GPIO15','radio3_gdo0':'GPIO16','radio3_gdo2':'GPIO17',
          'status_led_gpio':'GPIO48'}
for key, gpio in expected.items():
    assert re.search(r'^  '+key+r': '+gpio+r'(?:\s|$)', yaml, re.M),key
print('Update Wizard: hardware profile, tri-state GPIO defaults, auto fail-closed, identity, decoder gate, direct bundle, isolated DEFAULT: PASS (static)')
