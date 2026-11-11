#pragma once

#include <vector>
#include "decoder.h"

namespace esphome {
namespace rflink_universal {

std::vector<RFDecoder *> &decoder_registry();
// Force decoder pack object files into ESP-IDF static-link builds, even when
// their only other references would be static constructors.
void ensure_decoder_packs_linked();
// Registration is idempotent whether called from a static constructor or an anchor.
void register_decoder_once(RFDecoder *decoder);

class DecoderRegistrar {
 public:
  explicit DecoderRegistrar(RFDecoder *decoder);
};

#define RFLINK_REGISTER_DECODER(CLASS_NAME) \
  static CLASS_NAME g_##CLASS_NAME##_instance; \
  static ::esphome::rflink_universal::DecoderRegistrar \
      g_##CLASS_NAME##_registrar(&g_##CLASS_NAME##_instance); \
  extern "C" ::esphome::rflink_universal::RFDecoder *rflink_anchor_##CLASS_NAME() { return &g_##CLASS_NAME##_instance; }

}  // namespace rflink_universal
}  // namespace esphome
