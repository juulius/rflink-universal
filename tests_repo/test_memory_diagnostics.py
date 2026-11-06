"""Host-side memory monitoring tests; no ESP32 hardware or compiler needed."""
from pathlib import Path
from unittest.mock import patch
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
import memory_diagnostics
from memory_diagnostics import MemoryMonitor


def fake(used, rss=25*1024*1024):
    return {
        'source':'cgroup_v2','container_bytes':used,
        'container_peak_bytes':used, 'container_limit_bytes':1024*1024*1024,
        'python_rss_bytes':rss, 'cache_bytes':used//4,
        'host_total_bytes':8*1024*1024*1024,
        'percent_of_host':round(100*used/(8*1024*1024*1024),1),
        'percent_of_limit':round(100*used/(1024*1024*1024),1),
    }


class MemoryDiagnosticsTests(unittest.TestCase):
    def test_no_expensive_dependencies_or_background_processes(self):
        source=(ROOT/'rflink_builder/memory_diagnostics.py').read_text('utf-8')
        self.assertNotIn('import psutil',source)
        self.assertNotRegex(source,r'(?m)^\\s*(?:from\\s+subprocess\\b|import\\s+subprocess\\b)')
        self.assertNotIn('Popen(',source)
        self.assertNotIn('Thread(',source)

    def test_idle_build_peak_and_postbuild_comparison(self):
        monitor=MemoryMonitor()
        mb=1024*1024
        monitor.sample('idle',fake(100*mb),now=0)
        with patch.object(memory_diagnostics,'read_memory',return_value=fake(115*mb)):
            monitor.start_build('sample000001')
        self.assertEqual(monitor.snapshot()['last_build']['before']['container_bytes'],115*mb)
        monitor.sample('building',fake(240*mb),now=1)
        monitor.sample('building',fake(380*mb),now=5)
        monitor.sample('building',fake(330*mb),now=7)
        with patch.object(memory_diagnostics,'read_memory',return_value=fake(150*mb)):
            with patch.object(memory_diagnostics.time,'monotonic',return_value=10):
                monitor.finish_build('sample000001')
        report=monitor.snapshot()
        self.assertEqual(report['last_build']['peak_bytes'],380*mb)
        self.assertTrue(report['last_build']['finished'])
        monitor.sample('idle',fake(125*mb),now=24)
        self.assertIsNone(monitor.snapshot()['last_build']['after'])
        monitor.sample('idle',fake(119*mb),now=26)
        self.assertEqual(monitor.snapshot()['last_build']['after']['container_bytes'],119*mb)
        self.assertEqual(monitor.snapshot()['observed_peak_bytes'],380*mb)

    def test_idle_after_build_is_only_recorded_in_idle_phase(self):
        monitor=MemoryMonitor()
        with patch.object(memory_diagnostics,'read_memory',return_value=fake(100)):
            monitor.start_build('test')
            with patch.object(memory_diagnostics.time,'monotonic',return_value=10):
                monitor.finish_build('test')
        monitor.sample('toolchain_setup',fake(600),now=30)
        self.assertIsNone(monitor.snapshot()['last_build']['after'])
        monitor.sample('idle',fake(150),now=35)
        self.assertEqual(monitor.snapshot()['last_build']['after']['container_bytes'],150)

    def test_history_is_bounded_no_secret_values(self):
        monitor=MemoryMonitor()
        for n in range(300):
            monitor.sample('idle',fake(n),now=n)
        report=monitor.snapshot()
        self.assertEqual(len(report['history']),120)
        self.assertEqual(report['history'][-1]['container_bytes'],299)
        self.assertNotIn('password',str(report).lower())

    def test_missing_cgroup_data_does_not_crash(self):
        monitor=MemoryMonitor()
        empty={key:None for key in fake(0)}
        empty['source']='process_only'
        monitor.sample('idle',empty,now=0)
        with patch.object(memory_diagnostics,'read_memory',return_value=empty):
            monitor.start_build('job')
            monitor.finish_build('job')
        self.assertIsNone(monitor.snapshot()['last_build']['peak_bytes'])

    def test_frontend_and_api_routes_exist(self):
        index=(ROOT/'rflink_builder/static/index.html').read_text('utf-8')
        js=(ROOT/'rflink_builder/static/memory_diag.js').read_text('utf-8')
        server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        for ident in ('memory-container','memory-python','memory-build-peak',
                      'memory-build-after','memory-idle','memory-cache',
                      'memory-observed-peak','memory-kernel-peak',
                      'memory-copy','memory-phase'):
            self.assertIn('id="'+ident+'"',index)
        self.assertIn('memory_diag.js',index)
        self.assertIn('api/diagnostics/memory',js)
        self.assertIn("path == '/api/diagnostics/memory'",server)
        self.assertIn("'/memory_diag.js'",server)

if __name__=='__main__':unittest.main()
