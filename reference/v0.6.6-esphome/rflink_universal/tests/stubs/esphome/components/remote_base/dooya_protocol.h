#pragma once
#include <cstdint>
#include "esphome/components/remote_base/remote_base.h"
namespace esphome { namespace remote_base {
struct DooyaData { uint32_t id{0};uint8_t channel{0},button{0},check{0}; };
class DooyaProtocol {public: void encode(RemoteTransmitData *out,const DooyaData &d)const{
 out->device=d.id;out->channel=d.channel;out->button=d.button;out->check=d.check;
}};
}}
