"""Offline regressions for rate-safe firmware refresh and semantic ordering."""
import json
import sys
import time
import unittest
from pathlib import Path
from urllib.error import HTTPError
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from release_history import ReleaseHistory


def row(n):
    return {'id':n+100,'tag_name':f'firmware/v0.7.0-exp.{n}',
            'name':f'Experimental {n}','published_at':f'2026-11-09T12:{n:02d}:00Z',
            'prerelease':True,'draft':False,'assets':[],'body':''}


class FirmwareReleaseRefreshTests(unittest.TestCase):
    def test_out_of_order_github_release_list_sorts_exp_11_before_9(self):
        calls=[]
        def fetch(url,limit):
            calls.append(url)
            return json.dumps([row(n) for n in (9,8,7,6,5,4,11,10,3)]).encode()
        releases=ReleaseHistory(fetch=fetch)
        entries=releases.page('owner/repo')['releases']
        self.assertEqual([x['build_number'] for x in entries],
                         [11,10,9,8,7,6,5,4,3])
        self.assertEqual(len(calls),1)

    def test_cache_shared_across_clients_and_manual_cooldown(self):
        calls=[]
        def fetch(url,limit):
            calls.append(url)
            return json.dumps([row(11)]).encode()
        releases=ReleaseHistory(fetch=fetch,cache_seconds=900,manual_cooldown=120)
        self.assertFalse(releases.page('owner/repo')['cached'])
        self.assertTrue(releases.page('owner/repo')['cached'])
        cooldown=releases.page('owner/repo',refresh=True)
        self.assertTrue(cooldown['cached'])
        self.assertGreater(cooldown['retry_after_seconds'],0)
        self.assertEqual(len(calls),1)
        # A manual refresh after the server cooldown bypasses the 15m TTL.
        releases.last_attempt[('owner/repo',1)]=time.monotonic()-121
        manual=releases.page('owner/repo',refresh=True)
        self.assertFalse(manual['cached'])
        self.assertEqual(len(calls),2)
        # Another browser tab shares the same cached response.
        releases.page('owner/repo')
        self.assertEqual(len(calls),2)

    def test_304_reuses_saved_page_and_updates_checked_timestamp(self):
        h=ReleaseHistory(fetch=lambda *_:json.dumps([row(11)]).encode())
        first=h.page('owner/repo')
        h.last_attempt[('owner/repo',1)]=time.monotonic()-121
        with patch.object(h,'_release_page_http',return_value=(None,'"abc"',{})) as req:
            fresh=h.page('owner/repo',refresh=True)
        self.assertEqual(first['releases'],fresh['releases'])
        self.assertTrue(fresh['cached'])
        self.assertFalse(fresh['stale'])
        req.assert_called_once()

    def test_github_429_pauses_followup_without_discarding_cached_list(self):
        calls=[0]
        def fetch(url,limit):
            calls[0]+=1
            if calls[0]>1:
                raise HTTPError(url,429,'rate limited',{'Retry-After':'240'},None)
            return json.dumps([row(11)]).encode()
        h=ReleaseHistory(fetch=fetch)
        h.page('owner/repo')
        h.last_attempt[('owner/repo',1)]=time.monotonic()-121
        stale=h.page('owner/repo',refresh=True)
        self.assertTrue(stale['stale'])
        self.assertGreaterEqual(stale['retry_after_seconds'],239)
        self.assertEqual(stale['releases'][0]['build_number'],11)
        h.page('owner/repo',refresh=True)
        self.assertEqual(calls[0],2)  # no repeated call during pause

    def test_controls_present_and_timers_are_tab_scoped(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text()
        js=(ROOT/'rflink_builder/static/firmware_catalog.js').read_text()
        backend=(ROOT/'rflink_builder/server.py').read_text()
        self.assertIn('id="detail-firmware-refresh"',html)
        self.assertIn('id="detail-firmware-refresh-status"',html)
        self.assertIn('FIRMWARE_POLL_MS=15*60*1000',js)
        self.assertIn('firmwareTabVisible()',js)
        self.assertIn('sort(firmwareVersionSort)',js)
        self.assertIn("manual?'&refresh=1':''",js)
        self.assertIn("release_history.page(repo,page,refresh=force)",backend)


if __name__=='__main__':
    unittest.main()
