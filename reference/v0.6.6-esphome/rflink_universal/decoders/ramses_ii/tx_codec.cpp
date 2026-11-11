#include "tx_codec.h"
#include <cstring>

namespace rflink_ramses_ii {
namespace tx {
namespace {
const uint8_t MANCHESTER[16] = {
    0xAA, 0xA9, 0xA6, 0xA5, 0x9A, 0x99, 0x96, 0x95,
    0x6A, 0x69, 0x66, 0x65, 0x5A, 0x59, 0x56, 0x55};
const uint8_t HEADERS[16] = {
    0x0F, 0x0C, 0x0D, 0x0B, 0x27, 0x24, 0x25, 0x23,
    0x47, 0x44, 0x45, 0x43, 0x17, 0x14, 0x15, 0x13};
const uint8_t SYNC[] = {0xFF,0x00,0x33,0x55,0x53};
const uint32_t RAW_MAX = 0xFFFFFFU;
const uint64_t TIMEOUT_MS = 180000;
bool idValid(uint32_t id) { return id != 0 && id <= RAW_MAX; }
bool remoteValid(uint32_t id) { return idValid(id) && deviceClass(id) == 37; }
void setAddr(Frame& f, int index, uint32_t addr) {
  f.has_addr[index] = true;
  f.addr[index] = addr;
}
Frame toFan(uint32_t controller, uint32_t fan, uint16_t command) {
  Frame result;
  result.type = Frame::INFORMATION;
  setAddr(result, 0, controller);
  setAddr(result, 1, fan);
  result.command = command;
  return result;
}
Frame self(uint32_t controller, uint16_t command) {
  Frame result;
  result.type = Frame::INFORMATION;
  setAddr(result, 0, controller);
  setAddr(result, 2, controller);
  result.command = command;
  return result;
}
void appendAddr(std::vector<uint8_t>& out, uint32_t raw) {
  out.push_back(static_cast<uint8_t>(raw >> 16));
  out.push_back(static_cast<uint8_t>(raw >> 8));
  out.push_back(static_cast<uint8_t>(raw));
}
} // namespace

bool buildCommand(const Ids& id, Action action, Frame& out) {
  if (!remoteValid(id.controller) || !idValid(id.fan) || id.controller == id.fan) return false;
  Frame frame;
  if (action == Action::TIMER_15 || action == Action::TIMER_30 || action == Action::TIMER_60) {
    frame = toFan(id.controller, id.fan, 0x22F3);
    frame.payload_length = 7;
    // Orcon MVS15 examples: 00 02 <minutes> 03 04 00 00
    frame.payload[0] = 0; frame.payload[1] = 0x02;
    frame.payload[2] = action == Action::TIMER_15 ? 15 : action == Action::TIMER_30 ? 30 : 60;
    frame.payload[3] = 3; frame.payload[4] = 4;
    frame.payload[5] = 0; frame.payload[6] = 0;
  } else {
    unsigned mode;
    switch (action) {
      case Action::AWAY: mode = 0; break;
      case Action::LOW: mode = 1; break;
      case Action::MEDIUM: mode = 2; break;
      case Action::HIGH: mode = 3; break;
      case Action::AUTO: mode = 4; break;
      default: return false;
    }
    frame = toFan(id.controller, id.fan, 0x22F1);
    frame.payload_length = 3;
    frame.payload[0] = 0; frame.payload[1] = static_cast<uint8_t>(mode);
    frame.payload[2] = 0x04; // Orcon MVS 15RF example, not generic RAMSES field.
  }
  out = frame;
  return true;
}

bool buildBindOffer(uint32_t controller, Frame& out) {
  if (!remoteValid(controller)) return false;
  Frame frame = self(controller, 0x1FC9);
  const uint16_t commands[3] = {0x22F1,0x22F3,0x10E0};
  frame.payload_length = 18;
  for (unsigned i=0; i<3; ++i) {
    const unsigned n = i*6;
    frame.payload[n] = 0x00;
    frame.payload[n+1] = static_cast<uint8_t>(commands[i] >> 8);
    frame.payload[n+2] = static_cast<uint8_t>(commands[i]);
    frame.payload[n+3] = static_cast<uint8_t>(controller >> 16);
    frame.payload[n+4] = static_cast<uint8_t>(controller >> 8);
    frame.payload[n+5] = static_cast<uint8_t>(controller);
  }
  out = frame;
  return true;
}

bool buildBindConfirm(uint32_t controller, uint32_t fan, Frame& out) {
  if (!remoteValid(controller) || !idValid(fan) || controller == fan) return false;
  Frame frame = toFan(controller, fan, 0x1FC9);
  frame.payload_length = 1;
  frame.payload[0] = 0x21;
  out = frame;
  return true;
}

bool buildDeviceInfo(uint32_t controller, Frame& out) {
  if (!remoteValid(controller)) return false;
  // Captured Orcon 15RF 10E0 OEM 0x67 data from ramses_cc wiki.
  static const uint8_t data[] = {
      0x00,0x00,0x01,0xC8,0x94,0x03,0x01,0x67,
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x1B,0x08,
      0x07,0xE4,0x56,0x4D,0x49,0x2D,0x31,0x35,
      0x57,0x53,0x4A,0x35,0x33,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00};
  static_assert(sizeof(data) == 38, "Orcon device info must be 38 bytes");
  Frame frame = self(controller, 0x10E0);
  frame.payload_length = sizeof(data);
  std::memcpy(frame.payload, data, sizeof(data));
  out = frame;
  return true;
}

bool isBindAcknowledgement(const Frame& frame, uint32_t controller, uint32_t& fan) {
  if (!remoteValid(controller) || frame.type != Frame::WRITE || frame.command != 0x1FC9 ||
      !frame.has_addr[0] || !frame.has_addr[1] ||
      frame.addr[1] != controller || frame.addr[0] == controller ||
      frame.payload_length != 12 || frame.payload[0] != 0x21) return false;
  // Expect two 6-byte offer tuples carrying fan IDs; prevent echo/spurious ACK.
  if (frame.payload[1] != 0x31 || frame.payload[2] != 0xDA ||
      frame.payload[7] != 0x10 || frame.payload[8] != 0xE0) return false;
  const uint32_t id1 = (uint32_t(frame.payload[3])<<16) | (uint32_t(frame.payload[4])<<8) | frame.payload[5];
  const uint32_t id2 = (uint32_t(frame.payload[9])<<16) | (uint32_t(frame.payload[10])<<8) | frame.payload[11];
  if (id1 != frame.addr[0] || id2 != frame.addr[0] || !idValid(id1)) return false;
  fan = id1;
  return true;
}

bool encodeLogical(const Frame& f, std::vector<uint8_t>& out) {
  if (f.type == Frame::UNKNOWN || f.payload_length > MAX_PAYLOAD) return false;
  uint8_t flags;
  switch (f.type) {
    case Frame::INFORMATION: flags = 0x20; break;
    case Frame::REQUEST: flags = 0x08; break;
    case Frame::RESPONSE: flags = 0x10; break;
    case Frame::WRITE: flags = 0x40; break;
    default: return false;
  }
  std::vector<uint8_t> bytes;
  bytes.reserve(80);
  for (unsigned i=0; i<3; ++i) {
    if (f.has_addr[i]) {
      if (!idValid(f.addr[i])) return false;
      flags |= static_cast<uint8_t>(1U<<i);
    }
  }
  int header=-1;
  for (unsigned i=0; i<16; ++i) if (HEADERS[i] == flags) {header = static_cast<int>(i)<<2; break;}
  if (header < 0) return false;
  bytes.push_back(static_cast<uint8_t>(header));
  for (unsigned i=0; i<3; ++i) if (f.has_addr[i]) appendAddr(bytes, f.addr[i]);
  bytes.push_back(static_cast<uint8_t>(f.command >> 8));
  bytes.push_back(static_cast<uint8_t>(f.command));
  bytes.push_back(f.payload_length);
  bytes.insert(bytes.end(), f.payload, f.payload + f.payload_length);
  uint8_t sum=0;
  for (auto b : bytes) sum = static_cast<uint8_t>(sum + b);
  bytes.push_back(static_cast<uint8_t>(-sum));
  Frame verify;
  if (!parseLogical(bytes.data(),bytes.size(),verify)) return false;
  out.swap(bytes);
  return true;
}

bool encodeWire(const Frame& frame, std::vector<uint8_t>& out) {
  std::vector<uint8_t> logical;
  if (!encodeLogical(frame, logical)) return false;
  std::vector<uint8_t> bytes;
  bytes.reserve(sizeof(SYNC)+logical.size()*2+1);
  bytes.insert(bytes.end(), SYNC, SYNC+sizeof(SYNC));
  for (auto b : logical) {
    bytes.push_back(MANCHESTER[b>>4]);
    bytes.push_back(MANCHESTER[b&0x0F]);
  }
  bytes.push_back(0x35);
  Frame check;
  if (!decodeWire(bytes.data(),bytes.size(),check)) return false;
  out.swap(bytes);
  return true;
}

bool encodeOnAir(const Frame& frame, unsigned preamble, std::vector<uint8_t>& out, size_t& bitCount) {
  if (preamble > 32) return false;
  std::vector<uint8_t> wire;
  if (!encodeWire(frame,wire)) return false;
  // On-air 8N1 framing: start=0, eight bits LSB-first, stop=1.
  const size_t count = preamble + wire.size();
  std::vector<uint8_t> bits((count*10+7)/8,0);
  size_t pos = 0;
  for (size_t i=0; i<count; ++i) {
    const uint8_t byte = i < preamble ? 0xFF : wire[i-preamble];
    for (unsigned j=0;j<10;++j) {
      const unsigned b = j == 0 ? 0 : j == 9 ? 1 : (byte >> (j-1)) & 1;
      if (b) bits[pos/8] |= static_cast<uint8_t>(0x80 >> (pos%8));
      ++pos;
    }
  }
  bitCount = pos;
  out.swap(bits);
  return true;
}

PairingSession::PairingSession() : state_(State::IDLE), controller_(0), fan_(0), last_ms_(0) {}
bool PairingSession::begin(uint32_t controller, uint64_t now) {
  if (!remoteValid(controller)) return false;
  controller_=controller;fan_=0;last_ms_=now;state_=State::OFFER_READY;
  return true;
}
bool PairingSession::makeOffer(Frame& out) const {
  return state_ == State::OFFER_READY && buildBindOffer(controller_,out);
}
void PairingSession::offerSent(uint64_t now) {
  if (state_ == State::OFFER_READY) {state_=State::WAITING_ACK;last_ms_=now;}
}
bool PairingSession::receive(const Frame& frame, uint64_t now) {
  checkTimeout(now);
  if (state_ != State::WAITING_ACK) return false;
  uint32_t fan=0;
  if (!isBindAcknowledgement(frame,controller_,fan)) return false;
  fan_=fan;state_=State::CONFIRM_READY;last_ms_=now;
  return true;
}
bool PairingSession::makeConfirmation(Frame& out) const {
  return state_ == State::CONFIRM_READY && buildBindConfirm(controller_,fan_,out);
}
void PairingSession::confirmationSent(uint64_t now) {
  if (state_ == State::CONFIRM_READY) {state_=State::WAITING_VERIFICATION;last_ms_=now;}
}
bool PairingSession::verify(uint32_t fan, uint64_t now) {
  checkTimeout(now);
  if (state_ != State::WAITING_VERIFICATION || fan != fan_) return false;
  state_=State::VERIFIED;last_ms_=now;
  return true;
}
void PairingSession::checkTimeout(uint64_t now) {
  if (state_ == State::OFFER_READY || state_ == State::WAITING_ACK ||
      state_ == State::CONFIRM_READY || state_ == State::WAITING_VERIFICATION) {
    if (now < last_ms_ || now - last_ms_ > TIMEOUT_MS) state_ = State::EXPIRED;
  }
}
} // namespace tx
} // namespace rflink_ramses_ii
