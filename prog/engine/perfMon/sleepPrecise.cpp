// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_sleepPrecise.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_globDef.h> // min, max
#include <math.h>             // sqrtf
#include <osApiWrappers/dag_lockProfiler.h>

// Source: "Making an accurate Sleep() function"
// https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
void sleep_precise_usec(int time_usec, PreciseSleepContext &ctx)
{
  ScopeLockProfiler<da_profiler::DescSleep> lp;
  G_UNUSED(lp);

  float timeLeftUs = time_usec;
  while (timeLeftUs > ctx.estimate)
  {
    int64_t ref = profile_ref_ticks();
    sleep_usec(1000);
    float observed = min((unsigned)profile_time_usec(ref), 20000u);

    timeLeftUs -= observed;

    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
    ++ctx.count;
    float delta = observed - ctx.mean;
    ctx.mean += delta / ctx.count;
    ctx.m2 += delta * (observed - ctx.mean);
    float stddev = sqrtf(ctx.m2 / (ctx.count - 1));
    ctx.estimate = max(1e3f, ctx.mean + stddev); // we ask to sleep for 1msec, so regardless of estimate we shouldn't try to sleep if
                                                 // time left is less than estimate

    if (ctx.count >= 10000) // otherwise m2 will grow to infinity, and/or more likely stddev->0.
    // we could just stop counting at that point, but restarting gives us some sustainability on timer change
    {
      // 10000 iterations results in a average m2 of up to 500^2 * 10000 (2.5e9)
      // adding anything less than 10000 wouldn't even change m2, so difference of 100us wouldn't change m2 (but will continue to
      // change stddev)
      ctx.count = 1;          // restart counting
      ctx.m2 = delta * delta; // mean is still same
    }
  }

  unsigned iTimeLeftUs = max((int)timeLeftUs, 0);
  int64_t ref = profile_ref_ticks();
  while (!profile_usec_passed(ref, iTimeLeftUs))
    cpu_yield();
}