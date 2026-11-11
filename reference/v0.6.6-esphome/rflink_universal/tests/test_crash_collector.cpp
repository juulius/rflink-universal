#include <cassert>
#include <cstring>
#include "../components/rflink_universal/crash_trace_collector.h"
using esphome::rflink_universal::CrashTraceCollector;
static void emit(CrashTraceCollector &c, const char* m) { c.ingest("esp32.crash",m,std::strlen(m)); }
int main(){
 CrashTraceCollector a;emit(a,"*** CRASH DETECTED ON PREVIOUS BOOT ***");emit(a,"  Reason: Abort");emit(a,"  Crashed core: 1");emit(a,"  PC: 0x4037D364 (fault location)");emit(a,"  BT0: 0x4037D361");emit(a,"  BT1: 0x4200D123");
 assert(a.complete());assert(a.report().core==1);assert(a.report().pc==0x4037D364);assert(a.report().backtrace_count==2);assert(a.report().backtrace[1]==0x4200D123);assert(!a.report().foreign_build);
 CrashTraceCollector b;emit(b,"*** CRASH DETECTED ON PREVIOUS BOOT ***");emit(b," Reason: Task wdt");emit(b," Crashed core: 0");emit(b," Captured by a different firmware build; addresses belong to that build's ELF");emit(b," pc: 0x4037D364");emit(b," bt0: 0x4037D361");emit(b," bt1: 0x420BEF12");assert(b.complete());assert(b.report().foreign_build);assert(b.report().backtrace_count==2);
 CrashTraceCollector c;c.ingest("unrelated","*** CRASH DETECTED ON PREVIOUS BOOT ***",36);assert(!c.complete());
}
