#pragma once
#include <array>
#include <cstdint>

namespace rflink {

enum class RadioId : uint8_t { rf43342, rf43392, rf86830 };
enum class RadioMode : uint8_t { offline, idle, rx, tx, switching, fault };
enum class Modulation : uint8_t { ook, fsk2 };
struct RadioPins { int cs; int gdo0; int gdo2; uint32_t frequency_hz; Modulation modulation; };
struct SpiPins { int sclk; int mosi; int miso; uint32_t clock_hz; };
#include "rflink_board_generated.hpp"
struct RadioState { bool present=false; bool spi_healthy=false; RadioMode mode=RadioMode::offline; bool rx_processing=false; };
// Separate availability from activity: an active RAMSES lease is NOT offline.
inline bool is_online(const RadioState& s) {return s.present && s.spi_healthy && s.mode != RadioMode::fault && s.mode != RadioMode::offline;}

// Contract only. Implementation must enforce exclusive SPI/IRQ/GDO ownership.
class IRadioDriver {
 public:
  virtual ~IRadioDriver() = default;
  virtual bool probe(RadioId id) = 0;
  virtual bool start_rx(RadioId id) = 0;
  virtual bool stop_rx(RadioId id) = 0;
  virtual bool start_tx(RadioId id) = 0;
  virtual bool restore_rx(RadioId id) = 0;
  virtual RadioState state(RadioId id) const = 0;
};
}
