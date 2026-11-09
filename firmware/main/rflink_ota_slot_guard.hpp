#pragma once
// Pure, host-testable protection against writing the executing ESP32 app.
// This board revision has TWO fixed 4 MiB OTA app slots. NVS and otadata
// MUST NOT be erased to "repair" a conflict. A different live partition table
// requires a deliberate USB migration with a settings backup.
#include <cstdint>

namespace rflink {
struct OtaSlotDescriptor {
  bool present=false;
  uint8_t subtype=0;
  uint32_t address=0;
  uint32_t size=0;
};
struct OtaSlotPlan {
  bool safe=false;
  int target_index=-1;
  const char* reason="unknown";
};
constexpr OtaSlotPlan choose_native_ota_slot(
    OtaSlotDescriptor running, OtaSlotDescriptor ota0,
    OtaSlotDescriptor ota1) {
  if(!running.present) return {false,-1,"running_partition_unknown"};
  if(!ota0.present || !ota1.present)
    return {false,-1,"missing_second_ota_slot"};
  if(ota0.subtype!=16 || ota1.subtype!=17 ||
     ota0.address!=0x20000 || ota1.address!=0x420000 ||
     ota0.size!=0x400000 || ota1.size!=0x400000)
    return {false,-1,"unexpected_partition_layout"};
  if(running.subtype==ota0.subtype && running.address==ota0.address &&
     running.size==ota0.size) return {true,1,"ok"};
  if(running.subtype==ota1.subtype && running.address==ota1.address &&
     running.size==ota1.size) return {true,0,"ok"};
  return {false,-1,"running_partition_not_ota"};
}
} // namespace rflink
