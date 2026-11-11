#include "rflink_user_library_nvs.hpp"
#include "nvs.h"
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

namespace rflink {
namespace {
constexpr const char* kKeys[] = {UserLibraryNvs::kSlotA,UserLibraryNvs::kSlotB};
std::mutex library_nvs_mutex;

struct Slot {
  bool present=false;
  bool valid=false;
  uint32_t generation=0;
  UserLibrary library;
};
std::unique_ptr<UserLibraryPersistence::Blob> make_blob() {
  // The on-flash snapshot is 16 KiB; never allocate it on the RX task stack.
  return std::unique_ptr<UserLibraryPersistence::Blob>(
      new (std::nothrow) UserLibraryPersistence::Blob{});
}
bool read_slot(nvs_handle_t handle,unsigned index,Slot& slot,std::string& error) {
  size_t length=0;
  const esp_err_t size_error=nvs_get_blob(handle,kKeys[index],nullptr,&length);
  if(size_error==ESP_ERR_NVS_NOT_FOUND) return true;
  if(size_error!=ESP_OK) {
    error="NVS snapshot metadata read failed: "+std::to_string(size_error);return false;
  }
  slot.present=true;
  if(length!=sizeof(UserLibraryPersistence::Blob)) return true; // invalid bank
  auto blob=make_blob();
  if(!blob){error="Not enough heap for library snapshot";return false;}
  const esp_err_t read_error=nvs_get_blob(handle,kKeys[index],blob.get(),&length);
  if(read_error!=ESP_OK) {
    error="NVS snapshot read failed: "+std::to_string(read_error);return false;
  }
  if(length!=sizeof(*blob) || !UserLibraryPersistence::valid(*blob))return true;
  UserLibrary decoded;
  if(!UserLibraryPersistence::decode(*blob,decoded))return true;
  slot.valid=true;
  slot.generation=blob->generation;
  slot.library=std::move(decoded);
  return true;
}
UserLibraryReadResult load_unlocked(UserLibrary& output,std::string& error) {
  error.clear();
  UserLibraryReadResult result{};
  nvs_handle_t handle{};
  const esp_err_t open_error=nvs_open(UserLibraryNvs::kNamespace,NVS_READONLY,&handle);
  if(open_error==ESP_ERR_NVS_NOT_FOUND) {
    result.code=UserLibraryReadCode::empty;
    return result;
  }
  if(open_error!=ESP_OK) {
    error="NVS library open failed: "+std::to_string(open_error);
    return result;
  }
  Slot slots[2]{};
  const bool read_a=read_slot(handle,0,slots[0],error);
  const bool read_b=read_a && read_slot(handle,1,slots[1],error);
  nvs_close(handle);
  if(!read_a || !read_b)return result; // io_error, unchanged output
  result.a_present=slots[0].present;
  result.b_present=slots[1].present;
  result.a_valid=slots[0].valid;
  result.b_valid=slots[1].valid;
  if(!slots[0].valid && !slots[1].valid) {
    result.code=(!slots[0].present && !slots[1].present)
        ?UserLibraryReadCode::empty:UserLibraryReadCode::corrupt;
    if(result.code==UserLibraryReadCode::corrupt)
      error="No valid library snapshot; refusing an automatic reset";
    return result;
  }
  // Equal nonzero generations indicate diverged/duplicated slots, not a
  // well-ordered A/B checkpoint. Do not silently guess which one won.
  if(slots[0].valid && slots[1].valid &&
     slots[0].generation==slots[1].generation) {
    result.code=UserLibraryReadCode::corrupt;
    error="Two library snapshots have an ambiguous identical generation";
    return result;
  }
  const unsigned selected=(!slots[0].valid) ? 1u :
     (!slots[1].valid || slots[0].generation>slots[1].generation) ? 0u : 1u;
  result.generation=slots[selected].generation;
  result.active_slot=static_cast<uint8_t>(selected);
  result.code=(slots[1-selected].present && !slots[1-selected].valid)
      ?UserLibraryReadCode::recovered:UserLibraryReadCode::ready;
  output=std::move(slots[selected].library); // output only changes on valid load
  if(result.code==UserLibraryReadCode::recovered)
    error="Recovered library from the remaining valid NVS snapshot";
  return result;
}
bool verify_readback(unsigned slot,const UserLibraryPersistence::Blob& expected,
                     std::string& error) {
  nvs_handle_t handle{};
  const auto open_error=nvs_open(UserLibraryNvs::kNamespace,NVS_READONLY,&handle);
  if(open_error!=ESP_OK) {
    error="NVS verification open failed: "+std::to_string(open_error);return false;
  }
  auto actual=make_blob();
  if(!actual) {
    nvs_close(handle);
    error="Not enough heap to verify saved library";return false;
  }
  size_t length=sizeof(*actual);
  const auto e=nvs_get_blob(handle,kKeys[slot],actual.get(),&length);
  nvs_close(handle);
  if(e!=ESP_OK || length!=sizeof(*actual) ||
     !UserLibraryPersistence::valid(*actual) ||
     std::memcmp(actual.get(),&expected,sizeof(expected))!=0) {
    error="Saved library did not verify after NVS commit";return false;
  }
  return true;
}
} // namespace

UserLibraryReadResult UserLibraryNvs::load(UserLibrary& output,std::string& error) {
  std::lock_guard<std::mutex> guard(library_nvs_mutex);
  return load_unlocked(output,error);
}

bool UserLibraryNvs::save(const UserLibrary& library,uint32_t expected_generation,
                          uint32_t& committed_generation,std::string& error) {
  std::lock_guard<std::mutex> guard(library_nvs_mutex);
  error.clear();
  committed_generation=0;
  UserLibrary existing;
  std::string read_error;
  const auto current=load_unlocked(existing,read_error);
  if(current.code==UserLibraryReadCode::io_error ||
     current.code==UserLibraryReadCode::corrupt) {
    error=read_error;return false;
  }
  if(current.generation!=expected_generation) {
    error="Library changed; reload before saving";
    return false;
  }
  if(expected_generation==UINT32_MAX) {
    error="Library generation exhausted";return false;
  }
  // TX/rf counters never move backwards or disappear, including when callers
  // submit a replacement snapshot rather than a normal UserLibrary edit.
  for(const auto& [key,value]:existing.state()) {
    const auto it=library.state().find(key);
    if(it==library.state().end() || it->second<value) {
      error="Cannot remove or roll back stored protocol counters";
      return false;
    }
  }
  auto proposed=make_blob();
  if(!proposed) {error="Not enough heap to encode library";return false;}
  const uint32_t next_generation=expected_generation+1;
  if(!UserLibraryPersistence::encode(library,next_generation,*proposed)) {
    error="Library exceeds snapshot capacity or contains invalid data";
    return false;
  }
  // Ensure that the persisted representation is parseable before overwriting
  // even the inactive slot. This is not authorization to transmit.
  UserLibrary check;
  if(!UserLibraryPersistence::decode(*proposed,check)) {
    error="Proposed library failed snapshot validation";return false;
  }
  const unsigned target=current.active_slot==0?1u:0u;
  nvs_handle_t handle{};
  const auto open_error=nvs_open(kNamespace,NVS_READWRITE,&handle);
  if(open_error!=ESP_OK) {
    error="NVS library write-open failed: "+std::to_string(open_error);return false;
  }
  esp_err_t write_error=nvs_set_blob(handle,kKeys[target],proposed.get(),sizeof(*proposed));
  if(write_error==ESP_OK)write_error=nvs_commit(handle);
  nvs_close(handle);
  if(write_error!=ESP_OK) {
    error="NVS library commit failed: "+std::to_string(write_error);return false;
  }
  if(!verify_readback(target,*proposed,error))return false;
  committed_generation=next_generation;
  return true;
}

} // namespace rflink
