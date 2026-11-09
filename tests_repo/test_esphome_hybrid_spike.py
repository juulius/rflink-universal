"""Host-tested ESPHome v0.6.6 / native C++ shadow decoder experiment.

Uses a *synthetic* ESPHome ZIP. Actual working Radio Gaga source ZIP remains
with its owner and is never required by CI or stored in the repository.
"""
from pathlib import Path
import hashlib
import runpy
import shutil
import subprocess
import tempfile
import unittest
from zipfile import ZipFile

ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/"experiments/esphome_hybrid/prepare_hybrid.py"
SPIKE=runpy.run_path(str(SCRIPT))
prepare=SPIKE["prepare"]
BASE=SPIKE["ZIP_ROOT"]

YAML="""esphome:
  name: ${name}
  friendly_name: ${friendly_name}
esp32:
  variant: esp32s3
  framework:
    type: esp-idf
spi:
  id: cc1101_spi
cc1101:
  - frequency: 433.42MHz
  - frequency: 433.92MHz
  - frequency: 868.2999MHz
    on_packet:
      then:
        - lambda: |-
            id(rflink_core).receive_packet(2, x, rssi, lqi, freq_offset);
remote_receiver:
  - id: rx_a
    rmt_symbols: 96
  - id: rx_b
    rmt_symbols: 96
"""

def fake_source(path, *, unsafe=False):
    with ZipFile(path,"w") as archive:
        archive.writestr(BASE+"/rflink-universal.yaml",YAML)
        archive.writestr(BASE+"/rflink_universal/components/rflink_universal/__init__.py",
                         "# Proven ESPHome CC1101/RMT component remains unmodified\n")
        archive.writestr(BASE+"/rflink_universal/secrets.yaml",
                         "password: DO_NOT_COPY_PRIVATE_WIFI_PASSWORD\n")
        if unsafe:
            archive.writestr(BASE+"/rflink_universal/../escaped.txt","untrusted")

class HybridSpikeTests(unittest.TestCase):
    def test_shadows_only_868_and_preserves_radio_owner(self):
        with tempfile.TemporaryDirectory(prefix="rflink-hybrid-") as temp:
            work=Path(temp);source=work/"v066.zip";output=work/"hybrid"
            fake_source(source)
            old_checksum=hashlib.sha256(source.read_bytes()).hexdigest()
            prepared=prepare(source,output)
            self.assertTrue(prepared.exists())
            self.assertEqual(source.read_bytes(),source.read_bytes())
            self.assertEqual(old_checksum,hashlib.sha256(source.read_bytes()).hexdigest())
            yaml=prepared.read_text("utf-8")
            self.assertEqual(yaml.count("rflink_hybrid::observe_868_packet(x)"),1)
            self.assertIn("id(rflink_core).receive_packet(2, x, rssi, lqi, freq_offset);",yaml)
            self.assertLess(yaml.index("id(rflink_core).receive_packet"),
                            yaml.index("rflink_hybrid::observe_868_packet"))
            self.assertIn("includes:\n    - rflink_universal/hybrid_probe/hybrid_probe.hpp",yaml)
            self.assertIn("    - rflink_universal/hybrid_probe/",yaml)
            self.assertEqual(yaml.count("rmt_symbols: 96"),2)
            self.assertEqual(yaml.count("433.42MHz"),1)
            self.assertEqual(yaml.count("433.92MHz"),1)
            self.assertNotIn("DO_NOT_COPY_PRIVATE_WIFI_PASSWORD",
                             "".join(str(x) for x in output.rglob("*")))
            self.assertFalse((output/"rflink_universal/secrets.yaml").exists())
            self.assertTrue((output/"HYBRID_PROBE_MANIFEST.json").exists())
            self.assertFalse((output/"rflink_universal/hybrid_probe/rflink_cc1101_spi.cpp").exists())
            with self.assertRaises(FileExistsError):
                prepare(source,output)

    def test_rejects_unknown_baseline_without_writing(self):
        with tempfile.TemporaryDirectory(prefix="rflink-hybrid-") as temp:
            work=Path(temp)
            source=work/"bad.zip"
            with ZipFile(source,"w") as archive:
                archive.writestr(BASE+"/rflink-universal.yaml","esphome: {}\n")
                archive.writestr(BASE+"/rflink_universal/components/rflink_universal/__init__.py","# stub")
            with self.assertRaisesRegex(ValueError,"Unexpected v0.6.6 YAML"):
                prepare(source,work/"out")
            self.assertFalse((work/"out").exists())

    def test_zip_path_traversal_is_blocked(self):
        with tempfile.TemporaryDirectory(prefix="rflink-hybrid-") as temp:
            work=Path(temp);source=work/"unsafe.zip"
            fake_source(source,unsafe=True)
            with self.assertRaisesRegex(ValueError,"unsafe source ZIP"):
                prepare(source,work/"out")
            self.assertFalse((work/"escaped.txt").exists())

    @unittest.skipUnless(shutil.which("g++"),"C++ compiler unavailable")
    def test_real_native_decoder_runs_behind_shadow_bridge_without_hw(self):
        with tempfile.TemporaryDirectory(prefix="rflink-hybrid-") as temp:
            work=Path(temp);source=work/"v066.zip";output=work/"hybrid"
            fake_source(source);prepare(source,output)
            hybrid=output/"rflink_universal/hybrid_probe"
            cxx=work/"smoke.cpp";bin=work/"smoke"
            cxx.write_text(r"""
#include "hybrid_probe.hpp"
#include <cassert>
#include <vector>
int main(){
  using namespace rflink_hybrid;
  auto before=counters();
  assert(before.packet_inputs==0);
  std::vector<uint8_t> noise(63,0);
  observe_868_packet(noise);
  auto after=counters();
  assert(after.packet_inputs==1);
  assert(after.unknown_packet_inputs==1);
  observe_868_packet(std::vector<uint8_t>(6,0));
  after=counters();
  assert(after.packet_inputs==1);
  assert(after.rejected_inputs==1);
  assert(after.itho_messages==0);
  assert(after.ramses_messages==0);
  return 0;
}
""",encoding="utf-8")
            files=[
              "hybrid_probe.cpp","rflink_native_decoder.cpp",
              "rflink_itho_codec.cpp","rflink_ramses_codec.cpp",
            ]
            subprocess.run(
                ["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                 "-pthread","-I"+str(hybrid),
                 *[str(hybrid/x) for x in files],
                 str(cxx),"-o",str(bin)],
                check=True,capture_output=True,text=True,timeout=60)
            subprocess.run([str(bin)],check=True,capture_output=True,text=True,timeout=10)

if __name__=="__main__":
    unittest.main()
