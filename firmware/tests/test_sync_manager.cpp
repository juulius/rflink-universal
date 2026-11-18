// Pure C++ contract: a physical 868 serial-bit receiver must be owned by
// RadioManager and restored as the SAME RX path after every TX handover.
// Synthetic radio only; no CC1101 signal observed on real hardware.
#include <cassert>
#include "rflink_radio_manager.hpp"
using namespace rflink;

struct SyncTransportMock final : RadioTransport {
  bool offer_sync = false;
  bool fail_sync_start = false;
  bool fail_sync_stop = false;
  bool fail_tx = false;
  bool fail_tx_restore = false;
  int packet_starts = 0;
  int packet_stops = 0;
  int pulse_starts = 0;
  int pulse_stops = 0;
  int sync_starts = 0;
  int sync_stops = 0;
  int tx_calls = 0;
  bool start_packet_rx(RadioId, const RfProfile&) override {
    ++packet_starts; return true;
  }
  bool stop_packet_rx(RadioId) override {++packet_stops;return true;}
  bool start_pulse_rx(RadioId,const RfProfile&) override {
    ++pulse_starts; return true;
  }
  bool stop_pulse_rx(RadioId) override {++pulse_stops;return true;}
  bool supports_sync_rx(RadioId radio,const RfProfile& p) const override {
    return offer_sync && radio==RadioId::rf86830 &&
           p.id==RfProfileId::sync_868_bench;
  }
  bool start_sync_rx(RadioId radio,const RfProfile& p) override {
    assert(radio==RadioId::rf86830 && p.id==RfProfileId::sync_868_bench);
    ++sync_starts;return !fail_sync_start;
  }
  bool stop_sync_rx(RadioId radio) override {
    assert(radio==RadioId::rf86830); ++sync_stops;return !fail_sync_stop;
  }
  PhysicalTxResult transmit_uart(RadioId radio,const RfProfile& p,
                                 const EncodedTxFrame&) override {
    assert(radio==RadioId::rf86830 && p.id==RfProfileId::ramses_868_packet);
    ++tx_calls;return {!fail_tx,!fail_tx_restore,TxFailureStage::none};
  }
};
int main() {
  const auto& p=rf_profile(RfProfileId::sync_868_bench);
  assert(p.rx_transport==RxTransport::gdo_sync);
  assert(p.modulation==Modulation::fsk2);
  assert(p.frequency_hz==868299900);
  assert(!uart_tx_preset_valid(p)); // Receive-only; no new TX capability.
  assert(profile_has_registers(p));
  SyncTransportMock fake;
  RadioManager manager(fake);
  manager.set_presence(RadioId::rf86830,true,true);

  // Old firmware does not enable the bench adapter: no FIFO or UART fallback.
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::sync_868_bench)
         ==ManagerError::unsupported_transport);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::idle);
  assert(fake.sync_starts==0 && fake.packet_starts==0);
  fake.offer_sync=true;

  // A valid manager lease routes directly to the sync transport.
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::sync_868_bench)
         ==ManagerError::none);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::sync_868_bench);
  assert(fake.sync_starts==1 && fake.packet_starts==0);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)
         ==ManagerError::busy); // No background mode hopping.
  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)
         ==ManagerError::none); // Side-effect-free preflight.
  assert(fake.sync_stops==0);
  uint8_t wire[5]={0x33,0x55,0x53,0x00,0x35};
  EncodedTxFrame tx{};
  tx.wire=wire;tx.length=sizeof(wire);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)
         ==ManagerError::none);
  assert(fake.sync_stops==1 && fake.sync_starts==2 && fake.tx_calls==1);
  assert(fake.packet_starts==0 && fake.packet_stops==0);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::sync_868_bench);

  // A failed over-air TX does not permit fallback to packet FIFO either.
  fake.fail_tx=true;
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)
         ==ManagerError::tx_failure);
  assert(fake.sync_stops==2 && fake.sync_starts==3);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  fake.fail_tx=false;

  // Failure to restart SAME sync RX must show fault, not fake listening.
  fake.fail_sync_start=true;
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)
         ==ManagerError::restore_failure);
  assert(fake.sync_stops==3 && fake.sync_starts==4);
  assert(!manager.status(RadioId::rf86830).online());
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(fake.packet_starts==0);
  fake.fail_sync_start=false;
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::sync_868_bench)
         ==ManagerError::none);
  assert(fake.sync_starts==5);

  // Physical loss must release the sync capture, even if its stop fails.
  fake.fail_sync_stop=true;
  const int prior_stops=fake.sync_stops;
  assert(manager.mark_hardware_lost(RadioId::rf86830));
  assert(fake.sync_stops==prior_stops+1);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::unavailable);
  assert(!manager.status(RadioId::rf86830).online());
  assert(fake.packet_starts==0);

  // Fresh probe can start again under a new exclusive receiver lease.
  fake.fail_sync_stop=false;
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::sync_868_bench)
         ==ManagerError::none);
  assert(manager.stop_receive(RadioId::rf86830)==ManagerError::none);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::idle);
  assert(fake.packet_starts==0);
}
