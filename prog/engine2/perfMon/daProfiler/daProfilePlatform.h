#pragma once
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_pix.h>
#if _TARGET_C1 | _TARGET_C2

#endif
#include "stl/daProfilerFwdStl.h"
#include "daProfilerDefines.h"

//! signals to the processor that the thread is doing nothing. Not relevant to OS thread scheduling.
#if _TARGET_SIMD_SSE
#define cpu_yield _mm_pause
#elif defined(__arm__) || defined(__aarch64__)
#define cpu_yield() __asm__ __volatile__("yield")
#else
#define cpu_yield() ((void)0)
#endif

#define SPINS_BEFORE_SLEEP 8192
namespace da_profiler
{
void report_debug(const char *, ...);
void report_logerr(const char *);
inline void init_cpu_frequency() { init_profile_timer(); }
inline uint64_t cpu_frequency() { return profile_ticks_frequency(); }
inline uint64_t cpu_current_ticks() { return profile_ref_ticks(); }
const char *get_default_file_dir();      // can be null
const char *get_current_thread_name();   // can be null
const char *get_current_platform_name(); // can be null
const char *get_current_host_name();     // can be null
const char *get_cpu_name();              // can be null!
const char *get_executable_name();       // can NOT be null!
void sleep_usec(int usec);
void sleep_msec(int msec);
uint64_t global_timestamp();
uint32_t time_since_launch_msec();
uint64_t gpu_frequency();
uint32_t get_current_process_id();
uint32_t get_logical_cores();
uint64_t get_current_thread_id();
// sampling helper function
bool support_sample_other_threads();
bool gpu_cpu_ticks_ref(int64_t &cpu, int64_t &gpu);

bool stackhlp_enum_modules(const function<bool(const char *, size_t base, size_t size)> &cb);
bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname, size_t max_symbol_name);
bool can_resolve_symbols();

template <typename F>
__forceinline void spin_wait_no_profile(F keep_waiting_cb)
{
  size_t spinsBeforeSleep = 0;
  while (keep_waiting_cb())
  {
    if (++spinsBeforeSleep < SPINS_BEFORE_SLEEP)
      cpu_yield();
    else
      sleep_usec(spinsBeforeSleep >= (SPINS_BEFORE_SLEEP * 2) ? 1000 : 0);
  }
}
template <class T>
inline T max(const T &a, const T &b)
{
  return a > b ? a : b;
}
template <class T>
inline T min(const T &a, const T &b)
{
  return a < b ? a : b;
}
template <class T>
inline T clamp(const T &v, const T &mn, const T &mx)
{
  return v < mn ? mn : (v > mx ? mx : v);
}
} // namespace da_profiler

#if _TARGET_XBOX

#define PLATFORM_EVENT_START(enabled, label) \
  {                                          \
    if (enabled)                             \
    {                                        \
      PIX_BEGIN_CPU_EVENT(label);            \
    }                                        \
  }
#define PLATFORM_EVENT_STOP(enabled) \
  {                                  \
    if (enabled)                     \
    {                                \
      PIX_END_CPU_EVENT();           \
    }                                \
  }

#elif _TARGET_C1 | _TARGET_C2
















#else

#define PLATFORM_EVENT_START(enabled, label)
#define PLATFORM_EVENT_STOP(enabled)

#endif
