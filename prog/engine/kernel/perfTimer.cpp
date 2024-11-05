// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_log.h>
#include <util/dag_stdint.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_miscApi.h>

#define USE_NEON_TICKS_SCALE (_TARGET_SIMD_NEON && !(_TARGET_PC_WIN | _TARGET_C3))
#if USE_NEON_TICKS_SCALE
uint32_t profiler_ticks_scale = 1;
#endif

#if NATIVE_PROFILE_TICKS

uint32_t profiler_ticks_to_us = 1000;
uint64_t profiler_ticks_frequency = 1000000 * profiler_ticks_to_us;
static void measure_profile_timer();
static uint32_t native_tsc_khz();
static bool native_constant_tsc();

#if _TARGET_SIMD_NEON

// cntfrq_el0 can lie for some reason (or it is non constant for some devices?)
static bool verify_profile_timer()
{
  measure_cpu_freq();
  bool ret = true;

  // check in loop that wait time fits in error window
  // otherwise HW readed counter is incorrect
  // note: error window is for fact that sleep_msec can be a bit imprecise
  const int checkLoops = 10;
  const int checkWaitMs = 10;
  const int checkWaitMsErr = 2;

  for (int i = 0; i < checkLoops; ++i)
  {
    const uint64_t profileRef = profile_ref_ticks();
    // we can't use precise sleep at it uses profile timer!
    sleep_msec(checkWaitMs);
    const uint64_t now = profile_ref_ticks();

    uint64_t fq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(fq));
#if USE_NEON_TICKS_SCALE
    fq *= profiler_ticks_scale;
#endif

    uint64_t dt = now - profileRef;
    uint64_t dtUs = profile_usec_from_ticks_delta(dt);

    // check that profiled time fits in error window
    ret &= dtUs < ((checkWaitMs + checkWaitMsErr) * 1000);
    ret &= dtUs > ((checkWaitMs - checkWaitMsErr) * 1000);
    // check that frequency is not changed
    ret &= fq == profiler_ticks_frequency;

    if (!ret)
    {
      logwarn("verify_profile_timer: HW frequency is incorrect! tick dt %u dt_us %u fq %llu fq_old %llu", dt, dtUs, fq,
        profiler_ticks_frequency);
      break;
    }
  }

  return ret;
}

#endif

void adjust_freq_for_precise_ticks_to_us()
{
#if USE_NEON_TICKS_SCALE
  // some devices have freq that does not convert to us as int multiplier (1.92Mhz for ex.)
  // find that case and adjust scale to avoid precision issues
  uint32_t ticksToUsU;
  float ticksToUsF;
  float precDlt;

  auto recalc = [&]() {
    ticksToUsU = profiler_ticks_frequency / 1000000;
    ticksToUsF = profiler_ticks_frequency / 1000000.0f;
    precDlt = ticksToUsF - ticksToUsU;
    if (precDlt > 0.01)
      logwarn("profile timer ticks to us non precise should be %f instead of %u (%f dlt)", ticksToUsF, ticksToUsU, precDlt);
    return precDlt > 0.01;
  };

  // iterate with scale limited with something reasonable
  while (recalc() && (profiler_ticks_scale < 1000))
  {
    profiler_ticks_scale *= 10;
    profiler_ticks_frequency *= 10;
  }
#endif
  profiler_ticks_to_us = profiler_ticks_frequency / 1000000; // from 1Mhz to 50Mhz. M1 has 24 Mhz
}

void init_profile_timer()
{
  static bool inited = false;
  if (inited)
    return;
  inited = true;
#if _TARGET_SIMD_SSE
  if (!native_constant_tsc())
    logwarn("CPU doesn't support constant tsc, profiling is unreliable");
  const uint32_t native_khz = native_tsc_khz();
  if (native_khz)
  {
    profiler_ticks_to_us = native_khz / 1000;
    profiler_ticks_frequency = native_khz * 1000;
    return;
  }
#elif _TARGET_SIMD_NEON
  {
    asm volatile("mrs %0, cntfrq_el0" : "=r"(profiler_ticks_frequency));
    adjust_freq_for_precise_ticks_to_us();
    if (profiler_ticks_to_us && verify_profile_timer())
      return;
    else
      logwarn("ARM CPU provided incorrect frequency, using software measure");
  }
#endif
  measure_profile_timer();
}

