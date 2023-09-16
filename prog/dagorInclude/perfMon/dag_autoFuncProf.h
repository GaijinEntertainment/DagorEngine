//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum AfpUnits
{
  AFP_USEC = 1,
  AFP_MSEC = 1000,
  AFP_SEC = 1000000
};

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
template <int units, int64_t minv = 0>
struct AutoFuncProfT
{
  AutoFuncProfT(const char *fmtStr) { start(fmtStr); }
  ~AutoFuncProfT() { finish(); }
  inline void start(const char *fmtStr)
  {
    fmt = fmtStr;
    ref = ref_time_ticks();
  }
  inline void finish()
  {
    int64_t v = get_time_usec(ref) / units;
    fmt && (v >= minv) ? debug(fmt, v) : (void)0;
    fmt = NULL;
  }
  int64_t ref;
  const char *fmt;
};
#else
template <int units, int64_t minv = 0>
struct AutoFuncProfT
{
  AutoFuncProfT(const char *) {}
  void start(const char *) {}
  void finish() {}
};
#endif

typedef AutoFuncProfT<AFP_USEC> AutoFuncProf;
