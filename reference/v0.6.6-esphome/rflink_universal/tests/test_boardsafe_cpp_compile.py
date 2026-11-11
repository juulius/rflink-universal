"""Syntax-check RadioBootGuard with mock ESPHome/ESP-IDF APIs; not a firmware build."""
from pathlib import Path
import subprocess
import tempfile
import shutil

root = Path(__file__).resolve().parents[2]
source = root/'rflink_universal/components/rflink_universal/radio_boot_guard.cpp'
stubs = root/'rflink_universal/tests/stubs'
with tempfile.TemporaryDirectory() as tmp:
    inc = Path(tmp)
    for part in ('esphome/core/component.h','esphome/core/log.h','esphome/components/cc1101/cc1101.h'):
        dst = inc/part
        dst.parent.mkdir(parents=True,exist_ok=True)
        text = (stubs/part).read_text()
        if part.endswith('component.h'):
            text += '\nnamespace esphome { namespace setup_priority {inline constexpr float POWER = 800.0f;} inline void delay(unsigned){} inline void delayMicroseconds(unsigned){} inline unsigned micros(){ return 0; } }\n'
        elif part.endswith('cc1101.h'):
            text = text.replace('class CC1101Component {\n public:', 'class CC1101Component {\n public:\n  bool is_failed() const { return false; }\n  bool spi_is_ready() const { return true; }')
        dst.write_text(text)
    gpio = inc/'driver/gpio.h'
    gpio.parent.mkdir(parents=True,exist_ok=True)
    gpio.write_text('''#pragma once
using gpio_num_t = int;
constexpr int GPIO_MODE_INPUT=0, GPIO_MODE_OUTPUT=1, GPIO_FLOATING=0, GPIO_PULLUP_ONLY=1, GPIO_PULLDOWN_ONLY=2;
inline int gpio_set_level(int,int) { return 0; }
inline int gpio_set_direction(int,int) { return 0; }
inline int gpio_set_pull_mode(int,int) { return 0; }
inline int gpio_get_level(int) { return 1; }
using esphome::delayMicroseconds;
using esphome::delay;
using esphome::micros;
''')
    # Source refers to delay/micros from namespace esphome, so the Component stub suffices.
    cmd = ['g++','-std=c++17','-Wall','-Wextra','-Werror','-DUSE_ESP32','-fsyntax-only','-I',str(inc),str(source)]
    subprocess.run(cmd,check=True)
    # ESPHome `id(cc1101_...)` is CC1101Component*, not a reference. The
    # original BOARDSAFE1 compiled its component but failed main.cpp at callsites.
    contract = inc / 'pointer_contract.cpp'
    contract.write_text(r"""#include <type_traits>
#include "radio_boot_guard.h"
using esphome::rflink_universal::RadioBootGuard;
using esphome::cc1101::CC1101Component;
static_assert(std::is_invocable_v<decltype(&RadioBootGuard::set_async_gdo0_rx_safe), RadioBootGuard&, CC1101Component*>);
static_assert(std::is_invocable_v<decltype(&RadioBootGuard::set_async_gdo0_tx_data), RadioBootGuard&, CC1101Component*>);
static_assert(!std::is_invocable_v<decltype(&RadioBootGuard::set_async_gdo0_rx_safe), RadioBootGuard&, CC1101Component&>);
""")
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-I',str(inc), '-I',str(source.parent),str(contract)],check=True)
    print('COMPILEFIX1: RadioBootGuard C++ and ESPHome pointer call contract: PASS (host stubs, not ESPHome/ESP-IDF)')
