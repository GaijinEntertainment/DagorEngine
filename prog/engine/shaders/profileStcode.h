// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_perfTimer.h>

namespace stcode::profile
{

#if PROFILE_STCODE

void add_dyncode_time_usec(uint64_t usec);
void add_blkcode_time_usec(uint64_t usec);
void dump_avg_time();

#define STCODE_PROFILE_BEGIN() uint64_t refTicks__ = profile_ref_ticks()
#define STCODE_PROFILE_END()   stcode::profile::add_time_usec(profile_usec_from_ticks_delta(profile_ref_ticks() - refTicks__))

#define DYNSTCODE_PROFILE_BEGIN() uint64_t refTicks__ = profile_ref_ticks()
#define DYNSTCODE_PROFILE_END(...) \
  stcode::profile::add_dyncode_time_usec(profile_usec_from_ticks_delta(profile_ref_ticks() - refTicks__))
#define STBLKCODE_PROFILE_BEGIN() uint64_t refTicks__ = profile_ref_ticks()
#define STBLKCODE_PROFILE_END(...) \
  stcode::profile::add_blkcode_time_usec(profile_usec_from_ticks_delta(profile_ref_ticks() - refTicks__))

#else

inline void add_dyncode_time_usec(uint64_t usec) {}
inline void add_blkcode_time_usec(uint64_t usec) {}
inline void dump_avg_time() {}

#define DYNSTCODE_PROFILE_BEGIN(...)
#define DYNSTCODE_PROFILE_END(...)
#define STBLKCODE_PROFILE_BEGIN(...)
#define STBLKCODE_PROFILE_END(...)

#endif

} // namespace stcode::profile
