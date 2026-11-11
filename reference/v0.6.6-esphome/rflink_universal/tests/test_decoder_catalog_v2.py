"""Actual schema2 manifests -> generated C++ -> binary, negative validation cases."""
import copy
import importlib.util
import json
import subprocess
import shutil
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
C=ROOT/'components/rflink_universal'
spec=importlib.util.spec_from_file_location('manifest_schema', C/'manifest_schema.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
packs=[]
for folder in sorted((ROOT/'decoders').iterdir()):
    if folder.is_dir() and not folder.name.startswith('_'):
        manifest=m.load_manifest(folder/'manifest.json')
        if manifest.get('enabled',True):packs.append((folder,manifest))
head=m.generate_header(packs)
assert (C/'capability_catalog_generated.h').read_text()==head, 'fallback header stale'
protos, profiles=m.validate_catalog(packs)
assert len(protos)==13 and set(profiles)=={'temperature','humidity','battery_low','rain','fan_speed','fan_mode'}
assert protos['Nexa'][1]['transmit']['encoder']=='nexa'
assert protos['ByronSX'][1]['transmit']['encoder']=='byron_sx'
assert protos['Dooya'][1]['transmit']['encoder']=='dooya'
assert protos['RCSwitch'][1]['transmit']['encoder']=='rc_switch'
assert protos['SomfyRTS56'][1]['transmit']['encoder']=='somfy_rts_56'
assert protos['IthoRFT'][1]['transmit']['encoder']=='itho_rft'
assert protos['RamsesII'][1]['transmit']['encoder']=='ramses_ii'
assert all(x[1]['transmit']['encoder']=='none' for name,x in protos.items() if name not in ('Nexa','ByronSX','Dooya','RCSwitch','SomfyRTS56','IthoRFT','RamsesII'))
assert all(x[1]['transmit']['default_enabled'] is False for x in protos.values())

with tempfile.TemporaryDirectory(prefix='rflink-catalog-') as t:
    t=Path(t)
    # Exactly the location populated by the ESPHome to_code() generator.
    generated=t/'rflink_decoders';generated.mkdir()
    (generated/'capability_catalog_generated.h').write_text(head)
    comp=t/'esphome/components/rflink_universal';comp.parent.mkdir(parents=True)
    shutil.copytree(C, comp)
    core=t/'esphome/core';core.mkdir(parents=True)
    (core/'defines.h').write_text('#pragma once\n#define RFLINK_DECODER_EXPECTED_COUNT 13\n')
    source=t/'test.cpp'
    source.write_text('''#include "esphome/components/rflink_universal/capability_catalog.h"
#include <cassert>
#include <cstring>
using namespace esphome::rflink_universal;
int main(){
 size_t n=0;auto *p=capabilities::protocols(n);assert(p && n==13);
 assert(capabilities::find("Nexa")!=nullptr);
 assert(capabilities::tx_allowed("Nexa","on"));
 assert(capabilities::tx_allowed("ByronSX","command_1"));
 assert(capabilities::tx_allowed("Dooya","button_1"));
 assert(capabilities::tx_allowed("RCSwitch","code"));
 assert(!capabilities::tx_allowed("Dooya","up"));
 assert(!capabilities::tx_allowed("ByronSX","unapproved"));
 assert(!capabilities::tx_allowed("Nexa","toggle"));
 assert(capabilities::tx_allowed("SomfyRTS56","up"));
 assert(capabilities::tx_allowed("SomfyRTS56","prog"));
 assert(!capabilities::tx_allowed("KeeLoq","button_1"));
 assert(!capabilities::tx_allowed("UNKNOWN","on"));
 assert(capabilities::field("temperature")!=nullptr);
 assert(std::strcmp(capabilities::field("temperature")->unit,"°C")==0);
 assert(capabilities::field("rolling_code")==nullptr);
 assert(capabilities::field("code")==nullptr);
}''')
    proc=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(t),str(source),'-o',str(t/'test')],capture_output=True,text=True)
    assert proc.returncode==0,proc.stderr
    assert subprocess.run([str(t/'test')],capture_output=True).returncode==0
    # Corrupt a real manifest, not a mock: compiler must never see an unauthorized encoder.
    old=json.loads((ROOT/'decoders/somfy_rts_56/manifest.json').read_text())
    old['capabilities'][0]['transmit']={'encoder':'somfy','status':'hardware_verified','actions':[{'id':'up','label':'Open','entity':'button'}], 'default_enabled':True}
    path=t/'somfy_rts_56/manifest.json';path.parent.mkdir()
    path.write_text(json.dumps(old))
    try:m.load_manifest(path)
    except ValueError:pass
    else:raise AssertionError('unreviewed Somfy encoder accepted')
    # Duplicate names must be fatal, not silently overwrite capabilities.
    try:m.validate_catalog(packs+[packs[0]])
    except ValueError:pass
    else:raise AssertionError('duplicate decoder protocol accepted')
    old=json.loads((ROOT/'decoders/esphome_builtin/manifest.json').read_text())
    old['capabilities'][0]['transmit']['default_enabled']=True
    path=t/'esphome_builtin/manifest.json';path.parent.mkdir()
    path.write_text(json.dumps(old))
    try:m.load_manifest(path)
    except ValueError:pass
    else:raise AssertionError('default-enabled TX accepted')
    # A malicious/mistaken manifest may not impersonate the only tested encoder.
    old=json.loads((ROOT/'decoders/somfy_rts_56/manifest.json').read_text())
    old['capabilities'][0]['transmit']={
        'encoder':'nexa','status':'host_tested','radio':'433.92',
        'required_fields':['channel','group','level','tx_radio'],
        'actions':[{'id':'on','label':'Aan','entity':'button'},
                   {'id':'off','label':'Uit','entity':'button'}],
        'default_enabled':False}
    old['capabilities'][0]['rx_actions']=['on','off']
    path=t/'somfy_rts_56/manifest.json';path.write_text(json.dumps(old))
    try:m.load_manifest(path)
    except ValueError:pass
    else:raise AssertionError('Nexa encoder accepted for non-Nexa protocol')
print('Decoder API v2: 13 manifest protocols, generated ESPHome C++ catalog, HA fields, safe TX, invalid packs: PASS')
