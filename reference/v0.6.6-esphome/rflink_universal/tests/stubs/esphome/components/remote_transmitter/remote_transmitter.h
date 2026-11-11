#pragma once
#include <cstdint>
#include <vector>
namespace esphome { namespace remote_transmitter {
struct FakeData { uint32_t device{0}; uint8_t address{0}, command{0}; uint8_t group{0},state{0},channel{0},level{0},button{0},check{0},protocol{0},bit_length{0};uint64_t code{0}; std::vector<int32_t> pulses; void mark(uint32_t us){pulses.push_back(int32_t(us));} void space(uint32_t us){pulses.push_back(-int32_t(us));} };
class RemoteTransmitterComponent {
 public:
  unsigned sent_count{0};
  FakeData last_data{};
  int last_repeats{0};
  struct Call {
    RemoteTransmitterComponent *parent;
    FakeData data{};
    int repeats{0};
    FakeData *get_data(){return &data;}
    void set_send_times(int n){repeats=n;}
    void perform(){parent->last_data=data;parent->last_repeats=repeats;parent->sent_count++;}
  };
  Call transmit(){return {this};}
};
}}
