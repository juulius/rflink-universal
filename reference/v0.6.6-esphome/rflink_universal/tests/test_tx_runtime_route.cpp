#include "../components/rflink_universal/universal_tx_route.h"
#include <cassert>
#include <iostream>
using namespace esphome::rflink_universal;
int main() {
 RadioTxSpec a{"example", 2, TxTransport::GDO_SERIAL, TxTransport::SPI_FIFO_PACKET,"fsk",true};
 TxAdapterStatus hw; hw.spi_fifo=true;
 assert(resolve_transmit_route(&a,2,FrameKind::PACKET,hw)==TxTransport::SPI_FIFO_PACKET);
 hw.gdo_serial=true;
 assert(resolve_transmit_route(&a,2,FrameKind::PACKET,hw)==TxTransport::GDO_SERIAL);
 hw.spi_fifo=false;hw.gdo_serial=false;
 assert(resolve_transmit_route(&a,2,FrameKind::PACKET,hw)==TxTransport::NONE);
 hw.spi_fifo=true;
 assert(resolve_transmit_route(&a,1,FrameKind::PACKET,hw)==TxTransport::NONE);
 a.enabled=false;
 assert(resolve_transmit_route(&a,2,FrameKind::PACKET,hw)==TxTransport::NONE);
 a.enabled=true;
 assert(resolve_transmit_route(&a,2,FrameKind::RAW,hw)==TxTransport::NONE);
 RadioTxSpec b{"ook",0,TxTransport::RMT_RAW,TxTransport::NONE,"ook",true};
 hw={};hw.rmt=true;
 for(int i=0;i<10000;++i)assert(resolve_transmit_route(&b,0,FrameKind::RAW,hw)==TxTransport::RMT_RAW);
 RadioTxSpec orphan{"ramses",2,TxTransport::GDO_SERIAL,TxTransport::NONE,"lab",false};
 hw={};hw.gdo_serial=true;
 assert(resolve_transmit_route(&orphan,2,FrameKind::PACKET,hw)==TxTransport::NONE);
 std::cout<<"Runtime TX route and fallback: PASS\n";
}
