#pragma once

#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_assert.h>


namespace drv3d_dx12
{

#if DX12_ENABLE_MT_VALIDATION
class DriverMutex
{
  WinCritSec mutex;
  bool enabled = false;
  uint32_t recursionCount = 0;

public:
  void lock()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    // NOTE: During startup this will fire at least once as the work cycle just takes the lock to check
    //       the d3d context device state.
    G_ASSERTF(true == enabled, "DX12: Trying to lock the driver context without enabling "
                               "multithreading first");
#endif
    mutex.lock();
    ++recursionCount;
  }

  void unlock()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    G_ASSERTF(true == enabled, "DX12: Trying to unlock the driver context without enabling "
                               "multithreading first");
#endif
    --recursionCount;
    mutex.unlock();
  }

  bool validateOwnership()
  {
    if (mutex.tryLock())
    {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
      bool owns = false;
      if (enabled)
      {
        // If we end up here the two thing can either be true:
        // 1) this thread has taken the lock before this, then recustionCount is greater than 0
        // 2) no thread has taken the lock before this, then recursionCount is equal to 0
        owns = recursionCount > 0;
      }
      else
      {
        // if MT is not enabled, only the main thread can access it without taking a lock.
        owns = owns || (0 == recursionCount) && is_main_thread();
      }
#else
      // DX11 behavior replicating expression, it has a flaw, which allows a race between the main
      // thread and a different thread as long as MT is disabled.
      bool owns = (!enabled && (0 == recursionCount) && is_main_thread()) || (recursionCount > 0);
#endif
      mutex.unlock();
      return owns;
    }
    // Failed to take the mutex, some other thread has it, so we don't own it.
    return false;
  }

  void enableMT()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    // NOTE: This locking will show ordering issues during startup, esp with splash screen.
    mutex.lock();
    G_ASSERT(false == enabled);
#endif
    enabled = true;
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.unlock();
#endif
  }

  void disableMT()
  {
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.lock();
    G_ASSERT(true == enabled);
#endif
    enabled = false;
#if DX12_ENABLE_PEDANTIC_MT_VALIDATION
    mutex.unlock();
#endif
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

  void enableMT() {}

  void disableMT() {}
};
#endif

} // namespace drv3d_dx12
