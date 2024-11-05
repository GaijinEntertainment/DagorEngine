// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// global driver lock / GPU lock / GPU acquire
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_stdint.h>

#if VULKAN_VERIFY_GLOBAL_STATE_LOCK_ACCESS > 0
#define VERIFY_GLOBAL_LOCK_ACQUIRED() G_ASSERT(Globals::lock.isAcquired())
#else
#define VERIFY_GLOBAL_LOCK_ACQUIRED()
#endif

namespace drv3d_vulkan
{

struct GlobalDriverLock
{
  static thread_local uint32_t acquired;
  WinCritSec cs;

  static bool isAcquired() { return acquired > 0; }

  void acquire()
  {
    cs.lock();
    ++acquired;
  }

  void release()
  {
    --acquired;
    cs.unlock();
  }
};

} // namespace drv3d_vulkan
