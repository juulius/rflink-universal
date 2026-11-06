#pragma once
// RFLink Universal - RAMSES-II (RAMSES-II family) receive decoder.
// C++11, platform independent, no heap allocation, no radio register writes.
// Decodes bit-packed CC1101 packet captures and already UART-deframed wire bytes.
#include <stddef.h>
#include <stdint.h>

namespace rflink_ramses_ii {
static const size_t MAX_PAYLOAD = 64;
struct Frame {
  enum Type { UNKNOWN, INFORMATION, REQUEST, RESPONSE, WRITE } type;
  bool has_addr[3];
  uint32_t addr[3];   // 24 bit wire ID: (class << 18) | serial
  uint16_t command;
  uint8_t payload_length;
  uint8_t payload[MAX_PAYLOAD];
  Frame();
};

// Validates RAMSES-II checksum, declared payload length and address layout.
bool parseLogical(const uint8_t* logical, size_t length, Frame& result);

// Input is MSB-first, bit-packed demodulated stream as seen in the current
// RFLink 868.30 MHz PACKET logs (63-byte packet captures). Supports incomplete
// leading sync and arbitrary bit offset, rejects corrupted frames.
bool decodePacked(const uint8_t* packed, size_t length, Frame& result);

// Alternative for future CC1101 asynchronous GDO2 -> UART RX collection;
// input contains UART-deframed bytes (FF 00 33 55 53 ... 35).
bool decodeWire(const uint8_t* wire, size_t length, Frame& result);

// Classify command semantics; does not assert Orcon device vendor from protocol
// alone since RAMSES-II is shared with other HVAC equipment.
const char* commandName(uint16_t command);
const char* modeName(uint8_t mode);
// 31D9 status and 22F1 command have different payload offsets.
// Returns false for unsupported/truncated payloads.
bool extractMode(const Frame& frame, uint8_t& mode);
uint8_t deviceClass(uint32_t raw_addr);
uint32_t deviceSerial(uint32_t raw_addr);
}  // namespace rflink_ramses_ii
