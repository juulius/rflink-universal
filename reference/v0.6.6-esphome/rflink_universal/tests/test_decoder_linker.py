"""Build-route regression for ESPHome's generated decoder packs.

Runs the *actual* _copy_decoder_packs_to_build function with a minimal ESPHome
path helper, then compiles real decoder sources into a static archive.
No firmware toolchain is needed for this host regression.
"""
import ast
import re
import shutil
import subprocess
import sys
import tempfile
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
C = ROOT / 'components/rflink_universal'
PACKS = ['acurite_609txc', 'rubicson_temp', 'fineoffset_wh2']


def run(args, *, expect_success=True):
    p = subprocess.run(args, capture_output=True, text=True)
    if (p.returncode == 0) != expect_success:
        raise AssertionError(f'command exit={p.returncode}; expected_success={expect_success}: {args}\n{p.stdout}\n{p.stderr}')
    return p.stdout.strip()


with tempfile.TemporaryDirectory(prefix='rflink-linker-') as temp:
    tmp = Path(temp)
    src_root = tmp / 'src'
    component = src_root / 'esphome/components/rflink_universal'
    shutil.copytree(C, component)
    generated = src_root / 'rflink_decoders'

    # Execute the actual ESPHome source/anchor generator without requiring the
    # full ESPHome application on the developer machine.
    source = (C / '__init__.py').read_text()
    ast_func = next(n for n in ast.parse(source).body
                    if isinstance(n, ast.FunctionDef) and n.name == '_copy_decoder_packs_to_build')
    module = ast.Module(body=[ast_func], type_ignores=[])
    helpers = types.ModuleType('esphome.helpers')

    def write_file_if_changed(path, content):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_text() != content:
            path.write_text(content)

    def copy_file_if_changed(path, dst):
        dst = Path(dst)
        dst.parent.mkdir(parents=True, exist_ok=True)
        if not dst.exists() or dst.read_bytes() != Path(path).read_bytes():
            shutil.copy2(path, dst)

    helpers.write_file_if_changed = write_file_if_changed
    sys.modules['esphome'] = types.ModuleType('esphome')
    sys.modules['esphome.helpers'] = helpers

    class FakeCore:
        @staticmethod
        def relative_src_path(path):
            return src_root / path

    scope = dict(CORE=FakeCore, Path=Path, re=re, shutil=shutil,
                 walk_files=lambda folder: (p for p in Path(folder).rglob('*') if p.is_file()),
                 copy_file_if_changed=copy_file_if_changed,
                 cv=types.SimpleNamespace(Invalid=ValueError))
    exec(compile(module, '<actual_ESPhome_decoder_generator>', 'exec'), scope)
    copy_packs = scope['_copy_decoder_packs_to_build']
    packs = [(ROOT / 'decoders' / pack, __import__('json').loads((ROOT/'decoders'/pack/'manifest.json').read_text())) for pack in PACKS]
    count = copy_packs(packs)
    assert count == 3, f'generated anchor count {count}'
    header = (generated / 'decoder_anchors_generated.h').read_text()
    classes = []
    for pack, _ in packs:
        matches = re.findall(r'\bRFLINK_REGISTER_DECODER\(\s*([A-Za-z_]\w*)\s*\)',
                             (pack / 'decoder.cpp').read_text())
        classes.extend(matches)
        assert (generated / pack.name / 'decoder.cpp').read_bytes() == (pack / 'decoder.cpp').read_bytes()
    assert len(classes) == count and all(f'register_decoder_once(rflink_anchor_{n}());' in header for n in classes)

    # Build as ESP-IDF does: a source component + a static archive of decoder
    # objects. A real defines.h tells the production registry that anchors
    # MUST be present whenever code generation enabled decoders.
    core_dir = src_root / 'esphome/core'
    core_dir.mkdir(parents=True)
    (core_dir / 'defines.h').write_text('#pragma once\n#define RFLINK_DECODER_EXPECTED_COUNT 3\n')
    main = tmp / 'main.cpp'
    main.write_text('''#include "esphome/components/rflink_universal/decoder_registry.h"
#include <set>
#include <string>
#include <iostream>
using namespace esphome::rflink_universal;
int main() {
    ensure_decoder_packs_linked();
    ensure_decoder_packs_linked(); // must NOT register duplicates
    std::set<std::string> names;
    for (auto *d: decoder_registry()) if (d) names.insert(d->name());
    if (names.count("Acurite609TXC") != 1 || names.count("RubicsonTemp") != 1 ||
        names.count("FineOffsetWH2") != 1 || names.size() != 3 || decoder_registry().size() != 3) return 15;
    std::cout << "Actual ESPHome code generation / archive registration: PASS (3/3)\\n";
}
''')
    objects = []
    for pack in PACKS:
        obj = tmp / f'{pack}.o'
        run(['g++','-std=c++17','-O2','-flto','-ffunction-sections','-fdata-sections',
             '-I',str(src_root),'-c',str(generated / pack / 'decoder.cpp'),'-o',str(obj)])
        objects.append(obj)
    archive = tmp / 'libdecoderpacks.a'
    run(['ar','rcs',str(archive)] + [str(p) for p in objects])
    registry = component / 'decoder_registry.cpp'
    compile_link = lambda lib, outfile: ['g++','-std=c++17','-O2','-flto','-Wl,--gc-sections',
                                         '-I',str(src_root),str(main),str(registry),str(lib),'-o',str(outfile)]
    run(compile_link(archive,tmp/'linked'))
    print(run([str(tmp/'linked')]))

    # A missing generated header must break the compilation, not silently
    # produce a firmware that reports 0 registered decoders.
    (generated / 'decoder_anchors_generated.h').unlink()
    result = subprocess.run(compile_link(archive,tmp/'should_fail'),capture_output=True,text=True)
    assert result.returncode != 0 and 'decoder_anchors_generated.h' in result.stderr, result.stderr
    print('Missing generated header => compilation FAILED as intended: PASS')

    # Disabled/removed decoder packs must not survive in cached src files.
    copy_packs(packs)
    assert (generated / PACKS[2]).exists()
    count = copy_packs(packs[:2])
    assert count == 2 and not (generated / PACKS[2]).exists()
    assert len(re.findall(r'extern "C" .*rflink_anchor_',
                          (generated / 'decoder_anchors_generated.h').read_text())) == 2
    print('Disabled pack and stale build artifacts removed: PASS')

    all_packs = [(folder, __import__('json').loads((folder/'manifest.json').read_text())) for folder in sorted((ROOT / 'decoders').iterdir())
                 if folder.is_dir() and not folder.name.startswith('_')
                 and __import__('json').loads((folder / 'manifest.json').read_text()).get('enabled', True)]
    full_count = copy_packs(all_packs)
    assert len(all_packs) == 9 and full_count == 13
    full_header = (generated / 'decoder_anchors_generated.h').read_text()
    assert full_header.count('register_decoder_once(rflink_anchor_') == full_count
    tx_header=(generated/'tx_encoder_anchors_generated.h').read_text()
    expected_tx=sum(len(re.findall(r'\bRFLINK_REGISTER_TX_ENCODER\(\s*([A-Za-z_]\w*)\s*\)', (folder/'tx_encoder.cpp').read_text())) for folder, _ in all_packs if (folder/'tx_encoder.cpp').is_file())
    assert tx_header.count('register_once(rflink_tx_anchor_') == expected_tx
    assert expected_tx == 7
    assert (generated/'esphome_builtin/tx_encoder.cpp').is_file()
    assert (generated/'somfy_rts_56/tx_encoder.cpp').is_file()
    assert (generated/'itho_rft/tx_encoder.cpp').is_file()
    # A decoder pack may have multiple native .cpp files; copy them all.
    assert (generated/'ramses_ii/codec.cpp').is_file()
    assert all((generated / folder.name / 'decoder.cpp').is_file() for folder, _ in all_packs)
    print('Full shipped build inventory: PASS (9 enabled packs, 13 decoder classes)')

    # The UI must be backed by the actual runtime registry, not an unrelated
    # JSON manifest or compile-time inventory.
    cpp = (C / 'rflink_universal.cpp').read_text()
    assert '"Decoder registration: %u/%u enabled decoders"' in cpp
    assert 'const auto &registry = decoder_registry();' in cpp
    assert 'out << "],\\"count\\":" << registry.size()' in cpp
    print('Runtime count and web endpoint use the same registry: PASS')
