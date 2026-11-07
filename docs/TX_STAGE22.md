# Stage 22: universal TX readiness and RX restoration (native development)

## What changes for the user?

RFLink's radio manager is preparing for future transmit and pairing actions without sacrificing continuous reception. **No transmit button or pairing API is activated by this stage.** On a dedicated test board, the firmware may be built and its diagnostic behavior checked; do not replace a working v0.6.6 installation.

A listening receiver is now considered **eligible** when a decoder requests a compatible transmit profile. The readiness check does not touch the hardware, pause reception or send data. When an encoded frame is actually sent by a future authorized caller, the manager performs a controlled receive-to-transmit-to-receive sequence.

## Advanced: driver contract

1. `request_transmit(radio, profile)` validates hardware presence, mode, frequency and the decoder-selected transmit transport. A radio that is **receiving** passes this preflight; a switching, transmitting, faulted or unavailable radio does not.
2. `transmit_encoded` validates the full encoded frame **before** stopping the original receive session. On invalid payload, the previous physical TX failure stage is cleared so diagnostics cannot mistakenly attribute the new refusal to an older operation. No RF is emitted.
3. After validation, the manager exclusively pauses the original receive transport. If this fails, no transmit driver is called and the radio enters `fault` with `ManagerError::restore_failure` and the new `TxFailureStage::rx_pause`. The latter was appended after all existing stage enum values for numeric compatibility.
4. When the pause succeeds, the manager invokes **only** the transport selected by the RF profile: GDO/UART for RAMSES-II, GDO/RMT for the 433 MHz OOK profiles, or packet FIFO for supported 868 MHz packet profiles. It must not guess another transport.
5. If the physical transport reports that the CC1101 settings were restored, the manager restarts the exact prior receive profile, including the **shared 868.30 MHz packet receiver for both Itho and RAMSES-II**. RX is restarted after a recoverable failed transmit too. If hardware restoration fails, the manager marks the radio faulty rather than claiming it is receiving.

These tests prove only the native state machine and error handling. They do not verify waveform correctness, PA output power, over-the-air delivery or pairing.

## Validation

The host regression `tests_repo/test_tx_readiness_stage22.py` compiles the actual `RadioManager` and RF profiles, then exercises TX preflight while RX is active, no hardware activity on preflight, successful TX, UART failure, refused RX pause, stale-error reset, and restoration on all three radios across UART/RMT/FIFO transports. GitHub CI also compiles the full ESP-IDF native firmware.

Before enabling live TX controls, use a separate ESP32-S3 + three CC1101 test board to measure output, verify RX restoration and confirm that the Stage 21 RX watchdog does not interfere with TX. The RAMSES/Itho decoder overlap remains a separate unresolved classification bug; TX preflight changes do not address it.
