"""Exercise real native RmtPulseRx with fake ESP-IDF hardware and FreeRTOS queues."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"
STUBS=ROOT/"firmware/tests/host_idf_stubs"
SOURCE=ROOT/"firmware/tests/test_433_rmt_slot_runtime.cpp"

class RmtSlotRuntimeTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"C++ host compiler unavailable")
    def test_back_to_back_receive_and_bounded_queue_drop(self):
        with tempfile.TemporaryDirectory() as d:
            target=Path(d)/"rmt-test"
            cmd=["g++","-std=c++17","-Wall","-Wextra","-Werror","-pthread",
                 "-I"+str(STUBS),"-I"+str(HAL/"include"),
                 str(HAL/"rflink_rmt_rx.cpp"),str(SOURCE),"-o",str(target)]
            c=subprocess.run(cmd,capture_output=True,text=True,timeout=30)
            self.assertEqual(c.returncode,0,c.stderr)
            r=subprocess.run([str(target)],capture_output=True,text=True,timeout=10)
            self.assertEqual(r.returncode,0,r.stderr)

if __name__=="__main__":
    unittest.main()
