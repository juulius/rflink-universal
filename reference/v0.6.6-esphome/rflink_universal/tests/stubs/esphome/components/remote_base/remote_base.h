#pragma once
#include <vector>
#include <cstdint>
#include <utility>

#include "esphome/components/remote_transmitter/remote_transmitter.h"
namespace esphome { namespace remote_base {
using RemoteTransmitData = remote_transmitter::FakeData;
class RemoteReceiveData { public: explicit RemoteReceiveData(std::vector<int32_t> pulses = {}) : pulses_(std::move(pulses)) {} const std::vector<int32_t>& get_raw_data() const { return pulses_; } private: std::vector<int32_t> pulses_; };
class RemoteReceiverListener { public: virtual ~RemoteReceiverListener()=default; virtual bool on_receive(RemoteReceiveData){return false;} };
}}
