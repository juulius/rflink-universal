"""Native v0.7 mobile logbook: one noise control, readable touch cards.

Source contracts complement JS syntax, full ESP-IDF compilation and a real
phone-size manual test. They do not claim a browser screenshot or hardware test.
"""
from html.parser import HTMLParser
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "firmware/main/webui"
PAGE = (WEB / "index.html").read_text(encoding="utf-8")
CSS = (WEB / "app.css").read_text(encoding="utf-8")
JS = (WEB / "app.js").read_text(encoding="utf-8")


class LogTabs(HTMLParser):
    def __init__(self):
        super().__init__()
        self.sources = []
        self.ids = set()

    def handle_starttag(self, tag, attrs):
        row = dict(attrs)
        if row.get("id"):
            self.ids.add(row["id"])
        if tag == "button" and row.get("data-log-source"):
            self.sources.append(row["data-log-source"])


class MobileLogUsability(unittest.TestCase):
    def test_noise_is_only_a_checkbox_not_a_fifth_source_tab(self):
        parsed = LogTabs()
        parsed.feed(PAGE)
        self.assertEqual(parsed.sources, ["all", "radio", "system", "web"])
        self.assertIn('id="log-noise" type="checkbox"', PAGE)
        self.assertIn('id="log-filter-controls"', PAGE)
        self.assertNotIn('data-log-source="noise"', PAGE)
        self.assertIn("if(noise&&['all','radio'].includes(logSource))p.set('noise','1')", JS)
        self.assertIn("else if(logSource==='radio')p.set('view','radio')", JS)
        self.assertIn("if(!$('log-noise').checked&&event.stream==='noise')return false", JS)
        self.assertIn("logSource==='system'||logSource==='web'", JS)

    def test_mobile_cards_reuse_log_records_and_expand_metadata(self):
        parsed = LogTabs()
        parsed.feed(PAGE)
        for ident in ["log-rows", "log-buffer-details", "log-buffer-summary",
                      "log-buffer-totals", "log-export", "log-older", "log-newer",
                      "log-latest", "log-radio", "log-noise"]:
            self.assertIn(ident, parsed.ids)
        self.assertIn('max-width:750px', CSS)
        self.assertIn('grid-template-areas:"event stream" "time radio" "details details"', CSS)
        self.assertIn('.log-expanded-row[hidden]{display:none}', CSS)
        self.assertIn("row.classList.toggle('is-expanded',nowOpen)", JS)
        self.assertIn("if(open)tr.classList.add('is-expanded')", JS)
        self.assertIn('log-metadata-grid', CSS)
        self.assertIn('log-expand-button', JS)
        self.assertIn('detailRow.hidden=!open', JS)
        self.assertIn('logEventTitle(e),logDetails(e)', JS)
        self.assertIn("window.matchMedia('(max-width:750px)').matches", JS)
        # Mobile styles are a presentation change, not a duplicate log feed.
        self.assertNotIn("log-mobile-rows", PAGE)

    def test_web_action_titles_show_what_actually_happened(self):
        # Real logs can contain many generic "Web user action" entries. The
        # browser should decode the fixed action ID in the existing record.
        self.assertIn("if(e.code===39 || e.code===42)return readableWebAction(e.count)", JS)
        self.assertIn("'API failed · '+readableWebEndpoint(e.count)", JS)
        self.assertIn("'Page opened · '+readableWebPage(e.profile)", JS)
        self.assertIn("readableWebActionValue(e)", JS)
        self.assertIn("' · noise shown'", JS)
        self.assertIn("' · noise hidden'", JS)

    def test_all_six_buffers_stay_independent_and_export_unchanged(self):
        log = (ROOT / "firmware/main/rflink_log_store.hpp").read_text()
        self.assertIn('kSignalRadios=3', log)
        self.assertIn('kLogRings=6', log)
        self.assertIn('noise=2000', log)
        self.assertIn('kSystemLogRetention=512', log)
        self.assertIn('kWebLogRetention=512', log)
        self.assertIn("selectedLogQuery(false)", JS)
        self.assertIn("refreshLogBufferSummary", JS)
        self.assertIn("fmt(used)+' / '+fmt(limits[i])", JS)
        self.assertIn("fmt(overwritten)+' overwritten'", JS)


if __name__ == "__main__":
    unittest.main()
