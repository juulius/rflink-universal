"""Synthetic evidence: timing family, inversion, cadence and privacy."""
import json
import sys
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from rf_signal_families import timing_family, analyze_signal_families
from rf_replay import make_report, _parse_capture

def capture(seq,bits,t,radio=1):
    return _parse_capture(dict(radio=radio,format='signed_pulses_us',
        capture_phase='pre_filter_rmt',boot_id=1,sequence=seq,uptime_ms=t,
        pulses_us=[x for b in bits for x in (1875,-(4000 if b=='1' else 2000))]))

class TestFamilies(unittest.TestCase):
    def test_pattern_and_duplicates(self):
        a='00110101'*4
        b=''.join('1' if x=='0' else '0' for x in a)
        x=a+b+'0101010'
        y=a+b+'0101011'
        caps=[capture(1,x,1000),capture(2,x,1100),capture(3,y,61000)]
        caps.append(caps[0])
        report=make_report(caps,{},[],structural_only=True)
        f=report['signal_families']['families'][0]
        self.assertEqual(f['captures'],3)
        self.assertEqual(f['distinct_patterns'],2)
        self.assertEqual(f['largest_exact_repeat'],2)
        self.assertEqual(f['inverse32_exact'],3)
        self.assertEqual(f['fixed_positions'],70)
        self.assertEqual(f['median_interval_ms'],60000)
        self.assertFalse(f['protocol_verified'])
        self.assertEqual(report['signal_families']['summary']['duplicate_rows_ignored'],1)
        self.assertNotIn(x,json.dumps(report['signal_families']))
    def test_complementary(self):
        p=[x for i in range(24) for x in ((300,-1100) if i%2 else (1000,-425))]
        self.assertEqual(timing_family(p)[0],'complementary_widths')
        self.assertIsNone(timing_family([100]*40))
    def test_mirror(self):
        self.assertEqual((ROOT/'tools/rf_signal_families.py').read_bytes(),
            (ROOT/'rflink_builder/replay_tools/rf_signal_families.py').read_bytes())

if __name__=='__main__':
    unittest.main()
