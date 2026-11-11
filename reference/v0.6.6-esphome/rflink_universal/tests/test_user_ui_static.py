"""Check the complete embedded web UI without relying on a radio or browser binary."""
import json
import subprocess
import tempfile
from html.parser import HTMLParser
from pathlib import Path

root = Path(__file__).resolve().parents[1]
html = (root / "ui/index.html").read_text(encoding="utf-8")
locales = {name: json.loads((root / f"ui/locales/{name}.json").read_text(encoding="utf-8"))
           for name in ("en", "nl", "fr", "de")}
assert all(set(catalog) == set(locales["en"]) for catalog in locales.values())


class Scan(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.translations = set()

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        if "id" in values:
            assert values["id"] not in self.ids, f"duplicate HTML ID: {values['id']}"
            self.ids.add(values["id"])
        if "data-i18n" in values:
            self.translations.add(values["data-i18n"])


scan = Scan()
scan.feed(html)
assert scan.translations <= set(locales["en"]), scan.translations - set(locales["en"])
for name in ("userCapture", "userClear", "userCreate", "userSaveAdvanced", "userProfileDecoder",
             "userSaveProfile", "userExport", "userImport", "userImportFile", "userRadioProfile",
             "userAnalysis", "userDecoderList", "userProfileList", "userResult"):
    assert name in scan.ids, name
script = html.split("<script>", 1)[1].split("</script>", 1)[0]
script = script.replace("/*__RFLINK_LOCALES__*/", json.dumps(locales, ensure_ascii=False))

assert 'radio_profile' in script

assert 'libOfficialProfiles' in scan.ids
assert "p.source==='user'" in script, 'built-in profiles must not receive user remove controls'
assert 'knownProfilesHtml' in script and "source==='builtin'" in script
assert 'restore_state' in script and 'import_decoder' in script
assert 'TX permission' in locales["en"]["user.import_help"] or 'transmit' in locales["en"]["user.import_help"].lower()
assert 'local_tx_enabled' not in script.split('async function restoreBackup',1)[1].split('function ',1)[0] or 'approve_user_tx' not in script.split('async function restoreBackup',1)[1].split('function ',1)[0]

with tempfile.TemporaryDirectory() as temp:
    file = Path(temp) / "ui.js"
    file.write_text(script, encoding="utf-8")
    checked = subprocess.run(["node", "--check", str(file)], capture_output=True, text=True)
    assert checked.returncode == 0, checked.stderr
print("User UI: JavaScript syntax, controls, unique IDs and four complete translations PASS")
