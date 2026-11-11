"""Source-level guardrails for continuous, isolated 433 MHz RMT RX.

A real ESP32-S3 bench test is still necessary to validate callback rearming,
timing, RF reception quality, watchdog recovery, TX->RX and OTA.
"""
from pathlib import Path
import unittest

BASE=Path(__file__).resolve().parents[1]/"firmware/components/rflink_radio_hal"
SRC=(BASE/"rflink_rmt_rx.cpp").read_text()
HDR=(BASE/"include/rflink_rmt_rx.hpp").read_text()

class RmtIsrSlotOwnership(unittest.TestCase):
    def test_rearm_in_isr_not_worker(self):
        isr=SRC[SRC.index("bool IRAM_ATTR RmtPulseRx::on_rx_done("):SRC.index("void RmtPulseRx::release_queues()")]
        worker=SRC[SRC.index("void RmtPulseRx::poll_once()"):SRC.index("PulseStats RmtPulseRx::stats()")]
        self.assertIn("xQueueReceiveFromISR(self->free_slots_",isr)
        self.assertIn("xQueueSendFromISR(self->done_",isr)
        self.assertIn("self->active_slot_isr_=next_slot",isr)
        self.assertIn("const esp_err_t armed=self->arm()",isr)
        self.assertNotIn("sink_->",isr)
        self.assertNotIn("ESP_LOG",isr)
        self.assertNotIn("self->arm()",worker)
        self.assertLess(worker.index("xQueueSend(free_slots_"),worker.index("filter_short_pulses_inplace("))

    def test_no_overwrite_or_false_completion(self):
        self.assertIn("static constexpr size_t kMaxSymbols=512",HDR)
        self.assertIn("static constexpr size_t kCaptureSlots=3",HDR)
        self.assertIn("symbols_[kCaptureSlots][kMaxSymbols]",HDR)
        self.assertIn("if(!queued)",SRC)
        self.assertIn("++self->queue_drops_isr_",SRC)
        self.assertIn("symbols_[event.slot][i]",SRC)
        self.assertIn("if(!w.duration0)break",SRC)
        self.assertIn("if(!w.duration1)break",SRC)
        self.assertIn("const bool complete=event.symbols<kMaxSymbols",SRC)
        self.assertIn("if(complete && n>=2 && sink_)",SRC)
        self.assertIn("rearm_error_isr_",SRC)
        self.assertIn("stats_.last_driver_stage=RmtRxFailureStage::rearm",SRC)

if __name__=="__main__":
    unittest.main()
