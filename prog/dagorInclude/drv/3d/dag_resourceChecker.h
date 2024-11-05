//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_globDef.h>

struct ResourceChecker
{
public:
#if DAGOR_DBGLEVEL > 0
  static void init();
  static void report();
#else
  static void init() {}
  static void report() {}
#endif
protected:
  void checkLockParams(uint32_t offset, uint32_t size, int flags, int bufFlags)
  {
    G_UNUSED(offset);
    G_UNUSED(size);
    G_UNUSED(flags);
    G_UNUSED(bufFlags);
#if DAGOR_DBGLEVEL > 0
    interlocked_add(uploaded_total, size);
    if ((bufFlags & SBCF_FRAMEMEM) == 0)
      return;

    interlocked_add(uploaded_framemem, size);

    // this is max size of single upload, has to fit internal ring buffer chunk size
    // otherwise its gonna be always allocated/freed. could be logerr instead of assert
    const uint32_t max_dynamic_buffer_size = 2 << 20;
    G_UNUSED(max_dynamic_buffer_size);
    G_ASSERT(offset == 0);
    G_ASSERT(size <= max_dynamic_buffer_size);
    G_ASSERT(bufFlags & SBCF_DYNAMIC);
    G_ASSERT((bufFlags & SBCF_BIND_UNORDERED) == 0);
    G_ASSERT(flags & VBLOCK_DISCARD);
    G_ASSERT((flags & (VBLOCK_READONLY | VBLOCK_NOOVERWRITE)) == 0);
#endif
  }

#if DAGOR_DBGLEVEL > 0
  static uint32_t uploaded_framemem;
  static uint32_t uploaded_total;
  static uint32_t uploaded_framemem_limit;
  static uint32_t uploaded_total_limit;
#endif
};
