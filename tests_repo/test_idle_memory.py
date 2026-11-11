"""Idle allocator reclamation checks: no LAN access or ESP-IDF required."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
import idle_memory


class IdleMemoryTests(unittest.TestCase):
    def test_only_runs_once_after_quiet_period(self):
        cleaner = idle_memory.IdleMemoryReclaimer()
        cleaner.request("build", now=100)
        with patch.object(idle_memory.gc, "collect", return_value=2) as collect, \
             patch.object(idle_memory, "_trim_allocator", return_value=(True, True)) as trim, \
             patch.object(idle_memory, "_process_details", side_effect=[
                 {"python_rss_bytes": 80, "python_anon_bytes": 70},
                 {"python_rss_bytes": 60, "python_anon_bytes": 50},
             ]):
            self.assertIsNone(cleaner.run_if_idle(False, now=200))
            self.assertIsNone(cleaner.run_if_idle(True, now=109))
            self.assertTrue(cleaner.snapshot()["pending"])
            self.assertEqual(collect.call_count, 0)
            result = cleaner.run_if_idle(True, now=110)
            self.assertEqual(result["reason"], "build")
            self.assertEqual(result["python_rss_before_bytes"], 80)
            self.assertEqual(result["python_rss_after_bytes"], 60)
            self.assertEqual(result["python_anon_after_bytes"], 50)
            self.assertEqual(result["gc_cycles_collected"], 2)
            self.assertTrue(result["allocator_returned_pages"])
            self.assertIsNone(cleaner.run_if_idle(True, now=140))
            collect.assert_called_once_with()
            trim.assert_called_once_with()
        self.assertFalse(cleaner.snapshot()["pending"])

    def test_requests_coalesce_and_reset_quiet_period(self):
        cleaner = idle_memory.IdleMemoryReclaimer()
        cleaner.request("scanning", now=1)
        cleaner.request("build", now=9)
        self.assertIsNone(cleaner.run_if_idle(True, now=12))
        with patch.object(idle_memory.gc, "collect", return_value=0), \
             patch.object(idle_memory, "_trim_allocator", return_value=(False, False)), \
             patch.object(idle_memory, "_process_details", return_value={}):
            result = cleaner.run_if_idle(True, now=19)
        self.assertEqual(result["reason"], "build")
        self.assertFalse(result["allocator_trim_supported"])
        self.assertIsNone(result["python_anon_before_bytes"])

    def test_rejects_untrusted_reason_and_keeps_only_one_record(self):
        cleaner = idle_memory.IdleMemoryReclaimer()
        with self.assertRaises(ValueError):
            cleaner.request("rf-physical-id-secret")
        self.assertIsNone(cleaner.snapshot()["last"])
        self.assertFalse(cleaner.snapshot()["pending"])

    def test_no_extra_threads_or_subprocesses_and_workload_hooks(self):
        code = (ROOT / "rflink_builder" / "idle_memory.py").read_text("utf-8")
        self.assertNotIn("Thread(", code)
        self.assertNotIn("Popen(", code)
        server = (ROOT / "rflink_builder" / "server.py").read_text("utf-8")
        self.assertIn("idle_reclaimer.request('build')", server)
        self.assertIn("idle_reclaimer.run_if_idle(phase == 'idle')", server)
        self.assertIn("'idle_reclaim': idle_reclaimer.snapshot()", server)

    def test_allocator_fallback_is_safe_without_malloc_trim(self):
        class NoTrimLib:
            pass
        with patch("ctypes.CDLL", return_value=NoTrimLib()):
            self.assertEqual(idle_memory._trim_allocator(), (False, False))


if __name__ == "__main__":
    unittest.main()
