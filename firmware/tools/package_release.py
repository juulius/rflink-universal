#!/usr/bin/env python3
"""Package an already built ESP-IDF firmware, never fabricate a .bin."""
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path
from preflight import check
from source_manifest import verify
from generate_board_header import load as load_board, board_hash
ROOT=Path(__file__).resolve().parents[1]
def package():
    check()
    source_snapshot=verify()
    build=ROOT/'build'
    # Verify the complete IDF build produced the board-specific files.
    # CMake/ESP-IDF writes project_description.json as part of a configured build.
    project=build/'project_description.json'
    assert project.is_file(), 'Missing official IDF project_description.json: no full build'
    desc=json.loads(project.read_text())
    assert desc.get('target')=='esp32s3', 'Wrong build target'
    assert desc.get('project_name')=='rflink_universal', 'Wrong project name'
    expected_version='0.7.0-n16r8-s20-b'+board_hash(load_board())
    assert expected_version in (desc.get('project_version'), desc.get('version')), 'Built version does not match board profile'
    config=build/'config/sdkconfig.h'
    assert config.is_file(), 'Missing generated IDF sdkconfig.h: no full build'
    content=config.read_text()
    assert '#define CONFIG_IDF_TARGET_ESP32S3 1' in content, 'Binary target not confirmed ESP32-S3'
    assert '#define CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 1' in content, 'Rollback disabled in generated config'
    assert '#define CONFIG_SPIRAM_MODE_OCT 1' in content, 'Octal PSRAM not confirmed'
    source={
        'app_ota':build/'rflink_universal.bin',
        'bootloader':build/'bootloader/bootloader.bin',
        'partition_table':build/'partition_table/partition-table.bin',
        'ota_data_initial':build/'ota_data_initial.bin',
    }
    # IDF generates ota_data_initial.bin for ota_0 initial USB flashing.
    for name,file in source.items():
        assert file.is_file() and file.stat().st_size>=16, f'Missing built {name}: {file}'
    assert source['app_ota'].stat().st_size<0x400000,'Firmware does not fit 4 MiB OTA slot'
    assert source['app_ota'].read_bytes()[:1]==b'\xe9','Invalid ESP32 firmware magic'
    release=ROOT/'release'
    release.mkdir(exist_ok=True)
    names={
        'app_ota':'RFLink_Radioactive_v0.7.0_STAGE20_OTA.bin',
        'bootloader':'bootloader.bin',
        'partition_table':'partition-table.bin',
        'ota_data_initial':'ota_data_initial.bin',
    }
    files={}
    for key,src in source.items():
        dst=release/names[key]
        shutil.copy2(src,dst)
        files[key]={
            'filename':dst.name,
            'size':dst.stat().st_size,
            'sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),
        }
    manifest={'version':expected_version, 'board_profile_hash':board_hash(load_board()), 'board_id':load_board()['board_id'], 'chip':'esp32s3',
        'hardware':'ESP32-S3-WROOM-1-N16R8', 'test_only':True,
        'source_fingerprint':source_snapshot['source_fingerprint'],
        'full_build_required':True,'flash_size_bytes':16*1024*1024,
        'ota_slot_size_bytes':0x400000, 'partition_table_offset':0x8000,
        'bootloader_offset':0, 'ota_0_offset':0x20000,
        'otadata_offset':0x19000,'files':files}
    (release/'release_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (release/'LEES_EERST.txt').write_text(
        'ONTWIKKELVERSIE STAGE20. Alleen geschikt voor een APARTE TEST-ESP32-S3 N16R8.\n'
        'Niet flashen over de werkende v0.6.6: nieuwe partitietabel; bestaande data gaat verloren.\n'
        'Voor USB-installatie zijn bootloader, partition-table, ota-data initial en app vereist.\n'
        'Gebruik de door ESP-IDF gegenereerde flash_args/ flasher_args.json voor exacte offsets.\n'
        'OTA upload gebruikt alleen de OTA app .bin; nooit het volledige USB-pakket.\n')
    print('PASS: ESP-IDF binaries verified and packaged with SHA256 in release/')
if __name__=='__main__':
    try: package()
    except (AssertionError,ValueError) as exc:
        print(f'FAIL release packaging: {exc}', file=sys.stderr)
        sys.exit(1)
