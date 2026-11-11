"""End-to-end host tests of the actual native RX-only user recipe dispatcher.

No RF transmission, NVS mock, physical device registration or secret exposure.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
CORE=ROOT/"firmware/components/rflink_decoder_core"
INC=CORE/"include"
SRC=CORE/"src/rflink_user_rx_runtime.cpp"
LIB=CORE/"src/rflink_user_library.cpp"
VM=CORE/"src/rflink_user_vm.cpp"
MAIN=ROOT/"firmware/main/main.cpp"
WEB=ROOT/"firmware/main/rflink_web_ota.cpp"
HDR=INC/"rflink_native_decoder.hpp"
NATIVE=CORE/"src/rflink_native_decoder.cpp"
UI=ROOT/"firmware/main/webui/app.js"

TEST_CPP=r'''#include "rflink_user_rx_runtime.hpp"
#include <cassert>
#include <string>
#include <vector>
#include <cstdint>

using namespace rflink;

static UserDecoder learn(const std::string& id,int32_t pulse) {
  std::vector<CapturedFrame> recorded;
  for(int32_t delta: {-5,0,5}) {
    CapturedFrame f{};
    f.kind=FrameKind::RAW;f.channel=1;f.label="on";
    f.raw={pulse+delta,-450,pulse+delta,-450,pulse+delta,-450,pulse+delta,-450};
    recorded.push_back(f);
  }
  UserDecoder d;
  std::string error;
  assert(UserLibrary::codebook(id,id,recorded,d,error));
  return d;
}
static UserDecoder learn_packet(const std::string& id) {
  std::vector<CapturedFrame> recorded;
  for(unsigned i=0;i<3;i++){
    CapturedFrame f{};
    f.kind=FrameKind::PACKET;f.channel=2;f.label="pressed";
    f.packet.resize(63,0x6A);
    recorded.push_back(f);
  }
  UserDecoder d;
  std::string error;
  assert(UserLibrary::codebook(id,id,recorded,d,error));
  return d;
}
int main() {
  std::string error;
  UserLibrary lib;
  auto first=learn("user_one",500);
  assert(lib.upsert_decoder(first,error));
  UserRxRuntime rx;
  assert(rx.install(lib,error));
  assert(rx.enabled_count()==0); // drafts never receive before explicit enable
  int32_t timings[8]={502,-450,502,-450,502,-450,502,-450};
  DecodeResult result{};
  assert(rx.match_raw(1,433920000u,timings,8,result)==UserRxOutcome::none);
  assert(lib.enable_decoder("user_one",true,error));
  assert(rx.install(lib,error));
  assert(rx.enabled_count()==1);
  assert(rx.match_raw(1,433920000u,timings,8,result)==UserRxOutcome::match);
  assert(result.valid && result.protocol=="user_one" && result.action=="on");
  assert(!result.transmit_supported);
  assert(rx.match_raw(0,433420000u,timings,8,result)==UserRxOutcome::none);
  assert(rx.match_raw(1,433420000u,timings,8,result)==UserRxOutcome::none);
  assert(rx.match_raw(1,433920000u,timings,1,result)==UserRxOutcome::none);
  assert(rx.match_raw(1,433920000u,nullptr,8,result)==UserRxOutcome::none);
  std::vector<int32_t> too_big(513,400);
  assert(rx.match_raw(1,433920000u,too_big.data(),too_big.size(),result)==UserRxOutcome::none);
  timings[0]=1500;
  assert(rx.match_raw(1,433920000u,timings,8,result)==UserRxOutcome::none);
  timings[0]=502;
  auto second=learn("user_two",500);
  assert(lib.upsert_decoder(second,error));
  assert(lib.enable_decoder("user_two",true,error));
  assert(rx.install(lib,error));
  assert(rx.enabled_count()==2);
  assert(rx.match_raw(1,433920000u,timings,8,result)==UserRxOutcome::ambiguous);
  assert(!result.valid && result.protocol.empty()); // no arbitrary winner
  auto bad=learn("user_bad",800);
  bad.enabled=true;
  bad.rx_source="HALT\nACCEPT\n"; // intentionally contradicts its recordings
  auto direct_library=lib;
  assert(!direct_library.upsert_decoder(bad,error)); // activation replay refuses bad
  assert(rx.enabled_count()==2);
  // Installing a valid new library must replace, not append, its rules.
  UserLibrary packets;
  auto pkt=learn_packet("user_packet");
  assert(packets.upsert_decoder(pkt,error));
  assert(packets.enable_decoder("user_packet",true,error));
  assert(rx.install(packets,error));
  assert(rx.enabled_count()==1);
  assert(rx.match_raw(1,433920000u,timings,8,result)==UserRxOutcome::none);
  std::vector<uint8_t> bytes(63,0x6A);
  assert(rx.match_packet(2,868300000u,bytes.data(),bytes.size(),result)==UserRxOutcome::match);
  assert(result.valid && result.protocol=="user_packet" && !result.transmit_supported);
  assert(rx.match_packet(1,868300000u,bytes.data(),bytes.size(),result)==UserRxOutcome::none);
  assert(rx.match_packet(2,433920000u,bytes.data(),bytes.size(),result)==UserRxOutcome::none);
  assert(rx.match_packet(2,868300000u,bytes.data(),62,result)==UserRxOutcome::none);
  assert(rx.match_packet(2,868300000u,nullptr,63,result)==UserRxOutcome::none);
  assert(rx.install(UserLibrary{},error));
  assert(rx.enabled_count()==0);
  assert(rx.match_packet(2,868300000u,bytes.data(),bytes.size(),result)==UserRxOutcome::none);
  return 0;
}
'''

class UserRxRuntimeTests(unittest.TestCase):
    def test_source_routing_and_no_user_tx(self):
        source=SRC.read_text()
        native=NATIVE.read_text()
        main=MAIN.read_text()
        web=WEB.read_text()
        header=HDR.read_text()
        cmake=(CORE/"CMakeLists.txt").read_text()
        for item in ("user_vm::receive(","UserLibrary::MAX_DECODERS",
                     "user_vm::compile(","library.replay(",
                     "UserRxOutcome::ambiguous","count>512","count!=63"):
            self.assertIn(item,source)
        self.assertNotIn("user_vm::transmit(",source)
        self.assertIn('"src/rflink_user_rx_runtime.cpp"',cmake)
        self.assertIn("try_user_packet(",native)
        self.assertLess(native.index("if(is_ramses){"),
                        native.index("if(try_user_packet(",native.index("if(is_ramses){")))
        self.assertIn("user_runtime_.match_raw(",native)
        self.assertIn("UserLibraryNvs::load(",main)
        self.assertLess(main.index("decoder.install_user_library("),
                        main.index("xTaskCreate(radio_rx_task"))
        self.assertIn("user_rx_matches",header)
        self.assertIn("user_rx_matches",web)
        self.assertIn("user_library_storage_state",UI)
        self.assertIn("user_rx_match",web)
        self.assertIn("user_rx_ambiguous",web)
        self.assertNotIn("nvs_set_blob(",native)
        self.assertNotIn("rmt_transmit(",native)

    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_actual_cpp_recipe_routing_and_ambiguity(self):
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/"test_user_rx.cpp"
            exe=pathlib.Path(folder)/"test_user_rx"
            cpp.write_text(TEST_CPP)
            command=["g++","-std=c++17","-Wall","-Wextra","-Werror",
                     "-I"+str(INC),str(cpp),str(SRC),str(LIB),str(VM),"-o",str(exe)]
            build=subprocess.run(command,capture_output=True,text=True,timeout=60)
            self.assertEqual(build.returncode,0,build.stderr)
            execution=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
            self.assertEqual(execution.returncode,0,execution.stderr)

if __name__=="__main__":
    unittest.main()
