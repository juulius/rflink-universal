// Standalone host unit/stress tests for the per-protocol radio-v2 contract.
#include "../components/rflink_universal/decoder_dispatch_v2.h"
#include "../components/rflink_universal/radio_transport_policy.h"
#include "../components/rflink_universal/radio_profile_plan_v2.h"
#include "../components/rflink_universal/radio_v2_catalog_generated.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
using namespace esphome::rflink_universal;
struct Dummy : RFDecoder {
  std::string id;
  int calls{0};
  uint8_t score{0};
  bool succeeds{false};
  std::string identity{"one"};
  Dummy(std::string s,uint8_t n,bool ok) : id(std::move(s)),score(n),succeeds(ok){}
  const char *name() const override{return id.c_str();}
  uint8_t match(const RFFrame &) const override {const_cast<Dummy*>(this)->calls++;return score;}
  bool decode(const RFFrame &,DecodeResult &out) const override {
    if(!succeeds)return false;
    out.valid=true;out.protocol=id;out.identity_key=identity;out.action="status";return true;
  }
};
int main(){
  const auto &index=radio_v2_catalog::index();
  RFFrame frame;frame.channel=2;frame.kind=FrameKind::PACKET;
  assert(index.allows("RamsesII",frame));
  assert(!index.allows("SomfyRTS56",frame));
  Dummy false_positive("RamsesII",99,false),verified("IthoRFT",90,true),unrelated("SomfyRTS56",100,true);
  std::vector<RFDecoder*> decoders{&false_positive,&verified,&unrelated};
  auto r=DecoderDispatchV2::run(frame,decoders,index,60);
  assert(r.recognized && r.winner==&verified && r.confidence==90);
  assert(unrelated.calls==0 && r.diagnostics.skipped_by_route==1);
  Dummy conflict("RamsesII",90,true);decoders={&verified,&conflict};
  r=DecoderDispatchV2::run(frame,decoders,index,60);
  assert(r.ambiguous && !r.recognized); // never publish two equal-confidence devices
  RFFrame raw; raw.kind=FrameKind::RAW;raw.channel=1;
  assert(index.allows("Nexa",raw));assert(!index.allows("IthoRFT",raw));
  assert(index.allows("third_party_decoder",raw)); // migration-safe default
  std::vector<std::unique_ptr<Dummy>> owned;
  std::vector<RFDecoder*> many;
  for(int i=0;i<100;++i){
    // The irrelevant 97 packs should never execute match().
    owned.emplace_back(new Dummy(i<97?"SomfyRTS56":"AlectoV1",25,false));
    many.push_back(owned.back().get());
  }
  auto started=std::chrono::steady_clock::now();
  for(int n=0;n<10000;++n) {
    const auto got=DecoderDispatchV2::run(raw,many,index,60);
    assert(!got.recognized);
  }
  auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
  // Somfy is on channel 0, not channel 1.
  for(int i=0;i<97;++i)assert(owned[i]->calls==0);
  assert(owned[97]->calls==10000);
  size_t count=0; const auto *routes=radio_v2_catalog::specs(count);
  RadioProfilePlanV2 plan(routes,count);
  assert(plan.can_listen_without_switch(0));
  assert(plan.can_listen_without_switch(1));
  assert(!plan.can_listen_without_switch(2)); // Itho and RAMSES require unproven different capture paths.
  assert(plan.groups_on(2)>=2);
  // A physically compatible capture can be shared by many decoder subscribers.
  const RadioInputSpec compatible[]={{"a",1,InputKind::RAW,"ook_raw_43392","raw_rmt"},
                                     {"b",1,InputKind::RAW,"ook_raw_43392","raw_rmt"}};
  RadioProfilePlanV2 shared(compatible,2);
  assert(shared.groups_on(1)==1 && shared.groups()[0].subscribers==2);
  // Scan scheduler can handle 100 protocols, unlike the old 16-entry cap.
  std::vector<DecoderRadioSpec> specs;
  std::vector<std::string> names;
  names.reserve(100);specs.reserve(100);
  for(int i=0;i<100;++i){
    names.push_back("lab_"+std::to_string(i));
    specs.push_back({names.back().c_str(),2,RxTransport::UART_STREAM,RxTransport::SPI_PACKET,200,1000,100});
  }
  RadioTransportPolicy policy(specs.data(),specs.size());
  RadioScanScheduler scheduler;
  for(int i=0;i<100;++i){
    const auto *due=scheduler.due(policy,1234,true,true);
    assert(due && due->protocol==names[i]);
    scheduler.mark(policy,due->protocol,1234);
  }
  assert(scheduler.due(policy,1234,true,true)==nullptr);
  std::cout<<"Radio Engine V2 PASS: 100 decoders, 100 scheduler entries, 10k captures in "<<us<<" us\n";
}
