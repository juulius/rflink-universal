"""Real attachment name through Ingress + bounded transfer progress and timed UI dismissal."""
from pathlib import Path
import ast
import re
import threading
import time
import unittest
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[1]
def read(name):
    return (ROOT/name).read_text(encoding="utf-8")

class IngressCsvFilenameProgress(unittest.TestCase):
    def test_progress_state_token_security_and_lifecycle(self):
        tree=ast.parse(read("rflink_builder/server.py"))
        names={"_csv_progress_key", "_csv_progress_begin", "_csv_progress_step",
               "_csv_progress_snapshot"}
        nodes=[node for node in tree.body
               if isinstance(node, ast.FunctionDef) and node.name in names]
        self.assertEqual(len(nodes),len(names))
        code=compile(ast.Module(body=nodes,type_ignores=[]), "<progress_helpers>", "exec")
        ns={"re":re,"threading":threading,"time":time,
            "urlparse":urlparse,"parse_qs":parse_qs,
            "_CSV_PROGRESS":{},"_CSV_PROGRESS_LOCK":threading.Lock()}
        exec(code,ns)
        rf="rf-68ee8f479058"
        token="0123456789abcdef01234567"
        csv=f"/rfweb/{rf}/api/logs.csv?export_id={token}"
        status=f"/rfweb/{rf}/api/logs/export-status?export_id={token}"
        key=ns["_csv_progress_key"](csv,"api/logs.csv")
        self.assertEqual(key,(rf,token))
        self.assertEqual(key,ns["_csv_progress_key"](status,"api/logs/export-status"))
        self.assertIsNone(ns["_csv_progress_key"](csv+"&other=1","api/logs.csv"))
        self.assertIsNone(ns["_csv_progress_key"](csv.replace(token,"../../etc"),"api/logs.csv"))
        self.assertIsNone(ns["_csv_progress_key"](csv.replace(rf,"rf-000"),"api/logs.csv"))
        self.assertEqual(ns["_csv_progress_snapshot"](key)["state"],"pending")
        ns["_csv_progress_begin"](key)
        self.assertEqual(ns["_csv_progress_snapshot"](key)["state"],"starting")
        ns["_csv_progress_step"](key,count=32768,events=50,estimate=100,state="streaming")
        self.assertEqual(ns["_csv_progress_snapshot"](key),
            {"state":"streaming","bytes":32768,"events":50,"estimate":100})
        ns["_csv_progress_step"](key,count=7,events=1,state="done")
        self.assertEqual(ns["_csv_progress_snapshot"](key)["state"],"done")
        self.assertEqual(ns["_csv_progress_snapshot"](key)["bytes"],32775)

    def test_original_file_direct_download_is_not_blob_url(self):
        server=read("rflink_builder/server.py")
        app=read("rflink_builder/static/rflink_web/app.js")
        native=read("firmware/main/webui/app.js")
        self.assertIn('attachment; filename="RFLink_Diagnostics_Full.csv"',server)
        self.assertIn("source.read(32 * 1024)",server)
        self.assertIn("self.wfile.write(b'0\\r\\n\\r\\n')",server)
        self.assertIn("_csv_progress_step(progress_key, state='done')",server)
        self.assertIn("_csv_progress_step(progress_key, state='failed')",server)
        self.assertIn("_csv_progress_snapshot(key)",server)
        self.assertIn("statusUrl.searchParams.set('export_id',token)",app)
        self.assertIn("downloadUrl.searchParams.set('export_id',token)",app)
        self.assertIn("setTimeout(poll,250)",app)
        self.assertIn("This click intentionally leaves the native anchor default action intact",app)
        csv_block=app.split("function exportFullCsv(event)",1)[1].split("$('log-export').addEventListener",1)[0]
        self.assertNotIn("new Blob(chunks",csv_block)
        self.assertNotIn("URL.createObjectURL(blob)",csv_block)
        # Per-capture RF JSON remains an independent, working Blob export.
        self.assertIn("csvExportHideTimer=setTimeout(",app)
        self.assertIn("},5000)",app)
        self.assertIn("csvExportHideTimer=setTimeout(",native)
        self.assertIn("},5000)",native)

    def test_builder_release_number_and_prior_versions_retained(self):
        b=ROOT/"rflink_builder"
        self.assertIn("version: 0.6.36",(b/"config.yaml").read_text())
        self.assertIn("ARG BUILD_VERSION=0.6.36",(b/"Dockerfile").read_text())
        self.assertIn('v0.6.36</span>',(b/"static/index.html").read_text())
        notes=(b/"CHANGELOG.md").read_text()
        self.assertIn("## v0.6.36 ",notes)
        self.assertIn("## v0.6.35 ",notes)


if __name__=="__main__":
    unittest.main()
