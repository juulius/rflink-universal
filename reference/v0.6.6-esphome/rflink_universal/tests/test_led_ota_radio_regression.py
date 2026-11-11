"""Host integration: compile the actual ESPHome OTA YAML lambdas with a fake light driver.

No GPIO/ESP32 test claims: checks each real YAML callback calls LightState::loop()
to flush the queued color and that no-radio status is not READY. The test also
rejects the pointer-to-pointer expression reported by the actual ESPHome compiler.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
import yaml

ROOT = Path(__file__).resolve().parents[2]

class Loader(yaml.SafeLoader):
    pass

def any_tag(loader, suffix, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_mapping(node)

Loader.add_multi_constructor('!', any_tag)
config = yaml.load((ROOT / 'rflink-universal.yaml').read_text(), Loader=Loader)
callbacks = config['ota'][0]
assert len(config['cc1101']) == 3, 'Do not remove the 868MHz radio'
assert config['cc1101'][2]['packet_mode'] is True, 'Keep 868MHz packet mode'
source = (ROOT / 'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
assert 'void RFLinkUniversal::set_radio_available' in source
method = source.split('void RFLinkUniversal::set_radio_available(', 1)[1].split('void RFLinkUniversal::record_tx_command(', 1)[0]
for expected in ('channel_count_', 'all_checked', 'any_unavailable', 'radio.checked', 'radio.available',
                 'radio.rx_ready', 'radio.tx_ready', 'led_.radio_missing(all_checked && any_unavailable);'):
    assert expected in method, f'Radio fault integration missing: {expected}'

for trigger in ('on_begin', 'on_progress', 'on_end', 'on_error'):
    assert trigger in callbacks
    body = callbacks[trigger][0]['lambda']
    assert 'id(rflink_status_rgb).loop();' in body, f'{trigger} queues but never flushes LED'
    assert 'write_state(&id(' not in body, f'{trigger} has incompatible pointer-to-pointer LED call'
    assert body.find('call.perform()') < body.rfind('id(rflink_status_rgb).loop();')

header = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include "status_led.h"
using namespace esphome::rflink_universal;
static uint32_t test_now=6000;
uint32_t millis() { return test_now; }
class FakeLightState;
struct FakeOutput {
  int writes{0};
  float r{0},g{0},b{0},brightness{0};
  bool on{false};
  void write_state(FakeLightState *state);
};
struct FakeLightState {
  FakeOutput output;
  float r{0},g{0},b{0},brightness{0};
  bool on{false};
  struct Call {
    FakeLightState *state;
    bool on;
    float r,g,b,brightness;
    void set_transition_length(int) {}
    void set_rgb(float rr,float gg,float bb) {r=rr;g=gg;b=bb;}
    void set_brightness(float v) {brightness=v;}
    void perform() {state->on=on;state->r=r;state->g=g;state->b=b;state->brightness=brightness;}
  };
  Call turn_on() {return {this,true,r,g,b,brightness};}
  Call turn_off(){return {this,false,r,g,b,brightness};}
  FakeOutput *get_output(){return &output;}
  void loop() { output.write_state(this); } // Simplified LightState::loop(): commit queued color.
};
void FakeOutput::write_state(FakeLightState *s) {
  ++writes; on=s->on;r=s->r;g=s->g;b=s->b;brightness=s->brightness;
}
struct FakeCore {
  StatusLed led;
  StatusLed &status_led() {return led;}
  void led_ota(bool active) {led.ota(active);}
} rflink_core;
FakeLightState rflink_status_rgb;
#define id(name) (name)
'''
program = [header]
for name in ('on_begin', 'on_progress', 'on_end', 'on_error'):
    program.append(f'void {name}() {{\n{callbacks[name][0]["lambda"]}\n}}\n')
program.append(r'''
int main() {
  using Mode=StatusLed::Mode;
  rflink_core.led.wifi(true);
  rflink_core.led.radio_missing(true);
  rflink_core.led.settings(100,false);
  assert(rflink_core.led.mode(millis())==Mode::RADIO_MISSING);
  assert(rflink_core.led.sample(millis()).red==1.0f);
  assert(rflink_core.led.sample(millis()).green>0.0f);
  assert(rflink_core.led.sample(millis()).blue==0.0f);
  // Simulate OTA blocking ordinary LightState::loop(): never call it here.
  on_begin();
  assert(rflink_core.led.mode(millis())==Mode::OTA);
  assert(rflink_status_rgb.output.writes==1);
  assert(rflink_status_rgb.output.on);
  assert(rflink_status_rgb.output.r==1.0f && rflink_status_rgb.output.g==1.0f && rflink_status_rgb.output.b==1.0f);
  test_now += 150;
  on_progress();
  assert(rflink_status_rgb.output.writes==2);
  assert(rflink_status_rgb.output.r==1.0f && rflink_status_rgb.output.g==1.0f && rflink_status_rgb.output.b==1.0f);
  on_end();
  assert(rflink_core.led.mode(millis())==Mode::RADIO_MISSING);
  assert(rflink_status_rgb.output.writes==3);
  assert(rflink_status_rgb.output.b==0.0f);
  on_begin(); on_error();
  assert(rflink_status_rgb.output.writes==5);
  assert(rflink_status_rgb.output.r==1.0f && rflink_status_rgb.output.g==0.0f && rflink_status_rgb.output.b==0.0f);
  std::cout << "OTA YAML callbacks physically flush fake WS2812; missing radio amber; error red: PASS\\n";
}
''')
with TemporaryDirectory() as work:
    src = Path(work) / 'test.cpp'
    exe = Path(work) / 'test'
    src.write_text('\n'.join(program))
    cmd = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'rflink_universal/components/rflink_universal'), str(src), '-o', str(exe)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
print('3-radio availability / OTA LightState::loop() callback C++ compile: PASS (host stub only; ESPHome firmware compile not run)')
