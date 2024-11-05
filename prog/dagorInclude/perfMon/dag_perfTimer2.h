//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_perfTimer.h>
#include <util/dag_stdint.h>


class PerformanceTimer2
{
public:
  int64_t pm_t1, pm_total; // hold timing in highest resolution ticks
  bool stopped, paused;

public:
  PerformanceTimer2(bool sgo = false)
  {
    pm_total = 0;
    stopped = true;
    paused = false;
    if (sgo)
      go();
  }

  __forceinline void go();
  __forceinline void pause();

  __forceinline void stop()
  {
    pause();
    stopped = true;
  }

  __forceinline void reset()
  {
    stop();
    pm_total = 0;
  }

  int64_t getTotalUsec() { return profile_usec_from_ticks_delta(pm_total); }
  int64_t getTotalNsec() { return getTotalUsec() * 1000; }
};


__forceinline void PerformanceTimer2::go()
{
  pm_t1 = ::profile_ref_ticks();

  if (stopped)
  {
    pm_total = 0;
    stopped = false;
  }

  paused = false;
}


__forceinline void PerformanceTimer2::pause()
{
  if (paused)
    return;

  pm_total += ::profile_ref_ticks() - pm_t1;

  paused = true;
}
