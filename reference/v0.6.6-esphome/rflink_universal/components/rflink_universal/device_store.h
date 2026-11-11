#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "decoder.h"

namespace esphome {
namespace rflink_universal {

struct DeviceRecord {
  std::string uid;
  std::string name;
  std::string protocol;
  std::string device_type;
  uint8_t channel{255}; // transient last known RX radio, not persisted in v1 device blob
  std::string profile_id;  // Optional user profile selected for this saved device.
  IdentityConfidence identity_confidence{IdentityConfidence::LOW};
  std::vector<std::string> rf_identities;
  std::vector<std::string> actions;
  std::vector<FieldValue> fields;
  // Protocol-owned persistent TX state (for example Somfy virtual remote address
  // and rolling counter). This is never exposed as a Home Assistant sensor.
  std::vector<FieldValue> tx_state;
  bool pairing_supported{false};
  bool transmit_supported{false};
  bool transmit_enabled{false};  // Opt-in: never inferred from receive-only capabilities.
  bool local_transmit_enabled{false};  // Separate approval for user-decoder web TX.
  bool home_assistant_enabled{false};
  uint32_t last_seen_ms{0};
};

struct CandidateRecord {
  std::string identity_key;
  uint8_t channel{255};
  std::string protocol;
  std::string device_type;
  IdentityConfidence identity_confidence{IdentityConfidence::LOW};
  uint16_t observations{0};
  uint32_t first_seen_ms{0};
  uint32_t last_seen_ms{0};
  std::vector<std::string> actions;
  std::vector<FieldValue> fields;
  // Protocol-owned persistent TX state (for example Somfy virtual remote address
  // and rolling counter). This is never exposed as a Home Assistant sensor.
  std::vector<FieldValue> tx_state;
  bool pairing_supported{false};
  bool transmit_supported{false};
};

// Unapproved capabilities are runtime proposals, never persisted and NEVER
// included in MQTT Discovery. Existing approved fields still receive values.
struct PendingCapabilities {
  std::vector<std::string> actions;
  std::vector<FieldValue> fields;
};

class DeviceStore {
 public:
  explicit DeviceStore(uint16_t candidate_threshold = 3) : candidate_threshold_(candidate_threshold) {}

  DeviceRecord *observe(const DecodeResult &result, uint32_t now_ms, uint8_t channel = 255);
  DeviceRecord *find_by_identity(const std::string &identity);
  const DeviceRecord *find_by_identity(const std::string &identity) const;
  DeviceRecord *find_by_uid(const std::string &uid);
  const DeviceRecord *find_by_uid(const std::string &uid) const;
  CandidateRecord *find_candidate(const std::string &identity);
  const CandidateRecord *find_candidate(const std::string &identity) const;

  bool candidate_ready(const std::string &identity) const;
  // Optional, explicit approval lists from the first-enrolment wizard. Null
  // means the historical API behaviour; empty means approve no public actions
  // or HA fields. RF protocol metadata stays internal and is never HA-published.
  DeviceRecord *confirm_candidate(const std::string &identity, const std::string &name = "",
                                  const std::vector<std::string> *approved_actions = nullptr,
                                  const std::vector<std::string> *approved_fields = nullptr);
  bool link_candidate(const std::string &identity, const std::string &existing_uid);
  bool dismiss_candidate(const std::string &identity);
  const DeviceRecord *suggest_similar(const CandidateRecord &candidate) const;

  // User-approved changes only; candidates and RF history are intentionally volatile.
  bool rename_device(const std::string &uid, const std::string &name);
  bool remove_device(const std::string &uid);
  bool approve_action(const std::string &uid, const std::string &action);
  bool approve_field(const std::string &uid, const std::string &key);
  bool remove_action(const std::string &uid, const std::string &action);
  bool remove_field(const std::string &uid, const std::string &key);
  const PendingCapabilities *pending(const std::string &uid) const;
  // Never leave a saved device with zero RF identities: remove the complete
  // device explicitly instead. A replacement must be a confirmed capture,
  // not an arbitrary manually entered RF identifier.
  bool unlink_identity(const std::string &uid, const std::string &identity);
  bool replace_identity(const std::string &uid, const std::string &previous_identity,
                        const std::string &candidate_identity);
  bool restore_devices(std::vector<DeviceRecord> restored);
  const std::vector<DeviceRecord> &devices() const { return devices_; }
  const std::vector<CandidateRecord> &candidates() const { return candidates_; }
  void set_candidate_threshold(uint16_t value) { candidate_threshold_ = value == 0 ? 1 : value; }

 private:
  static void merge_action_(std::vector<std::string> &actions, const std::string &action);
  static void merge_fields_(std::vector<FieldValue> &fields, const std::vector<FieldValue> &incoming);
  void apply_(DeviceRecord &device, const DecodeResult &result, uint32_t now_ms);
  void propose_action_(const DeviceRecord &device, const std::string &action);
  void propose_fields_(DeviceRecord &device, const std::vector<FieldValue> &fields);
  static void apply_(CandidateRecord &candidate, const DecodeResult &result, uint32_t now_ms);
  std::string next_uid_();

  uint16_t candidate_threshold_{3};
  uint32_t uid_counter_{1};
  std::vector<DeviceRecord> devices_;
  std::vector<CandidateRecord> candidates_;
  std::map<std::string, PendingCapabilities> pending_;
};

}  // namespace rflink_universal
}  // namespace esphome
