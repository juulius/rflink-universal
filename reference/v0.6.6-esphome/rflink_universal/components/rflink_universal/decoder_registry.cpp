#include "decoder_registry.h"

// IMPORTANT: ESPHome generates defines.h with RFLINK_DECODER_EXPECTED_COUNT.
// When enabled packs exist the generated anchors file is REQUIRED. The old
// optional __has_include silently returned an empty registry on real devices.
#if defined(__has_include)
#  if __has_include("esphome/core/defines.h")
#    include "esphome/core/defines.h"
#  endif
#endif
#if defined(RFLINK_DECODER_EXPECTED_COUNT) && RFLINK_DECODER_EXPECTED_COUNT > 0
#  include "../../../rflink_decoders/decoder_anchors_generated.h"
#  define RFLINK_HAS_GENERATED_DECODER_ANCHORS 1
#endif
#include <algorithm>

namespace esphome {
namespace rflink_universal {

std::vector<RFDecoder *> &decoder_registry() {
  static std::vector<RFDecoder *> registry;
  return registry;
}

void ensure_decoder_packs_linked() {
#ifdef RFLINK_HAS_GENERATED_DECODER_ANCHORS
  rflink_generated_force_link_decoders();
#endif
}

void register_decoder_once(RFDecoder *decoder) {
  if (decoder == nullptr) return;
  auto &registry = decoder_registry();
  if (std::find(registry.begin(), registry.end(), decoder) == registry.end()) registry.push_back(decoder);
}

DecoderRegistrar::DecoderRegistrar(RFDecoder *decoder) { register_decoder_once(decoder); }

}  // namespace rflink_universal
}  // namespace esphome
