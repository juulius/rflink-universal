#include "device_persistence.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <string>

namespace esphome {
namespace rflink_universal {
namespace {

uint32_t checksum(const DevicePersistence::Blob &b) {
  uint32_t hash = 2166136261u;
  const auto update = [&](uint8_t byte) { hash ^= byte; hash *= 16777619u; };
  for (unsigned shift = 0; shift < 32; shift += 8) update(uint8_t(b.magic >> shift));
  update(uint8_t(b.version)); update(uint8_t(b.version >> 8));
  update(uint8_t(b.used)); update(uint8_t(b.used >> 8));
  for (unsigned shift = 0; shift < 32; shift += 8) update(uint8_t(b.generation >> shift));
  for (size_t i = 0; i < b.used; i++) update(b.bytes[i]);
  return hash;
}

class Writer {
 public:
  explicit Writer(DevicePersistence::Blob &blob) : blob_(blob) {}
  bool byte(uint8_t value) {
    if (blob_.used >= DevicePersistence::CAPACITY) return false;
    blob_.bytes[blob_.used++] = value;
    return true;
  }
  bool str(const std::string &value) {
    if (value.size() > 255 || blob_.used + 1u + value.size() > DevicePersistence::CAPACITY) return false;
    if (!byte(static_cast<uint8_t>(value.size()))) return false;
    for (char c : value) if (!byte(static_cast<uint8_t>(c))) return false;
    return true;
  }
  bool strings(const std::vector<std::string> &values) {
    if (values.size() > 32 || !byte(static_cast<uint8_t>(values.size()))) return false;
    for (const auto &s : values) if (!str(s)) return false;
    return true;
  }
 private:
  DevicePersistence::Blob &blob_;
};

class Reader {
 public:
  explicit Reader(const DevicePersistence::Blob &blob) : blob_(blob) {}
  bool byte(uint8_t &out) {
    if (pos_ >= blob_.used) return false;
    out = blob_.bytes[pos_++]; return true;
  }
  bool str(std::string &out) {
    uint8_t length = 0;
    if (!byte(length) || pos_ + length > blob_.used) return false;
    out.assign(reinterpret_cast<const char *>(blob_.bytes + pos_), length);
    pos_ += length; return true;
  }
  bool strings(std::vector<std::string> &out) {
    uint8_t count = 0;
    if (!byte(count) || count > 32) return false;
    out.clear();
    for (uint8_t i = 0; i < count; ++i) {
      std::string text;
      if (!str(text)) return false;
      out.push_back(std::move(text));
    }
    return true;
  }
  bool finished() const { return pos_ == blob_.used; }
 private:
  const DevicePersistence::Blob &blob_;
  size_t pos_{0};
};

}  // namespace

bool DevicePersistence::valid(const Blob &blob) {
  return blob.magic == MAGIC && blob.version >= 1 && blob.version <= VERSION && blob.used <= CAPACITY &&
         blob.generation > 0 && blob.checksum == checksum(blob);
}

bool DevicePersistence::encode(const DeviceStore &store, uint32_t generation, Blob &blob) {
  if (generation == 0 || store.devices().size() > MAX_DEVICES) return false;
  // A failed attempt must never leave a valid-looking partial blob.
  std::fill_n(blob.bytes, CAPACITY, uint8_t{0});
  blob.used = 0;
  blob.checksum = 0;
  blob.magic = MAGIC;
  blob.version = VERSION;
  blob.generation = generation;
  Writer writer(blob);
  if (!writer.byte(static_cast<uint8_t>(store.devices().size()))) return false;
  for (const auto &d : store.devices()) {
    if (d.uid.empty() || d.name.empty() || d.rf_identities.empty() || d.rf_identities.size() > 32 ||
        d.actions.size() > 32 || d.fields.size() > 32) return false;
    if (!writer.str(d.uid) || !writer.str(d.name) || !writer.str(d.protocol) ||
        !writer.str(d.device_type) || !writer.byte(static_cast<uint8_t>(d.identity_confidence)) ||
        !writer.strings(d.rf_identities) || !writer.strings(d.actions) ||
        !writer.byte(static_cast<uint8_t>(d.fields.size()))) return false;
    for (const auto &field : d.fields) {
      if (!writer.str(field.key) || !writer.str(field.value) || !writer.str(field.unit)) return false;
    }
    const uint8_t flags = (d.pairing_supported ? 1 : 0) | (d.transmit_supported ? 2 : 0) |
                          (d.home_assistant_enabled ? 4 : 0) | (d.transmit_enabled ? 8 : 0) |
                          (d.local_transmit_enabled ? 16 : 0);
    if (!writer.byte(flags) || !writer.str(d.profile_id) || d.tx_state.size() > 32 ||
        !writer.byte(static_cast<uint8_t>(d.tx_state.size()))) return false;
    for (const auto &field : d.tx_state) {
      if (!writer.str(field.key) || !writer.str(field.value) || !writer.str(field.unit)) return false;
    }
  }
  blob.checksum = checksum(blob);
  return true;
}

bool DevicePersistence::decode(const Blob &blob, DeviceStore &store) {
  if (!valid(blob)) return false;
  Reader reader(blob);
  uint8_t count = 0;
  if (!reader.byte(count) || count > MAX_DEVICES) return false;
  std::vector<DeviceRecord> loaded;
  std::set<std::string> uids, identities;
  for (uint8_t i = 0; i < count; ++i) {
    DeviceRecord d;
    uint8_t confidence = 0, fields = 0, flags = 0;
    if (!reader.str(d.uid) || !reader.str(d.name) || !reader.str(d.protocol) ||
        !reader.str(d.device_type) || !reader.byte(confidence) || confidence > static_cast<uint8_t>(IdentityConfidence::HIGH) ||
        !reader.strings(d.rf_identities) || !reader.strings(d.actions) || !reader.byte(fields) || fields > 32) return false;
    for (uint8_t j = 0; j < fields; ++j) {
      FieldValue f;
      if (!reader.str(f.key) || !reader.str(f.value) || !reader.str(f.unit)) return false;
      d.fields.push_back(std::move(f));
    }
    if (!reader.byte(flags) || flags > (blob.version == 1 ? 15 : 31) ||
        (blob.version >= 3 && !reader.str(d.profile_id))) return false;
    if (blob.version >= 4) {
      uint8_t tx_fields = 0;
      if (!reader.byte(tx_fields) || tx_fields > 32) return false;
      for (uint8_t j = 0; j < tx_fields; ++j) {
        FieldValue f;
        if (!reader.str(f.key) || !reader.str(f.value) || !reader.str(f.unit)) return false;
        d.tx_state.push_back(std::move(f));
      }
    }
    if (d.uid.empty() || d.name.empty() || d.rf_identities.empty() ||
        !uids.insert(d.uid).second) return false;
    for (const auto &identity : d.rf_identities) {
      if (identity.empty() || !identities.insert(identity).second) return false;
    }
    d.identity_confidence = static_cast<IdentityConfidence>(confidence);
    d.pairing_supported = (flags & 1) != 0;
    d.transmit_supported = (flags & 2) != 0;
    d.home_assistant_enabled = (flags & 4) != 0;
    d.transmit_enabled = (flags & 8) != 0;
    d.local_transmit_enabled = (flags & 16) != 0;
    d.last_seen_ms = 0;  // millis() timestamps become invalid after a reboot.
    loaded.push_back(std::move(d));
  }
  if (!reader.finished()) return false;
  return store.restore_devices(std::move(loaded));
}

}  // namespace rflink_universal
}  // namespace esphome