static void measure_profile_timer()
{
#if USE_NEON_TICKS_SCALE
  // reset scale if any was generated, to recalculate it with new results more precisely
  profiler_ticks_scale = 1;
#endif
  measure_cpu_freq();
  const int64_t reft = ref_time_ticks();
  const uint64_t profileRef = profile_ref_ticks();
  sleep_msec(10);
  const uint64_t nowt = profile_ref_ticks();
  const int64_t reft2 = ref_time_ticks();
  const double ref_time_passed_seconds = double(reft2 - reft) / ref_ticks_frequency();
  const double profFreq = double(int64_t(nowt) - int64_t(profileRef)) / ref_time_passed_seconds;
  profiler_ticks_frequency = uint64_t(profFreq);
  profiler_ticks_to_us = uint32_t(profFreq / 1000000.0);
  adjust_freq_for_precise_ticks_to_us();
  if (profiler_ticks_to_us < 1)
    profiler_ticks_to_us = 1;
  if (profiler_ticks_frequency < 1)
    profiler_ticks_frequency = 1;
  debug("measure_profile_timer: freq %llu ticks_to_us %lu", profiler_ticks_frequency, profiler_ticks_to_us);
}

#if _TARGET_SIMD_SSE
#if !_MSC_VER || defined(__clang__)
#include <cpuid.h>
#else

static inline void __get_cpuid(uint32_t l, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  int regs[4];
  __cpuid(regs, l);
  *eax = regs[0];
  *ebx = regs[1];
  *ecx = regs[2];
  *edx = regs[3];
}

#endif

static bool native_constant_tsc()
{
  uint32_t regs[4];
  // detect if the Advanced Power Management feature is supported
  __get_cpuid(0x80000000, &regs[0], &regs[1], &regs[2], &regs[3]);
  if (regs[0] < 0x80000007)
    return false;
  __get_cpuid(0x80000007, &regs[0], &regs[1], &regs[2], &regs[3]);
  // if bit 8 is set than TSC will run at a constant rate
  // in all ACPI P-state, C-states and T-states
  if (regs[3] & (1 << 8))
    return true;
  return false;
}

static uint32_t native_tsc_khz()
{
  uint32_t regs[4];
  __get_cpuid(0x00000000, &regs[0], &regs[1], &regs[2], &regs[3]);
  uint32_t cpuid_level = regs[0];
  if (cpuid_level < 0x15)
    return 0;

  unsigned int eax_denominator, ebx_numerator, ecx_hz, edx;
  unsigned int crystal_khz;

  eax_denominator = ebx_numerator = ecx_hz = edx = 0;

  // CPUID 15H TSC/Crystal ratio, plus optionally Crystal Hz
  __get_cpuid(0x15, &eax_denominator, &ebx_numerator, &ecx_hz, &edx);

  if (ebx_numerator == 0 || eax_denominator == 0)
    return 0;

  crystal_khz = ecx_hz / 1000;

  // Some Intel SoCs like Skylake and Kabylake don't report the crystal
  // clock, but we can easily calculate it to a high degree of accuracy
  // by considering the crystal ratio and the CPU speed.
  if (crystal_khz == 0 && cpuid_level >= 0x16)
  {
    unsigned int eax_base_mhz, ebx, ecx, edx;

    __get_cpuid(0x16, &eax_base_mhz, &ebx, &ecx, &edx);
    return eax_base_mhz * 1000;
  }

  if (crystal_khz == 0)
    return 0;

  return uint32_t(uint64_t(crystal_khz * ebx_numerator) / uint64_t(eax_denominator));
}
#endif //_TARGET_SIMD_SSE

#endif // NATIVE_PROFILE_TICKS
#define EXPORT_PULL dll_pull_kernel_perfTimer
#include <supp/exportPull.h>
