#pragma once
// RAMSES4: experimental SPI-only CC1101 FIFO transport, bench-only.
// No UART, no pin reconfiguration. Uses the same SPI bus/CS already owned by the CC1101.
// All I/O must run in the same single ESPHome event loop while RadioModeManager owns radio 2.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#if __has_include("rflink_decoders/ramses_ii/codec.h")
#include "rflink_decoders/ramses_ii/codec.h"
#include "rflink_decoders/ramses_ii/tx_codec.h"
#else
#include "../../decoders/ramses_ii/codec.h"
#include "../../decoders/ramses_ii/tx_codec.h"
#endif

namespace esphome { namespace rflink_universal { namespace ramses_spi {

struct Bus {
  virtual ~Bus() = default;
  virtual bool read(uint8_t reg, uint8_t &value) = 0;
  virtual bool write(uint8_t reg, uint8_t value) = 0;
  virtual bool strobe(uint8_t command) = 0;
  virtual bool read_fifo(uint8_t* data, size_t count) = 0;
  virtual bool write_fifo(const uint8_t* data, size_t count) = 0;
};

struct Stats {
  uint32_t rx_frames{0}, rx_overflows{0}, rx_timeouts{0}, rx_invalid{0};
  uint32_t tx_frames{0}, tx_underflows{0}, io_errors{0};
  // Hardware-lab diagnostics. Number of bytes actually transferred, not inferred RF frames.
  uint32_t rx_bytes{0}, rx_polls{0}, tx_bytes{0};
  uint8_t rx_fifo_peak{0}, tx_fifo_peak{0};
  uint16_t rx_max_poll_gap_ms{0};
  uint32_t rx_slow_polls{0};
};

class FifoTransport {
 public:
  enum class State: uint8_t { OFF, RX, TX, ERROR };
  explicit FifoTransport(Bus &bus, bool unsynced_capture=false)
      : bus_(bus), unsynced_capture_(unsynced_capture) {}
  bool start_rx(uint32_t now_ms);
  bool poll_rx(uint32_t now_ms, rflink_ramses_ii::Frame &frame);
  // Only for isolated lab use; TX waveform is experimental, not field verified.
  bool start_tx(const rflink_ramses_ii::Frame &frame, uint32_t now_ms);
  bool poll_tx(uint32_t now_ms);
  bool stop();
  State state() const {return state_;}
  bool unsynced_capture() const {return unsynced_capture_;}
  const Stats& stats() const {return stats_;}
  // Nominal 16-bit on-air sync B32A is situated at bit 19 of the UART header.
  // The payload starts at bit 35. Decoding prepends bits [0..34] to recover
  // 33 55 53 for checksum-protected RAMSES validation.
  static constexpr uint16_t SYNC_WORD = 0xB32A;
  static constexpr size_t SYNC_END_BIT = 35;
  static bool make_fifo_payload(const rflink_ramses_ii::Frame &frame,
                                std::vector<uint8_t> &payload);
  static bool parse_fifo_capture(const uint8_t *payload, size_t length,
                                 rflink_ramses_ii::Frame &frame);
 private:
  bool config(bool transmit);
  bool fail();
  bool flush_rx();
  bool fill_tx();
  bool read_rx_count(uint8_t &count);
  Bus &bus_;
  bool unsynced_capture_{false};
  State state_{State::OFF};
  Stats stats_{};
  std::array<uint8_t,224> rx_{};
  size_t rx_len_{0};
  uint32_t rx_started_{0}, rx_last_activity_ms_{0}, rx_last_poll_ms_{0}, tx_started_{0};
  uint32_t singleton_first_seen_ms_{0};
  bool singleton_seen_{false};
  std::vector<uint8_t> tx_{};
  size_t tx_pos_{0};
  bool tx_done_{false};
};

}}} // esphome::rflink_universal::ramses_spi
