// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_alternateProfiler.h>

#include <util/dag_convar.h>
#include <osApiWrappers/dag_miscApi.h>
#include <EASTL/array.h>

CONSOLE_INT_VAL("alternating_profiler", variant, 0, 0, 100,
  "Set the config id for the profiler. Use it while the period is set to 0.");
CONSOLE_INT_VAL("alternating_profiler", period, 0, 0, 1000000,
  "Duration in milliseconds. 0: measurements are off, and results are cleared; otherwise: config id is periodically changed at this "
  "interval and performance is measured.");
CONSOLE_BOOL_VAL("alternating_profiler", print, false, "Print measurement results without clearing them while measuring (period > 0)");

float AlternatingProfilerResult::getFrameTimeMs() const
{
  return numFrames > 0 ? (float)totalRunTimeUs / 1000.0f / (float)numFrames : 0;
}

float AlternatingProfilerResult::getFps() const
{
  return totalRunTimeUs > 0 ? (float)numFrames * 1000000.f / (float)totalRunTimeUs : 0;
}

bool AlternatingConfigProfiler::pop_new_variant_id(int &new_value)
{
  if (!variant.pullValueChange())
    return false;
  new_value = variant.get();
  return true;
}

void AlternatingConfigProfiler::set_auto_variant_id(int value)
{
  variant.set(value);
  variant.pullValueChange(); // don't handle own change events
}

int AlternatingConfigProfiler::get_current_period_ms(bool *has_changed)
{
  if (has_changed)
    *has_changed = period.pullValueChange();
  return period;
}

bool AlternatingConfigProfiler::pop_should_print()
{
  bool ret = ::print;
  ::print.set(false);
  return ret;
}

AlternatingConfigProfiler::AlternatingConfigProfiler(int num_variants) : numVariants(num_variants), results(num_variants)
{
  G_ASSERT(num_variants > 0);
}

void AlternatingConfigProfiler::clear() { results = eastl::vector<AlternatingProfilerResult>(numVariants); }

const AlternatingProfilerResult &AlternatingConfigProfiler::getResult(int index) const { return results[index]; }

int AlternatingConfigProfiler::getCurrentConfigId() const { return lastConfig; }

void AlternatingConfigProfiler::example_update()
{
  static int sleep_us = 0;
  const eastl::array<int, 5> sleepDurations = {0, 50, 250, 1000, 5000};
  AlternatingConfigProfiler::update<5>([&](int id) { sleep_us = sleepDurations[id]; });
  if (sleep_us > 0)
  {
    int64_t timerStart = ::ref_time_ticks();
    while (::get_time_usec(timerStart) < sleep_us)
      ;
  }
}
