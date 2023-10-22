#pragma once
#include <perfMon/dag_perfTimer.h>

class ScopedTimer
{
  int64_t &outRef;

public:
  ScopedTimer(int64_t &out) : outRef(out) { outRef = profile_ref_ticks(); }

  ~ScopedTimer() { outRef = profile_time_usec(outRef); }
};


class ScopedTimerTicks
{
  int64_t &outRef;

public:
  ScopedTimerTicks(int64_t &out) : outRef(out) { out = profile_ref_ticks(); }

  ~ScopedTimerTicks() { outRef = profile_ref_ticks() - outRef; }
};
