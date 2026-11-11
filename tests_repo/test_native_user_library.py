"""Native v0.6.6 user library parity and safety checks (no hardware TX).

Build and execute the ACTUAL library + interpreter with g++, independent of
ESPHome, NVS and firmware hardware/HTTP services.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CORE = ROOT / "firmware/components/rflink_decoder_core"
SRC = CORE / "src/rflink_user_library.cpp"
VM = CORE / "src/rflink_user_vm.cpp"
INCLUDE = CORE / "include"

TEST_CPP = r'''#include "rflink_user_library.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <map>

using namespace rflink;
static CapturedFrame sample(const std::string& label,int32_t n) {
  CapturedFrame f{};
  f.channel=1; f.kind=FrameKind::RAW; f.label=label;
  f.raw={n,-400,n,-400,n,-400,n,-400};
  return f;
}
static CapturedFrame packet(uint8_t variable) {
  CapturedFrame f{};
  f.channel=2;f.kind=FrameKind::PACKET;f.label="press";
  f.packet.resize(63,0xa5);f.packet[23]=variable;
  return f;
}
int main() {
  std::string error;
  std::vector<CapturedFrame> captures{
    sample("on",500),sample("on",505),sample("on",495),
    sample("off",900),sample("off",910),sample("off",890)
  };
  UserDecoder automatic{};
  assert(!UserLibrary::codebook("invalid","Living",captures,automatic,error));
  assert(UserLibrary::codebook("user_living","Living remote",captures,automatic,error));
  assert(automatic.id=="user_living");
  assert(automatic.radio==1 && automatic.kind==FrameKind::RAW);
  assert(automatic.actions.size()==2 && !automatic.rx_source.empty());
  assert(!automatic.enabled);
  assert(automatic.tx_source.size()>0);  // offline *preview* only, no RF transmission
  const uint32_t fp=UserLibrary::fingerprint(automatic);
  assert(fp!=0);
  UserLibrary lib;
  assert(lib.upsert_decoder(automatic,error));
  assert(!lib.decoders().at(0).enabled);
  assert(lib.enable_decoder("user_living",true,error));
  assert(lib.decoder("user_living")->enabled);
  auto changed=*lib.decoder("user_living");
  changed.rx_source="HALT\nACCEPT\n";
  assert(!lib.upsert_decoder(changed,error)); // active rules cannot be overwritten

  UserProfile p{};
  p.id="user_living_profile";p.name="Living switch";
  p.protocol="user_living";p.expected_actions={"on","off"};
  p.controls={"on","off"};p.setup_steps={"Press the buttons three times"};
  p.pair_method="manual";p.learn_help="Hold close to antenna";
  assert(lib.upsert_profile(p,error));
  assert(!lib.remove_decoder("user_living")); // active and profile-bound

  assert(lib.commit_state({{"device:living:rolling",3}}));
  assert(!lib.commit_state({{"device:living:rolling",3}}));
  assert(!lib.commit_state({{"device:living:rolling",2}}));
  assert(!lib.commit_state({{"device:living:rolling",4},{"other",0}})); // all-or-none
  assert(lib.state().at("device:living:rolling")==3);
  assert(lib.commit_state({{"device:living:rolling",4}}));

  auto blob=std::make_unique<UserLibraryPersistence::Blob>();
  assert(UserLibraryPersistence::encode(lib,17,*blob));
  assert(blob->used>0 && UserLibraryPersistence::valid(*blob));
  UserLibrary restored;
  assert(UserLibraryPersistence::decode(*blob,restored));
  assert(restored.decoders().size()==1 && restored.profiles().size()==1);
  assert(restored.decoder("user_living")->enabled);
  assert(restored.profiles().front().controls.size()==2);
  assert(restored.state().at("device:living:rolling")==4);
  assert(UserLibrary::fingerprint(*restored.decoder("user_living"))==fp);
  blob->bytes[0]^=0x80;
  assert(!UserLibraryPersistence::valid(*blob));
  assert(!UserLibraryPersistence::decode(*blob,restored));
  assert(restored.decoder("user_living")!=nullptr); // failed decode never mutates target
  blob->bytes[0]^=0x80;

  assert(restored.enable_decoder("user_living",false,error));
  assert(!restored.remove_decoder("user_living")); // profile still references it
  assert(restored.remove_profile("user_living_profile"));
  assert(restored.remove_decoder("user_living"));

  UserLibrary drafts;
  auto bad=automatic;
  bad.id="user_wrong_radio";bad.enabled=false;bad.samples.front().channel=0;
  assert(!drafts.upsert_decoder(bad,error));
  bad=automatic;
  bad.id="user_mixed";bad.enabled=false;bad.samples[0].packet={0x01};
  assert(!drafts.upsert_decoder(bad,error));
  bad=automatic;
  bad.id="user_short";bad.enabled=false;bad.samples[0].raw={100};
  assert(!drafts.upsert_decoder(bad,error));
  bad=automatic;
  bad.id="user_huge";bad.enabled=false;bad.samples[0].raw.resize(513,200);
  assert(!drafts.upsert_decoder(bad,error));
  bad=automatic;
  bad.id="built_in_spoof";bad.enabled=false;
  assert(!drafts.upsert_decoder(bad,error));

  auto examples=captures;
  examples[3].raw=examples[0].raw;
  examples[4].raw=examples[0].raw;
  examples[5].raw=examples[0].raw;
  UserDecoder ambiguous{};
  assert(!UserLibrary::codebook("user_ambiguous","Ambiguous",examples,ambiguous,error));

  std::vector<CapturedFrame> rolling{packet(1),packet(2),packet(3)};
  UserDecoder packet_rec{};
  assert(UserLibrary::codebook("user_rolling","Rolling record",rolling,packet_rec,error));
  assert(packet_rec.rx_source.size()>0);
  assert(packet_rec.tx_source.empty()); // varying byte must never auto-replay
  assert(drafts.upsert_decoder(packet_rec,error));
  assert(drafts.enable_decoder("user_rolling",true,error));
  assert(drafts.decoder("user_rolling")->tx_source.empty());

  UserProfile invalid_profile{};
  invalid_profile.id="user_bad_profile";invalid_profile.name="Bad";
  invalid_profile.protocol="user_rolling";invalid_profile.expected_actions={"on"};
  invalid_profile.controls={"off"};
  assert(!drafts.upsert_profile(invalid_profile,error));
  assert(!UserLibraryPersistence::encode(drafts,0,*blob));
  return 0;
}
'''

class NativeUserLibraryTests(unittest.TestCase):
    def test_native_source_contract(self):
        source=SRC.read_text()
        header=(INCLUDE/"rflink_user_library.hpp").read_text()
        cmake=(CORE/"CMakeLists.txt").read_text()
        for token in ("MAX_DECODERS = 8", "MAX_PROFILES = 16",
                      "MAX_SAMPLES = 12", "MAX_STATE = 32",
                      "CAPACITY = 16384", "VERSION = 2"):
            self.assertIn(token,header)
        for token in ("UserLibrary::codebook(", "UserLibrary::replay(",
                      "UserLibraryPersistence::decode(", "UserLibraryPersistence::encode(",
                      "invalid or mixed-format example capture"):
            self.assertIn(token,source)
        self.assertIn('"src/rflink_user_library.cpp"',cmake)
        self.assertNotIn("namespace esphome",source)
        self.assertNotIn("developer_session.h",header)
        self.assertNotIn("user_library.h",source)

    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_native_library_build_and_behavior(self):
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/"library_test.cpp"
            exe=pathlib.Path(folder)/"library_test"
            cpp.write_text(TEST_CPP)
            compile=subprocess.run(
                ["g++","-std=c++17","-Wall","-Wextra","-Werror",
                 "-I"+str(INCLUDE),str(cpp),str(SRC),str(VM),"-o",str(exe)],
                capture_output=True,text=True,timeout=60)
            self.assertEqual(compile.returncode,0,compile.stderr)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
            self.assertEqual(run.returncode,0,run.stderr)


if __name__=="__main__":
    unittest.main()
