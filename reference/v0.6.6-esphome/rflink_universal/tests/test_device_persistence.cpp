#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "device_persistence.h"

using namespace esphome::rflink_universal;

static uint32_t legacy_checksum(const DevicePersistence::Blob &blob) {
  uint32_t value = 2166136261u;
  auto step = [&](uint8_t byte) { value = (value ^ byte) * 16777619u; };
  for (int bit = 0; bit < 32; bit += 8) step(uint8_t(blob.magic >> bit));
  step(uint8_t(blob.version)); step(uint8_t(blob.version >> 8));
  step(uint8_t(blob.used)); step(uint8_t(blob.used >> 8));
  for (int bit = 0; bit < 32; bit += 8) step(uint8_t(blob.generation >> bit));
  for (size_t i = 0; i < blob.used; ++i) step(blob.bytes[i]);
  return value;
}

static DecodeResult input(const std::string &identity) {
  DecodeResult r;
  r.valid = true;
  r.protocol = "Acurite";
  r.device_type = "thermometer";
  r.identity_key = identity;
  r.action = "temperature";
  r.identity_confidence = IdentityConfidence::HIGH;
  r.fields = {{"temperature", "20.1", "C"}, {"battery", "ok", ""}};
  r.pairing_supported = true;
  return r;
}

