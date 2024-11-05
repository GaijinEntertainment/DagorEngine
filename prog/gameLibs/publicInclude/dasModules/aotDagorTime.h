//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

inline char *iso8601_format_msec(uint64_t timeMSec, das::Context *ctx, das::LineInfoArg *at)
{
  char buffer[64];
  iso8601_format(timeMSec, buffer, sizeof(buffer));
  return ctx->allocateString(buffer, strlen(buffer), at);
}

inline char *iso8601_format_sec(uint64_t timeSec, das::Context *ctx, das::LineInfoArg *at)
{
  return iso8601_format_msec(timeSec * 1000LL, ctx, at);
}

inline das::int4 get_local_time()
{
  DagorDateTime st;
  ::get_local_time(&st);
  return das::int4(st.hour, st.minute, st.second, st.microsecond);
}

} // namespace bind_dascript
