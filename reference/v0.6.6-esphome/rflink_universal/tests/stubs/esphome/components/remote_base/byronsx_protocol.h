#pragma once
#include <cstdint>
#include "esphome/components/remote_base/remote_base.h"
namespace esphome { namespace remote_base {
struct ByronSXData { uint8_t address{0},command{0}; };
class ByronSXProtocol { public: void encode(RemoteTransmitData *out,const ByronSXData &d) const {
  out->address=d.address;out->command=d.command;
}};
}}
