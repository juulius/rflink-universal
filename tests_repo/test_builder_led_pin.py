"""Builder status LED GPIO profile integration and legacy migration regression."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
from local_profile import LocalProfile


class BuilderLedPinTests(unittest.TestCase):
    def test_default_pin_in_global_and_new_device_builder_views(self):
        app=(ROOT/"rflink_builder/static/app.js").read_text()
        wizard=(ROOT/"rflink_builder/static/wizard.js").read_text()
        self.assertIn("['led.gpio','Status LED",app)
        self.assertIn("if(a==='led')",app)
        self.assertIn("board.led||={gpio:48}",app)
        self.assertIn("wizardBoard.led||={gpio:48}",wizard)
        self.assertIn("pinInput('GPIO data pin',wizardBoard.led.gpio",wizard)
        self.assertIn("v=>{wizardBoard.led.gpio=v;}",wizard)

    def test_legacy_saved_profile_normalizes_without_writing(self):
        with tempfile.TemporaryDirectory() as folder:
            defaults=LocalProfile(folder,ROOT/"firmware")
            board=copy.deepcopy(defaults.defaults())
            self.assertEqual(board["led"],{"gpio":48})
            old=copy.deepcopy(board);old.pop("led")
            defaults.file.parent.mkdir(parents=True,exist_ok=True)
            defaults.file.write_text(json.dumps(old),encoding="utf-8")
            current=defaults.current()
            self.assertEqual(current["led"],{"gpio":48})
            self.assertEqual(json.loads(defaults.file.read_text()),old)
            self.assertEqual(defaults._validate(current),defaults._validate(old))

    def test_saving_47_changes_hash_and_rejects_collisions(self):
        with tempfile.TemporaryDirectory() as folder:
            cfg=LocalProfile(folder,ROOT/"firmware")
            board=cfg.defaults()
            old_hash=cfg._validate(board)
            board["led"]["gpio"]=47
            saved=cfg.save(board)
            self.assertNotEqual(saved["hash"],old_hash)
            self.assertEqual(cfg.current()["led"]["gpio"],47)
            for pin in [12,11,13,4,5,6,9,8,14,15,16,17,20,0,45,46]:
                changed=copy.deepcopy(board)
                changed["led"]["gpio"]=pin
                with self.subTest(pin=pin),self.assertRaisesRegex(ValueError,"GPIO|conflicts"):
                    cfg.save(changed)
            self.assertEqual(cfg.current()["led"]["gpio"],47)

    def test_older_firmware_pin_override_cannot_silently_succeed(self):
        server=(ROOT/"rflink_builder/server.py").read_text()
        self.assertIn("cannot change LED GPIO",server)
        self.assertIn("'kStatusLedGpio' not in generator",server)
        self.assertIn("gpio':48",server)

if __name__=="__main__":
    unittest.main()
