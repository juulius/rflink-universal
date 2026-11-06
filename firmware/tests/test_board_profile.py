#!/usr/bin/env python3
import copy
import importlib.util
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('g',ROOT/'tools/generate_board_header.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
original=g.load()
assert g.render(original)==g.DEFAULT_HEADER.read_text()
assert len(g.board_hash(original))==8
for modifier in [
    lambda x:x['spi'].__setitem__('clock_hz',6_000_001),
    lambda x:x['spi'].__setitem__('miso',x['spi']['mosi']),
    lambda x:x['radios'][0].__setitem__('gdo2',20),
    lambda x:x['radios'][2].__setitem__('frequency_hz',433920000),
    lambda x:x.__setitem__('mcu','ESP32'),
]:
    p=copy.deepcopy(original);modifier(p)
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        f=Path(d)/'bad.json';f.write_text(json.dumps(p))
        try:g.load(f)
        except g.ProfileError:pass
        else:raise AssertionError('Unsafe board config accepted')
p=copy.deepcopy(original);p['spi']['clock_hz']=1_000_000
with __import__('tempfile').TemporaryDirectory() as d:
    f=Path(d)/'ok.json';f.write_text(json.dumps(p));assert g.load(f)['spi']['clock_hz']==1_000_000
    assert g.board_hash(p)!=g.board_hash(original)
print('PASS board profile JSON validation, unsafe pin/speed rejection, fingerprint change')
