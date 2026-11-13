"""The trusted Ingress mirror must be published as a new Builder image."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
BUILDER=ROOT/"rflink_builder"


class Builder635MirrorRelease(unittest.TestCase):
    def test_home_assistant_sees_new_builder_image_version(self):
        config=(BUILDER/"config.yaml").read_text()
        docker=(BUILDER/"Dockerfile").read_text()
        index=(BUILDER/"static/index.html").read_text()
        notes=(BUILDER/"CHANGELOG.md").read_text()
        self.assertIn("version: 0.6.35",config)
        self.assertIn("ARG BUILD_VERSION=0.6.35",docker)
        self.assertIn('<span class="version">v0.6.35</span>',index)
        self.assertIn("## v0.6.35 ",notes)

    def test_current_native_features_exist_in_trusted_ingress_assets(self):
        html=(BUILDER/"static/rflink_web/index.html").read_text()
        script=(BUILDER/"static/rflink_web/app.js").read_text()
        style=(BUILDER/"static/rflink_web/app.css").read_text()
        self.assertIn('id="log-export-progress"',html)
        self.assertIn('id="log-export-meter"',html)
        self.assertIn('id="info-clock"',html)
        self.assertIn("function exportFullCsv(event)",script)
        self.assertIn("function renderUptimeWithBoot(status)",script)
        self.assertIn("X-RFLink-Export-Estimated-Events",script)
        self.assertIn(".log-export-progress[hidden]",style)

    def test_ingress_never_runs_untrusted_device_web_assets(self):
        proxy=(BUILDER/"device_web_proxy.py").read_text()
        server=(BUILDER/"server.py").read_text()
        self.assertIn("static/rflink_web",proxy)
        self.assertIn("content = (root / name).read_bytes()",proxy)
        self.assertIn("resource in _STATIC",proxy)
        self.assertIn("Cache-Control', 'no-store'",server)
        self.assertIn("relay_rfweb(device_book, self.path, 'GET', CSRF)",server)


if __name__ == "__main__":
    unittest.main()
