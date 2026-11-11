#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace esphome {
namespace rflink_universal {
namespace pulse {

struct Timing {
  uint32_t short_us{0};
  uint32_t long_us{0};
  uint8_t tolerance_percent{25};
};

enum class PairEncoding : uint8_t {
  SHORT_LONG_IS_ZERO,
  SHORT_LONG_IS_ONE,
};

inline bool near_us(int32_t pulse, uint32_t target, uint8_t tolerance_percent) {
  const uint32_t value = static_cast<uint32_t>(std::abs(pulse));
  const uint32_t lo = target * (100u - tolerance_percent) / 100u;
  const uint32_t hi = target * (100u + tolerance_percent) / 100u;
  return value >= lo && value <= hi;
}

inline bool decode_pairs(const std::vector<int32_t> &raw, size_t start, size_t bit_count,
                         const Timing &timing, PairEncoding encoding,
                         std::vector<bool> &bits) {
  if (start + bit_count * 2 > raw.size()) return false;
  bits.clear();
  bits.reserve(bit_count);
  for (size_t i = 0; i < bit_count; i++) {
    const int32_t a = raw[start + i * 2];
    const int32_t b = raw[start + i * 2 + 1];
    const bool sl = near_us(a, timing.short_us, timing.tolerance_percent) &&
                    near_us(b, timing.long_us, timing.tolerance_percent);
    const bool ls = near_us(a, timing.long_us, timing.tolerance_percent) &&
                    near_us(b, timing.short_us, timing.tolerance_percent);
    if (!sl && !ls) return false;
    const bool one = encoding == PairEncoding::SHORT_LONG_IS_ONE ? sl : ls;
    bits.push_back(one);
  }
  return true;
}

inline uint64_t bits_to_uint(const std::vector<bool> &bits, size_t start, size_t count) {
  if (start >= bits.size()) return 0;
  count = std::min(count, bits.size() - start);
  uint64_t value = 0;
  for (size_t i = 0; i < count; i++) value = (value << 1u) | (bits[start + i] ? 1u : 0u);
  return value;
}

inline uint8_t reverse8(uint8_t value) {
  value = static_cast<uint8_t>(((value & 0xF0u) >> 4u) | ((value & 0x0Fu) << 4u));
  value = static_cast<uint8_t>(((value & 0xCCu) >> 2u) | ((value & 0x33u) << 2u));
  return static_cast<uint8_t>(((value & 0xAAu) >> 1u) | ((value & 0x55u) << 1u));
}

inline std::vector<uint32_t> cluster_widths(const std::vector<int32_t> &raw, uint32_t quantum_us = 20) {
  std::vector<uint32_t> buckets;
  for (auto value : raw) {
    const uint32_t width = static_cast<uint32_t>(std::abs(value));
    if (width == 0) continue;
    buckets.push_back(((width + quantum_us / 2u) / quantum_us) * quantum_us);
  }
  std::sort(buckets.begin(), buckets.end());
  return buckets;
}

}  // namespace pulse
}  // namespace rflink_universal
}  // namespace esphome
