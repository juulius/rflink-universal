#include "../components/rflink_universal/system_trace.h"
#include <cassert>
#include <cstring>
int main(){
 esphome::rflink_universal::SystemTrace t;
 t.push(1000,"info","learn","dev_01","candidate saved");
 t.push(1200,"info","learn","dev_01","candidate saved");
 assert(t.snapshot().size()==1);
 t.push(2400,"info","learn","dev_01","candidate saved");
 for(unsigned i=0;i<200;i++)t.push(3000+i*1300,"info","tx","dev_01","test sent");
 assert(t.snapshot().size()==128);
 assert(t.snapshot(0,10).size()==10);
 assert(t.snapshot(10000).empty());
 assert(t.set_limit(20));assert(t.limit()==20);assert(t.snapshot().size()==20);
 assert(t.set_limit(256));assert(t.limit()==256);
 for(unsigned i=0;i<300;i++)t.push(300000+i*1300,"info","tx","dev_01","more events");
 assert(t.snapshot().size()==256);
 assert(t.set_limit(0));assert(t.snapshot().empty());
 t.push(800000,"info","tx","dev_01","ignored while disabled");
 assert(t.snapshot().empty());
 assert(t.set_limit(128));
 char long_msg[300];std::memset(long_msg,'X',sizeof(long_msg));long_msg[299]=0;
 t.push(999999,"error","radio","dev_01",long_msg);
 assert(std::strlen(t.snapshot().back().message)==149);
}
