"""Generate first-boot NVS for an *unflashed* RFLink using official ESP-IDF.

Never write credentials into a source tree or OTA application binary.
The NVS image is unencrypted on this prototype. Treat downloads as secrets.
"""
from __future__ import annotations
import csv
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

NVS_OFFSET = 0x9000
NVS_SIZE = 0x10000

def create_nvs_image(config, output: Path, idf_path: Path):
    ssid, password, key = (config['wifi_ssid'], config['wifi_password'], config['key'])
    if not isinstance(ssid,str) or not 1 <= len(ssid.encode('utf-8')) <= 32:
        raise ValueError('Invalid RFLink Wi-Fi SSID')
    if not isinstance(password,str) or not 8 <= len(password) <= 63:
        raise ValueError('Wi-Fi password must be 8-63 characters')
    if not isinstance(key,str) or not 20 <= len(key) <= 64:
        raise ValueError('OTA administrator key must be 20-64 characters')
    if any(ord(c) < 32 for c in ssid+password+key):
        raise ValueError('Credentials cannot contain control characters')
    tool = Path(idf_path) / 'components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py'
    if not tool.is_file():
        raise RuntimeError('ESP-IDF NVS partition generator not installed')
    output = Path(output)
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='rflink-private-nvs-') as tmpdir:
        folder=Path(tmpdir)
        folder.chmod(0o700)
        csvpath=folder/'input.csv'
        with csvpath.open('w',encoding='utf-8',newline='') as fp:
            writer=csv.writer(fp,lineterminator='\n')
            writer.writerow(('key','type','encoding','value'))
            writer.writerow(('rf_net','namespace','',''))
            writer.writerow(('ssid','data','string',ssid))
            writer.writerow(('pass','data','string',password))
            # Native v0.7 still compares the plaintext OTA key in NVS.
            # Do not pretend a hash is supported by the existing firmware.
            writer.writerow(('update_key','data','string',key))
        csvpath.chmod(0o600)
        tempbin=folder/'nvs.bin'
        result=subprocess.run([os.environ.get('PYTHON','python3'),str(tool),'generate',
                               str(csvpath),str(tempbin),hex(NVS_SIZE)],
                              capture_output=True,text=True,timeout=90,check=False)
        if result.returncode!=0:
            # The subprocess output can contain CSV paths or values: never surface it.
            raise RuntimeError('Official ESP-IDF NVS generation failed; check toolchain installation')
        if not tempbin.is_file() or tempbin.stat().st_size!=NVS_SIZE:
            raise RuntimeError('NVS image size is incorrect')
        data=tempbin.read_bytes()
    output.write_bytes(data)
    output.chmod(0o600)
    return {'filename':output.name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
            'offset':NVS_OFFSET,'confidential':True}
