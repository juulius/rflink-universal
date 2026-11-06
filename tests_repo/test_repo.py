import io
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from repo_sync import SourceStore,valid_repo,is_private
from local_profile import LocalProfile
from local_secrets import LocalSecrets
from server import inspect_zip

NEEDED=['CMakeLists.txt','main/CMakeLists.txt','sdkconfig.defaults','partitions.csv',
        'boards/rflink_s3_n16r8.json','tools/preflight.py','tools/package_release.py']

def mkzip(overrides=None):
    d={x:b'example' for x in NEEDED}
    d.update(overrides or {})
    fp=io.BytesIO()
    with zipfile.ZipFile(fp,'w',zipfile.ZIP_DEFLATED) as z:
        for k,v in d.items():z.writestr('fw/'+k,v)
    return fp.getvalue()

class RepoTests(unittest.TestCase):
    def test_github_name_allowlist(self):
        self.assertEqual(valid_repo('someone/rflink'), 'someone/rflink')
        for bad in ('https://github.com/foo/bar','foo//bar','foo/bar/extra','.foo/bar','', 'a/b?x=1'):
            with self.assertRaises(ValueError):valid_repo(bad)

    def test_private_and_local_conflicts(self):
        with tempfile.TemporaryDirectory() as base:
            data=Path(base);proj=data/'project';proj.mkdir()
            first=mkzip({'components/file.cpp':b'old'})
            store=SourceStore(data,proj,inspect_zip)
            p=store.preview(first)
            self.assertEqual(p['counts']['new'],len(NEEDED)+1)
            self.assertEqual(store.apply(p['token'])['applied'],len(NEEDED)+1)
            (proj/'components/file.cpp').write_bytes(b'personal change')
            more=mkzip({'components/file.cpp':b'new','main/CMakeLists.txt':b'changed',
                        'secrets.yaml':b'never committed','drivers/new.cpp':b'add'})
            p=store.preview(more)
            self.assertEqual(p['counts']['conflict'],1)
            self.assertGreaterEqual(p['counts']['updated'],1)
            self.assertEqual(p['counts']['new'],1)
            r=store.apply(p['token'])
            self.assertEqual((proj/'components/file.cpp').read_bytes(),b'personal change')
            self.assertEqual((proj/'main/CMakeLists.txt').read_bytes(),b'changed')
            self.assertEqual((proj/'drivers/new.cpp').read_bytes(),b'add')
            self.assertFalse((proj/'secrets.yaml').exists())
            self.assertTrue(Path(r['backup']).is_dir())
            self.assertTrue((data/'source_baseline.json').is_file())

    def test_source_deletions_not_applied(self):
        with tempfile.TemporaryDirectory() as base:
            d=Path(base);store=SourceStore(d,d/'project',inspect_zip)
            p=store.preview(mkzip({'driver.cpp':b'keep'}));store.apply(p['token'])
            p=store.preview(mkzip())
            self.assertEqual(p['counts']['obsolete'],1)
            store.apply(p['token'])
            self.assertTrue((d/'project/driver.cpp').is_file())

    def test_board_gpio_validation_and_persistence(self):
        with tempfile.TemporaryDirectory() as base:
            local=LocalProfile(Path(base), ROOT/'firmware')
            board=local.current()
            self.assertEqual(board['spi']['clock_hz'],200000)
            board['spi']['clock_hz']=600000
            result=local.save(board)
            self.assertEqual(result['board']['spi']['clock_hz'],600000)
            self.assertTrue(local.file.exists())
            self.assertNotEqual(local.file,ROOT/'firmware/boards/rflink_s3_n16r8.json')
            board['radios'][0]['cs']=board['spi']['sclk']
            with self.assertRaises(ValueError):local.save(board)
            local.reset()
            self.assertEqual(local.current()['spi']['clock_hz'],200000)

    def test_secrets_not_returned_and_separate_from_source(self):
        with tempfile.TemporaryDirectory() as base:
            secrets=LocalSecrets(base)
            s=secrets.save({'wifi_ssid':'TestNet','wifi_password':'supersecret123','ota_key':'a-very-secret-key'})
            self.assertTrue(s['wifi_password_set'])
            self.assertNotIn('supersecret123',repr(s))
            self.assertIn('supersecret123',secrets.file.read_text())
            self.assertTrue(is_private('secrets.yaml'))
            self.assertTrue(is_private('local_settings/a.json'))
            secrets.clear()
            self.assertFalse(secrets.file.exists())

if __name__=='__main__':unittest.main()
