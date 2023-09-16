//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <iso8601Time/iso8601Time.h>
#include <perfMon/dag_cpuFreq.h>

#include <osApiWrappers/dag_miscApi.h>

namespace bind_dascript
{

inline uint64_t iso8601_parse_msec(const char *v) { return iso8601_parse(v); }
inline uint64_t iso8601_parse_sec(const char *v)
{
  long long timeMsec = iso8601_parse(v);
  return (timeMsec / 1000LL);
}

inline char *iso8601_format_msec(uint64_t timeMSec, das::Context *ctx)
{
  char buffer[64];
  iso8601_format(timeMSec, buffer, sizeof(buffer));
  return ctx->stringHeap->allocateString(buffer, strlen(buffer));
}

inline char *iso8601_format_sec(uint64_t timeSec, das::Context *ctx) { return iso8601_format_msec(timeSec * 1000LL, ctx); }

inline das::int3 get_local_time()
{
  DagorDateTime st;
  ::get_local_time(&st);
  return das::int3(st.hour, st.minute, st.second);
}

} // namespace bind_dascript
