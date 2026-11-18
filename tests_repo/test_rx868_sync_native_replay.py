"""Synthetic-only offline protocol replay regression for the PR #206 CSV.

Compiles real native C++ Itho/RAMSES codecs. Never transmits or accesses
hardware; synthetic IDs and bits are not production RF captures.
"""
import csv
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
CORE=FW/"components/rflink_decoder_core"
TOOLS=FW/"tools"
sys.path.insert(0,str(TOOLS))
from replay_sync_bits import replay  # noqa: E402


def make_bits(hex_msb, count):
    return "".join(f"{b:08b}" for b in bytes.fromhex(hex_msb))[:count]


def hex_packed(bits):
    padded=bits+"0"*((-len(bits))%8)
    return "".join(f"{int(padded[i:i+8],2):02X}" for i in range(0,len(padded),8))


def write_capture(path,records):
    """Records: (bits, epoch, window_end, discontinuity_before)."""
    meta=[
        ("#schema","rflink-sync-bits-v1"),
        ("#mode","BENCH_ONLY"),
        ("#radio","868.30 MHz"),
        ("#generation","1"),
        ("#capacity","256"),
        ("#blocks",str(len(records))),
        ("#recorded_bits",str(sum(len(r[0]) for r in records))),
        ("#full","0"),
    ]
    with path.open("w",newline="",encoding="utf-8") as output:
        writer=csv.writer(output,lineterminator="\n")
        writer.writerows(meta)
        writer.writerow(["index","epoch","bit_count","first_us","last_us",
                         "discontinuity_before","window_end","hex_msb"])
        for i,(bits,epoch,window,gap) in enumerate(records):
            assert bits and len(bits)<=2048
            first=1000+i*100000
            writer.writerow([i,epoch,len(bits),first,first+len(bits)*26,
                             int(gap),int(window),hex_packed(bits)])


@unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                     "Host C++17 compiler required for real native codec tests")
class NativeSyncReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory=tempfile.TemporaryDirectory(prefix="rflink-sync-fixtures-")
        cls.addClassCleanup(cls.directory.cleanup)
        compiler=shutil.which("g++") or shutil.which("clang++")
        exe=Path(cls.directory.name)/"fixture"
        compile_cmd=[
            compiler,"-std=c++17","-O2","-Wall","-Wextra","-Werror",
            "-I"+str(CORE/"include"),
            str(FW/"tests/fixture_sync_replay.cpp"),
            str(CORE/"src/rflink_itho_codec.cpp"),
            str(CORE/"src/rflink_ramses_codec.cpp"),
            str(CORE/"src/rflink_ramses_tx.cpp"),
            "-o",str(exe),
        ]
        compiled=subprocess.run(compile_cmd,text=True,capture_output=True,timeout=75)
        if compiled.returncode:
            raise AssertionError(compiled.stderr)
        generated=subprocess.run([str(exe)],text=True,capture_output=True,timeout=10)
        if generated.returncode:
            raise AssertionError(generated.stderr)
        cls.bits={}
        for row in generated.stdout.splitlines():
            name,count,raw=row.split()
            cls.bits[name]=make_bits(raw,int(count))
        assert {"itho","ramses"}==set(cls.bits)

    def test_unaligned_both_native_decoders_same_contiguous_segment(self):
        data="101"+self.bits["itho"]+"101101"+self.bits["ramses"]+"110"
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"record.csv"
            write_capture(path,[(data,11,True,False)])
            result=replay(path)
        self.assertEqual(result["segments"],1)
        self.assertEqual(result["candidate_itho_structure_only"],1)
        self.assertEqual(result["checksum_valid_ramses_ii"],1)
        itho=next(m for m in result["matches"] if m["protocol"]=="itho")
        ramses=next(m for m in result["matches"] if m["protocol"]=="ramses_ii")
        self.assertEqual(itho["bit_offset"],3)
        self.assertEqual(itho["evidence"],"structure_only")
        self.assertEqual(ramses["bit_offset"],3+504+6)
        self.assertEqual(ramses["evidence"],"checksum_valid")
        self.assertEqual(ramses["summary"],"command=0x22F1")
        self.assertTrue(all(m["hardware_verified"] is False for m in result["matches"]))
        self.assertNotIn("1234",json.dumps(result)) # Raw address is never disclosed.

    def test_reassemble_only_across_truly_continuous_blocks(self):
        bits=self.bits["ramses"]
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"record.csv"
            write_capture(path,[(bits[:147],22,False,False),
                                (bits[147:],22,True,False)])
            good=replay(path)
            self.assertEqual(good["segments"],1)
            self.assertEqual(good["checksum_valid_ramses_ii"],1)
            for first_window, second_gap, second_epoch in (
                (True,False,22),(False,True,22),(False,False,23)
            ):
                write_capture(path,[(bits[:147],22,first_window,False),
                                    (bits[147:],second_epoch,True,second_gap)])
                broken=replay(path)
                self.assertEqual(broken["segments"],2)
                self.assertEqual(broken["checksum_valid_ramses_ii"],0)

    def test_invalid_checksum_or_header_does_not_claim_match(self):
        corrupt_ramses=list(self.bits["ramses"])
        corrupt_ramses[120]="0" if corrupt_ramses[120]=="1" else "1"
        corrupt_itho="1"+self.bits["itho"][1:]
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"record.csv"
            write_capture(path,[("".join(corrupt_ramses),1,True,False),
                                (corrupt_itho,2,True,False)])
            result=replay(path)
        self.assertEqual(result["checksum_valid_ramses_ii"],0)
        self.assertEqual(result["candidate_itho_structure_only"],0)

    def test_explicit_inversion_finds_frames_but_marks_hypothesis(self):
        inverted="".join("1" if b=="0" else "0" for b in self.bits["ramses"])
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"record.csv"
            write_capture(path,[(inverted,3,True,False)])
            plain=replay(path)
            flipped=replay(path,try_inverted=True)
        self.assertEqual(plain["checksum_valid_ramses_ii"],0)
        self.assertEqual(flipped["checksum_valid_ramses_ii"],1)
        self.assertEqual(flipped["matches"][0]["polarity"],"inverted")

    def test_inspector_rejects_corrupted_csv_before_decoder(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"record.csv"
            write_capture(path,[(self.bits["ramses"],1,True,False)])
            contents=path.read_text()
            path.write_text(contents.replace("#recorded_bits,", "#recorded_bits,999",1))
            with self.assertRaises(ValueError):
                replay(path)


if __name__=="__main__":
    unittest.main()
