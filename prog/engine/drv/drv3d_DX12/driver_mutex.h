// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <osApiWrappers/dag_critSec.h>


namespace drv3d_dx12
{

#if DX12_ENABLE_MT_VALIDATION
class DriverMutex
{
  WinCritSec mutex;
  uint32_t recursionCount = 0;

public:
  void lock()
  {
    mutex.lock();
    ++recursionCount;
  }

  void unlock()
  {
    --recursionCount;
    mutex.unlock();
  }

  bool validateOwnership()
  {
    if (mutex.tryLock())
    {
      // If we end up here the two thing can either be true:
      // 1) this thread has taken the lock before this, then recustionCount is greater than 0
      // 2) no thread has taken the lock before this, then recursionCount is equal to 0
      bool owns = recursionCount > 0;
      mutex.unlock();
      return owns;
    }
    // Failed to take the mutex, some other thread has it, so we don't own it.
    return false;
  }
};
#else
class DriverMutex
{
  WinCritSec mutex;

public:
  void lock() { mutex.lock(); }

  void unlock() { mutex.unlock(); }

  bool validateOwnership() { return true; }
};
#endif

} // namespace drv3d_dx12
