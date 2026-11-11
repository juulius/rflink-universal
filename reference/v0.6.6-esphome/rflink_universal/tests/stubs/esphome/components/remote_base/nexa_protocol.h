#pragma once
#include <cstdint>
#include "esphome/components/remote_transmitter/remote_transmitter.h"
namespace esphome { namespace remote_base {
struct NexaData { uint32_t device{0}; uint8_t group{0},state{0},channel{0},level{0}; };
class NexaProtocol {public: void encode(remote_transmitter::FakeData *out,const NexaData &d) const {
  out->device=d.device;out->group=d.group;out->state=d.state;out->channel=d.channel;out->level=d.level;
}};
}}
