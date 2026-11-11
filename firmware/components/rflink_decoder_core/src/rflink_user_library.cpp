// Native port of v0.6.6 UserLibrary. Program and snapshot data only.
// No ESPHome, NVS, web endpoints, device commands or RF transmit integration.
// Safety checks are expanded below to reject malformed recordings early.
#include "rflink_user_library.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <sstream>
#include <cmath>

namespace rflink {
namespace {

bool text_ok(const std::string &text, size_t limit) {
  if (text.empty() || text.size() > limit) return false;
  for (unsigned char c : text) if (c < 32 || c == 127) return false;
  return true;
}

uint32_t hash_blob(const UserLibraryPersistence::Blob &blob) {
  uint32_t value = 2166136261u;
  auto step = [&](uint8_t c) { value = (value ^ c) * 16777619u; };
  for (unsigned n = 0; n < 32; n += 8) step(uint8_t(blob.magic >> n));
  step(uint8_t(blob.version)); step(uint8_t(blob.version >> 8));
  step(uint8_t(blob.used)); step(uint8_t(blob.used >> 8));
  for (unsigned n = 0; n < 32; n += 8) step(uint8_t(blob.generation >> n));
  for (size_t i = 0; i < blob.used; ++i) step(blob.bytes[i]);
  return value;
}

struct Writer {
  UserLibraryPersistence::Blob &blob;
  bool byte(uint8_t n) {
    if (blob.used >= UserLibraryPersistence::CAPACITY) return false;
    blob.bytes[blob.used++] = n; return true;
  }
  bool u16(uint16_t n) { return byte(uint8_t(n)) && byte(uint8_t(n >> 8)); }
  bool u32(uint32_t n) { return u16(uint16_t(n)) && u16(uint16_t(n >> 16)); }
  bool str(const std::string &s) {
    if (s.size() > 4096 || blob.used + 2 + s.size() > UserLibraryPersistence::CAPACITY ||
        !u16(uint16_t(s.size()))) return false;
    for (unsigned char c : s) if (!byte(c)) return false;
    return true;
  }
  bool strings(const std::vector<std::string> &s) {
    if (s.size() > 32 || !byte(uint8_t(s.size()))) return false;
    for (const auto &part : s) if (!str(part)) return false;
    return true;
  }
};
struct Reader {
  const UserLibraryPersistence::Blob &blob;
  size_t at{0};
  bool byte(uint8_t &n) { if (at >= blob.used) return false; n = blob.bytes[at++]; return true; }
  bool u16(uint16_t &n) {
    uint8_t a, b; if (!byte(a) || !byte(b)) return false; n = uint16_t(a | (uint16_t(b) << 8)); return true;
  }
  bool u32(uint32_t &n) {
    uint16_t a, b; if (!u16(a) || !u16(b)) return false; n = uint32_t(a) | (uint32_t(b) << 16); return true;
  }
  bool str(std::string &s) {
    uint16_t size; if (!u16(size) || size > 4096 || at + size > blob.used) return false;
    s.assign(reinterpret_cast<const char *>(blob.bytes + at), size); at += size; return true;
  }
  bool strings(std::vector<std::string> &s) {
    uint8_t count; if (!byte(count) || count > 32) return false;
    s.clear();
    while (count--) { std::string value; if (!str(value)) return false; s.push_back(std::move(value)); }
    return true;
  }
};

}  // namespace

const UserDecoder *UserLibrary::decoder(const std::string &id) const {
  for (const auto &item : decoders_) if (item.id == id) return &item;
  return nullptr;
}

bool UserLibrary::replay(const UserDecoder &item, std::string &error) const {
  if (item.samples.size() < 3 || item.samples.size() > MAX_SAMPLES) {
    error = "record at least three example frames"; return false;
  }
  for (const auto &action : item.actions) {
    if (std::count_if(item.samples.begin(), item.samples.end(), [&](const CapturedFrame &s) {
          return s.label == action;
        }) < 3) {
      error = "record each action at least three times: " + action; return false;
    }
  }
  for (const auto &sample : item.samples) {
    if (!user_vm::identifier(sample.label) || sample.channel != item.radio || sample.kind != item.kind ||
        std::find(item.actions.begin(), item.actions.end(), sample.label) == item.actions.end()) {
      error = "capture label or radio does not match decoder"; return false;
    }
    RFFrame frame;
    frame.kind = sample.kind; frame.channel = sample.channel;
    frame.raw = &sample.raw; frame.packet = &sample.packet;
    DecodeResult decoded;
    std::string reason;
    if (!user_vm::receive(item.rx, item.id, frame, decoded, reason) || decoded.action != sample.label) {
      error = "decoder did not recognize recorded action: " + sample.label +
              (reason.empty() ? " (matched " + decoded.action + ")" : " (" + reason + ")");
      return false;
    }
  }
  if (!item.tx_source.empty()) {
    for (const auto &action : item.actions) {
      user_vm::TxFrame tx;
      if (!user_vm::transmit(item.tx, action, item.radio, {}, tx, error) ||
          (item.kind == FrameKind::RAW && tx.raw.empty()) ||
          (item.kind == FrameKind::PACKET && tx.packet.empty())) {
        error = "TX preview failed for " + action + ": " + error; return false;
      }
    }
  }
  error.clear(); return true;
}

bool UserLibrary::upsert_decoder(UserDecoder value, std::string &error) {
  if (!user_vm::identifier(value.id) || value.id.rfind("user_", 0) != 0 ||
      !text_ok(value.name, 64) || value.radio > 2 ||
      value.actions.empty() || value.actions.size() > 16 || value.samples.size() > MAX_SAMPLES ||
      (value.kind != FrameKind::RAW && value.kind != FrameKind::PACKET) ||
      ((value.radio == 2) != (value.kind == FrameKind::PACKET))) {
    error = "invalid decoder metadata"; return false;
  }
  // Never admit malformed or mixed-format captures, even for inactive drafts.
  // Replay only verifies active decoders; recording validation always applies.
  for (const auto &sample : value.samples) {
    if (!user_vm::identifier(sample.label) ||
        sample.channel != value.radio || sample.kind != value.kind ||
        std::find(value.actions.begin(), value.actions.end(), sample.label) == value.actions.end() ||
        (sample.kind == FrameKind::RAW &&
          (sample.raw.size() < 2 || sample.raw.size() > 512 || !sample.packet.empty())) ||
        (sample.kind == FrameKind::PACKET &&
          (sample.packet.empty() || sample.packet.size() > 64 || !sample.raw.empty()))) {
      error = "invalid or mixed-format example capture"; return false;
    }
  }
  for (const auto &action : value.actions) if (!user_vm::identifier(action)) {
    error = "invalid action name"; return false;
  }
  if (std::set<std::string>(value.actions.begin(), value.actions.end()).size() != value.actions.size()) {
    error = "duplicate action name"; return false;
  }
  if (!user_vm::compile(value.rx_source, user_vm::Mode::RX, value.rx, error)) return false;
  if (!value.tx_source.empty() && !user_vm::compile(value.tx_source, user_vm::Mode::TX, value.tx, error)) return false;
  if (value.enabled && !replay(value, error)) return false;
  for (auto &item : decoders_) {
    if (item.id != value.id) continue;
    if (item.enabled && (item.rx_source != value.rx_source || item.tx_source != value.tx_source ||
                         item.radio != value.radio || item.kind != value.kind ||
                         item.actions != value.actions)) {
      error = "disable the active decoder before changing its rules"; return false;
    }
    value.version = item.version + 1;
    if (value.version == 0) { error = "decoder version limit"; return false; }
    item = std::move(value); return true;
  }
  if (decoders_.size() >= MAX_DECODERS) { error = "user decoder library is full"; return false; }
  decoders_.push_back(std::move(value)); return true;
}

bool UserLibrary::enable_decoder(const std::string &id, bool enabled, std::string &error) {
  for (auto &item : decoders_) if (item.id == id) {
    if (item.enabled == enabled) return true;
    if (enabled && !replay(item, error)) return false;
    item.enabled = enabled; return true;
  }
  error = "decoder not found"; return false;
}

bool UserLibrary::remove_decoder(const std::string &id) {
  auto it = std::find_if(decoders_.begin(), decoders_.end(), [&](const UserDecoder &d) {return d.id == id;});
  if (it == decoders_.end() || it->enabled) return false;
  for (const auto &profile : profiles_) if (profile.protocol == id) return false;
  decoders_.erase(it); return true;
}

bool UserLibrary::upsert_profile(UserProfile profile, std::string &error) {
  if (!user_vm::identifier(profile.id) || !text_ok(profile.name, 64) ||
      !text_ok(profile.protocol, 64) || profile.expected_actions.size() > 16 ||
      profile.learn_help.size() > 255 || profile.controls.size() > 24 ||
      profile.setup_steps.size() > 8 ||
      (profile.pair_method!="none" && profile.pair_method!="manual" &&
       profile.pair_method!="receiver_learn" && profile.pair_method!="unsupported")) {
    error = "invalid device profile"; return false; }
  for(const auto &step: profile.setup_steps) if(!text_ok(step,240) || step.find('|')!=std::string::npos) {
    error="invalid wizard step"; return false;
  }
  for(const auto &action:profile.controls) if(!user_vm::identifier(action)) {
    error="invalid control action"; return false;
  }
  if(std::set<std::string>(profile.controls.begin(),profile.controls.end()).size()!=profile.controls.size()) {
    error="duplicate control"; return false;
  }
  // Controls must be a subset of the declared profile actions. Only reviewed
  // encoders decide whether any of them are really transmissible.
  for(const auto &action:profile.controls)
    if(std::find(profile.expected_actions.begin(),profile.expected_actions.end(),action)==profile.expected_actions.end()) {
      error="control is not a declared profile action"; return false;
    }
  // A profile is presentation/learning metadata and may point at either a
  // persistent user decoder or a normal built-in decoder. The runtime layer
  // validates that the referenced protocol exists before persisting it.
  const auto *decoder = this->decoder(profile.protocol);
  for (const auto &action : profile.expected_actions) if (!user_vm::identifier(action)) {
    error = "invalid profile action"; return false;
  }
  if (std::set<std::string>(profile.expected_actions.begin(), profile.expected_actions.end()).size() != profile.expected_actions.size()) {
    error = "duplicate profile action"; return false;
  }
  for (const auto &action : profile.expected_actions)
    if (decoder && std::find(decoder->actions.begin(), decoder->actions.end(), action) == decoder->actions.end()) {
      error = "profile action is absent from decoder"; return false;
    }
  for (auto &item : profiles_) if (item.id == profile.id) {
    profile.version = item.version + 1;
    if (profile.version == 0) { error = "profile version limit"; return false; }
    item = std::move(profile); return true;
  }
  if (profiles_.size() >= MAX_PROFILES) { error = "profile library is full"; return false; }
  profiles_.push_back(std::move(profile)); return true;
}

bool UserLibrary::remove_profile(const std::string &id) {
  auto it = std::find_if(profiles_.begin(), profiles_.end(), [&](const UserProfile &p) {return p.id == id;});
  if (it == profiles_.end()) return false;
  profiles_.erase(it); return true;
}

bool UserLibrary::commit_state(const std::map<std::string, uint32_t> &updates) {
  size_t added = 0;
  for (const auto &entry : updates) if (!state_.count(entry.first)) ++added;
  if (state_.size() + added > MAX_STATE) return false;
  for (const auto &entry : updates) {
    // A key contains the device UID, decoder ID and STATE name. Each of the
    // latter two may be 32 characters, so 64 would reject valid programs.
    if (entry.first.empty() || entry.first.size() > 128 || entry.second == 0 ||
        (state_.count(entry.first) && entry.second <= state_[entry.first])) return false;
  }
  for (const auto &entry : updates) state_[entry.first] = entry.second;
  return true;
}

bool UserLibrary::codebook(const std::string &id, const std::string &name,
                           const std::vector<CapturedFrame> &samples, UserDecoder &out,
                           std::string &error) {
  if (!user_vm::identifier(id) || id.rfind("user_",0)!=0 || !text_ok(name, 64) || samples.size() < 3 ||
      samples.size() > MAX_SAMPLES || samples.front().channel > 2) {
    error = "name, identifier or capture count is invalid"; return false;
  }
  UserDecoder next;
  next.id = id; next.name = name;
  next.radio = samples.front().channel; next.kind = samples.front().kind; next.samples = samples;
  for (const auto &sample : samples) {
    if (!user_vm::identifier(sample.label) || sample.channel != next.radio || sample.kind != next.kind ||
        (next.kind == FrameKind::RAW && (sample.raw.size()<2 || sample.raw.size()>512 || !sample.packet.empty())) ||
        (next.kind == FrameKind::PACKET && (sample.packet.empty() || sample.packet.size()>64 || !sample.raw.empty()))) {
      error = "captures must have one radio, one format and valid action labels"; return false;
    }
    if (std::find(next.actions.begin(), next.actions.end(), sample.label) == next.actions.end())
      next.actions.push_back(sample.label);
  }
  if (next.actions.size() > 4) { error = "automatic mode supports up to four actions"; return false; }
  std::vector<std::string> receive;
  std::vector<std::string> transmit;
  bool can_transmit = true;
  // All branches end in ACCEPT/SEND; the jump target skips directly to the
  // next action. Independent replay checks every positive label.
  for (size_t action_index = 0; action_index < next.actions.size(); ++action_index) {
    const auto &label = next.actions[action_index];
    std::vector<const CapturedFrame *> group;
    for (const auto &sample : samples) if (sample.label == label) group.push_back(&sample);
    if (group.size() < 3) { error = "capture each action at least three times"; return false; }
    const auto &first = *group.front();
    const size_t count = next.kind == FrameKind::RAW ? first.raw.size() : first.packet.size();
    size_t unstable_count = 0;
    for (size_t i = 0; i < count; ++i) {
      bool changed = false;
      for (const auto *item : group) {
        if (next.kind == FrameKind::RAW) {
          if (item->raw.size() != count ||
              std::llabs(int64_t(item->raw[i]) - first.raw[i]) >
                  std::max<int64_t>(100, std::llabs(int64_t(first.raw[i])) / 4)) changed = true;
        } else if (item->packet.size() != count || item->packet[i] != first.packet[i]) changed = true;
      }
      if (changed) ++unstable_count;
    }
    // Replay of a changing counter/encrypted telegram is not a transmitter
    // implementation. Users can supply an algorithm with persistent STATE/BUMP.
    // One varying symbol can be a rolling counter or checksum. Automatic
    // replay would reuse an old code, so only byte-for-byte stable packets
    // (and RAW captures within timing tolerance) receive a TX rule.
    if (unstable_count != 0) can_transmit = false;
    std::vector<std::string> checks;
    // Use stable symbols only, spread across the frame. Timing drift within
    // 25% is accepted by MATCHRAW. Unstable counters/checksums are omitted.
    size_t matched = 0;
    for (size_t i = 0; i < count && matched < 18; ++i) {
      if (count > 32 && i >= 4 && i % ((count + 27) / 28) != 0) continue;
      bool stable = true;
      if (next.kind == FrameKind::RAW) {
        const int64_t baseline = first.raw[i];
        for (auto *item : group)
          if (item->raw.size() != count || (int64_t(item->raw[i]) < 0) != (baseline < 0) ||
              std::llabs(int64_t(item->raw[i]) - baseline) > std::max<int64_t>(100, std::llabs(baseline) / 4)) stable = false;
        if (stable) checks.push_back("MATCHRAW " + std::to_string(i) + " " + std::to_string(baseline));
      } else {
        for (auto *item : group) if (item->packet.size() != count || item->packet[i] != first.packet[i]) stable = false;
        if (stable) checks.push_back("MATCHBYTE " + std::to_string(i) + " " + std::to_string(first.packet[i]));
      }
      if (stable) ++matched;
    }
    if (matched < 4) { error = "not enough stable symbols; use Advanced to describe the protocol"; return false; }
    const size_t next_line = receive.size() + 4 + checks.size() * 2 + 4;
    receive.push_back("LEN");
    receive.push_back("CONST " + std::to_string(count));
    receive.push_back("EQ");
    receive.push_back("JZ " + std::to_string(next_line));
    for (const auto &check : checks) { receive.push_back(check); receive.push_back("JZ " + std::to_string(next_line)); }
    // Each codebook is tied to one learned physical device. Advanced programs
    // can derive IDs from packets if several devices share the protocol.
    uint32_t identity = 2166136261u;
    for (unsigned char c : id) identity = (identity ^ c) * 16777619u;
    receive.push_back("CONST " + std::to_string(identity));
    receive.push_back("ID"); receive.push_back("ACTION " + label); receive.push_back("ACCEPT");

    if (can_transmit) {
      std::vector<std::string> branch{"ACTEQ " + label, "JZ ?"};
      if (next.kind == FrameKind::RAW) {
        // Compact immediate timing opcodes keep one pulse at one instruction,
        // so substantially longer fixed-code remotes fit the bounded VM.
        if (first.raw.front() < 0 || first.raw.size() % 2 != 0) can_transmit = false;
        else for (int32_t pulse : first.raw) {
          if (std::llabs(int64_t(pulse)) < 50 || std::llabs(int64_t(pulse)) > 1000000) { can_transmit = false; break; }
          branch.push_back(std::string(pulse > 0 ? "MARKUS " : "SPACEUS ") +
                           std::to_string(std::llabs(int64_t(pulse))));
        }
      } else {
        if (first.packet.size() != 63) can_transmit = false; // Current board CC1101 fixed packet_length: 63.
        else for (uint8_t byte : first.packet) branch.push_back("BYTEVAL " + std::to_string(byte));
      }
      branch.push_back("REPEATS 3"); branch.push_back("SEND");
      if (can_transmit && transmit.size() + branch.size() + 2 <= user_vm::MAX_INSTRUCTIONS) {
        branch[1] = "JZ " + std::to_string(transmit.size() + branch.size());
        transmit.insert(transmit.end(), branch.begin(), branch.end());
      } else {
        can_transmit = false;
      }
    }
  }
  receive.push_back("HALT"); receive.push_back("ACCEPT");
  if (receive.size() > user_vm::MAX_INSTRUCTIONS) { error = "too many symbols; use Advanced"; return false; }
  for (const auto &line : receive) next.rx_source += line + "\n";
  if (can_transmit) {
    transmit.push_back("HALT"); transmit.push_back("SEND");
    for (const auto &line : transmit) next.tx_source += line + "\n";
  }
  if (!user_vm::compile(next.rx_source, user_vm::Mode::RX, next.rx, error) ||
      (!next.tx_source.empty() && !user_vm::compile(next.tx_source, user_vm::Mode::TX, next.tx, error))) return false;
  UserLibrary verifier;
  if (!verifier.replay(next, error)) return false;
  out = std::move(next);
  return true;
}

uint32_t UserLibrary::fingerprint(const UserDecoder &item) {
  uint32_t hash = 2166136261u;
  for (const auto *s : {&item.id, &item.rx_source, &item.tx_source})
    for (unsigned char c : *s) hash = (hash ^ c) * 16777619u;
  return hash;
}

bool UserLibraryPersistence::valid(const Blob &blob) {
  return blob.magic == MAGIC && (blob.version == 1 || blob.version == VERSION) && blob.used <= CAPACITY &&
         blob.generation > 0 && blob.checksum == hash_blob(blob);
}

bool UserLibraryPersistence::encode(const UserLibrary &library, uint32_t generation, Blob &blob) {
  if (generation == 0 || library.decoders().size() > UserLibrary::MAX_DECODERS ||
      library.profiles().size() > UserLibrary::MAX_PROFILES || library.state().size() > UserLibrary::MAX_STATE) return false;
  blob = Blob{}; blob.generation = generation;
  Writer out{blob};
  if (!out.byte(uint8_t(library.decoders().size()))) return false;
  for (const auto &d : library.decoders()) {
    if (d.samples.size() > UserLibrary::MAX_SAMPLES || !out.str(d.id) || !out.str(d.name) ||
        !out.byte(d.radio) || !out.byte(d.kind == FrameKind::RAW ? 0 : 1) ||
        !out.u16(d.version) || !out.byte(d.enabled ? 1 : 0) ||
        !out.str(d.rx_source) || !out.str(d.tx_source) || !out.strings(d.actions) ||
        !out.byte(uint8_t(d.samples.size()))) return false;
    for (const auto &sample : d.samples) {
      if (!out.str(sample.label) || !out.byte(sample.channel) || !out.byte(sample.kind == FrameKind::RAW ? 0 : 1) ||
          !out.u16(uint16_t(sample.raw.size())) || !out.u16(uint16_t(sample.packet.size()))) return false;
      for (int32_t pulse : sample.raw) if (!out.u32(uint32_t(pulse))) return false;
      for (uint8_t byte : sample.packet) if (!out.byte(byte)) return false;
    }
  }
  if (!out.byte(uint8_t(library.profiles().size()))) return false;
  for (const auto &p : library.profiles())
    if (!out.str(p.id) || !out.str(p.name) || !out.str(p.protocol) || !out.strings(p.expected_actions) ||
        !out.str(p.learn_help) || !out.u16(p.version) ||
        !out.strings(p.controls) || !out.strings(p.setup_steps) || !out.str(p.pair_method)) return false;
  if (!out.byte(uint8_t(library.state().size()))) return false;
  for (const auto &entry : library.state()) if (!out.str(entry.first) || !out.u32(entry.second)) return false;
  blob.checksum = hash_blob(blob);
  return true;
}

bool UserLibraryPersistence::decode(const Blob &blob, UserLibrary &library) {
  if (!valid(blob)) return false;
  Reader in{blob};
  UserLibrary next;
  uint8_t count = 0;
  if (!in.byte(count) || count > UserLibrary::MAX_DECODERS) return false;
  for (uint8_t i = 0; i < count; ++i) {
    UserDecoder d;
    uint8_t kind, active, samples;
    if (!in.str(d.id) || !in.str(d.name) || !in.byte(d.radio) || !in.byte(kind) || kind > 1 ||
        !in.u16(d.version) || !in.byte(active) || active > 1 || !in.str(d.rx_source) ||
        !in.str(d.tx_source) || !in.strings(d.actions) || !in.byte(samples) || samples > UserLibrary::MAX_SAMPLES) return false;
    d.kind = kind == 0 ? FrameKind::RAW : FrameKind::PACKET;
    // Validate without activating until all test vectors have been restored.
    for (uint8_t n = 0; n < samples; ++n) {
      CapturedFrame sample;
      uint8_t sample_kind; uint16_t raw_count, packet_count;
      if (!in.str(sample.label) || !in.byte(sample.channel) || !in.byte(sample_kind) || sample_kind > 1 ||
          !in.u16(raw_count) || raw_count > 512 || !in.u16(packet_count) || packet_count > 64) return false;
      sample.kind = sample_kind == 0 ? FrameKind::RAW : FrameKind::PACKET;
      for (uint16_t j = 0; j < raw_count; ++j) {
        uint32_t value; if (!in.u32(value)) return false; sample.raw.push_back(int32_t(value));
      }
      for (uint16_t j = 0; j < packet_count; ++j) {
        uint8_t value; if (!in.byte(value)) return false; sample.packet.push_back(value);
      }
      d.samples.push_back(std::move(sample));
    }
    if (d.version == 0 || next.decoder(d.id)) return false;
    const uint16_t saved_version = d.version;
    const std::string id = d.id;
    std::string error;
    if (!next.upsert_decoder(std::move(d), error)) return false;
    next.decoders_.back().version = saved_version;
    if (active && !next.enable_decoder(id, true, error)) return false;
  }
  if (!in.byte(count) || count > UserLibrary::MAX_PROFILES) return false;
  for (uint8_t i = 0; i < count; ++i) {
    UserProfile p;
    if (!in.str(p.id) || !in.str(p.name) || !in.str(p.protocol) || !in.strings(p.expected_actions) ||
        !in.str(p.learn_help) || !in.u16(p.version)) return false;
    if(blob.version>=2 && (!in.strings(p.controls) || !in.strings(p.setup_steps) ||
                           !in.str(p.pair_method))) return false;
    if (p.version == 0 || std::any_of(next.profiles_.begin(), next.profiles_.end(),
                                      [&](const UserProfile &existing) { return existing.id == p.id; })) return false;
    const uint16_t saved_version = p.version;
    std::string error; if (!next.upsert_profile(std::move(p), error)) return false;
    next.profiles_.back().version = saved_version;
  }
  if (!in.byte(count) || count > UserLibrary::MAX_STATE) return false;
  for (uint8_t i = 0; i < count; ++i) {
    std::string key; uint32_t value;
    if (!in.str(key) || !in.u32(value) || value == 0 || !next.commit_state({{key, value}})) return false;
  }
  if (in.at != blob.used) return false;
  library = std::move(next); return true;
}

}  // namespace rflink
