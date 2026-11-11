"""Compile generated catalog headers in the same filesystem layout ESPHome uses.

ESPHome writes these catalogues into src/rflink_decoders, *not* the custom
component's directory. Both generated headers must resolve their policy
headers from src/esphome/components/rflink_universal without -I magic.
"""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMP = ROOT / 'components' / 'rflink_universal'
PACKS = [(d, json.loads((d/'manifest.json').read_text()))
         for d in sorted((ROOT/'decoders').iterdir())
         if not d.name.startswith('_') and (d/'manifest.json').is_file()
         and json.loads((d/'manifest.json').read_text()).get('enabled',True)]

import sys
sys.path.insert(0, str(COMP))
from radio_transport_manifest import generate_radio_header
from radio_contract_v2 import generate_contract_header

assert generate_radio_header(PACKS) == (COMP/'radio_transport_catalog_generated.h').read_text()
assert generate_contract_header(PACKS) == (COMP/'radio_v2_catalog_generated.h').read_text()

with tempfile.TemporaryDirectory(prefix='rflink-esphome-include-') as temp:
    src = Path(temp)/'src'
    gen = src/'rflink_decoders'
    component = src/'esphome'/'components'/'rflink_universal'
    gen.mkdir(parents=True)
    component.mkdir(parents=True)
    for fname in ['radio_transport_policy.h','radio_contract_v2.h','decoder.h']:
        shutil.copy2(COMP/fname, component/fname)
    (gen/'radio_transport_catalog_generated.h').write_text(generate_radio_header(PACKS))
    (gen/'radio_v2_catalog_generated.h').write_text(generate_contract_header(PACKS))
    code = '''
#include "rflink_decoders/radio_transport_catalog_generated.h"
#include "rflink_decoders/radio_v2_catalog_generated.h"
int main() {
  using namespace esphome::rflink_universal;
  auto policy = radio_catalog::policy();
  size_t n=0;
  auto specs=radio_v2_catalog::specs(n);
  return (policy.find("IthoRFT") && specs && n>=13) ? 0 : 1;
}
'''
    main = src/'main.cpp'
    main.write_text(code)
    cpp = shutil.which('g++') or shutil.which('clang++')
    assert cpp, 'C++ compiler required to validate generated includes'
    subprocess.run([cpp, '-std=c++17','-Wall','-Wextra','-Werror',
                    '-I'+str(src), str(main),'-o',str(src/'check')], check=True)
    subprocess.run([str(src/'check')],check=True)
print('ESPHome-like generated header compilation: PASS')
