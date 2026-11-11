#include <cassert>
#include <cstdio>
#include <cstring>
#include "user_library.h"
#include "profile_catalog.h"
using namespace esphome::rflink_universal;
static uint32_t hash(const UserLibraryPersistence::Blob &blob) {
 uint32_t h=2166136261u;auto step=[&](uint8_t v){h=(h^v)*16777619u;};
 for(unsigned n=0;n<32;n+=8)step(uint8_t(blob.magic>>n));
 step(uint8_t(blob.version));step(uint8_t(blob.version>>8));
 step(uint8_t(blob.used));step(uint8_t(blob.used>>8));
 for(unsigned n=0;n<32;n+=8)step(uint8_t(blob.generation>>n));
 for(unsigned n=0;n<blob.used;++n)step(blob.bytes[n]);
 return h;
}
static void skipstr(const UserLibraryPersistence::Blob& b,size_t &at){const size_t n=b.bytes[at]|(size_t(b.bytes[at+1])<<8);at+=2+n;assert(at<=b.used);}
int main(){
 size_t profiles_count=0;(void)profiles::all(profiles_count);
 assert(profiles_count==26);
 const auto *orcon=profiles::find("orcon_mvs15r_15rf");
 assert(orcon && std::strcmp(orcon->pair_method,"virtual_remote")==0&&orcon->pair_tx_required);
 const auto *itho=profiles::find("itho_rft_5360124");
 assert(itho && std::strcmp(itho->pair_method,"receiver_learn")==0);
 const auto *sensor=profiles::find("acurite_609txc");
 assert(sensor&&std::strcmp(sensor->pair_method,"none")==0&&!sensor->pair_tx_required);
 UserProfile profile{"custom_fan","Custom fan","RamsesII",{"auto","low"},"Press the remote",1};
 profile.controls={"auto","low"};profile.setup_steps={"Listen to the remote","Check the unit"};profile.pair_method="manual";
 UserLibrary store;std::string error;assert(store.upsert_profile(profile,error));
 UserLibraryPersistence::Blob v2;assert(UserLibraryPersistence::encode(store,10,v2));
 assert(UserLibraryPersistence::valid(v2)&&v2.version==2);
 UserLibrary restored;assert(UserLibraryPersistence::decode(v2,restored));
 assert(restored.profiles().size()==1&&restored.profiles()[0].controls.size()==2);
 assert(restored.profiles()[0].setup_steps[1]=="Check the unit");
 assert(restored.profiles()[0].pair_method=="manual");
 // Reconstitute exact persisted v1 profile format by trimming appended v2 fields,
 // then recompute the version-specific checksum. This is an OTA migration test.
 UserLibraryPersistence::Blob v1=v2;size_t at=0;
 assert(v1.bytes[at++]==0); // no decoders
 assert(v1.bytes[at++]==1); // one profile
 skipstr(v1,at);skipstr(v1,at);skipstr(v1,at);
 assert(v1.bytes[at++]==2);skipstr(v1,at);skipstr(v1,at);
 skipstr(v1,at);at+=2; // old per-profile version
 assert(v1.bytes[v1.used-1]==0); // no VM state entries
 v1.bytes[at]=0;v1.used=uint16_t(at+1);v1.version=1;v1.checksum=hash(v1);
 assert(UserLibraryPersistence::valid(v1));
 UserLibrary from_old;assert(UserLibraryPersistence::decode(v1,from_old));
 assert(from_old.profiles().size()==1);
 assert(from_old.profiles()[0].controls.empty()&&from_old.profiles()[0].setup_steps.empty());
 assert(from_old.profiles()[0].pair_method=="none");
 UserLibraryPersistence::Blob upgraded;
 assert(UserLibraryPersistence::encode(from_old,11,upgraded));
 assert(upgraded.version==2&&UserLibraryPersistence::valid(upgraded));
 UserProfile bad{"bad","bad","RamsesII",{"auto"},"help",1};bad.controls={"on"};
 assert(!from_old.upsert_profile(bad,error)); // can't invent a TX control
 puts("PAIRING1 builtins, custom controls, snapshot v1->v2 migration, safety PASS");
}
