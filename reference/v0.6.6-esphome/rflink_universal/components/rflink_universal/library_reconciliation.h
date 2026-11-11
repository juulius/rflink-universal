#pragma once

#include <string>

namespace esphome {
namespace rflink_universal {
namespace library {

enum class ItemKind { DECODER, DEVICE_PROFILE };
enum class Source { BUILTIN, USER_LEARNED, USER_IMPORTED };
enum class ReconcileState {
  ONLY_BUILTIN,
  ONLY_USER,
  IDENTICAL_DUPLICATE,
  BUILTIN_NEWER,
  USER_NEWER,
  USER_MODIFIED,
  CONFLICT,
};
enum class Decision {
  NONE,
  USE_BUILTIN,
  KEEP_USER,
  KEEP_BOTH,
  ARCHIVE_USER,
};

struct ItemIdentity {
  ItemKind kind{ItemKind::DECODER};
  std::string stable_id;
  unsigned version{1};
  std::string fingerprint;
  Source source{Source::BUILTIN};
  std::string based_on_id;
  unsigned based_on_version{0};
};

struct ReconcileResult {
  ReconcileState state{ReconcileState::CONFLICT};
  Decision recommended{Decision::NONE};
  bool needs_user_attention{true};
  bool safe_auto_resolution{false};
  bool keep_current_active_until_confirmed{true};
};

inline ReconcileResult compare(const ItemIdentity *builtin, const ItemIdentity *user) {
  if (builtin == nullptr && user == nullptr)
    return {ReconcileState::CONFLICT, Decision::NONE, true, false, true};
  if (builtin != nullptr && user == nullptr)
    return {ReconcileState::ONLY_BUILTIN, Decision::USE_BUILTIN, false, true, false};
  if (builtin == nullptr && user != nullptr)
    return {ReconcileState::ONLY_USER, Decision::KEEP_USER, false, true, false};

  // Same stable id and byte/content fingerprint: the local copy adds nothing.
  if (builtin->stable_id == user->stable_id && !builtin->fingerprint.empty() &&
      builtin->fingerprint == user->fingerprint) {
    return {ReconcileState::IDENTICAL_DUPLICATE, Decision::USE_BUILTIN, false, true, false};
  }

  // A user item explicitly based on this built-in item is a local fork once its
  // fingerprint differs. Never overwrite it silently, even when firmware is newer.
  const bool based_on_builtin = !user->based_on_id.empty() && user->based_on_id == builtin->stable_id;
  if (based_on_builtin && user->fingerprint != builtin->fingerprint) {
    return {ReconcileState::USER_MODIFIED, Decision::USE_BUILTIN, true, false, true};
  }

  if (builtin->stable_id != user->stable_id)
    return {ReconcileState::CONFLICT, Decision::NONE, true, false, true};
  if (builtin->version > user->version)
    return {ReconcileState::BUILTIN_NEWER, Decision::USE_BUILTIN, true, false, true};
  if (user->version > builtin->version)
    return {ReconcileState::USER_NEWER, Decision::KEEP_USER, true, false, true};

  // Same id/version but different content must be reviewed; version numbers are
  // metadata, fingerprints are the authoritative content comparison.
  return {ReconcileState::CONFLICT, Decision::NONE, true, false, true};
}

inline bool keep_both_allowed(ItemKind kind) {
  // Two decoder engines with the same stable identity may not run side by side.
  // Device profiles can coexist after the user copy is given a distinct custom id.
  return kind == ItemKind::DEVICE_PROFILE;
}

}  // namespace library
}  // namespace rflink_universal
}  // namespace esphome
