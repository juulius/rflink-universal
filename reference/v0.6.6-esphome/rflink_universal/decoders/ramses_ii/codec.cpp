#include "codec.h"
#include <string.h>

namespace rflink_ramses_ii {
namespace {
// Manchester and header tables cross-checked against the community RAMSES-II
// implementation by me-processware (MIT licensed, see ATTRIBUTION.md).
const uint8_t kManchester[16] = {
    0xAA, 0xA9, 0xA6, 0xA5, 0x9A, 0x99, 0x96, 0x95,
    0x6A, 0x69, 0x66, 0x65, 0x5A, 0x59, 0x56, 0x55};
const uint8_t kHeaderFlags[16] = {
    0x0F, 0x0C, 0x0D, 0x0B, 0x27, 0x24, 0x25, 0x23,
    0x47, 0x44, 0x45, 0x43, 0x17, 0x14, 0x15, 0x13};

int manchesterNibble(uint8_t wire) {
  for (int i = 0; i < 16; ++i) if (kManchester[i] == wire) return i;
  return -1;
}
uint32_t readAddress(const uint8_t* d) {
  return (uint32_t(d[0]) << 16) | (uint32_t(d[1]) << 8) | d[2];
}
int msbBit(const uint8_t* data, size_t bit) {
  return (data[bit >> 3] >> (7U - (bit & 7U))) & 1;
}
// Decode RAMSES UART 8N1 (38.4k baud) starting at an arbitrary bit position.
// False when UART start or stop bit is corrupt; avoids heuristic best guesses.
bool uartByte(const uint8_t* bytes, size_t total_bits, size_t start, uint8_t& b) {
  if (start + 10 > total_bits) return false;
  if (msbBit(bytes, start) != 0 || msbBit(bytes, start + 9) != 1) return false;
  b = 0;
  for (int bit = 0; bit < 8; ++bit)
    b |= static_cast<uint8_t>(msbBit(bytes, start + 1 + bit) << bit);
  return true;
}
// A validated RAMSES frame follows the sync: 2 Manchester bytes per logical
// byte and terminates with 0x35. For a truncated packet, no match is returned.
bool decodeEncoded(const uint8_t* encoded, size_t count, Frame& f) {
  uint8_t logical[96] = {};
  size_t n = 0;
  for (size_t p = 0; p < count;) {
    if (encoded[p] == 0x35) return n > 0 && parseLogical(logical, n, f);
    if (p + 1 >= count || n >= sizeof(logical)) return false;
    const int hi = manchesterNibble(encoded[p++]);
    const int lo = manchesterNibble(encoded[p++]);
    if (hi < 0 || lo < 0) return false;
    logical[n++] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return false;
}
}  // namespace

Frame::Frame() : type(UNKNOWN), command(0), payload_length(0) {
  memset(has_addr, 0, sizeof(has_addr));
  memset(addr, 0, sizeof(addr));
  memset(payload, 0, sizeof(payload));
}

bool parseLogical(const uint8_t* logical, size_t n, Frame& result) {
  if (!logical || n < 5 || n > 96) return false;
  unsigned sum = 0;
  for (size_t i = 0; i < n; ++i) sum += logical[i];
  if ((sum & 0xFFU) != 0U) return false;
  Frame parsed;
  const uint8_t hdr = logical[0];
  if (hdr & 0xC0U) return false;  // reserved bits must be clear
  const uint8_t flags = kHeaderFlags[(hdr >> 2) & 0x0F];
  const uint8_t types = flags & 0x78;
  switch (types) {
    case 0x20: parsed.type = Frame::INFORMATION; break;
    case 0x08: parsed.type = Frame::REQUEST; break;
    case 0x10: parsed.type = Frame::RESPONSE; break;
    case 0x40: parsed.type = Frame::WRITE; break;
    default: return false;
  }
  size_t pos = 1;
  for (int i = 0; i < 3; ++i) {
    parsed.has_addr[i] = (flags & (1 << i)) != 0;
    if (parsed.has_addr[i]) {
      if (pos + 3 > n - 1) return false;
      parsed.addr[i] = readAddress(logical + pos);
      pos += 3;
    }
  }
  if (hdr & 0x02U) { if (pos >= n - 1) return false; ++pos; }
  if (hdr & 0x01U) { if (pos >= n - 1) return false; ++pos; }
  if (pos + 3 > n - 1) return false;
  parsed.command = (uint16_t(logical[pos]) << 8) | logical[pos + 1];
  pos += 2;
  const size_t count = logical[pos++];
  if (count > MAX_PAYLOAD || pos + count + 1 != n) return false;
  parsed.payload_length = static_cast<uint8_t>(count);
  memcpy(parsed.payload, logical + pos, count);
  result = parsed;
  return true;
}

bool decodeWire(const uint8_t* wire, size_t n, Frame& result) {
  if (!wire || n < 6) return false;
  // Full sync FF 00 33 55 53, or its unambiguous captured suffix 33 55 53.
  for (size_t i = 0; i + 3 < n; ++i) {
    if (wire[i] != 0x33 || wire[i+1] != 0x55 || wire[i+2] != 0x53) continue;
    if (decodeEncoded(wire + i + 3, n - i - 3, result)) return true;
  }
  return false;
}

bool decodePacked(const uint8_t* packed, size_t n, Frame& result) {
  if (!packed || n < 16 || n > 256) return false;
  const size_t total_bits = n * 8;
  // Search bit-by-bit for UART-framed last three sync bytes 33 55 53.
  // Unlike byte-aligned decoders, this supports the partial preamble in the
  // 63-byte packet captures. Checksum+exact length prevent spurious matches.
  for (size_t start = 0; start + 40 <= total_bits; ++start) {
    uint8_t a = 0, b = 0, c = 0;
    if (!uartByte(packed, total_bits, start, a) || a != 0x33 ||
        !uartByte(packed, total_bits, start + 10, b) || b != 0x55 ||
        !uartByte(packed, total_bits, start + 20, c) || c != 0x53) continue;
    uint8_t encoded[192] = {};
    size_t decoded_bytes = 0;
    for (size_t pos = start + 30; pos + 10 <= total_bits &&
                                   decoded_bytes < sizeof(encoded); pos += 10) {
      uint8_t v = 0;
      if (!uartByte(packed, total_bits, pos, v)) break;
      encoded[decoded_bytes++] = v;
      if (v == 0x35) {
        if (decodeEncoded(encoded, decoded_bytes, result)) return true;
        break;
      }
    }
  }
  return false;
}

const char* commandName(uint16_t c) {
  switch (c) {
    case 0x31D9: return "Ventilation status";
    case 0x31DA: return "Extended ventilation status";
    case 0x22F1: return "Set ventilation mode";
    case 0x22F3: return "Timed ventilation boost";
    case 0x1FC9: return "RF pairing";
    case 0x1298: return "CO2 measurement";
    case 0x12A0: return "Humidity";
    case 0x10D0: return "Filter information";
    case 0x042F: return "Device reboot counter";
    case 0x10E0: return "Device information";
    case 0x31E0: return "Ventilation demand";
    default: return "RAMSES-II message";
  }
}
const char* modeName(uint8_t code) {
  switch (code) {
    case 0: return "Away"; case 1: return "Low (1)";
    case 2: return "Medium (2)"; case 3: return "High (3)";
    case 4: return "Auto"; case 5: return "Auto 2";
    case 6: return "Boost"; case 7: return "Disabled";
    default: return "Unknown";
  }
}
bool extractMode(const Frame& frame, uint8_t& mode) {
  if (frame.command == 0x31D9 && frame.payload_length >= 3) {
    mode = frame.payload[2];
    return mode <= 7;
  }
  if (frame.command == 0x22F1 && frame.payload_length >= 3) {
    mode = frame.payload[1];
    return mode <= 7;
  }
  return false;
}
uint8_t deviceClass(uint32_t id) { return static_cast<uint8_t>((id >> 18) & 0x3F); }
uint32_t deviceSerial(uint32_t id) { return id & 0x3FFFFU; }
}  // namespace rflink_ramses_ii
