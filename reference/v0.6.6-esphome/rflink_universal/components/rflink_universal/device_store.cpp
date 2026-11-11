#include "device_store.h"

#include <algorithm>
#include <cstdio>
#include "capability_catalog.h"

namespace esphome {
namespace rflink_universal {

void DeviceStore::merge_action_(std::vector<std::string> &actions, const std::string &action) {
  if (action.empty()) return;
  if (std::find(actions.begin(), actions.end(), action) == actions.end()) actions.push_back(action);
}

void DeviceStore::merge_fields_(std::vector<FieldValue> &fields, const std::vector<FieldValue> &incoming) {
  for (const auto &field : incoming) {
    auto it = std::find_if(fields.begin(), fields.end(), [&](const FieldValue &f) { return f.key == field.key; });
    if (it == fields.end()) fields.push_back(field);
    else *it = field;
  }
}

void DeviceStore::propose_action_(const DeviceRecord &device, const std::string &action) {
  if (action.empty() || action.size() > 255 ||
      std::find(device.actions.begin(), device.actions.end(), action) != device.actions.end()) return;
  auto &p = pending_[device.uid];
  if (p.actions.size() < 32) merge_action_(p.actions, action);
}

void DeviceStore::propose_fields_(DeviceRecord &device, const std::vector<FieldValue> &fields) {
  for (const auto &field : fields) {
    auto it = std::find_if(device.fields.begin(), device.fields.end(),
                           [&](const FieldValue &f) { return f.key == field.key; });
    if (it != device.fields.end()) { *it = field; continue; } // readings only, no topology change
    if (field.key.empty() || field.key.size() > 255 || field.value.size() > 255 || field.unit.size() > 255) continue;
    auto &p = pending_[device.uid];
    auto proposal = std::find_if(p.fields.begin(), p.fields.end(),
                                 [&](const FieldValue &f) { return f.key == field.key; });
    if (proposal != p.fields.end()) *proposal = field;
    else if (p.fields.size() < 32) p.fields.push_back(field);
  }
}

void DeviceStore::apply_(DeviceRecord &device, const DecodeResult &result, uint32_t now_ms) {
  const auto *protocol = capabilities::find(device.protocol.c_str());
  const bool sensor = protocol && std::strcmp(protocol->learn_category, "sensor") == 0;
  // Sensor packet kinds (measurement/weather_update) are decoder bookkeeping;
  // they are not physical buttons or new HA entities. New actual measurements
  // still require per-field approval through propose_fields_.
  if (sensor) {
    merge_action_(device.actions, result.action);
    for (const auto &action : result.expected_actions) merge_action_(device.actions, action);
  } else {
    propose_action_(device, result.action);
    for (const auto &action : result.expected_actions) propose_action_(device, action);
  }
  propose_fields_(device, result.fields);
  device.pairing_supported = device.pairing_supported || result.pairing_supported;
  device.transmit_supported = device.transmit_supported || result.transmit_supported;
  device.last_seen_ms = now_ms;
}

void DeviceStore::apply_(CandidateRecord &candidate, const DecodeResult &result, uint32_t now_ms) {
  merge_action_(candidate.actions, result.action);
  for (const auto &action : result.expected_actions) merge_action_(candidate.actions, action);
  merge_fields_(candidate.fields, result.fields);
  candidate.pairing_supported = candidate.pairing_supported || result.pairing_supported;
  candidate.transmit_supported = candidate.transmit_supported || result.transmit_supported;
  candidate.last_seen_ms = now_ms;
}

DeviceRecord *DeviceStore::find_by_identity(const std::string &identity) {
  for (auto &device : devices_) {
    if (std::find(device.rf_identities.begin(), device.rf_identities.end(), identity) != device.rf_identities.end()) return &device;
  }
  return nullptr;
}

const DeviceRecord *DeviceStore::find_by_identity(const std::string &identity) const {
  for (const auto &device : devices_) {
    if (std::find(device.rf_identities.begin(), device.rf_identities.end(), identity) != device.rf_identities.end()) return &device;
  }
  return nullptr;
}

DeviceRecord *DeviceStore::find_by_uid(const std::string &uid) {
  for (auto &device : devices_) if (device.uid == uid) return &device;
  return nullptr;
}

const DeviceRecord *DeviceStore::find_by_uid(const std::string &uid) const {
  for (const auto &device : devices_) if (device.uid == uid) return &device;
  return nullptr;
}

CandidateRecord *DeviceStore::find_candidate(const std::string &identity) {
  for (auto &candidate : candidates_) if (candidate.identity_key == identity) return &candidate;
  return nullptr;
}

const CandidateRecord *DeviceStore::find_candidate(const std::string &identity) const {
  for (const auto &candidate : candidates_) if (candidate.identity_key == identity) return &candidate;
  return nullptr;
}

DeviceRecord *DeviceStore::observe(const DecodeResult &result, uint32_t now_ms, uint8_t channel) {
  if (!result.valid || result.identity_key.empty()) return nullptr;
  if (auto *device = find_by_identity(result.identity_key)) {
    if (channel <= 2) device->channel = channel;
    apply_(*device, result, now_ms);
    return device;
  }
  auto *candidate = find_candidate(result.identity_key);
  if (candidate == nullptr) {
    CandidateRecord created;
    created.identity_key = result.identity_key;
    created.channel = channel;
    created.protocol = result.protocol;
    created.device_type = result.device_type;
    created.identity_confidence = result.identity_confidence;
    created.first_seen_ms = now_ms;
    created.last_seen_ms = now_ms;
    candidates_.push_back(created);
    candidate = &candidates_.back();
  }
  if (channel != 255) candidate->channel = channel;
  candidate->observations++;
  apply_(*candidate, result, now_ms);
  return nullptr;
}

bool DeviceStore::candidate_ready(const std::string &identity) const {
  const auto *candidate = find_candidate(identity);
  return candidate != nullptr && candidate->observations >= candidate_threshold_;
}

std::string DeviceStore::next_uid_() {
  char buffer[20];
  std::snprintf(buffer, sizeof(buffer), "dev_%05lu", static_cast<unsigned long>(uid_counter_++));
  return buffer;
}

DeviceRecord *DeviceStore::confirm_candidate(const std::string &identity, const std::string &name,
                                              const std::vector<std::string> *approved_actions,
                                              const std::vector<std::string> *approved_fields) {
  auto it = std::find_if(candidates_.begin(), candidates_.end(), [&](const CandidateRecord &c) { return c.identity_key == identity; });
  if (it == candidates_.end() || it->observations < candidate_threshold_ || devices_.size() >= 24) return nullptr;
  DeviceRecord device;
  device.uid = next_uid_();
  device.name = name.empty() ? (it->protocol + " " + device.uid) : name;
  device.protocol = it->protocol;
  device.channel = it->channel;
  device.device_type = it->device_type;
  device.identity_confidence = it->identity_confidence;
  device.rf_identities.push_back(it->identity_key);
  const auto *protocol = capabilities::find(it->protocol.c_str());
  const bool sensor = protocol && std::strcmp(protocol->learn_category, "sensor") == 0;
  for (const auto &action : it->actions) {
    if (sensor || !approved_actions || std::find(approved_actions->begin(), approved_actions->end(), action) != approved_actions->end())
      device.actions.push_back(action);
  }
  for (const auto &field : it->fields) {
    // Decoder bookkeeping such as channel, code, rolling counter and TX radio
    // must not be treated as a user-facing HA sensor or silently discarded.
    const bool internal = capabilities::field(field.key.c_str()) == nullptr;
    if (internal || !approved_fields ||
        std::find(approved_fields->begin(), approved_fields->end(), field.key) != approved_fields->end())
      device.fields.push_back(field);
  }
  device.pairing_supported = it->pairing_supported;
  device.transmit_supported = it->transmit_supported;
  device.last_seen_ms = it->last_seen_ms;
  devices_.push_back(device);
  if (approved_actions && !sensor) {
    for (const auto &action : it->actions) propose_action_(devices_.back(), action);
  }
  if (approved_fields) {
    propose_fields_(devices_.back(), it->fields);
  }
  candidates_.erase(it);
  return &devices_.back();
}

bool DeviceStore::dismiss_candidate(const std::string &identity) {
  auto it = std::find_if(candidates_.begin(), candidates_.end(),
                         [&](const CandidateRecord &c) { return c.identity_key == identity; });
  if(it==candidates_.end())return false;
  candidates_.erase(it);
  return true;
}

bool DeviceStore::link_candidate(const std::string &identity, const std::string &existing_uid) {
  auto *device = find_by_uid(existing_uid);
  auto it = std::find_if(candidates_.begin(), candidates_.end(), [&](const CandidateRecord &c) { return c.identity_key == identity; });
  if (device == nullptr || it == candidates_.end() || !candidate_ready(identity) ||
      device->protocol != it->protocol || device->device_type != it->device_type ||
      device->rf_identities.size() >= 32 || identity.size() > 255) return false;
  if (std::find(device->rf_identities.begin(), device->rf_identities.end(), identity) == device->rf_identities.end())
    device->rf_identities.push_back(identity);
  for (const auto &a : it->actions) propose_action_(*device, a);
  propose_fields_(*device, it->fields);
  device->pairing_supported = device->pairing_supported || it->pairing_supported;
  device->transmit_supported = device->transmit_supported || it->transmit_supported;
  device->last_seen_ms = std::max(device->last_seen_ms, it->last_seen_ms);
  candidates_.erase(it);
  return true;
}

bool DeviceStore::rename_device(const std::string &uid, const std::string &name) {
  auto *d = find_by_uid(uid);
  if (d == nullptr || name.empty() || name.size() > 80) return false;
  d->name = name;
  return true;
}

bool DeviceStore::remove_device(const std::string &uid) {
  const auto previous = devices_.size();
  devices_.erase(std::remove_if(devices_.begin(), devices_.end(), [&](const DeviceRecord &d) { return d.uid == uid; }), devices_.end());
  if (previous == devices_.size()) return false;
  pending_.erase(uid);
  return true;
}

const PendingCapabilities *DeviceStore::pending(const std::string &uid) const {
  const auto it = pending_.find(uid);
  return it == pending_.end() ? nullptr : &it->second;
}

bool DeviceStore::approve_action(const std::string &uid, const std::string &action) {
  auto *device = find_by_uid(uid);
  auto it = pending_.find(uid);
  if (!device || it == pending_.end() || device->actions.size() >= 32) return false;
  auto &actions = it->second.actions;
  auto found = std::find(actions.begin(), actions.end(), action);
  if (found == actions.end()) return false;
  merge_action_(device->actions, action);
  actions.erase(found);
  return true;
}

bool DeviceStore::approve_field(const std::string &uid, const std::string &key) {
  auto *device = find_by_uid(uid);
  auto it = pending_.find(uid);
  if (!device || it == pending_.end() || device->fields.size() >= 32) return false;
  auto &fields = it->second.fields;
  auto found = std::find_if(fields.begin(), fields.end(), [&](const FieldValue &f) { return f.key == key; });
  if (found == fields.end()) return false;
  device->fields.push_back(*found);
  fields.erase(found);
  return true;
}

bool DeviceStore::remove_action(const std::string &uid, const std::string &action) {
  auto *device = find_by_uid(uid);
  if (!device) return false;
  auto &actions = device->actions;
  auto found = std::find(actions.begin(), actions.end(), action);
  if (found == actions.end()) return false;
  actions.erase(found);
  return true;
}

bool DeviceStore::remove_field(const std::string &uid, const std::string &key) {
  auto *device = find_by_uid(uid);
  if (!device) return false;
  auto &fields = device->fields;
  auto found = std::find_if(fields.begin(), fields.end(), [&](const FieldValue &f) { return f.key == key; });
  if (found == fields.end()) return false;
  fields.erase(found);
  return true;
}

bool DeviceStore::unlink_identity(const std::string &uid, const std::string &identity) {
  auto *device = find_by_uid(uid);
  if (device == nullptr || device->rf_identities.size() <= 1 || identity.empty()) return false;
  auto it = std::find(device->rf_identities.begin(), device->rf_identities.end(), identity);
  if (it == device->rf_identities.end()) return false;
  device->rf_identities.erase(it);
  // An observed action can be shared by multiple RF IDs. Do not pretend to
  // know which action belongs to the removed identity; retain device history.
  return true;
}

bool DeviceStore::replace_identity(const std::string &uid, const std::string &previous_identity,
                                   const std::string &candidate_identity) {
  auto *device = find_by_uid(uid);
  if (device == nullptr || previous_identity.empty() || candidate_identity.empty() ||
      previous_identity == candidate_identity) return false;
  auto previous = std::find(device->rf_identities.begin(), device->rf_identities.end(), previous_identity);
  if (previous == device->rf_identities.end() || find_by_identity(candidate_identity) != nullptr ||
      !candidate_ready(candidate_identity)) return false;
  auto candidate = std::find_if(candidates_.begin(), candidates_.end(),
                                [&](const CandidateRecord &c) { return c.identity_key == candidate_identity; });
  if (candidate == candidates_.end() || candidate->identity_key.size() > 255 ||
      candidate->protocol != device->protocol || candidate->device_type != device->device_type) return false;
  *previous = candidate_identity;
  for (const auto &action : candidate->actions) propose_action_(*device, action);
  propose_fields_(*device, candidate->fields);
  device->last_seen_ms = std::max(device->last_seen_ms, candidate->last_seen_ms);
  candidates_.erase(candidate);
  return true;
}

bool DeviceStore::restore_devices(std::vector<DeviceRecord> restored) {
  uint32_t maximum = 0;
  for (const auto &d : restored) {
    unsigned long n = 0;
    if (std::sscanf(d.uid.c_str(), "dev_%lu", &n) == 1 && n < UINT32_MAX) maximum = std::max(maximum, static_cast<uint32_t>(n));
  }
  if (maximum == UINT32_MAX) return false;
  devices_ = std::move(restored);
  candidates_.clear();
  pending_.clear();
  uid_counter_ = maximum + 1;
  return true;
}

const DeviceRecord *DeviceStore::suggest_similar(const CandidateRecord &candidate) const {
  // Suggestion only. Never merge automatically. Identity confidence and temporal
  // context should be shown to the user before linking.
  for (const auto &device : devices_) {
    if (device.protocol == candidate.protocol && device.device_type == candidate.device_type) return &device;
  }
  return nullptr;
}

}  // namespace rflink_universal
}  // namespace esphome
