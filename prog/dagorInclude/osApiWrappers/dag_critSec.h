//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

#if _TARGET_APPLE | _TARGET_ANDROID | _TARGET_64BIT
static constexpr int CRITICAL_SECTION_OBJECT_SIZE = 64;
#else
static constexpr int CRITICAL_SECTION_OBJECT_SIZE = 32;
#endif

struct CritSecStorage
{
#if defined(__GNUC__)
  char critSec[CRITICAL_SECTION_OBJECT_SIZE] __attribute__((aligned(16)));
  volatile int locksCount;
#elif defined(_MSC_VER)
  alignas(void *) char critSec[CRITICAL_SECTION_OBJECT_SIZE];
#else
#error Compiler is not supported
#endif
  operator void *() { return critSec; }
};

//
// critical sections
//
KRNLIMP void create_critical_section(void *cc_storage, const char *name = NULL);
KRNLIMP void destroy_critical_section(void *cc);
KRNLIMP void enter_critical_section_raw(void *cc);                                  // no profiling
KRNLIMP void enter_critical_section(void *cc, const char *waiter_perf_name = NULL); // with profiling
KRNLIMP void leave_critical_section(void *cc);
KRNLIMP bool try_enter_critical_section(void *cc);

KRNLIMP int full_leave_critical_section(void *cc);
KRNLIMP void multi_enter_critical_section(void *cc, int enter_cnt);
KRNLIMP bool try_timed_enter_critical_section(void *cc, int timeout_ms, const char *waiter_perf_name = NULL);

//
// Critical section wrapper
//
class WinCritSec
{
private:
  // make copy constructor and assignment operator inaccessible
  WinCritSec(const WinCritSec &refCritSec);
  WinCritSec &operator=(const WinCritSec &refCritSec);

  friend class WinAutoLock;

protected:
  CritSecStorage critSec;

public:
  WinCritSec(const char *name = NULL) { create_critical_section(critSec, name); } //-V730
  ~WinCritSec() { destroy_critical_section(critSec); }
  void lock(const char *wpn = NULL) { enter_critical_section(critSec, wpn); }
  bool tryLock() { return try_enter_critical_section(critSec); }
  bool timedLock(int timeout_ms, const char *wpn = NULL) { return try_timed_enter_critical_section(critSec, timeout_ms, wpn); }
  void unlock() { leave_critical_section(critSec); }
  int fullUnlock() { return full_leave_critical_section(critSec); }
  void reLock(int cnt) { multi_enter_critical_section(critSec, cnt); }
};

//
// Auto-lock object;
//   locks a critical section, and unlocks it automatically when the lock goes out of scope
//
class WinAutoLock
{
private:
  // make copy constructor and assignment operator inaccessible
  WinAutoLock(const WinAutoLock &refAutoLock) = delete;
  WinAutoLock &operator=(const WinAutoLock &refAutoLock) = delete;

protected:
  CritSecStorage *pLock;

  // Unavailable, use WinAutoLockOpt if you need these
  WinAutoLock(CritSecStorage *pcss) : pLock(pcss) { lock(); }
  WinAutoLock(WinCritSec *pwcs) : WinAutoLock(&pwcs->critSec) {}

public:
  WinAutoLock(CritSecStorage &css) : WinAutoLock(&css) {}
  WinAutoLock(WinCritSec &wcs) : WinAutoLock(&wcs) {}
  ~WinAutoLock() { unlock(); }
  void lock(const char *wpn = NULL)
  {
    if (pLock)
      enter_critical_section(pLock, wpn);
  }
  void unlock()
  {
    if (pLock)
      leave_critical_section(pLock);
  }
  void unlockFinal()
  {
    unlock();
    pLock = NULL;
  }
};

class WinAutoLockOpt : public WinAutoLock
{
public:
  WinAutoLockOpt(WinCritSec *pwcs) : WinAutoLock(pwcs) {}
  WinAutoLockOpt(CritSecStorage *pcss) : WinAutoLock(pcss) {}
};

#include <supp/dag_undef_COREIMP.h>
