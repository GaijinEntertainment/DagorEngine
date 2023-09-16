//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <perfMon/dag_cpuFreq.h>
#include <supp/dag_define_COREIMP.h>

// this is for profiling only, and it uses rdtsc on all x86 platforms
// on all modern architectures, it is guaranteed to work constant and usually even invariant (i.e. continue to tick in deep-c state)
// in reality we never care about timer ticking in deep-c state
// we really care about constant (i.e. independent from frequency) TSC
// hypothetically, some code can run on older HW (older than 2008).
// in that case, you can't rely on consistency of profile_ref_ticks functions (you can still use it for profiling), as it can goes back
// in time it is still safe, if you don't assume it always return 'reasonable' values.

#if _TARGET_SIMD_NEON && !_TARGET_C3

#define NATIVE_PROFILE_TICKS 1

inline uint64_t profile_ref_ticks()
{
  // __builtin_readcyclecounter() doesn't work for arm64 on Android, iOS and macOS (maybe doesn't work anywhere at all)
  uint64_t ticks;
  asm volatile("mrs %0, cntvct_el0" : "=r"(ticks));
  return ticks;
}
#elif _TARGET_SIMD_SSE
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#define NATIVE_PROFILE_TICKS 1
inline uint64_t profile_ref_ticks() { return __rdtsc(); }
#else

#define NATIVE_PROFILE_TICKS 0

inline uint64_t profile_ref_ticks() { return ref_time_ticks(); }
#endif

#if NATIVE_PROFILE_TICKS
extern KRNLIMP uint32_t profiler_ticks_to_us;
extern KRNLIMP uint64_t profiler_ticks_frequency;
inline uint64_t profile_ticks_frequency() { return profiler_ticks_frequency; }
inline uint64_t profile_ticks_per_usec() { return profiler_ticks_to_us; }
inline uint32_t profile_usec_from_ticks_delta(uint64_t delta)
{
  return delta / profiler_ticks_to_us; // division isn't fast, but we use it even in ref_time_*
}
inline bool profile_usec_passed(uint64_t ref, uint32_t usec)
{
  return (profile_ref_ticks() - ref) > uint64_t(usec) * uint64_t(profiler_ticks_to_us);
}
extern KRNLIMP void init_profile_timer();
#else
inline uint64_t profile_ticks_frequency() { return ref_ticks_frequency(); }
inline uint64_t profile_ticks_per_usec() { return profile_ticks_frequency() / 1000000; }
inline uint32_t profile_usec_from_ticks_delta(uint64_t a) { return ref_time_delta_to_usec(a); }
inline bool profile_usec_passed(uint64_t ref, uint32_t usec)
{
  return profile_usec_from_ticks_delta(profile_ref_ticks() - ref) > usec;
}
inline void init_profile_timer() {}
#endif

inline int profile_time_usec(int64_t ref) { return profile_usec_from_ticks_delta(profile_ref_ticks() - ref); } // convert to usec

#include <supp/dag_undef_COREIMP.h>
