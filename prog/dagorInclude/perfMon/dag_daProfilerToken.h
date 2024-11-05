//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace da_profiler
{
// predefined descriptions ids
// desc_id_t is not a strong class, so interlocked operations works on it
enum
{
  NoDesc,
  DescCritsec,
  DescMutex,
  DescSpinlock,
  DescGlobalMutex,
  DescWriteLock,
  DescReadLock,
  DescThreadPool,
  DescRpc,
  DescSleep,
  DescCount
};
typedef uint32_t desc_id_t;
enum
{
  Normal = 0,
  IsWait = 1
}; // flag
} // namespace da_profiler
