# Shared 868.30 MHz reception for Itho RFT and RAMSES-II (native v0.7)

## What the Builder and RFLink do

On the 868.30 MHz CC1101 radio, RFLink runs **one receive configuration**. A received 63-byte packet goes to the protocol decoder, which attempts both RAMSES-II and Itho RFT recognition. There is no need to switch the receiver's CC1101 register profile just because a RAMSES-II frame arrives. This is the intended Stage 22 contract; it is host-tested but still needs RF hardware validation.

1. Start native RFLink on the dedicated test board, without overwriting a working v0.6.6 installation.
2. Inspect the 868.30 MHz receiver on the native status page. A listening receiver can accept candidate packets for both protocols.
3. In a future authorized TX workflow, the manager pauses this shared RX, temporarily programs the *transmit* settings supplied by the RAMSES-II encoder, and restores the **same previous RX** configuration afterward, on both successful transmissions and recoverable TX failures.

## Advanced: physical RX versus protocol decoding versus TX

- `RfProfileId::itho_868_packet` is the **legacy internal identifier** for the shared 868.30 MHz packet receiver; its descriptive name does not mean it only accepts Itho. The profile sets 868.2999 MHz, 63-byte fixed packet reception, and the CC1101 modem registers.
- `NativeDecoder::on_packet` checks the candidate 63-byte frame with `rflink_ramses_ii::decodePacked` and `itho::decode_packet`. These are decoders, not commands to change the CC1101 receive state.
- `RfProfileId::ramses_868_packet` currently provides a UART TX preset. It has **no modem register table for RX** and cannot be started as an alternate receive profile.
- `RadioManager::transmit_encoded` snapshots the prior RX profile, stops it, sends using the encoder-selected TX transport, and restarts the **original** RX profile. This remains mandatory while the 868 MHz radio uses half-duplex CC1101 hardware: *no protocol-specific RX switch*, but a deliberate RX–TX–RX handover to transmit.
- An independent host regression test in `tests_repo/test_shared_868_rx_contract.py` compiles the actual decoder and radio manager and checks RAMSES decoding from the shared RX path and restoration after successful and failed UART TX. It does not send an RF signal.

## Open decoder issue

Some valid Itho test frames are **also accepted by the present RAMSES-II decoder**. The current native decoder gives RAMSES-II priority when both return valid; this can misclassify Itho messages. We should improve classification using validated protocol-specific integrity and framing checks before calling the full RX/TX device pairing and control features production ready. Changing the CC1101 RX register profile is **not** an appropriate fix for this software classification ambiguity.

## Validation still required

Confirm on the actual three-radio test PCB that Itho and RAMSES-II frames can both arrive through the same 868.30 MHz receive configuration, that both are classified correctly, and that the original receiver resumes after RAMSES UART transmission. No on-air TX success or production readiness is implied by the C++ host tests or by a successful ESP-IDF build.
