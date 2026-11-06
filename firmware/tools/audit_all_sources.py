#!/usr/bin/env python3
"""Host-only compile/link-symbol audit; cannot replace Espressif compiler or on-board tests."""
from pathlib import Path
import shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
sources=sorted([*(ROOT/'main').glob('*.cpp'),*(ROOT/'components/rflink_radio_hal').glob('*.cpp'),*(ROOT/'components/rflink_decoder_core/src').glob('*.cpp')])
assert len(sources)>=16, 'Missing production source files'
for directory in (ROOT/'main',ROOT/'components/rflink_radio_hal',ROOT/'components/rflink_decoder_core/src'):
    cmake=(directory.parent if directory.name=='src' else directory)/'CMakeLists.txt'
    registered=cmake.read_text()
    for file in directory.glob('*.cpp'):
        assert file.name in registered, f'Source not registered with ESP-IDF: {file}'
compiler=shutil.which('g++')
assert compiler, 'g++ missing (host-only audit)'
incs=[ROOT/'tests/host_idf_stubs',ROOT/'main',ROOT/'components/rflink_radio_hal/include',ROOT/'components/rflink_decoder_core/include']
with tempfile.TemporaryDirectory(prefix='rflink-idf-stub-audit-') as temp:
    objs=[]
    for source in sources:
        obj=Path(temp)/(source.stem+'.o')
        command=[compiler,'-std=c++17','-O0','-c','-pthread',*('-I'+str(p) for p in incs),str(source),'-o',str(obj)]
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        if result.returncode:
            raise RuntimeError(f"Host stub compilation failed for {source.name}:\\n{result.stderr}")
        objs.append(obj)
    defined=set();undefined=set()
    for obj in objs:
        result=subprocess.run(['nm',str(obj)],check=True,capture_output=True,text=True)
        for raw in result.stdout.splitlines():
            parts=raw.split()
            if len(parts)==2 and parts[0]=='U':undefined.add(parts[1])
            elif len(parts)==3 and parts[1].isupper() and parts[1]!='U':defined.add(parts[2])
    # Only detect user component symbols; IDF stubs intentionally have unresolved C symbols.
    missing=sorted(sym for sym in undefined if sym.startswith('_ZN6rflink') and sym not in defined)
    assert not missing, 'Unlinked RFLink symbols: '+str(missing)
print(f'PASS full native host-stub audit: {len(sources)} translation units, source registration, no missing RFLink symbols; NOT an ESP-IDF build')
