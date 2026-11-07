#include "rflink_radio_manager.hpp"
namespace rflink {
void RadioManager::set_presence(RadioId radio, bool present, bool healthy) {
  std::lock_guard<std::mutex> guard(mutex_);
  const auto i=index(radio); if(i>=states_.size())return;
  auto& st=states_[i];
  if(st.operation==RadioOperation::receiving || st.operation==RadioOperation::transmitting || st.operation==RadioOperation::switching)return;
  st.present=present; st.spi_healthy=healthy;
  st.operation=present&&healthy?RadioOperation::idle:RadioOperation::unavailable;
  st.last_error=ManagerError::none;
}
ManagerError RadioManager::start_receive(RadioId radio, RfProfileId profile_id) {
  std::lock_guard<std::mutex> guard(mutex_);
  const auto i=index(radio); if(i>=states_.size())return ManagerError::invalid_radio;
  auto& st=states_[i];
  if(!st.present || !st.spi_healthy)return st.last_error=ManagerError::no_hardware;
  if(st.operation!=RadioOperation::idle)return st.last_error=ManagerError::busy;
  const auto& p=rf_profile(profile_id);
  if(p.id!=profile_id || !profile_compatible_with_radio(p,radio))return st.last_error=ManagerError::incompatible_profile;
  if(!profile_has_registers(p))return st.last_error=ManagerError::missing_profile;
  const bool packet = p.rx_transport==RxTransport::packet_fifo && radio==RadioId::rf86830 && profile_id==RfProfileId::itho_868_packet;
  const bool pulses = p.rx_transport==RxTransport::gdo_rmt && (radio==RadioId::rf43342 || radio==RadioId::rf43392);
  if(!packet && !pulses)return st.last_error=ManagerError::unsupported_transport;
  st.operation=RadioOperation::switching;
  if(!(packet ? transport_.start_packet_rx(radio,p) : transport_.start_pulse_rx(radio,p))) { st.operation=RadioOperation::fault; return st.last_error=ManagerError::driver_failure; }
  st.profile=profile_id;st.operation=RadioOperation::receiving;st.transitions++;return st.last_error=ManagerError::none;
}
ManagerError RadioManager::stop_receive(RadioId radio) {
  std::lock_guard<std::mutex> guard(mutex_);
  const auto i=index(radio); if(i>=states_.size())return ManagerError::invalid_radio;
  auto& st=states_[i];
  if(st.operation!=RadioOperation::receiving)return st.last_error=ManagerError::busy;
  st.operation=RadioOperation::switching;
  if(!(st.profile==RfProfileId::itho_868_packet ? transport_.stop_packet_rx(radio) : transport_.stop_pulse_rx(radio))) {st.operation=RadioOperation::fault;return st.last_error=ManagerError::driver_failure;}
  st.operation=RadioOperation::idle;st.transitions++;return st.last_error=ManagerError::none;
}
ManagerError RadioManager::validate_tx(RadioId radio, RfProfileId profile_id, const EncodedTxFrame* frame) {
  const auto i=index(radio); if(i>=states_.size())return ManagerError::invalid_radio;
  auto& st=states_[i];
  if(!st.present || !st.spi_healthy)return st.last_error=ManagerError::no_hardware;
  if(st.operation!=RadioOperation::idle && st.operation!=RadioOperation::receiving)return st.last_error=ManagerError::busy;
  const auto& p=rf_profile(profile_id);
  if(p.id!=profile_id || !profile_compatible_with_radio(p,radio))return st.last_error=ManagerError::incompatible_profile;
  bool supported=false;
  switch(p.tx_transport){
    case TxTransport::gdo_uart: supported=uart_tx_preset_valid(p);break;
    case TxTransport::gdo_rmt: supported=rmt_tx_preset_valid(p);break;
    case TxTransport::packet_fifo: supported=packet_tx_preset_valid(p);break;
  }
  if(!supported)return st.last_error=ManagerError::unsupported_transport;
  if(frame){
    if(frame->repeats<1 || frame->repeats>10 || frame->gap_ms>1000)
      return st.last_error=ManagerError::invalid_payload;
    if(p.tx_transport==TxTransport::gdo_rmt){
      if(!frame->pulses || frame->pulse_count==0 || frame->pulse_count>256 || frame->wire || frame->length)
        return st.last_error=ManagerError::invalid_payload;
      uint32_t total_us=0;
      for(size_t j=0;j<frame->pulse_count;j++) {
        const auto& pulse=frame->pulses[j];
        if(pulse.high_us==0 || pulse.low_us==0 || pulse.high_us>32767 || pulse.low_us>32767)
          return st.last_error=ManagerError::invalid_payload;
        total_us+=uint32_t(pulse.high_us)+pulse.low_us;
      }
      if(total_us>500000)return st.last_error=ManagerError::invalid_payload;
    }else if(p.tx_transport==TxTransport::packet_fifo){
      if(!frame->wire || frame->length!=p.packet_length || frame->pulses || frame->pulse_count)
        return st.last_error=ManagerError::invalid_payload;
    }else{
      if(!frame->wire || frame->length<4 || frame->length>512 || frame->pulses || frame->pulse_count)
        return st.last_error=ManagerError::invalid_payload;
    }
  }
  return ManagerError::none;
}
ManagerError RadioManager::request_transmit(RadioId radio, RfProfileId profile_id) {
  std::lock_guard<std::mutex> guard(mutex_);
  // This is a side-effect-free TX preflight, not a request to claim the
  // hardware. A live receiver is eligible: transmit_encoded() performs the
  // serialized RX -> TX -> original RX handover when the actual frame arrives.
  // Reject only genuinely busy/switching/faulted radios via validate_tx().
  return validate_tx(radio,profile_id,nullptr);
}
ManagerError RadioManager::transmit_encoded(RadioId radio, RfProfileId profile_id, const EncodedTxFrame& frame) {
  std::lock_guard<std::mutex> guard(mutex_);
  TxTrace trace{};
  // A side-effect-free request_transmit() is not a send attempt and does not
  // get a correlation ID. Only actual transmit_encoded() calls are traced.
  trace.correlation_id=++next_tx_correlation_id_;
  if(trace.correlation_id==0)trace.correlation_id=++next_tx_correlation_id_;
  trace.radio=radio;
  trace.profile=profile_id;
  trace.units=static_cast<uint16_t>(
    (frame.pulse_count?frame.pulse_count:frame.length)>65535u
       ?65535u:(frame.pulse_count?frame.pulse_count:frame.length));
  trace.repeats=frame.repeats;
  auto report=[&](TxTracePhase phase, ManagerError err,TxFailureStage stage)->ManagerError{
    trace.phase=phase;
    trace.error=err;
    trace.stage=stage;
    // Observers receive only timing/transport/error metadata: no payload,
    // device key, rolling code, user identifier or claimed on-air success.
    if(trace_observer_)trace_observer_(trace_context_,trace);
    return err;
  };
  const auto i=index(radio);
  if(i>=states_.size())
    return report(TxTracePhase::rejected,ManagerError::invalid_radio,TxFailureStage::none);
  // A rejected preflight must not preserve a previous TX failure stage.
  tx_stages_[i]=TxFailureStage::none;
  const auto valid=validate_tx(radio,profile_id,&frame);
  if(valid!=ManagerError::none)
    return report(TxTracePhase::rejected,valid,TxFailureStage::none);
  auto& st=states_[i];
  const auto& profile=rf_profile(profile_id);
  trace.transport=profile.tx_transport;
  trace.was_receiving=st.operation==RadioOperation::receiving;
  trace.rx_restored=!trace.was_receiving; // No prior RX to restore for an idle radio.
  const RfProfileId prior=st.profile;
  const bool prior_packet=trace.was_receiving && prior==RfProfileId::itho_868_packet;
  report(TxTracePhase::started,ManagerError::none,TxFailureStage::none);
  st.operation=RadioOperation::switching;
  if(trace.was_receiving &&
     !(prior_packet ? transport_.stop_packet_rx(radio) : transport_.stop_pulse_rx(radio))) {
    st.operation=RadioOperation::fault;
    tx_stages_[i]=TxFailureStage::rx_pause;
    trace.rx_restored=false;
    return report(TxTracePhase::finished,st.last_error=ManagerError::restore_failure,
                  TxFailureStage::rx_pause);
  }
  st.operation=RadioOperation::transmitting;
  trace.driver_attempted=true;
  PhysicalTxResult outcome{};
  switch(profile.tx_transport){
    case TxTransport::gdo_uart: outcome=transport_.transmit_uart(radio,profile,frame);break;
    case TxTransport::gdo_rmt: outcome=transport_.transmit_rmt(radio,profile,frame);break;
    case TxTransport::packet_fifo: outcome=transport_.transmit_packet(radio,profile,frame);break;
  }
  trace.driver_accepted=outcome.sent;
  trace.hardware_restored=outcome.hardware_restored;
  tx_stages_[i]=outcome.stage;
  if(!outcome.hardware_restored){
    st.operation=RadioOperation::fault;
    return report(TxTracePhase::finished,st.last_error=ManagerError::restore_failure,
                  outcome.stage);
  }
  // Restore the exact prior RX profile, even if the physical TX driver failed.
  st.operation=RadioOperation::switching;
  if(trace.was_receiving &&
     !(prior_packet ? transport_.start_packet_rx(radio,rf_profile(prior)) :
                      transport_.start_pulse_rx(radio,rf_profile(prior)))) {
    st.operation=RadioOperation::fault;
    tx_stages_[i]=TxFailureStage::restore;
    return report(TxTracePhase::finished,st.last_error=ManagerError::restore_failure,
                  TxFailureStage::restore);
  }
  st.operation=trace.was_receiving?RadioOperation::receiving:RadioOperation::idle;
  st.profile=prior;
  ++st.transitions;
  trace.rx_restored=true;
  return report(TxTracePhase::finished,
                st.last_error=outcome.sent?ManagerError::none:ManagerError::tx_failure,
                outcome.stage);
}
ManagedRadio RadioManager::status(RadioId radio) const {
  std::lock_guard<std::mutex> guard(mutex_);
  const auto i=index(radio);return i<states_.size()?states_[i]:ManagedRadio{};
}
TxFailureStage RadioManager::last_tx_stage(RadioId radio) const {
  std::lock_guard<std::mutex> guard(mutex_);
  const auto i=index(radio);return i<tx_stages_.size()?tx_stages_[i]:TxFailureStage::unsupported;
}
}
