"""RFLink Builder ccache: bounded disk use, safe fallback and real measured stats."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from compiler_cache import CACHE_SIZE_MIB,setup_cache,cache_counters,cache_delta

class CompilerCacheTests(unittest.TestCase):
    def test_cache_stays_outside_disposable_workspace_and_is_bounded(self):
        with tempfile.TemporaryDirectory() as tmp, patch(
            'compiler_cache.shutil.which',return_value='/usr/bin/ccache'
        ) as which:
            root=Path(tmp)
            env,info=setup_cache(root,{'PATH':'/usr/bin','IDF_CCACHE_ENABLE':'0'})
            which.assert_called_once()
            self.assertTrue(info['enabled'])
            self.assertEqual(CACHE_SIZE_MIB,384)
            self.assertEqual(env['IDF_CCACHE_ENABLE'],'1')
            self.assertEqual(env['CCACHE_MAXSIZE'],'384Mi')
            self.assertEqual(env['CCACHE_COMPRESS'],'1')
            self.assertEqual(env['CCACHE_DIR'],str(root/'compiler_cache'))
            self.assertNotIn('/build_workspace',env['CCACHE_DIR'])
            self.assertTrue((root/'compiler_cache').is_dir())
            self.assertEqual((root/'compiler_cache').stat().st_mode&0o777,0o700)
            self.assertEqual(env['PATH'],'/usr/bin')

    def test_unavailable_or_disabled_cache_keeps_correct_plain_compilation(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch('compiler_cache.shutil.which',return_value=None):
                env,diag=setup_cache(Path(tmp),{'PATH':'/usr/bin','IDF_CCACHE_ENABLE':'1'})
                self.assertFalse(diag['enabled'])
                self.assertEqual(env['IDF_CCACHE_ENABLE'],'0')
            with patch('compiler_cache.shutil.which',return_value='/usr/bin/ccache'):
                env,diag=setup_cache(Path(tmp),{'PATH':'/usr/bin','RFLINK_DISABLE_CCACHE':'1'})
                self.assertFalse(diag['enabled'])
                self.assertEqual(env['IDF_CCACHE_ENABLE'],'0')

    def test_cache_symlink_rejected_and_fallback_used(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            outside=root/'elsewhere'
            outside.mkdir()
            (root/'compiler_cache').symlink_to(outside,target_is_directory=True)
            with patch('compiler_cache.shutil.which',return_value='/usr/bin/ccache'):
                env,diag=setup_cache(root,{'PATH':'/usr/bin'})
            self.assertFalse(diag['enabled'])
            self.assertEqual(env['IDF_CCACHE_ENABLE'],'0')

    def test_cache_statistics_are_actual_per_build_delta(self):
        before={'direct_cache_hit':3,'preprocessed_cache_hit':2,'cache_miss':19}
        after={'direct_cache_hit':113,'preprocessed_cache_hit':12,'cache_miss':59}
        result=cache_delta(before,after)
        self.assertEqual(result['hits'],120)
        self.assertEqual(result['misses'],40)
        self.assertEqual(result['requests'],160)
        self.assertEqual(result['hit_rate_percent'],75.0)
        self.assertEqual(cache_delta({},after),{})
        no_requests=cache_delta(before,before)
        self.assertIsNone(no_requests['hit_rate_percent'])

    def test_read_machine_stats_only_when_cache_enabled(self):
        values={'direct_cache_hit':6,'preprocessed_cache_hit':2,'cache_miss':3}
        with patch('compiler_cache.subprocess.run',return_value=SimpleNamespace(
            returncode=0,stdout='direct_cache_hit\t6\npreprocessed_cache_hit\t2\ncache_miss\t3\n')
        ) as run:
            result=cache_counters({'IDF_CCACHE_ENABLE':'1','CCACHE_DIR':'/data/ccache'})
        self.assertEqual(result,values)
        self.assertEqual(run.call_args.args[0],
                         ['ccache','--print-stats'])
        with patch('compiler_cache.subprocess.run') as run:
            self.assertEqual(cache_counters({'IDF_CCACHE_ENABLE':'0'}),{})
            run.assert_not_called()

    def test_real_phases_are_reported_without_fake_percent_progress(self):
        server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        html=(ROOT/'rflink_builder/static/index.html').read_text('utf-8')
        js=(ROOT/'rflink_builder/static/device_firmware.js').read_text('utf-8')
        self.assertIn('env,cache_details=setup_cache(DATA,env)',server)
        self.assertIn("'phase_history': list(self.phase_history)",server)
        self.assertIn("'phase_elapsed_seconds':",server)
        for phase in ('Board profile','Preflight checks','Source verification',
                      'ESP-IDF compilation','Firmware packaging','Saving device package',
                      'Cleaning workspace'):
            self.assertIn(phase,server)
        for ident in ('firmware-build-active-phase','firmware-build-stage-timings',
                      'firmware-build-cache-summary'):
            self.assertIn('id="'+ident+'"',html)
        self.assertIn('renderFirmwareBuildPhases(s)',js)
        self.assertNotIn('percentComplete',js)

if __name__=='__main__':
    unittest.main()
