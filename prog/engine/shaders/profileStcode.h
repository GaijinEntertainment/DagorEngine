// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_perfTimer.h>

namespace stcode::profile
{

#if PROFILE_STCODE

void add_time_usec(uint64_t usec);
void dump_avg_time();

#define STCODE_PROFILE_BEGIN() uint64_t refTicks__ = profile_ref_ticks()
#define STCODE_PROFILE_END()   stcode::profile::add_time_usec(profile_usec_from_ticks_delta(profile_ref_ticks() - refTicks__))

#else

inline void add_time_usec(uint64_t usec) {}
inline void dump_avg_time() {}

#define STCODE_PROFILE_BEGIN(...)
#define STCODE_PROFILE_END(...)

#endif

} // namespace stcode::profile
