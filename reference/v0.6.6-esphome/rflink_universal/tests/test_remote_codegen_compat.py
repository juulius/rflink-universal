"""Execute the real RFLink to_code channel loop with mocked ESPHome helpers.

2026.9 registers in C++ setup; 2026.10 counts slots via remote_base.add_listener.
Does not replace a full ESPHome build.
"""
import ast
import asyncio
from pathlib import Path
from types import SimpleNamespace

source = Path(__file__).resolve().parents[1] / 'components/rflink_universal/__init__.py'
tree = ast.parse(source.read_text(encoding='utf-8'))
to_code = next(n for n in tree.body if isinstance(n, ast.AsyncFunctionDef) and n.name == 'to_code')
channel_loop = [n for n in to_code.body if isinstance(n, ast.For) and isinstance(n.iter, ast.Call)
                    and isinstance(n.iter.func, ast.Name) and n.iter.func.id == 'enumerate'][-1]
function = ast.AsyncFunctionDef(name='run_channel_loop', args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]), body=[channel_loop], decorator_list=[])
namespace_tree = ast.fix_missing_locations(ast.Module(body=[function], type_ignores=[]))

class MockVar:
    def set_raw_channel(self, *args): return ('set_raw', *args)
    def set_packet_channel(self, *args): return ('set_packet', *args)
    def get_raw_channel_listener(self, i): return ('listener', i)

class MockCG:
    def __init__(self): self.calls = []
    async def get_variable(self, x): return ('receiver', x)
    def add(self, x): self.calls.append(x)

async def test(legacy, async_registration):
    cg = MockCG()
    registrations = []
    remote_base = SimpleNamespace()
    if not legacy:
        if async_registration:
            async def add_listener(receiver, listener): registrations.append((receiver, listener))
        else:
            def add_listener(receiver, listener): registrations.append((receiver, listener))
        remote_base.add_listener = add_listener
    env = dict(inspect=__import__('inspect'), cg=cg, remote_base=remote_base, var=MockVar(),
               config={'channels': [{'mode':'raw', 'receiver_id':'a', 'frequency':433420000,'name':'one'},
                                    {'mode':'packet','frequency':868300000,'name':'two'},
                                    {'mode':'raw','receiver_id':'b', 'frequency':433920000,'name':'three'}]},
               CONF_CHANNELS='channels',CONF_MODE='mode',CONF_RECEIVER_ID='receiver_id',
               CONF_FREQUENCY='frequency',CONF_NAME='name')
    exec(compile(namespace_tree, str(source), 'exec'), env)
    await env['run_channel_loop']()
    assert len(cg.calls) == 3
    assert cg.calls[1][0] == 'set_packet'
    assert cg.calls[0][0] == cg.calls[2][0] == 'set_raw'
    assert registrations == ([] if legacy else [(('receiver', 'a'), ('listener', 0)),
                                                   (('receiver', 'b'), ('listener', 2))])

async def main():
    await test(True, False)
    await test(False, False)
    await test(False, True)
    print('RFLink actual channel codegen loop 2026.9/2026.10 (sync and async helper): PASS')

asyncio.run(main())
