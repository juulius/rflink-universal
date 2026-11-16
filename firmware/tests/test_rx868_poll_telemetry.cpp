// Deterministic, host-only 868 MHz FIFO telemetry tests.
// No radio calls or live RF data; tests actual counter semantics.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "rflink_rx868_poll_telemetry.hpp"

int main() {
  rflink::Rx868PollTelemetry telemetry{};
  uint64_t previous=0;
  rflink::observe_rx868_poll(telemetry,1000000,previous);
  assert(telemetry.polls==1 && telemetry.last_gap_us==0);
  rflink::observe_rx868_poll(telemetry,1010000,previous); // 10 ms
  assert(telemetry.polls==2 && telemetry.max_gap_us==10000);
  assert(telemetry.gaps_over_20ms==0);
  rflink::observe_rx868_poll(telemetry,1041000,previous); // 31 ms
  assert(telemetry.last_gap_us==31000);
  assert(telemetry.max_gap_us==31000);
  assert(telemetry.gaps_over_20ms==1);
  rflink::observe_rx868_poll(telemetry,1051000,previous);
  assert(telemetry.last_gap_us==10000 && telemetry.max_gap_us==31000);

  // Values are actual successful status samples, not validated RF frames.
  rflink::observe_rx868_status(telemetry,0x0d,0x00);
  rflink::observe_rx868_status(telemetry,0x0d,0x01);
  rflink::observe_rx868_status(telemetry,0x0d,0x3e);
  rflink::observe_rx868_status(telemetry,0x0d,0x3f);
  assert(telemetry.empty_fifo_polls==1);
  assert(telemetry.partial_fifo_polls==2);
  assert(telemetry.ready_fifo_polls==1);
  assert(telemetry.peak_fifo_bytes==63);
  assert(telemetry.last_fifo_bytes==63);
  assert(telemetry.valid_status_polls==4);

  // RXBYTES overflow bit and MARCSTATE RXFIFO_OVERFLOW must not
  // turn the low seven bits into fictitious FIFO occupancies.
  rflink::observe_rx868_status(telemetry,0x11,0x41);
  rflink::observe_rx868_status(telemetry,0x0d,0xc1);
  assert(telemetry.valid_status_polls==6);
  assert(telemetry.last_fifo_bytes==63);
  assert(telemetry.peak_fifo_bytes==63);
  assert(telemetry.ready_fifo_polls==1);

  // No stale interval after a stop/start or TX handover.
  previous=0;
  rflink::observe_rx868_poll(telemetry,9000000,previous);
  assert(telemetry.polls==5 && telemetry.last_gap_us==10000);
  rflink::observe_rx868_poll(telemetry,9010000,previous);
  assert(telemetry.max_gap_us==31000);

  // A time wrap/backward clock cannot create an artificial huge latency.
  rflink::observe_rx868_poll(telemetry,1000,previous);
  assert(telemetry.max_gap_us==31000);

  std::puts("RX868 FIFO polling evidence: PASS");
  return 0;
}
