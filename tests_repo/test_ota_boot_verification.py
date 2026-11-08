"""Builder v0.6.20 — an OTA HTTP success must not mean boot success.

Offline simulated responses include the ESP32's PENDING_VERIFY → VALID
transition and rollback to the previously active firmware. No RF hardware,
network access or real user credentials are needed.
"""
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from ota_verification import confirm_ota_restart, OTAUnverified, OTARollback

TARGET="0.7.0-exp.2-n16r8-b5e871b06"
OLD="0.7.0-exp.1-n16r8-b5e871b06"
IDENTITY="rf-001122334455"
BOARD={"board_id":"rflink-s3-n16r8-reva","board_hash":"5e871b06",
       "device_id":IDENTITY}
BASE={**BOARD,"firmware":OLD,"ota_slot":16,"ota_state":2,"uptime_s":60}
NEW_PENDING={**BOARD,"firmware":TARGET,"ota_slot":17,"ota_state":1,"uptime_s":3}
NEW_VALID={**BOARD,"firmware":TARGET,"ota_slot":17,"ota_state":2,"uptime_s":14}

class Clock:
    def __init__(self,responses):
        self.now=0;self.responses=list(responses);self.last=self.responses[-1]
    def tick(self,n):
        self.now+=n
    def read(self):
        if self.responses:
            self.last=self.responses.pop(0)
        if isinstance(self.last,BaseException):
            raise self.last
        return dict(self.last)

class BootVerificationTests(unittest.TestCase):
    def run_check(self, responses, before=BASE, timeout=24):
        clock=Clock(responses)
        events=[]
        def report(phase,message,observation):
            events.append((phase,message,observation))
        def action():
            return confirm_ota_restart(clock.read,before,TARGET,report,
                                       clock=lambda:clock.now,sleep=clock.tick,
                                       poll_every=2,timeout=timeout)
        return action,events,clock

    def test_same_version_and_valid_health_in_new_slot_is_confirmed(self):
        # The board can be reflashed with the same version; slot + health
        # prove its new boot, not just a coincidentally matching version string.
        previous={**BASE,"firmware":TARGET}
        action,events,_=self.run_check([NEW_PENDING,NEW_VALID],before=previous)
        observed=action()
        self.assertEqual(observed["slot"],17)
        self.assertEqual(observed["ota_state"],2)
        self.assertEqual([x[0] for x in events],["validating","verified"])

    def test_http_success_but_no_reboot_cannot_be_called_success(self):
        action,events,_=self.run_check([BASE],timeout=12)
        with self.assertRaisesRegex(OTAUnverified,"not all be confirmed"):
            action()
        self.assertFalse(any(x[0]=="verified" for x in events))

    def test_http_lost_and_wifi_reconnect_can_still_verify(self):
        action,events,_=self.run_check([
            OSError("connection refused"),OSError("timed out"),
            NEW_PENDING,NEW_VALID
        ])
        self.assertEqual(action()["ota_state"],2)
        self.assertIn("reconnecting",[x[0] for x in events])

    def test_pending_health_does_not_prematurely_mark_success(self):
        action,events,_=self.run_check([NEW_PENDING],timeout=10)
        with self.assertRaises(OTAUnverified):
            action()
        self.assertFalse(any(x[0]=="verified" for x in events))

    def test_roll_back_to_previous_slot_after_new_boot(self):
        action,events,_=self.run_check([NEW_PENDING,{**BASE,"uptime_s":5}])
        with self.assertRaisesRegex(OTARollback,"previous OTA slot"):
            action()
        self.assertFalse(any(x[0]=="verified" for x in events))

    def test_previous_slot_after_reboot_is_rollback_suspect(self):
        prior={**BASE,"uptime_s":150}
        old_restarted={**BASE,"uptime_s":4}
        action,_,_=self.run_check([old_restarted],before=prior,timeout=16)
        with self.assertRaisesRegex(OTARollback,"likely rolled back"):
            action()

    def test_wrong_device_at_same_ip_is_rejected(self):
        action,_,_=self.run_check([{**NEW_VALID,"device_id":"rf-bbbbbbbbbbbb"}])
        with self.assertRaisesRegex(OTAUnverified,"different RFLink"):
            action()

    def test_wrong_profile_at_same_ip_is_rejected(self):
        action,_,_=self.run_check([{**NEW_VALID,"board_hash":"deadbeef"}])
        with self.assertRaisesRegex(OTAUnverified,"different RFLink"):
            action()

    def test_wrong_firmware_in_new_slot_is_not_a_success(self):
        action,_,_=self.run_check([{**NEW_VALID,"firmware":OLD}])
        with self.assertRaisesRegex(OTAUnverified,"Different image"):
            action()

    def test_image_invalid_state_never_counts_as_success(self):
        for state in (3,4):
            with self.subTest(state=state):
                action,_,_=self.run_check([{**NEW_VALID,"ota_state":state}])
                with self.assertRaises(OTARollback):
                    action()

    def test_unknown_ota_slot_rejected(self):
        action,_,_=self.run_check([NEW_VALID],before={**BASE,"ota_slot":None})
        with self.assertRaisesRegex(OTAUnverified,"initial OTA slot"):
            action()

    def test_frontend_100_percent_only_when_ota_verified(self):
        js=(ROOT/'rflink_builder/static/rf_flash.js').read_text('utf-8')
        server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        self.assertIn("state==='success' && ota && deployment.verified===true",js)
        self.assertIn("state==='unverified'",js)
        self.assertIn("state==='rolled_back'",js)
        self.assertIn("'phase':self.phase,'verified':self.verified",server)
        self.assertIn("confirm_ota_restart(lambda:http_device_json(ip)",server)
        self.assertIn("[4/4] VERIFIED",server)
        self.assertNotIn("self.status='success';self.message='Installatieopdracht uitgevoerd",server)


if __name__=="__main__":
    unittest.main()
