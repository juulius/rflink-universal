#include "rflink_native_safety.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace rflink;
 constexpr const char* a="0.7.0-n16r8-s16-b1234abcd";
 constexpr const char* b="0.7.1-n16r8-s16-b1234abcd";
 constexpr const char* c="0.7.1-n16r8-s16-bffff0000";
 assert(same_board_profile_for_ota(a,32,b,32));
 assert(!same_board_profile_for_ota(a,32,c,32));
 assert(!same_board_profile_for_ota(a,32,"0.7.0-n16r8-s15",32));
 assert(!same_board_profile_for_ota("0.7.0-n16r8-s15",32,b,32));
 std::cout<<"PASS board OTA profile matching\n";
}
