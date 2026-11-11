#pragma once

// RFLink crash-report transport: observe ESPHome's own crash logger, never
// poke at the private .noinit panic record or reimplement a panic handler.
// Fixed-size capture; safe to keep in internal memory even after an OOM panic.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace esphome::rflink_universal {

struct CrashTraceRecord {
  uint32_t boot_number{0};  // 0: record came from a different build; boot association uncertain
  uint32_t pc{0};
  uint32_t backtrace[16]{};
  uint32_t other_backtrace[16]{};
  char reason[48]{};
  char source_hash[13]{};  // Only recorded when ESPHome verifies this build matches
  uint8_t core{255};
  uint8_t backtrace_count{0};
  uint8_t other_count{0};
  uint8_t foreign_build{0};
};

struct CrashTraceHistory {
  static constexpr uint32_t MARKER = 0x52464348u;  // RFCH, independent NVS key
  uint32_t marker{MARKER};
  uint8_t version{1};
  uint8_t count{0};
  uint16_t reserved{0};
  CrashTraceRecord records[4]{};  // newest first: at most four *actual* technical reports
};

class CrashTraceCollector {
 public:
  void reset() { report_ = {}; collecting_ = false; other_core_ = false; saw_header_ = false; }
  void ingest(const char *tag, const char *message, size_t length) {
    if (!tag || std::strcmp(tag, "esp32.crash") != 0 || !message || !length) return;
    // ESPHome logger may pass the complete formatted line, not a null-terminated message.
    char line[320]{};
    const size_t n = length < sizeof(line) - 1 ? length : sizeof(line) - 1;
    std::memcpy(line, message, n);
    line[n] = '\0';
    if (std::strstr(line, "CRASH DETECTED ON PREVIOUS BOOT")) {
      reset(); collecting_ = true; saw_header_ = true; return;
    }
    if (!collecting_) return;
    if (const char *s = std::strstr(line, "Reason: ")) {
      s += 8;
      std::snprintf(report_.reason, sizeof(report_.reason), "%.*s", 47, s);
      // Some logger implementations append a trailing newline.
      for (char &c : report_.reason) if (c == '\n' || c == '\r') c = '\0';
    }
    if (const char *s = std::strstr(line, "Crashed core: ")) {
      const long value = std::strtol(s + 14, nullptr, 10);
      if (value >= 0 && value <= 1) report_.core = static_cast<uint8_t>(value);
    }
    if (std::strstr(line, "different firmware build")) report_.foreign_build = 1;
    if (std::strstr(line, "Other core (") || std::strstr(line, "other core (")) other_core_ = true;
    const char *pc = std::strstr(line, " PC: 0x");
    if (!pc) pc = std::strstr(line, " pc: 0x");
    if (pc) {
      const unsigned long value = std::strtoul(pc + 7, nullptr, 16);
      if (value) report_.pc = static_cast<uint32_t>(value);
    }
    // Backtrace log lines look like ' BT0: 0x...', or ' bt0: 0x...' for foreign builds.
    const char *bt = std::strstr(line, " BT");
    if (!bt) bt = std::strstr(line, " bt");
    if (bt) {
      bt += 3;
      char *end = nullptr;
      const unsigned long index = std::strtoul(bt, &end, 10);
      if (end != bt && *end == ':' && index < 32) {
        const char *hex = std::strstr(end, "0x");
        if (hex) {
          const uint32_t address = static_cast<uint32_t>(std::strtoul(hex + 2, nullptr, 16));
          if (address) {
            uint8_t &count = other_core_ ? report_.other_count : report_.backtrace_count;
            uint32_t *dest = other_core_ ? report_.other_backtrace : report_.backtrace;
            if (count < 16) dest[count++] = address;
          }
        }
      }
    }
  }
  bool complete() const { return saw_header_ && report_.pc != 0 && report_.core <= 1 && report_.reason[0] != '\0'; }
  CrashTraceRecord &report() { return report_; }
  const CrashTraceRecord &report() const { return report_; }
 private:
  CrashTraceRecord report_{};
  bool collecting_{false};
  bool other_core_{false};
  bool saw_header_{false};
};

}  // namespace esphome::rflink_universal
