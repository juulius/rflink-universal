#pragma once
#include "rflink_radio_manager.hpp"

namespace rflink {
// Pure decision logic: the watchdog may act only on a radio that the manager
// is NOT currently switching or transmitting. No SPI calls or hardware
// resets happen here. Caller checks the same state again via the manager.
enum class RxWatchdogAction : uint8_t { none, stop_then_restart, probe_then_start, start };
inline RxWatchdogAction rx_watchdog_action(const ManagedRadio& state, bool listening) {
  switch (state.operation) {
    case RadioOperation::transmitting:
    case RadioOperation::switching:
      return RxWatchdogAction::none;
    case RadioOperation::receiving:
      return listening ? RxWatchdogAction::none : RxWatchdogAction::stop_then_restart;
    case RadioOperation::idle:
      return state.present && state.spi_healthy ? RxWatchdogAction::start :
        RxWatchdogAction::probe_then_start;
    case RadioOperation::fault:
    case RadioOperation::unavailable:
      return RxWatchdogAction::probe_then_start;
  }
  return RxWatchdogAction::none;
}
}  // namespace rflink
