"""Exact 868 FIFO fault/recovery chronology without changing SPI/RF tuning."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class PacketFaultTimeline(unittest.TestCase):
    def test_transport_emits_only_when_failing_or_rearming(self):
        rx=read("firmware/components/rflink_radio_hal/rflink_packet_rx.cpp")
        hdr=read("firmware/components/rflink_radio_hal/include/rflink_packet_rx.hpp")
        main=read("firmware/main/main.cpp")
        self.assertIn("using FaultObserver=void(*)(void*,PacketRxFault",hdr)
        self.assertIn("void set_fault_observer(",hdr)
        self.assertIn("report_fault(PacketRxFault::status_spi_error,0xff,0xff,e)",rx)
        self.assertIn("report_fault(PacketRxFault::idle_rearmed,marc,rxbytes,recovery)",rx)
        self.assertIn("report_fault(PacketRxFault::fifo_overflow,marc,rxbytes,recovery)",rx)
        self.assertIn("report_fault(PacketRxFault::fifo_read_error,marc,rxbytes,recovery)",rx)
        self.assertIn("const esp_err_t recovery=recover_fifo();",rx)
        self.assertIn("packet_rx.set_fault_observer(&log_868_fifo_fault,&logs)",main)
        self.assertIn("static void log_868_fifo_fault(",main)
        self.assertIn("static_cast<uint8_t>(rflink::RadioId::rf86830)",main)
        self.assertIn('MARCSTATE=0x%02X, RXBYTES=0x%02X',main)
        self.assertIn('status_valid=%s',main)
        # 0xFF represents failed SPI status collection, not 127 FIFO bytes.
        self.assertIn('const bool status_valid=cause!=rflink::PacketRxFault::status_spi_error;',main)
        self.assertIn('?static_cast<uint16_t>(rxbytes&0x7f):0u;',main)
        self.assertIn('available_bytes,static_cast<int32_t>(outcome)',main)
        self.assertNotIn('static_cast<uint16_t>(rxbytes&0x7f),static_cast<int32_t>(outcome)',main)
        self.assertIn("FIFO CRC check is disabled",main)
        self.assertIn("detail_logs->save_text(4,seq,detail",main)
        self.assertNotIn("report_fault(PacketRxFault",rx.split("if(sink_)sink_->on_packet_with_metadata",1)[1])
        # Modem configuration and recovery primitives stay unchanged.
        self.assertIn("bus_.enter_rx_verified(kRadio)",rx)
        self.assertIn("bus_.read_rx_fifo(kRadio,packet,sizeof(packet))",rx)
        self.assertIn("rx_poll_delay_ticks(configTICK_RATE_HZ)",main)

    @unittest.skipUnless(shutil.which("g++"), "C++ host compiler required")
    def test_exact_faults_show_up_in_radio_and_system_but_not_noise(self):
        source=r"""
#include "rflink_log_store.hpp"
#include <cassert>
using namespace rflink;
int main(){
 LogStore store;
 LogLimits limits{};
 assert(store.configure(limits));
 const auto valid=store.push(LogStream::signal,LogCode::ramses,100,2);
 const auto overflow=store.push(LogStream::system,LogCode::rx868_fifo_overflow,
    101,2,LogSeverity::warning,31,0);
 const auto fail=store.push(LogStream::system,LogCode::rx868_spi_status_error,
    102,2,LogSeverity::error,0,258);
 const auto recovered=store.push(LogStream::system,LogCode::rx868_idle_rearmed,
    103,2,LogSeverity::warning,0,0);
 assert(valid&&overflow&&fail&&recovered);
 LogRecord rows[16]{};
 LogQuery q{};
 q.radio=2;q.radio_activity=true;q.include_noise=false;
 auto n=store.recent(rows,16,q);
 assert(n==4);
 assert(rows[0].sequence==valid && rows[1].sequence==overflow);
 assert(rows[2].sequence==fail && rows[3].sequence==recovered);
 assert(rows[1].uptime_ms==101 && rows[1].value==0);
 assert(rows[2].severity==static_cast<uint8_t>(LogSeverity::error));
 q.radio_activity=false;
 q.stream=static_cast<int>(LogStream::system);
 n=store.recent(rows,16,q);assert(n==3);
 q.stream=static_cast<int>(LogStream::signal);
 n=store.recent(rows,16,q);assert(n==1 && rows[0].sequence==valid);
 // Noise records are still isolated and not visible unless requested.
 store.push(LogStream::noise,LogCode::unknown_packet,110,2);
 q.stream=-1;q.radio_activity=true;q.include_noise=false;
 n=store.recent(rows,16,q);assert(n==4);
 return 0;
}"""
        with tempfile.TemporaryDirectory(prefix="rx868-chronology-") as temp:
            source_file=Path(temp)/"timeline.cpp"
            executable=Path(temp)/"timeline"
            source_file.write_text(source,encoding="utf-8")
            result=subprocess.run(["g++","-std=c++17","-pthread","-O2",
                "-Wall","-Wextra","-Werror",
                "-I"+str(ROOT/"firmware/main"),
                str(source_file),"-o",str(executable)],
                capture_output=True,text=True,timeout=40)
            self.assertEqual(result.returncode,0,result.stderr)
            tested=subprocess.run([str(executable)],capture_output=True,text=True,timeout=15)
            self.assertEqual(tested.returncode,0,tested.stderr)

    def test_csv_code_names_and_no_builder_mirror_change(self):
        cpp=read("firmware/main/rflink_web_ota.cpp")
        for phrase in ("868 RX FIFO overflow (recovery result)",
                       "868 RX unexpectedly IDLE (rearm result)",
                       "868 RX SPI status read failed",
                       "868 RX FIFO read failed (rearm result)"):
            self.assertIn(phrase,cpp)
        log=read("firmware/main/rflink_log_store.hpp")
        for key in ("rx868_fifo_overflow=47","rx868_idle_rearmed=48",
                    "rx868_spi_status_error=49","rx868_fifo_read_error=50"):
            self.assertIn(key,log)
        self.assertIn("LogCode::rx868_fifo_overflow",log)
        self.assertIn("LogCode::rx868_idle_rearmed",log)


if __name__=="__main__":
    unittest.main()
