"""Persistent local board overrides, not part of the GitHub source tree."""
import importlib.util
import json
import os
from pathlib import Path

class LocalProfile:
    def __init__(self,data,project):
        self.root=Path(data)/'local'
        self.project=Path(project)
        self.file=self.root/'board_profile.json'

    def defaults(self):
        return json.loads((self.project/'boards/rflink_s3_n16r8.json').read_text('utf-8'))

    def current(self):
        board=json.loads(self.file.read_text('utf-8')) if self.file.exists() else self.defaults()
        # Older Builder profiles omitted the onboard LED pin. Present the
        # firmware's original GPIO48 default without modifying saved bytes.
        if isinstance(board,dict) and 'led' not in board:
            board['led']={'gpio':48}
        return board

    def _validate(self,board):
        if not isinstance(board,dict): raise ValueError('Invalid board profile')
        baseline=self.defaults()
        if board.get('board_id')!=baseline['board_id'] or board.get('mcu')!=baseline['mcu']:
            raise ValueError('Board identity cannot be changed from the website')
        if [(r.get('id'),r.get('frequency_hz'),r.get('modulation')) for r in board.get('radios',[])] != [
           (r['id'],r['frequency_hz'],r['modulation']) for r in baseline['radios']]:
            raise ValueError('Radio identities and frequencies are not GPIO settings')
        # The selected development source may be older than this Builder,
        # so validate the LED pin independently before passing it onward.
        led=board.get('led',{'gpio':48})
        if not isinstance(led,dict) or set(led)!={'gpio'} or type(led.get('gpio')) is not int:
            raise ValueError('LED GPIO must be a single integer pin')
        pins=[board.get('spi',{}).get(k) for k in ('sclk','mosi','miso')]
        pins += [r.get(k) for r in board.get('radios',[]) for k in ('cs','gdo0','gdo2')]
        if led['gpio'] not in {1,2,*range(4,19),21,*range(38,43),47,48}:
            raise ValueError('LED GPIO is unavailable/reserved for this ESP32-S3 N16R8')
        if led['gpio'] in pins:
            raise ValueError(f'LED GPIO {led["gpio"]} conflicts with a SPI or CC1101 pin')
        # Use the actual firmware generator for the whitelist and pin collision checks.
        script=self.project/'tools/generate_board_header.py'
        if led['gpio']!=48 and 'kStatusLedGpio' not in script.read_text('utf-8'):
            raise ValueError('This installed RFLink source does not support a configurable LED GPIO. Update the firmware source before changing GPIO48.')
        spec=importlib.util.spec_from_file_location('rf_board_validate',script)
        mod=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        self.root.mkdir(parents=True,exist_ok=True)
        testfile=self.root/'board_check.tmp'
        try:
            testfile.write_text(json.dumps(board),encoding='utf-8')
            mod.load(testfile)
            return mod.board_hash(board)
        finally:testfile.unlink(missing_ok=True)

    def save(self,board):
        digest=self._validate(board)
        self.root.mkdir(parents=True,exist_ok=True)
        temp=self.file.with_suffix('.tmp')
        temp.write_text(json.dumps(board,indent=2)+'\n',encoding='utf-8')
        temp.chmod(0o600)
        temp.replace(self.file)
        return {'ok':True,'hash':digest,'board':board,'note':'Saved locally. A changed hash may require USB migration.'}

    def apply_to_build(self, workspace=None):
        board=self.current()
        digest=self._validate(board)
        dest=(Path(workspace) if workspace else self.project)/'boards/rflink_s3_n16r8.json'
        # build workspace is private (/data), not the GitHub repository
        dest.write_text(json.dumps(board,indent=2)+'\n',encoding='utf-8')
        return digest

    def reset(self):
        self.file.unlink(missing_ok=True)
        return {'ok':True,'board':self.defaults()}
