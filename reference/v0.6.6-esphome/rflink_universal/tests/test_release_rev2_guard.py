"""Regression against accidental removal of rev2 PCB guard from Windows packer.

This checks source guards only. Run 00_RFLink_START.bat (clean release) on Windows to execute the
PowerShell packer and verify actual archive generation.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
ps=(root/'rflink_universal/tools/maak_release.ps1').read_text(encoding='utf-8-sig')
updater=(root/'rflink_universal/tools/update_rflink.ps1').read_text(encoding='utf-8-sig')
yaml=(root/'rflink-universal.yaml').read_text()
labels=('spi_sck','spi_mosi','spi_miso','radio1_csn','radio1_gdo0','radio1_gdo2',
        'radio2_csn','radio2_gdo0','radio2_gdo2', 'radio3_csn','radio3_gdo0','radio3_gdo2','status_led_gpio')
pin_map=('GPIO12','GPIO11','GPIO13','GPIO4','GPIO5','GPIO6',
         'GPIO9','GPIO8','GPIO14','GPIO15','GPIO16','GPIO17','GPIO48')
assert "'eigen_rfid_board_rev2_gpio14'" in ps
assert "'custom_existing'" in ps
assert 'Unknown PCB revision:' in ps
assert "if ($selectedRevision -eq 'eigen_rfid_board_rev2_gpio14')" in ps
section=ps.split("if ($selectedRevision -eq 'eigen_rfid_board_rev2_gpio14')",1)[1].split("if (@($pinValues",1)[0]
assert '$expectedPins = @('+','.join(repr(x) for x in pin_map)+')' in section
assert '($pinValues -join' in section and 'Fail ' in section
assert 'GPIO collision in central PCB pinout' in ps
assert "$expectedBoardRevision = 'eigen_rfid_board_rev2_gpio14'" in updater
assert re.search(r'^  board_revision: eigen_rfid_board_rev2_gpio14$',yaml,re.M)
for key,pin in zip(labels,pin_map):
    assert re.search(r'^  '+key+r': '+pin+r'(?:\s|$)',yaml,re.M), (key,pin)
print('Windows release revision guard: rev2 exact pin map + custom/rev1 compatibility: PASS (static)')
