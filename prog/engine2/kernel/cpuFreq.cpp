#include <perfMon/dag_cpuFreq.h>
#include <supp/_platform.h>
#include <debug/dag_assert.h>
#include <util/dag_stdint.h>

#if _TARGET_PC_WIN
#pragma comment(lib, "winmm.lib")
// dummy wrapper to make windows late imports binding work (important for calls before measure_cpu_freq(), e.g. from static ctors)
static void queryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) { QueryPerformanceCounter(lpPerformanceCount); }
union WrapAroundState // helper struct
{
  int as_int;
  struct
  {
    uint8_t last8;  // top 8 bits of last time that actually used for checking wrap-arounds
    uint16_t wraps; // number wrap arounds that added to number of milliseconds
  } values;
};
static volatile int wrapAroundState = 0;
static void queryPerformanceCounterLowRes(LARGE_INTEGER *lpPerformanceCount)
{
  WrapAroundState wstate;
  DWORD now;
  G_STATIC_ASSERT(sizeof(wstate) == sizeof(wrapAroundState));
  while (1)
  {
    int orig = wrapAroundState; // volatiles have acquire semantic on Windows
    wstate.as_int = orig;
    now = timeGetTime();
    uint8_t now8 = static_cast<uint8_t>(now >> 24);
    if (now8 < wstate.values.last8) // wrapped?
      ++wstate.values.wraps;
    wstate.values.last8 = now8;
    if (wstate.as_int == orig) // not changed
      break;
    if (InterlockedCompareExchange((volatile long *)&wrapAroundState, wstate.as_int, orig) == orig)
      break;
    // Another thread changed 'wrapAroundState', retry
  }
  lpPerformanceCount->QuadPart = uint64_t(now) + (uint64_t(wstate.values.wraps) << 32);
}
static void (*pQueryPerformanceCounter)(LARGE_INTEGER *lpPerformanceCount) = &queryPerformanceCounter;
#define QPC pQueryPerformanceCounter
#else
#define QPC QueryPerformanceCounter
#endif

static LARGE_INTEGER qpcCpuFreq = {{1, 1}};
static int64_t startup_ref_time = 0;
static double ticks_per_usec = 1.0;

static bool default_cpu_freq()
{
  QueryPerformanceFrequency(&qpcCpuFreq);
  startup_ref_time = ref_time_ticks();
  return false;
}

static bool measured = default_cpu_freq();

void measure_cpu_freq(bool force_lowres_timer)
{
  if (measured)
    return;
  measured = true;

#if _TARGET_PC_WIN
  timeBeginPeriod(1); // always increase timer resolution
  if (force_lowres_timer)
  {
    pQueryPerformanceCounter = &queryPerformanceCounterLowRes;
    qpcCpuFreq.QuadPart = 1000; // 1000 ms in one sec
  }
  else
#else
  (void)force_lowres_timer;
#endif
  {
    QueryPerformanceFrequency(&qpcCpuFreq);
  }
  ticks_per_usec = double(qpcCpuFreq.QuadPart) / 1000000.0;
  startup_ref_time = ref_time_ticks();
}

int64_t ref_time_ticks()
{
  LARGE_INTEGER ref;
  QPC(&ref);
  return ref.QuadPart;
}

int64_t rel_ref_time_ticks(int64_t ref, int time_usec) { return ref + int64_t(time_usec * ticks_per_usec); }

int64_t ref_ticks_frequency() { return qpcCpuFreq.QuadPart; }
int64_t ref_time_delta_to_nsec(int64_t ref)
{
  return (ref < 7000000000LL) ? ref * 1000000000 / qpcCpuFreq.QuadPart : ref_time_delta_to_usec(ref) * 1000;
}
int64_t ref_time_delta_to_usec(int64_t ref)
{
  return (ref < 7000000000000LL) ? ref * 1000000 / qpcCpuFreq.QuadPart : int64_t(ref / ticks_per_usec);
}

int64_t get_time_nsec(int64_t ref)
{
  LARGE_INTEGER t;
  QPC(&t);
  return ref_time_delta_to_nsec(t.QuadPart - ref);
}

int get_time_usec(int64_t ref)
{
  LARGE_INTEGER t;
  QPC(&t);
  return ref_time_delta_to_usec(t.QuadPart - ref);
}

int get_time_msec() { return int(ref_time_delta_to_usec(ref_time_ticks() - startup_ref_time) / 1000); }

#define EXPORT_PULL dll_pull_kernel_cpuFreq
#include <supp/exportPull.h>
