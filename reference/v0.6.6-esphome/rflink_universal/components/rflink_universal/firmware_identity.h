#pragma once
namespace esphome {
namespace rflink_universal {
// Bump the semantic version on each shipped code change; playful new names for minor releases.
static constexpr const char *RFLINK_FIRMWARE_VERSION = "v0.6.6";
static constexpr const char *RFLINK_FIRMWARE_NAME = "Radio Gaga";
// SHA-256 source fingerprint, truncated to 12 hex chars; NOT a Git commit or ELF hash.
static constexpr const char *RFLINK_FIRMWARE_BUILD_HASH = "b9038c00af2f";
static constexpr const char *RFLINK_FIRMWARE_REVISION = "v0.6.6-pairrx2";
}  // namespace rflink_universal
}  // namespace esphome
