"""Bench-only physical GDO CLOCK ISR + bounded SPSC handoff contracts.

Host compilation covers the real queue and already-integrated bit accumulator.
The actual GPIO adapter is compiled by the official ESP-IDF CI job, not
executed on the CI host. No live CC1101 switching or RX claim is made.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"

class GdoClockBenchContract(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "C++17 compiler required")
    def test_bound_isr_queue_and_native_capture(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/"sync_edge_ring"
            cmd=[compiler,"-std=c++17","-O2","-Wall","-Wextra","-Werror",
                 "-pthread","-I"+str(HAL/"include"),
                 str(HAL/"rflink_clocked_bit_capture.cpp"),
                 str(ROOT/"firmware/tests/test_sync_edge_ring.cpp"),
                 "-o",str(exe)]
            built=subprocess.run(cmd,capture_output=True,text=True,timeout=60)
            self.assertEqual(built.returncode,0,built.stderr)
            result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertIn("PASS 868 synchronous GPIO clock-edge ring",result.stdout)

    def test_sampler_not_started_and_cc1101_configuration_unchanged(self):
        main=(ROOT/"firmware/main/main.cpp").read_text()
        profiles=(HAL/"rflink_rf_profiles.cpp").read_text()
        sampler=(HAL/"rflink_sync_gdo_sampler.cpp").read_text()
        header=(HAL/"include/rflink_sync_gdo_sampler.hpp").read_text()
        cmake=(HAL/"CMakeLists.txt").read_text()
        self.assertNotIn("Cc1101SyncGdoSampler",main)
        self.assertIn("kItho868[]",profiles)
        self.assertIn("{0x06,63}",profiles)
        self.assertIn("{0x08,0x00}",profiles) # Existing 63-byte packet mode.
        self.assertIn("rflink_sync_gdo_sampler.cpp",cmake)
        self.assertIn("gpio_isr_handler_add",sampler)
        self.assertIn("gpio_isr_handler_remove",sampler)
        self.assertIn("gpio_intr_disable",sampler)
        self.assertIn("esp_ptr_internal(this)",sampler)
        self.assertIn("GPIO_IN_REG",sampler)
        self.assertIn("report_lost_edges",sampler)
        self.assertIn("NOT instantiated or started by main.cpp",header)
        isr=sampler.split("void IRAM_ATTR Cc1101SyncGdoSampler::on_clock",1)[1].split("esp_err_t Cc1101SyncGdoSampler::start",1)[0]
        for disallowed in ("ESP_LOG","xQueue","malloc(","new ","bus_.","write_register(",
                           "gpio_config(","gpio_isr_handler_add(","printf("):
            self.assertNotIn(disallowed,isr)

if __name__=="__main__":
    unittest.main()
