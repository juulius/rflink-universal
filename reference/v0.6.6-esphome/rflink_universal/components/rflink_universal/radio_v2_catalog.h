#pragma once
// ESPHome generates this file from the enabled decoder packs in each build.
#if defined(__has_include)
# if __has_include("esphome/core/defines.h")
#  include "esphome/core/defines.h"
# endif
#endif
#if defined(RFLINK_DECODER_EXPECTED_COUNT) && RFLINK_DECODER_EXPECTED_COUNT > 0
# include "../../../rflink_decoders/radio_v2_catalog_generated.h"
#else
# include "radio_v2_catalog_generated.h"
#endif
