#!/usr/bin/env python3
"""RFLink RC4 ESPHome WebHandler link fix, without replacing other RC4 source files.

Usage: python3 apply_hotfix.py /config/esphome/rflink_universal
Uses only the Python standard library. This makes backups before editing anything.
"""
from datetime import datetime
from pathlib import Path
import shutil
import sys

MARKER = '// RFLINK_WEBHANDLER_LINK_HOTFIX_20260918'


def main() -> int:
    if len(sys.argv) != 2:
        print('Gebruik: python3 apply_hotfix.py /config/esphome/rflink_universal')
        return 2
    root = Path(sys.argv[1]).expanduser().resolve()
    component = root / 'components' / 'rflink_universal'
    if not component.is_dir():
        component = root / 'rflink_universal' if (root / 'rflink_universal' / 'rflink_universal.cpp').is_file() else component
    core = component / 'rflink_universal.cpp'
    web = component / 'rflink_web.cpp'
    if not core.is_file():
        print(f'FOUT: hoofdbron niet gevonden: {core}')
        return 2
    original = core.read_text(encoding='utf-8')
    if MARKER in original:
        print('Reeds gepatcht. Geen bestanden gewijzigd.')
        return 0
    if not web.is_file():
        print(f'FOUT: webimplementatie ontbreekt: {web}. Niet verder aanpassen.')
        return 2
    implementation = web.read_text(encoding='utf-8')
    if ('RFLinkWebHandler::canHandle' not in implementation
            or 'RFLinkWebHandler::handleRequest' not in implementation
            or 'RFLinkWebHandler::' not in original + implementation):
        print('FOUT: onverwachte RC4-webhandlerstructuur. Niet verder aanpassen.')
        return 2
    if 'new RFLinkWebHandler' not in original:
        print('FOUT: hoofdcomponent registreert webhandler niet zoals verwacht.')
        return 2
    if not original.rstrip().endswith('}  // namespace esphome'):
        print('FOUT: onverwacht einde hoofdbron. Niet verder aanpassen.')
        return 2
    suffix = datetime.now().strftime('%Y%m%d_%H%M%S')
    core_bak = core.with_name(f'{core.name}.backup_{suffix}')
    web_bak = web.with_name(f'{web.name}.backup_{suffix}')
    shutil.copy2(core, core_bak)
    shutil.copy2(web, web_bak)
    # Keep both implementations inside the guaranteed-to-build main translation unit.
    # Duplicate linked symbols are prevented by removing the separate .cpp after backup.
    merged = (original.rstrip() + '\n\n' + MARKER + '\n' + implementation + '\n')
    tmp = core.with_name(core.name + '.hotfix_tmp')
    try:
        tmp.write_text(merged, encoding='utf-8')
        tmp.replace(core)
        web.unlink()
    except OSError as error:
        shutil.copy2(core_bak, core)
        shutil.copy2(web_bak, web)
        if tmp.exists():
            tmp.unlink()
        print(f'FOUT: wijziging teruggedraaid: {error}')
        return 1
    print('OK: RFLinkWebHandler is opgenomen in rflink_universal.cpp.')
    print('OK: apart rflink_web.cpp is verwijderd om dubbele symbolen te voorkomen.')
    print(f'Back-ups: {core_bak.name} en {web_bak.name}')
    print('Volgende stap: ESPHome -> Clean Build Files -> Install / Compile.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
