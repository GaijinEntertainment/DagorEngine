//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <util/dag_preprocessor.h>

#if DAGOR_DBGLEVEL > 0
#define PROFILER_TRACKER_ENABLED 1
#endif

namespace profiler_tracker
{
void init();
void close();
void start_frame();
// recommended: use milliseconds for consistency between different values
// record_value behavior:
//  history[currnent_frame - frameOffset] += value
void record_value(const eastl::string &group, const eastl::string &label, float value, uint32_t frame_offset);
void record_value(const char *group, const char *label, float value, uint32_t frame_offset);

void clear();

class Timer
{
  int64_t stamp;

public:
  Timer();
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
  Timer(Timer &&) = default;
  Timer &operator=(Timer &&) = default;

  int timeUSec() const;
};

struct ScopeTimeTracker
{
  const char *g;
  const char *n;
  Timer t;
  explicit ScopeTimeTracker(const char *group, const char *name) : g(group), n(name) {}
  ~ScopeTimeTracker()
  {
    record_value(g, n, static_cast<float>(t.timeUSec()) / 1000.0f, 0); // to msec
  }
};

} // namespace profiler_tracker

#if PROFILER_TRACKER_ENABLED
// Use these for scopes, which are executed once pre frame
#define TRACK_SCOPE_TIME(group, name) profiler_tracker::ScopeTimeTracker DAG_CONCAT(profile_tracker_, __LINE__)(#group, #name)
#else
#define TRACK_SCOPE_TIME(group, name) static_cast<void>(0)
#endif
