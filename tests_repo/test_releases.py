"""No GitHub/network access or ESP-IDF SDK required for these contract tests."""
import hashlib
import io
import json
import os
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
import server
from release_history import ReleaseHistory, normalise_release, version_tuple
from local_profile import LocalProfile


def release(tag, ident, published='2026-11-01T10:00:00Z', prerelease=False, draft=False):
    return {'id':ident,'tag_name':tag,'name':'RFLink '+tag,
            'published_at':published,'prerelease':prerelease,'draft':draft,
            'assets': [], 'body':'Verbeterde ontvangst\n* OTA getest'}


class ReleasesTests(unittest.TestCase):
    def test_release_tag_filter_and_versions(self):
        self.assertEqual(version_tuple('firmware/v0.7.12'),(0,7,12,1))
        self.assertEqual(version_tuple('v0.7.12-rc1'),(0,7,12,0))
        self.assertIsNone(version_tuple('builder/v0.5.0'))
        self.assertIsNone(normalise_release(release('builder-v0.5.0',101)))
        self.assertIsNone(normalise_release(release('firmware/v0.7.4',102,draft=True)))
        self.assertIsNotNone(normalise_release(release('firmware/v0.7.4',102)))

    def test_recent_five_previous_and_newer_visible_in_full_list(self):
        rows=[release('builder-v0.5.0',70),release('firmware/v0.7.9',69)]
        rows.extend(release('firmware/v0.7.'+str(n),n+10) for n in range(8,-1,-1))
        rows.append(release('firmware/v0.7.10-rc1',500,prerelease=True))
        def fake(url, limit):
            if 'releases?per_page' in url:return json.dumps(rows).encode()
            raise AssertionError('unexpected request: '+url)
        h=ReleaseHistory(fetch=fake)
        # Installed 0.7.7: five earlier. A newer v0.7.9 is only in full view.
        recent=h.recent('tester/rflink','v0.7.7')
        self.assertEqual([r['tag'] for r in recent['recent']],
                         ['firmware/v0.7.'+str(n) for n in range(6,1,-1)])
        full=h.page('tester/rflink',1)
        self.assertIn('firmware/v0.7.9',[r['tag'] for r in full['releases']])
        self.assertIn('firmware/v0.7.10-rc1',[r['tag'] for r in full['releases']])
        self.assertNotIn('builder-v0.5.0',[r['tag'] for r in full['releases']])
        self.assertFalse(recent['recent'][0]['prerelease'])

    def test_default_previous_five_uses_latest_firmware(self):
        entries=[release('firmware/v0.8.'+str(n),n+20) for n in range(8,0,-1)]
        h=ReleaseHistory(fetch=lambda url,limit:json.dumps(entries).encode())
        result=h.recent('maker/board')
        self.assertEqual(result['current'],'firmware/v0.8.8')
        self.assertEqual(len(result['recent']),5)
        self.assertEqual(result['recent'][0]['tag'],'firmware/v0.8.7')

    def test_resolve_tag_to_commit_sha_ignores_target_commitish(self):
        observed=[]
        def fake(url,limit):
            observed.append(url)
            if 'releases/tags/' in url:return json.dumps(dict(release('firmware/v0.7.4',17),target_commitish='main')).encode()
            if 'commits/' in url:return json.dumps({'sha':'a'*40}).encode()
            raise AssertionError(url)
        h=ReleaseHistory(fetch=fake,source_fetch=lambda repo,sha:b'ZIP:'+(repo+sha).encode())
        pinned=h.resolve('owner/repo','firmware/v0.7.4',17)
        self.assertEqual(pinned['sha'],'a'*40)
        self.assertEqual(h.source(pinned),b'ZIP:owner/repo'+(b'a'*40))
        self.assertIn('commits/firmware%2Fv0.7.4',observed[-1])
        with self.assertRaises(ValueError):h.resolve('owner/repo','firmware/v0.7.4',999)
        with self.assertRaises(ValueError):h.resolve('owner/repo','../main',17)
        with self.assertRaises(ValueError):h.resolve('owner/repo','builder-v0.3.0',17)

    def test_historical_source_stays_isolated_and_preserves_pinout(self):
        from test_repo import mkzip
        with tempfile.TemporaryDirectory() as folder:
            data=Path(folder)
            project=data/'project';project.mkdir()
            (project/'CMakeLists.txt').write_text('CURRENT SOURCE')
            workspace=data/'build_workspace'
            older=mkzip({'CMakeLists.txt':b'OLD RELEASE CONTENT',
                         'main/CMakeLists.txt':b'HISTORIC MAIN',
                         'secrets.yaml':b'PRIVATE SHOULD NOT IMPORT'})
            calls=[]
            # A build profile must be JSON; use a minimal valid LED-pin
            # fixture, not arbitrary text that bypasses the real contract.
            local_override=json.dumps({'led':{'gpio':48}})
            class FakeProfile:
                def apply_to_build(self, root):
                    calls.append(Path(root))
                    (Path(root)/'boards/rflink_s3_n16r8.json').write_text(local_override)
            with patch.object(server,'PROJECT',project),patch.object(server,'local_profile',FakeProfile()), \
                 patch.object(server.release_history,'source',return_value=older) as source:
                staged=server.prepare_build_workspace(workspace, {'repo':'x/y','sha':'a'*40})
            self.assertEqual((staged/'CMakeLists.txt').read_text(),'OLD RELEASE CONTENT')
            self.assertEqual((staged/'boards/rflink_s3_n16r8.json').read_text(),local_override)
            self.assertEqual((project/'CMakeLists.txt').read_text(),'CURRENT SOURCE')
            self.assertEqual(calls,[workspace])
            source.assert_called_once()

    def test_page_bounds(self):
        h=ReleaseHistory(fetch=lambda *a: b'[]')
        for p in (0,21,-1):
            with self.assertRaises(ValueError):h.page('a/b',p)

    def test_saved_build_manifest_hashes_and_wrong_profile(self):
        with tempfile.TemporaryDirectory() as base:
            data=Path(base); release_root=data/'releases';release_root.mkdir()
            folder=release_root/('b'*12);folder.mkdir()
            sourcefiles={key: name for key,name in [('app_ota','app.bin'),('bootloader','bootloader.bin'),('partition_table','partitions.bin'),('ota_data_initial','ota.bin')]}
            desc={}
            for key,name in sourcefiles.items():
                raw=(key+'-genuine-build-fixture').encode()
                (folder/name).write_bytes(raw)
                desc[key]={'filename':name,'sha256':hashlib.sha256(raw).hexdigest()}
            with zipfile.ZipFile(folder/'RFLink_Radioactive_USB_OTA.zip','w') as z:
                for name in sourcefiles.values():z.write(folder/name,arcname=name)
            board=LocalProfile(data,ROOT/'firmware')
            expected=board._validate(board.current())
            manifest={'chip':'esp32s3','test_only':True,'board_profile_hash':expected,'files':desc,
                      'release_tag':'firmware/v0.7.0','source_commit_sha':'a'*40,
                      'version':'0.7.0-n16r8-b'+expected,'source_fingerprint':'abcdef123456'}
            (folder/'release_manifest.json').write_text(json.dumps(manifest))
            (folder/'build_info.json').write_text(json.dumps({'job_id':'b'*12,'source':'github_release',
                       'release_tag':'firmware/v0.7.0','source_sha':'a'*40,
                       'created_at':'2026-11-06T11:00:00Z'}))
            with patch.object(server,'DATA',data),patch.object(server,'RELEASES',release_root),\
                 patch.object(server,'local_profile',board),patch.object(server,'DEFAULT_SOURCE',data/'none.zip'),\
                 patch.object(server.deployment,'snapshot',lambda:{'status':'idle'}):
                b=server.Builder()
                # A legacy published package with no source commit must not be reused.
                legacy=dict(manifest);legacy.pop('source_commit_sha')
                (folder/'release_manifest.json').write_text(json.dumps(legacy))
                self.assertEqual(b.saved_builds(),[])
                (folder/'release_manifest.json').write_text(json.dumps(manifest))
                self.assertEqual(len(b.saved_builds()),1)
                result=b.activate_build('b'*12)
                self.assertEqual(result['build']['release_tag'],'firmware/v0.7.0')
                self.assertEqual(b.status,'success')
                # Changing the local SPI clock cannot silently reuse old binaries.
                altered=board.current();altered['spi']['clock_hz']=400000;board.save(altered)
                with self.assertRaises(RuntimeError):b.activate_build('b'*12)
                board.reset()
                (folder/'app.bin').write_bytes(b'corrupted')
                self.assertEqual(b.saved_builds(),[])

if __name__=='__main__':unittest.main()
