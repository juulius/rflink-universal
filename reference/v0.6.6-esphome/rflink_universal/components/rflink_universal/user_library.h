#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "developer_session.h"
#include "user_vm.h"

namespace esphome {
namespace rflink_universal {

struct UserDecoder {
  std::string id;
  std::string name;
  uint8_t radio{255};
  FrameKind kind{FrameKind::RAW};
  std::string rx_source;
  std::string tx_source;
  std::vector<std::string> actions;
  std::vector<CapturedFrame> samples;
  uint16_t version{1};
  bool enabled{false};
  user_vm::Program rx;
  user_vm::Program tx;
};

struct UserProfile {
  std::string id;
  std::string name;
  std::string protocol;
  std::vector<std::string> expected_actions;
  std::string learn_help;
  uint16_t version{1}; // keep aggregate v1 initializers compatible
  std::vector<std::string> controls;
  std::vector<std::string> setup_steps;
  std::string pair_method{"none"};
  UserProfile() = default;
  UserProfile(std::string p_id, std::string p_name, std::string p_protocol,
              std::vector<std::string> p_actions, std::string p_help, uint16_t p_version)
      : id(std::move(p_id)), name(std::move(p_name)), protocol(std::move(p_protocol)),
        expected_actions(std::move(p_actions)), learn_help(std::move(p_help)), version(p_version) {}
};

class UserLibrary {
 public:
  static constexpr size_t MAX_DECODERS = 8;
  static constexpr size_t MAX_PROFILES = 16;
  static constexpr size_t MAX_SAMPLES = 12;
  static constexpr size_t MAX_STATE = 32;
  const std::vector<UserDecoder> &decoders() const { return decoders_; }
  const std::vector<UserProfile> &profiles() const { return profiles_; }
  const std::map<std::string, uint32_t> &state() const { return state_; }
  const UserDecoder *decoder(const std::string &id) const;
  bool upsert_decoder(UserDecoder value, std::string &error);
  bool enable_decoder(const std::string &id, bool enabled, std::string &error);
  bool remove_decoder(const std::string &id);
  bool upsert_profile(UserProfile value, std::string &error);
  bool remove_profile(const std::string &id);
  bool commit_state(const std::map<std::string, uint32_t> &updates);
  bool replay(const UserDecoder &decoder, std::string &error) const;
  static uint32_t fingerprint(const UserDecoder &decoder);
  // Generate a fixed-code recipe from labelled captures; validate by replay.
  static bool codebook(const std::string &id, const std::string &name,
                       const std::vector<CapturedFrame> &samples, UserDecoder &out,
                       std::string &error);
 private:
  friend class UserLibraryPersistence;
  std::vector<UserDecoder> decoders_;
  std::vector<UserProfile> profiles_;
  std::map<std::string, uint32_t> state_;
};

// Two complete snapshots; a failed write leaves the previous generation live.
// Unlike the existing device data these snapshots contain only user library
// content, and normal ESPHome OTA does not update either preference key.
class UserLibraryPersistence {
 public:
  static constexpr size_t CAPACITY = 16384;
  static constexpr uint32_t MAGIC = 0x52464C42u;
  static constexpr uint16_t VERSION = 2;  // Reads persisted v1 and v2 snapshots
  struct Blob {
    uint32_t magic{MAGIC};
    uint16_t version{VERSION};
    uint16_t used{0};
    uint32_t generation{0};
    uint32_t checksum{0};
    uint8_t bytes[CAPACITY]{};
  };
  static bool valid(const Blob &blob);
  static bool encode(const UserLibrary &library, uint32_t generation, Blob &blob);
  static bool decode(const Blob &blob, UserLibrary &library);
};

}  // namespace rflink_universal
}  // namespace esphome
