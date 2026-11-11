#include <cassert>
#include <cstring>
#include "../components/rflink_universal/radio_transport_catalog_generated.h"
using namespace esphome::rflink_universal;
int main() {
  auto policy=radio_catalog::policy();
  RadioScanScheduler scheduler;
  assert(scheduler.due(policy,100,true,true)!=nullptr);
  scheduler.mark(policy,"RamsesII",100);
  assert(scheduler.due(policy,101,true,true)==nullptr);
  assert(scheduler.due(policy,5100,true,true)!=nullptr);
  assert(scheduler.due(policy,5100,false,true)==nullptr);
  assert(scheduler.due(policy,5100,true,false)==nullptr);
  uint32_t uart=RadioTransportPolicy::bit(RxTransport::UART_STREAM);
  uint32_t spi=RadioTransportPolicy::bit(RxTransport::SPI_FIFO_SYNC);
  auto r=policy.choose("RamsesII",2,uart|spi,false,true,3000);
  assert(r.decision==RadioTransportPolicy::Decision::SELECTED && r.transport==RxTransport::UART_STREAM);
  r=policy.choose("RamsesII",2,spi,false,true,3000);
  assert(r.transport==RxTransport::SPI_FIFO_SYNC); // only when UART cannot be allocated
  r=policy.choose("RamsesII",2,spi,true,true,3000);
  assert(r.decision==RadioTransportPolicy::Decision::RADIO_BUSY);
  r=policy.choose("RamsesII",2,0,false,true,3000);
  assert(r.decision==RadioTransportPolicy::Decision::UNAVAILABLE);
  r=policy.choose("RamsesII",2,uart,false,false,3000);
  assert(r.decision==RadioTransportPolicy::Decision::DISABLED);
  r=policy.choose("RamsesII",2,uart,false,true,30001);
  assert(r.decision==RadioTransportPolicy::Decision::INVALID_WINDOW);
  r=policy.choose("IthoRFT",2,RadioTransportPolicy::bit(RxTransport::SPI_PACKET),false,true,1000);
  assert(r.transport==RxTransport::SPI_PACKET);
  r=policy.choose("RamsesII",0,uart,false,true,1000);
  assert(r.decision==RadioTransportPolicy::Decision::NO_DECODER);
  r=policy.choose("Orcon",2,uart,false,true,1000);
  assert(r.decision==RadioTransportPolicy::Decision::NO_DECODER);
  assert(std::strcmp(transport_name(RxTransport::UART_STREAM),"uart_stream")==0);
}
