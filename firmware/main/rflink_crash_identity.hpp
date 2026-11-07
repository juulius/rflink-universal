#pragma once
// Pure C++ crash identity helpers. ESP-IDF core dumps carry the crashing
// ELF SHA-256 as a bounded hexadecimal prefix; never assume it is full-length.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rflink {
enum class ElfBuildMatch : uint8_t { unknown, same_full_sha, matching_prefix, different };

// Reject malformed or unterminated data rather than accidentally printing
// arbitrary stack/flash bytes in an unauthenticated diagnostics JSON response.
inline bool copy_elf_sha(const uint8_t* source,size_t available,
                         char (&dest)[65]) {
  dest[0]=0;
  if(!source || available<9)return false;
  const size_t limit=available<sizeof(dest)?available:sizeof(dest);
  size_t len=0;
  while(len<limit && source[len]) {
    const unsigned char c=source[len];
    if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) {
      dest[0]=0;return false;
    }
    dest[len]=(c>='A'&&c<='F')?static_cast<char>(c-'A'+'a'):static_cast<char>(c);
    ++len;
  }
  if(len<8 || len>=limit) { dest[0]=0; return false; }
  dest[len]=0;
  return true;
}

inline ElfBuildMatch compare_elf_sha(const char* running,const char* crashed) {
  if(!running || !crashed)return ElfBuildMatch::unknown;
  const size_t run_len=std::strlen(running),crash_len=std::strlen(crashed);
  if(run_len<8 || crash_len<8 || run_len>64 || crash_len>64)
    return ElfBuildMatch::unknown;
  const size_t prefix=run_len<crash_len?run_len:crash_len;
  if(std::strncmp(running,crashed,prefix)!=0)return ElfBuildMatch::different;
  if(run_len==64 && crash_len==64)return ElfBuildMatch::same_full_sha;
  return ElfBuildMatch::matching_prefix;
}

inline const char* elf_match_label(ElfBuildMatch match) {
  switch(match) {
    case ElfBuildMatch::same_full_sha:return "same_full_sha";
    case ElfBuildMatch::matching_prefix:return "matching_prefix";
    case ElfBuildMatch::different:return "different";
    default:return "unknown";
  }
}
} // namespace rflink
