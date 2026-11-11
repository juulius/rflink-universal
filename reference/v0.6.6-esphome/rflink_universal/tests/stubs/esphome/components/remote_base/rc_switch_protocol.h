#pragma once
#include <cstdint>
#include "esphome/components/remote_base/remote_base.h"
namespace esphome {namespace remote_base {
struct FakeRCSwitchProtocol {
 uint8_t proto{0};
 void transmit(RemoteTransmitData *out,uint64_t code,uint8_t bits) const {
 out->protocol=proto;out->code=code;out->bit_length=bits;
 }
};
static const FakeRCSwitchProtocol RC_SWITCH_PROTOCOLS[9]={{0},{1},{2},{3},{4},{5},{6},{7},{8}};
}}
