#include <xxhash.h>
//if we called dispatch function on non x86/x64 or if we are on avx arch, or if we are in clang (clang is uncapable of generating avx intrinsincs on non avx target)
#if !(defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)) || (defined(__AVX__) && !defined(XXH_X86DISPATCH_ALLOW_AVX)) || defined(__clang__)
#include "xxh_x86dispatch_stub.c"
#else
#include "xxh_x86dispatch.c"
#endif
