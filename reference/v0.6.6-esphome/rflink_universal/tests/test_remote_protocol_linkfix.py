"""Regression for ESPHome 2026.10 link-only remote_base protocols.

Exercises the actual registration branch extracted from RFLink's to_code(),
against legacy and current ESPHome mock APIs. Does not emulate full IDF link.
"""
import ast
from pathlib import Path
from types import SimpleNamespace

component = Path(__file__).resolve().parents[1] / 'components/rflink_universal/__init__.py'
source = component.read_text(encoding='utf-8')
tree = ast.parse(source)
to_code = next(x for x in tree.body if isinstance(x, ast.AsyncFunctionDef) and x.name == 'to_code')
registration = next(x for x in to_code.body
                    if isinstance(x, ast.If) and isinstance(x.test, ast.Call)
                    and isinstance(x.test.func, ast.Name) and x.test.func.id == 'any')
body = ast.fix_missing_locations(ast.Module(body=[registration], type_ignores=[]))

expected = ['nexa', 'byronsx', 'dooya', 'keeloq', 'rc_switch']

def run(enabled=True, recent=True):
    calls = []
    remote_base = SimpleNamespace()
    if recent:
        remote_base.request_protocol = calls.append
    packs = [(SimpleNamespace(name='esphome_builtin'), {})] if enabled else [
        (SimpleNamespace(name='itho_rft'), {})]
    exec(compile(body, str(component), 'exec'), {'packs': packs, 'remote_base': remote_base})
    return calls

assert run(True, True) == expected, 'all C++ bundled protocols must be linked'
assert run(False, True) == [], 'disabled builtin pack must not force protocols'
assert run(True, False) == [], 'ESPHome 2026.9 has no request_protocol API'

base = Path(__file__).resolve().parents[1] / 'decoders/esphome_builtin'
headers = (base/'decoder.cpp').read_text() + (base/'tx_encoder.cpp').read_text()
for p in expected:
    assert f'remote_base/{p}_protocol.h' in headers, p
print('ESPhome 2026.9/2026.10 protocol source registration and decoder dependencies: PASS')
