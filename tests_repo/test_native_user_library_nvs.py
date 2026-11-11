"""Host compilation and fault-injection tests for actual ESP-IDF NVS A/B backend.

The NVS mock models handle/commit durability and readback corruption. The
same native source (not a test double) is compiled with the real user VM and
snapshot codec. CI's ESP-IDF build separately checks native target headers.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
CORE=ROOT/"firmware/components/rflink_decoder_core"
STUBS=ROOT/"firmware/tests/host_idf_stubs"
SRC=CORE/"src/rflink_user_library_nvs.cpp"
LIB=CORE/"src/rflink_user_library.cpp"
VM=CORE/"src/rflink_user_vm.cpp"
INC=CORE/"include"

TEST_CPP=r'''#include "rflink_user_library_nvs.hpp"
#include "nvs.h"
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <utility>
using namespace rflink;

using Bytes=std::vector<uint8_t>;
using Namespace=std::map<std::string,Bytes>;
static std::map<std::string,Namespace> flash;
struct Handle {std::string ns;bool writable;Namespace pending;};
static std::map<unsigned,Handle> handles;
static unsigned next_handle=1;
static bool fail_set=false, fail_commit=false, fail_read=false,corrupt_commit=false;

extern "C" esp_err_t nvs_open(const char* ns,int mode,nvs_handle_t* h){
  if(!ns || !h)return ESP_ERR_INVALID_ARG;
  if(mode==NVS_READONLY && flash.find(ns)==flash.end())return ESP_ERR_NVS_NOT_FOUND;
  if(mode!=NVS_READWRITE && mode!=NVS_READONLY)return ESP_ERR_INVALID_ARG;
  if(mode==NVS_READWRITE)flash[ns];
  *h=next_handle++;
  handles[*h]=Handle{ns,mode==NVS_READWRITE,{}};
  return ESP_OK;
}
extern "C" esp_err_t nvs_get_blob(nvs_handle_t h,const char* key,void* out,size_t* length){
  if(fail_read)return ESP_FAIL;
  auto i=handles.find(h);
  if(i==handles.end()||!key||!length)return ESP_ERR_INVALID_ARG;
  auto n=flash.find(i->second.ns);
  if(n==flash.end())return ESP_ERR_NVS_NOT_FOUND;
  auto b=n->second.find(key);
  if(b==n->second.end())return ESP_ERR_NVS_NOT_FOUND;
  if(!out){*length=b->second.size();return ESP_OK;}
  if(*length<b->second.size())return ESP_ERR_INVALID_ARG;
  std::memcpy(out,b->second.data(),b->second.size());
  *length=b->second.size();
  return ESP_OK;
}
extern "C" esp_err_t nvs_set_blob(nvs_handle_t h,const char* key,const void* data,size_t size){
  auto i=handles.find(h);
  if(fail_set)return ESP_FAIL;
  if(i==handles.end()||!i->second.writable||!key||!data)return ESP_ERR_INVALID_ARG;
  const auto* bytes=static_cast<const uint8_t*>(data);
  i->second.pending[key]=Bytes(bytes,bytes+size);
  return ESP_OK;
}
extern "C" esp_err_t nvs_commit(nvs_handle_t h){
  auto i=handles.find(h);
  if(i==handles.end()||!i->second.writable)return ESP_ERR_INVALID_ARG;
  if(fail_commit)return ESP_FAIL;
  for(auto& [key, bytes]:i->second.pending) {
    if(corrupt_commit && bytes.size()>24)bytes[20]^=1;
    flash[i->second.ns][key]=bytes;
  }
  i->second.pending.clear();
  return ESP_OK;
}
extern "C" void nvs_close(nvs_handle_t h){handles.erase(h);}

int main(){
  std::string error;
  UserLibrary original;
  auto first=UserLibraryNvs::load(original,error);
  assert(first.code==UserLibraryReadCode::empty && first.active_slot==255);
  assert(first.generation==0 && error.empty());
  assert(flash.empty()); // inspection must not create a namespace

  // Build a decoder from real, labelled 433 MHz capture examples.
  std::vector<CapturedFrame> recordings;
  for(int n: {499,500,501}) {
    CapturedFrame frame;
    frame.channel=1;frame.kind=FrameKind::RAW;frame.label="on";
    frame.raw={n,-400,n,-400,n,-400,n,-400};
    recordings.push_back(frame);
  }
  UserDecoder d;
  assert(UserLibrary::codebook("user_living","Living RF remote",recordings,d,error));
  assert(original.upsert_decoder(d,error));
  assert(original.enable_decoder("user_living",true,error));
  uint32_t committed=99;
  assert(UserLibraryNvs::save(original,0,committed,error) && committed==1);
  assert(flash.size()==1);
  assert(flash.count(UserLibraryNvs::kNamespace)==1);
  auto& ns=flash[UserLibraryNvs::kNamespace];
  assert(ns.count(UserLibraryNvs::kSlotA)==1 && ns.count(UserLibraryNvs::kSlotB)==0);
  UserLibrary got;
  auto status=UserLibraryNvs::load(got,error);
  assert(status.code==UserLibraryReadCode::ready && status.active_slot==0);
  assert(status.generation==1 && got.decoder("user_living")->enabled);

  UserProfile profile;
  profile.id="user_living_profile";profile.name="Living RF";
  profile.protocol="user_living";profile.expected_actions={"on"};
  profile.controls={"on"};profile.pair_method="manual";
  assert(original.upsert_profile(profile,error));
  assert(original.commit_state({{"device:rolling",7}}));
  assert(UserLibraryNvs::save(original,1,committed,error) && committed==2);
  assert(ns.count(UserLibraryNvs::kSlotB)==1);
  status=UserLibraryNvs::load(got,error);
  assert(status.code==UserLibraryReadCode::ready && status.active_slot==1);
  assert(status.a_valid && status.b_valid && status.generation==2);
  assert(got.profiles().size()==1 && got.state().at("device:rolling")==7);

  // The server may not overwrite another edit based on an old generation.
  assert(!UserLibraryNvs::save(original,1,committed,error));
  assert(error.find("reload")!=std::string::npos);

  auto with_newer_counter=original;
  assert(with_newer_counter.commit_state({{"device:rolling",8}}));
  fail_set=true;
  assert(!UserLibraryNvs::save(with_newer_counter,2,committed,error));
  fail_set=false;
  assert(UserLibraryNvs::load(got,error).generation==2);
  fail_commit=true;
  assert(!UserLibraryNvs::save(with_newer_counter,2,committed,error));
  fail_commit=false;
  assert(UserLibraryNvs::load(got,error).generation==2);
  assert(ns.count(UserLibraryNvs::kSlotA)==1);

  // A replacement without the high watermark must never roll the counter back.
  UserLibrary removes_counter;
  assert(!UserLibraryNvs::save(removes_counter,2,committed,error));
  assert(error.find("roll back")!=std::string::npos);
  assert(UserLibraryNvs::save(with_newer_counter,2,committed,error) && committed==3);
  assert(UserLibraryNvs::load(got,error).generation==3);
  assert(got.state().at("device:rolling")==8);

  fail_read=true;
  UserLibrary kept=got;
  status=UserLibraryNvs::load(kept,error);
  assert(status.code==UserLibraryReadCode::io_error);
  assert(kept.state().at("device:rolling")==8); // failed read leaves output unchanged
  assert(!UserLibraryNvs::save(with_newer_counter,3,committed,error));
  fail_read=false;

  // Simulated torn/altered commit: readback catches a bad write.
  const auto saved=flash;
  auto newer=with_newer_counter;
  assert(newer.commit_state({{"device:rolling",9}}));
  corrupt_commit=true;
  assert(!UserLibraryNvs::save(newer,3,committed,error));
  assert(error.find("verify")!=std::string::npos);
  corrupt_commit=false;
  status=UserLibraryNvs::load(got,error);
  assert(status.code==UserLibraryReadCode::recovered);
  assert(status.generation==3); // old slot still valid
  assert(got.state().at("device:rolling")==8);
  assert(!UserLibraryNvs::save(newer,3,committed,error)); // no silent fallback TX rollback
  assert(error.find("manual review")!=std::string::npos);
  flash=saved;

  // A physically corrupted bank cannot silently erase the good library.
  ns[UserLibraryNvs::kSlotA][18]^=1; // target active generation 3
  status=UserLibraryNvs::load(got,error);
  assert(status.code==UserLibraryReadCode::recovered);
  assert(status.generation==2 && got.state().at("device:rolling")==7);
  assert(!UserLibraryNvs::save(got,2,committed,error));
  ns[UserLibraryNvs::kSlotB][18]^=1;
  UserLibrary unchanged=got;
  status=UserLibraryNvs::load(unchanged,error);
  assert(status.code==UserLibraryReadCode::corrupt);
  assert(unchanged.state().at("device:rolling")==7);
  assert(!UserLibraryNvs::save(original,0,committed,error));
  flash=saved;
  // Equal bank generation is ambiguous; never silently choose a winner.
  ns[UserLibraryNvs::kSlotA]=ns[UserLibraryNvs::kSlotB];
  status=UserLibraryNvs::load(unchanged,error);
  assert(status.code==UserLibraryReadCode::corrupt);
  assert(!UserLibraryNvs::save(original,3,committed,error));
  return 0;
}
'''

class NativeUserLibraryNvsTests(unittest.TestCase):
    def test_contract_and_isolation(self):
        source=SRC.read_text()
        header=(INC/"rflink_user_library_nvs.hpp").read_text()
        cmake=(CORE/"CMakeLists.txt").read_text()
        self.assertIn('"src/rflink_user_library_nvs.cpp"',cmake)
        self.assertIn("nvs_flash",cmake)
        self.assertIn('kNamespace="rf_user_library"',header)
        for key in ("kSlotA","kSlotB","NVS_READONLY","NVS_READWRITE",
                    "nvs_set_blob","nvs_commit","verify_readback",
                    "expected_generation","UserLibraryReadCode::recovered"):
            self.assertTrue(key in source or key in header,key)
        self.assertNotIn("nvs_flash_erase",source)
        self.assertNotIn("nvs_erase",source)
        self.assertNotIn("rmt_transmit",source)
        self.assertNotIn("gpio_set_level",source)

    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_real_native_backend_with_power_loss_faults(self):
        with tempfile.TemporaryDirectory() as temp:
            test=pathlib.Path(temp)/"nvs_host.cpp"
            exe=pathlib.Path(temp)/"nvs_host"
            test.write_text(TEST_CPP)
            cmd=["g++","-std=c++17","-Wall","-Wextra","-Werror",
                 "-I"+str(INC),"-I"+str(STUBS),str(test),str(SRC),
                 str(LIB),str(VM),"-pthread","-o",str(exe)]
            compiled=subprocess.run(cmd,capture_output=True,text=True,timeout=75)
            self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
            ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
            self.assertEqual(ran.returncode,0,ran.stdout+ran.stderr)

if __name__=="__main__":
    unittest.main()
