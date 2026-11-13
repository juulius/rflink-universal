"""CSV gap report must use actual consecutive records, not page cursor."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]

def expected_gap(records, snapshot_end, chunk=16):
    previous=0
    lost=False
    for start in range(0, len(records), chunk):
        page=records[start:start+chunk]
        for seq in page:
            if seq>previous+1:
                lost=True
            previous=seq
    return lost or previous<snapshot_end

class CsvSequenceGaps(unittest.TestCase):
    def test_source_tracks_rows_across_page_boundaries(self):
        cpp=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text(encoding="utf-8")
        section=cpp.split('static esp_err_t logs_csv_get(httpd_req_t* req)',1)[1].split(
            'static std::atomic<bool> csv_export_busy',1)[0]
        self.assertIn('uint64_t last_exported_sequence=0;',section)
        self.assertIn('if(e.sequence>last_exported_sequence+1)lost=true;',section)
        self.assertIn('last_exported_sequence=e.sequence;',section)
        self.assertIn('if(last_exported_sequence<snapshot_end)lost=true;',section)
        self.assertNotIn('if(e.sequence>after+1 && after>0)lost=true;',section)
        self.assertIn('after=records[count-1].sequence;',section)

    def test_contiguous_pages_have_no_false_positive(self):
        # Reproduces exp.48 CSV: 149 consecutive events in 16-record pages.
        self.assertFalse(expected_gap(list(range(1,150)),149))
        self.assertFalse(expected_gap(list(range(1,33)),32))
        self.assertFalse(expected_gap([1],1))

    def test_actual_missing_entries_remain_reported(self):
        self.assertTrue(expected_gap([x for x in range(1,150) if x!=19],149))
        self.assertTrue(expected_gap(list(range(4,22)),21))  # old entries evicted
        self.assertTrue(expected_gap(list(range(1,99)),99))  # missing tail
        self.assertTrue(expected_gap([],20))

if __name__=="__main__":
    unittest.main()
