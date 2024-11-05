// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_profilerTracker.h>

void profiler_tracker::init() {}

void profiler_tracker::close() {}

void profiler_tracker::start_frame() {}

void profiler_tracker::record_value(const eastl::string &, const eastl::string &, float, uint32_t) {}

void profiler_tracker::record_value(const char *, const char *, float, uint32_t) {}

void profiler_tracker::clear() {}

profiler_tracker::Timer::Timer() : stamp(0) {}

int profiler_tracker::Timer::timeUSec() const { return 0; }
