// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFx/dafx.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_mathUtils.h> // saturate
#include <daFx/dafx_def.hlsli>

// #define DAFX_VERBOSE_DEBUG 1
// #define DAFX_FLUSH_BEFORE_UPDATE 1 // only for perf testing

#if DAGOR_DBGLEVEL > 0
#define DAFX_STAT 1
#else
#undef DAFX_VERBOSE_DEBUG
#undef DAFX_FLUSH_BEFORE_UPDATE
#endif

#ifndef DAFX_STAT_DEF
#define DAFX_STAT_DEF
#if DAFX_STAT
inline void stat_inc(int &v) { ++v; }
inline void stat_add(int &v, int b) { v += b; }
inline void stat_set(int &v, int b) { v = b; }
#else
inline void stat_inc(int &) {}
inline void stat_add(int &, int) {}
inline void stat_set(int &, int) {}
#endif
#endif

#if DAFX_VERBOSE_DEBUG
#define DBG_OPT(...) logmessage(_MAKE4C('DAFX'), __VA_ARGS__)
#else
#define DBG_OPT(...) (void)0
#endif

namespace dafx
{
inline int get_aligned_size(int sz)
{
  if (sz == 0)
    return 0;

  sz = (sz - 1) / DAFX_ELEM_STRIDE;
  sz = (sz + 1) * DAFX_ELEM_STRIDE;
  return sz;
}
} // namespace dafx