int main() {
  DeviceStore original(3);
  for (int i = 0; i < 3; ++i) original.observe(input("acurite:001"), 100 + i, 1);
  auto *dev = original.confirm_candidate("acurite:001", "Tuin sensor");
  assert(dev && dev->uid == "dev_00001");
  dev->home_assistant_enabled = true;
  for (int i = 0; i < 3; ++i) original.observe(input("acurite:002"), 1000 + i, 1);
  assert(original.link_candidate("acurite:002", "dev_00001"));
  auto blob = std::make_unique<DevicePersistence::Blob>();
  assert(DevicePersistence::encode(original, 1, *blob));
  assert(DevicePersistence::valid(*blob));
  // v3 snapshots lack the native TX-state count. v1/v2 also lack the
  // trailing profile length. All older snapshots must remain readable.
  auto legacy = std::make_unique<DevicePersistence::Blob>(*blob);
  assert(legacy->bytes[legacy->used - 1] == 0);
  --legacy->used;  // remove v4 tx_state count
  legacy->version = 3;
  legacy->checksum = legacy_checksum(*legacy);
  DeviceStore from_v3;
  assert(DevicePersistence::decode(*legacy, from_v3));
  assert(from_v3.devices().front().profile_id.empty() && from_v3.devices().front().tx_state.empty());
  assert(legacy->bytes[legacy->used - 1] == 0);
  --legacy->used;  // remove v3 profile length
  legacy->version = 2;
  legacy->checksum = legacy_checksum(*legacy);
  DeviceStore from_v2;
  assert(DevicePersistence::decode(*legacy, from_v2));
  assert(from_v2.devices().front().profile_id.empty());
  legacy->version = 1;
  legacy->checksum = legacy_checksum(*legacy);
  DeviceStore from_v1;
  assert(DevicePersistence::decode(*legacy, from_v1));
  assert(from_v1.devices().front().profile_id.empty() && !from_v1.devices().front().local_transmit_enabled);
  dev->profile_id = "custom_thermometer";
  dev->local_transmit_enabled = true;
  assert(DevicePersistence::encode(original, 2, *blob));
  DeviceStore with_profile;
  assert(DevicePersistence::decode(*blob, with_profile));
  assert(with_profile.devices().front().profile_id == "custom_thermometer");
  assert(with_profile.devices().front().local_transmit_enabled);
  dev->profile_id.clear();
  dev->local_transmit_enabled = false;
  assert(DevicePersistence::encode(original, 1, *blob));
  DeviceStore rebooted;
  assert(DevicePersistence::decode(*blob, rebooted));
  assert(rebooted.devices().size() == 1);
  auto *restored = rebooted.find_by_uid("dev_00001");
  assert(restored && restored->name == "Tuin sensor" && restored->rf_identities.size() == 2);
  assert(restored->fields.size() == 2 && restored->pairing_supported && restored->home_assistant_enabled);
  assert(restored->last_seen_ms == 0);
  assert(rebooted.rename_device(restored->uid, "Buiten"));
  assert(rebooted.devices().front().name == "Buiten");
  // Manage RF identities without deleting the device or silently accepting
  // arbitrary/unverified identifiers. These edits must survive serialization.
  assert(!rebooted.unlink_identity("dev_00001", "not-linked"));
  assert(!rebooted.replace_identity("dev_00001", "acurite:001", "acurite:003"));
  for (int i = 0; i < 2; ++i) rebooted.observe(input("acurite:003"), 2000 + i, 1);
  assert(!rebooted.replace_identity("dev_00001", "acurite:001", "acurite:003"));
  rebooted.observe(input("acurite:003"), 2002, 1);
  assert(rebooted.replace_identity("dev_00001", "acurite:001", "acurite:003"));
  assert(rebooted.find_by_identity("acurite:001") == nullptr);
  assert(rebooted.find_by_identity("acurite:003") != nullptr);
  assert(rebooted.find_candidate("acurite:003") == nullptr);
  assert(!rebooted.replace_identity("dev_00001", "acurite:002", "acurite:003"));
  auto mismatch = input("acurite:004"); mismatch.protocol = "Different protocol";
  for (int i = 0; i < 3; ++i) rebooted.observe(mismatch, 3000 + i, 1);
  assert(!rebooted.replace_identity("dev_00001", "acurite:002", "acurite:004"));
  auto signals_saved = std::make_unique<DevicePersistence::Blob>();
  assert(DevicePersistence::encode(rebooted, 2, *signals_saved));
  DeviceStore restored_signals;
  assert(DevicePersistence::decode(*signals_saved, restored_signals));
  assert(restored_signals.find_by_identity("acurite:003") != nullptr);
  assert(restored_signals.find_by_identity("acurite:001") == nullptr);
  assert(restored_signals.unlink_identity("dev_00001", "acurite:002"));
  assert(restored_signals.find_by_uid("dev_00001") != nullptr);
  assert(restored_signals.find_by_uid("dev_00001")->rf_identities.size() == 1);
  assert(!restored_signals.unlink_identity("dev_00001", "acurite:003"));
  assert(restored_signals.find_by_identity("acurite:003") != nullptr);
  auto second = std::make_unique<DevicePersistence::Blob>();
  assert(DevicePersistence::encode(rebooted, 2, *second));
  assert(second->generation > blob->generation);
  assert(rebooted.remove_device("dev_00001"));
  assert(DevicePersistence::encode(rebooted, 3, *blob));
  DeviceStore emptied;
  assert(DevicePersistence::decode(*blob, emptied) && emptied.devices().empty());
  assert(!rebooted.remove_device("dev_00001"));
  // Reject corrupt data without modifying the current device store.
  second->bytes[4] ^= 0x08;
  assert(!DevicePersistence::valid(*second));
  assert(!DevicePersistence::decode(*second, original));
  assert(original.devices().size() == 1);
  // Preserve post-reboot next UID, not reusing an existing ID.
  for (int i = 0; i < 3; ++i) original.observe(input("acurite:003"), 5000 + i, 1);
  const auto *added = original.confirm_candidate("acurite:003", "Third");
  assert(added && added->uid == "dev_00002");
  // Never truncate a too-long field silently.
  DeviceStore oversized;
  for (int i = 0; i < 3; ++i) oversized.observe(input("id"), 100 + i);
  auto *too_long = oversized.confirm_candidate("id", "Valid");
  assert(too_long);
  too_long->name = std::string(256, 'x');
  assert(!DevicePersistence::encode(oversized, 1, *blob));
  // A corrupt checksum and unknown schema must not pass validation.
  assert(!DevicePersistence::valid(*blob));
  std::cout << "Device persistence: serialize, restore, verified RF-code replacement, unlink guard, fields, UID, delete, corruption, bounds: PASS\n";
}
