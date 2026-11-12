"""UTC + uptime diagnostics must be explicit, bounded and backward-compatible."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text("utf-8")


class UtcTimeDiagnostics(unittest.TestCase):
    def test_time_service_has_nonblocking_sntp_and_bounded_nvs(self):
        source = read("firmware/main/rflink_time.cpp")
        self.assertIn('ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org")', source)
        self.assertIn("config.sync_cb=&on_sync", source)
        self.assertIn("constexpr size_t kEntries=12", source)
        self.assertIn('nvs_set_blob(handle,"time_history_v1"', source)
        self.assertIn("if(!pending_save.exchange(false", source)
        self.assertIn("if(epoch<kMinEpochMs || epoch>=kMaxEpochMs", source)
        self.assertIn("std::lock_guard<std::mutex> guard(history_mutex)", source)
        self.assertNotIn("nvs_set_blob", source.split("void on_sync(", 1)[1].split("} // namespace", 1)[0])
        main = read("firmware/main/main.cpp")
        self.assertIn("rflink::time_service_poll()", main)
        self.assertIn("time_service_set_boot(snapshot.boot_sequence)", read("firmware/main/rflink_crash_report.cpp"))
        self.assertIn("rflink_time.cpp", read("firmware/main/CMakeLists.txt"))

    def test_legacy_uptime_and_csv_columns_remain(self):
        backend = read("firmware/main/rflink_web_ota.cpp")
        self.assertIn('rflink::time_service_start()', backend)
        self.assertIn('"timestamp_utc_ms"', backend.replace("\\\"", '"'))
        self.assertIn('"clock_synced"', backend.replace("\\\"", '"'))
        self.assertIn("record_type,sequence,uptime_ms,stream,radio", backend)
        self.assertIn("snapshot_json,event_utc,boot_id", backend)
        self.assertIn("row[24]=iso_utc_ms(", backend)
        self.assertIn("row[25]=std::to_string(rflink::time_boot_id())", backend)
        self.assertIn('"time_anchor"', backend)
        self.assertIn("time_event_epoch_ms(e.uptime_ms)", backend)
        self.assertIn("uint32_t uptime_ms=0", read("firmware/main/rflink_log_store.hpp"))

    def test_unknown_time_not_guessed_and_ui_shows_both_clocks(self):
        t = read("firmware/main/rflink_time.cpp")
        self.assertIn("if(anchor<=0)return 0", t)
        self.assertIn("if(elapsed>now)return 0", t)
        crash = read("firmware/main/rflink_crash_report.cpp")
        self.assertIn("boot_started_utc_ms", crash)
        self.assertIn('started?std::to_string(started):"null"', crash)
        for prefix in ("firmware/main/webui", "rflink_builder/static/rflink_web"):
            js = read(prefix + "/app.js")
            html = read(prefix + "/index.html")
            self.assertIn("localTimestamp(e.timestamp_utc_ms)", js)
            self.assertIn("'Uptime '+uptimeMs(e.uptime_ms)", js)
            self.assertIn("localTimestamp(item.boot_started_utc_ms)", js)
            self.assertIn('id="info-clock"', html)
            self.assertIn("Date/time + uptime", html)
        proxy = read("rflink_builder/diagnostic_proxy.py")
        self.assertIn('"boot_started_utc_ms"', proxy)
        self.assertIn('"clock_synced"', proxy)


if __name__ == "__main__":
    unittest.main()
