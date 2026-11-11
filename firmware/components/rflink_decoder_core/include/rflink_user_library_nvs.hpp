#pragma once
#include <cstdint>
#include <string>
#include "rflink_user_library.hpp"

namespace rflink {

// Read-only inspection never changes flash or the caller's library on failure.
// A missing namespace is a clean first boot, NOT a corrupted stored library.
enum class UserLibraryReadCode : uint8_t {
  empty=0, ready=1, recovered=2, corrupt=3, io_error=4
};
struct UserLibraryReadResult {
  UserLibraryReadCode code=UserLibraryReadCode::io_error;
  uint32_t generation=0;
  uint8_t active_slot=255; // 0=lib_a, 1=lib_b, 255=none
  bool a_valid=false, b_valid=false;
  bool a_present=false, b_present=false;
};

// Native ESP-IDF NVS A/B backend for the bounded v0.6.6-compatible codec.
// No auto-import from ESPHome, no erasure, no RF/HTTP/TX operations.
// The caller remains responsible for serializing edits and publishing them
// only after a confirmed durable save. This backend also serializes its own
// reads/writes and checks the expected generation to prevent stale updates.
class UserLibraryNvs {
 public:
  static constexpr const char* kNamespace="rf_user_library";
  static constexpr const char* kSlotA="lib_a";
  static constexpr const char* kSlotB="lib_b";

  static UserLibraryReadResult load(UserLibrary& output, std::string& error);

  // Writes the *other* slot, commits and verifies exact persisted bytes.
  // expected_generation=0 is only accepted on clean, never-written NVS.
  // On any failure the previous valid bank is left untouched.
  // A failure after commit is ambiguous: reload before attempting a retry.
  static bool save(const UserLibrary& library, uint32_t expected_generation,
                   uint32_t& committed_generation, std::string& error);
};

} // namespace rflink
