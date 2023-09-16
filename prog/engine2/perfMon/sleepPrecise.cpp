#include <perfMon/dag_sleepPrecise.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_globDef.h> // min, max
#include <math.h>             // sqrtf

// Source: "Making an accurate Sleep() function"
// https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
void sleep_precise_usec(int time_usec)
{
  // Note: float min step (precision) on values 16000 us (i.e 16 ms) is ~1ns
  static float estimate = 2e3f, mean = estimate, m2 = 0;
  static int64_t count = 1;

  float timeLeftUs = time_usec;
  while (timeLeftUs > estimate)
  {
    int64_t ref = profile_ref_ticks();
    sleep_msec(1);
    float observed = min((unsigned)profile_time_usec(ref), 20000u);

    timeLeftUs -= observed;

    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
    ++count;
    float delta = observed - mean;
    mean += delta / count;
    m2 += delta * (observed - mean);
    float stddev = sqrtf(m2 / (count - 1));
    estimate = max(1e3f, mean + stddev); // we ask to sleep for 1msec, so regardless of estimate we shouldn't try to sleep if time left
                                         // is less than estimate

    if (count >= 10000) // otherwise m2 will grow to infinity, and/or more likely stddev->0.
    // we could just stop counting at that point, but restarting gives us some sustainability on timer change
    {
      // 10000 iterations results in a average m2 of up to 500^2 * 10000 (2.5e9)
      // adding anything less than 10000 wouldn't even change m2, so difference of 100us wouldn't change m2 (but will continue to
      // change stddev)
      count = 1;          // restart counting
      m2 = delta * delta; // mean is still same
    }
  }

  unsigned iTimeLeftUs = max((int)timeLeftUs, 0);
  int64_t ref = profile_ref_ticks();
  while (!profile_usec_passed(ref, iTimeLeftUs))
    cpu_yield();
}