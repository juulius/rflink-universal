#pragma once
// Decoder capabilities: generated from enabled schema-2 manifests at ESPHome build.
// Host tests use a checked-in snapshot validated against all source manifests.
#if defined(__has_include)
#  if __has_include("esphome/core/defines.h")
#    include "esphome/core/defines.h"
#  endif
#endif
#if defined(RFLINK_DECODER_EXPECTED_COUNT) && RFLINK_DECODER_EXPECTED_COUNT > 0
#  include "../../../rflink_decoders/capability_catalog_generated.h"
#else
#  include "capability_catalog_generated.h"
#endif
