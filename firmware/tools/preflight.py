#!/usr/bin/env python3
"""Strict offline preflight for the ESP32-S3-WROOM-1-N16R8 test build."""
import csv
import json
import re
import sys
from pathlib import Path
from generate_board_header import load as load_board, render as render_board, DEFAULT_HEADER

ROOT=Path(__file__).resolve().parents[1]
SIZE=16*1024*1024
EXPECTED={
    'nvs':(0x9000,0x10000,'data','nvs'),
    'otadata':(0x19000,0x2000,'data','ota'),
    'phy_init':(0x1B000,0x1000,'data','phy'),
    'ota_0':(0x20000,0x400000,'app','ota_0'),
    'ota_1':(0x420000,0x400000,'app','ota_1'),
    'rf_storage':(0x820000,0x200000,'data','0x40'),
    'coredump':(0xA20000,0x10000,'data','coredump'),
}
REQUIRED_CONFIG={
    'CONFIG_IDF_TARGET':'"esp32s3"',
    'CONFIG_ESPTOOLPY_FLASHSIZE_16MB':'y',
    'CONFIG_ESPTOOLPY_FLASHMODE_QIO':'y',
    'CONFIG_SPIRAM':'y',
    'CONFIG_SPIRAM_MODE_OCT':'y',
    'CONFIG_PARTITION_TABLE_CUSTOM':'y',
    'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME':'"partitions.csv"',
    'CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE':'y',
}
def check():
    cmake=(ROOT/'CMakeLists.txt').read_text()
    assert 'project(rflink_universal)' in cmake
    assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in cmake
    board=load_board()
    assert DEFAULT_HEADER.read_text(encoding='utf-8')==render_board(board), 'Regenerate board header before building'
    cfg=dict(re.findall(r'^(CONFIG_[A-Z0-9_]+)=(.+)$',(ROOT/'sdkconfig.defaults').read_text(),re.M))
    for key, value in REQUIRED_CONFIG.items():
        assert cfg.get(key)==value, f'sdkconfig.defaults mismatch: {key}={cfg.get(key)!r}, expected {value!r}'
    # A cached sdkconfig can override sdkconfig.defaults; reject incompatible config.
    existing=ROOT/'sdkconfig'
    if existing.exists():
        active=dict(re.findall(r'^(CONFIG_[A-Z0-9_]+)=(.+)$',existing.read_text(),re.M))
        for key,value in REQUIRED_CONFIG.items():
            assert active.get(key)==value, f'Active sdkconfig mismatch ({key}); remove sdkconfig and rebuild after backing it up'
    actual={}
    with (ROOT/'partitions.csv').open(newline='') as fp:
        for row in csv.reader(line for line in fp if line.strip() and not line.lstrip().startswith('#')):
            assert len(row)>=5, f'Bad CSV row: {row}'
            name,typ,subtype,offset,size=(v.strip() for v in row[:5])
            assert name not in actual, f'Duplicate partition name {name}'
            actual[name]=(int(offset,0),int(size,0),typ,subtype)
    assert actual==EXPECTED, f'Partition layout mismatch: {actual!r}'
    spans=sorted((offset,offset+length,name) for name,(offset,length,_,_) in actual.items())
    for start,end,name in spans:
        assert start>=0x9000 and end<=SIZE and start<end, f'Partition out of flash: {name}'
    for (_,end,name),(start,_,next_name) in zip(spans,spans[1:]):
        assert end<=start, f'Overlapping partitions {name} and {next_name}'
    for name,(off,_,typ,_) in actual.items():
        assert off % (0x10000 if typ=='app' else 0x1000)==0, f'Partition alignment: {name}'
    # Keep a deliberate maintenance/test build boundary: no HTTP TX API.
    web=(ROOT/'main/rflink_web_ota.cpp').read_text()
    assert '"/api/ota"' in web and '"/api/status"' in web
    assert not re.search(r'\.uri\s*=\s*"/api/(?:tx|send|transmit)"',web)
    print('PASS preflight: N16R8 board JSON+header, unique safe GPIOs, 16MB partitions, OTA, no public TX API')
if __name__=='__main__':
    try: check()
    except (AssertionError,ValueError) as e:
        print(f'FAIL offline preflight: {e}',file=sys.stderr)
        sys.exit(1)
