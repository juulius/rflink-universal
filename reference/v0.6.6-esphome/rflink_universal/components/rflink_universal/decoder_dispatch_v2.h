#pragma once
// Stateless, allocation-light multi-candidate decoder dispatch.
// Every eligible decoder gets its chance at full validation. A high scoring
// false positive cannot hide a lower scoring valid protocol.
#include "decoder_registry.h"
#include "radio_contract_v2.h"
#include <cstdint>
#include <utility>

namespace esphome { namespace rflink_universal {
struct DispatchDiagnostics {
  uint16_t skipped_by_route{0};
  uint16_t matched{0};
  uint16_t decoded{0};
  uint16_t rejected{0};
};
struct DispatchResult {
  bool recognized{false};
  bool ambiguous{false};
  RFDecoder *winner{nullptr};
  RFDecoder *top_candidate{nullptr};
  uint8_t confidence{0};
  uint8_t top_score{0};
  DecodeResult result;
  DispatchDiagnostics diagnostics;
};

class DecoderDispatchV2 {
 public:
  static DispatchResult run(const RFFrame &frame, const std::vector<RFDecoder *> &decoders,
                            const RadioInputIndex &input_index, uint8_t threshold) {
    DispatchResult out;
    for(auto *decoder : decoders) {
      if(!decoder) continue;
      if(!input_index.allows(decoder->name(), frame)) {++out.diagnostics.skipped_by_route; continue;}
      const uint8_t score=decoder->match(frame);
      if(score>out.top_score) {out.top_score=score;out.top_candidate=decoder;}
      if(score<threshold) continue;
      ++out.diagnostics.matched;
      DecodeResult candidate{};
      if(!decoder->decode(frame,candidate) || !candidate.valid) {++out.diagnostics.rejected;continue;}
      ++out.diagnostics.decoded;
      if(!out.recognized || score>out.confidence) {
        out.recognized=true;
        out.ambiguous=false;
        out.confidence=score;
        out.winner=decoder;
        out.result=std::move(candidate);
      } else if (score==out.confidence &&
                 (candidate.protocol!=out.result.protocol || candidate.identity_key!=out.result.identity_key ||
                  candidate.action!=out.result.action)) {
        out.ambiguous=true;
      }
    }
    if(out.ambiguous) {out.recognized=false;out.result=DecodeResult{};}
    return out;
  }
};
}} // namespace
