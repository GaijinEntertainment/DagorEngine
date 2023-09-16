#include <osApiWrappers/dag_critSec.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <string.h>
#include "critsec.h"

void create_critical_section(void *p, const char *name)
{
  csimpl::type *cc = csimpl::get(p);

  G_STATIC_ASSERT(sizeof(csimpl::type) <= CRITICAL_SECTION_OBJECT_SIZE);
#if HAVE_WINAPI
  // By default, CRITICAL_SECTIONs are initialized with a spin count of 0. MSDN says the Windows heap manager uses a spin count of
  // 4000, but Joe Duffy's "Concurrent Programming on Windows" book suggests 1500 is a "more reasonable starting point". On
  // single-processor systems, the spin count is ignored and the critical section spin count is set to 0.
  constexpr int CS_SPIN_COUNT = 1500;
#if _TARGET_64BIT
  int initFlags = 0;
#if DAGOR_DBGLEVEL == 0 && _TARGET_PC_WIN
  initFlags = CRITICAL_SECTION_NO_DEBUG_INFO; // this flag is required in order to fully clean-up critical sections on destroy
#endif
  if (InitializeCriticalSectionEx(cc, CS_SPIN_COUNT, initFlags))
    return;
  fatal("InitializeCriticalSectionEx failed with %d", ::GetLastError());
#else // winxp compat
  InitializeCriticalSectionAndSpinCount(cc, CS_SPIN_COUNT);
#endif
  (void)name;
#else

#if _TARGET_C1 | _TARGET_C2

#else
  (void)name;
  pthread_mutexattr_t cca;
  pthread_mutexattr_init(&cca);
  // pthread_mutexattr_setprotocol(&cca, SYS_SYNC_PRIORITY);
  pthread_mutexattr_settype(&cca, PTHREAD_MUTEX_RECURSIVE);
  int ret = pthread_mutex_init(cc, &cca);
#endif
  csimpl::lockcount::release(p);
#if DAGOR_DBGLEVEL > 0
  if (ret != 0)
    fatal_x("pthread_mutex_create failed: 0x%08X", ret);
#endif
#endif
}

void destroy_critical_section(void *p)
{
  csimpl::type *cc = csimpl::get(p);
  if (!cc)
    return;
#if HAVE_WINAPI

  DeleteCriticalSection(cc);
  memset(cc, 0, sizeof(CRITICAL_SECTION));
#else

#if _TARGET_C1 | _TARGET_C2

#else
  int ret = pthread_mutex_destroy(cc);
#endif
#if DAGOR_DBGLEVEL > 0
  if (ret != 0)
    fatal_x("pthread_mutex_destroy failed: 0x%08X\n", ret);
#endif

  memset(cc, 0, sizeof(pthread_mutex_t));
  csimpl::lockcount::release(p);
#endif
}

void enter_critical_section_raw(void *p)
{
  csimpl::type *cc = csimpl::get(p);
#if HAVE_WINAPI
  G_ANALYSIS_ASSUME(cc != NULL);
  EnterCriticalSection(cc);
#else
  G_ASSERT(cc && "critical section is NULL!");

#if _TARGET_C1 | _TARGET_C2

#else
  pthread_mutex_lock(cc);
#endif
  csimpl::lockcount::increment(p);
#endif
}

void leave_critical_section(void *p)
{
  csimpl::type *cc = csimpl::get(p);
#if HAVE_WINAPI
  G_ANALYSIS_ASSUME(cc != NULL);
  LeaveCriticalSection(cc);
#else
  G_ASSERT(cc && "critical section is NULL!");
  csimpl::lockcount::decrement(p);
#if _TARGET_C1 | _TARGET_C2

#else
  pthread_mutex_unlock(cc);
#endif
#endif
}

bool try_enter_critical_section(void *p)
{
  csimpl::type *cc = csimpl::get(p);
#if HAVE_WINAPI
  G_ANALYSIS_ASSUME(cc != NULL);

  return TryEnterCriticalSection(cc) != FALSE;
#else
  G_ASSERT(cc && "critical section is NULL!");

#if _TARGET_C1 | _TARGET_C2

#else
  int result = pthread_mutex_trylock(cc);
#endif

  if (result == 0)
    csimpl::lockcount::increment(p);
  return result == 0;
#endif
}

int full_leave_critical_section(void *p)
{
  csimpl::type *cc = csimpl::get(p);
  G_ASSERT(cc && "critical section is NULL!");
#if HAVE_WINAPI

  // do not access cc->RecursionCount after last LeaveCriticalSection
  // calling LeaveCriticalSection for not owned critical session leads to deadlock on EnterCriticalSection
  int cnt = cc->RecursionCount;
  for (int i = 0; i < cnt; i++)
    LeaveCriticalSection(cc);
#else
  int cnt = csimpl::lockcount::value(p);
  for (int i = 0; i < cnt; i++)
  {
    csimpl::lockcount::decrement(p);
#if _TARGET_C1 | _TARGET_C2

#else
    pthread_mutex_unlock(cc);
#endif
  }
#endif
  return cnt;
}

void multi_enter_critical_section(void *p, int enter_cnt)
{
  csimpl::type *cc = csimpl::get(p);
  G_ASSERT(cc && "critical section is NULL!");
  G_ASSERT(enter_cnt > 0);
#if HAVE_WINAPI

  for (; enter_cnt; enter_cnt--)
    EnterCriticalSection(cc);
#else
  for (; enter_cnt; enter_cnt--)
  {
#if _TARGET_C1 | _TARGET_C2

#else
    pthread_mutex_lock(cc);
#endif
    csimpl::lockcount::increment(p);
  }
#endif
}

void enter_critical_section(void *p, const char *)
{
  ScopeLockProfiler<da_profiler::DescCritsec> lp;
  G_UNUSED(lp);
  enter_critical_section_raw(p);
}

#define EXPORT_PULL dll_pull_osapiwrappers_critsec
#include <supp/exportPull.h>
