"""Adapter regression: measured pairs, no fabricated RF, private IDs."""
import json
import subprocess
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import rf_rtl433

def cap(seq=1):
    return dict(radio=1,format="signed_pulses_us",capture_phase="pre_filter_rmt",
                frequency_hz=433920000,capture_id=str(seq),
                pulses_us=[-100]+[300,-900]*8+[200])

class AdapterTest(unittest.TestCase):
    def test_ook(self):
        data,omitted=rf_rtl433.ook_text(cap())
        self.assertEqual(omitted,2)
        self.assertEqual(data.count("300 900\n"),8)
        self.assertNotIn("200 0",data)
        self.assertIsNone(rf_rtl433.ook_text({**cap(),"radio":2}))
        self.assertIsNone(rf_rtl433.ook_text({**cap(),"capture_phase":"decoder_input"}))
    def test_local_output_and_cache(self):
        response=json.dumps(dict(model="Example-Sensor",id=987654321,channel=2,
            mic="CRC",protocol=10,temperature_C=23.4,secret="PRIVATE"))
        rows=[{},{}]
        with patch.object(rf_rtl433.subprocess,"run",return_value=subprocess.CompletedProcess([],0,response)) as run:
            result=rf_rtl433.compare([cap(1),cap(2)],rows,executable="/usr/bin/rtl_433")
        self.assertEqual(run.call_count,1)
        self.assertEqual(result["summary"]["matched_captures"],2)
        self.assertEqual(result["summary"]["repeated_device_candidates"],1)
        self.assertEqual(rows[0]["rtl433"]["matches"][0]["device_candidate"],"D1")
        self.assertNotIn("987654321",json.dumps([result,rows]))
        self.assertNotIn("PRIVATE",json.dumps([result,rows]))
        self.assertFalse(rows[0]["rtl433"]["device_verified"])
    def test_failure_is_not_no_match(self):
        with patch.object(rf_rtl433.subprocess,"run",side_effect=subprocess.TimeoutExpired([],2)):
            result=rf_rtl433.compare([cap()], [{}], executable="/usr/bin/rtl_433")
        self.assertEqual(result["state"],"partial")
        self.assertEqual(result["summary"]["errors"],1)
    def test_no_binary(self):
        with patch.object(rf_rtl433.shutil,"which",return_value=None):
            self.assertEqual(rf_rtl433.compare([cap()],[{}])["state"],"unavailable")
    def test_mirror(self):
        self.assertEqual((ROOT/"tools/rf_rtl433.py").read_bytes(),
                         (ROOT/"rflink_builder/replay_tools/rf_rtl433.py").read_bytes())

if __name__=="__main__":
    unittest.main()
