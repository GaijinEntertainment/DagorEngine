// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_threads.h>
#include <supp/_platform.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>
#include <debug/dag_logSys.h>
#include <startup/dag_globalSettings.h>
#include <supp/dag_cpuControl.h>
#include <stdlib.h>
#include <string.h>
#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
#include <pthread.h>
#include <sched.h>
#include <limits.h>
#define HAVE_PTHREAD
#else
#include <process.h>
inline void strerror_r(int e, char *b, size_t bsz) { strerror_s(b, bsz, e); }
#endif

#define GET_FLEXA_KB() (-1) // N/a
#if _TARGET_C1 | _TARGET_C2



#elif _TARGET_TVOS | _TARGET_IOS
#include <mach/thread_act.h>
#elif _TARGET_PC_LINUX | _TARGET_C3
#include <sys/prctl.h>
#elif _TARGET_ANDROID
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>
#elif _TARGET_PC_WIN
#include <malloc.h>
#endif

#include <util/engineInternals.h>
#include "threadPriorities.h"

#include <perfMon/dag_statDrv.h>

static DaThread *threads_list_head = NULL, *threads_list_tail = NULL;
static OSSpinlock threads_sl;

#if _TARGET_XBOX
#define HAVE_THREAD_NAMES 1
#else
#define HAVE_THREAD_NAMES (DAGOR_DBGLEVEL != 0)
#endif


#if (_TARGET_PC_WIN | _TARGET_XBOX) && HAVE_THREAD_NAMES
static void win_set_cur_thread_description(const char *tname)
{
  auto pSetThreadDescription =
#if _TARGET_XBOX
    &SetThreadDescription;
#else
#pragma warning(disable : 4191) // // 'type cast' : unsafe conversion from 'FARPROC'
    (decltype(&SetThreadDescription))GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription"); // Win10+
#endif
  if (pSetThreadDescription)
  {
    wchar_t tmp[32];
    size_t nameLenZ = mbstowcs(NULL, tname, 0) + 1;
    wchar_t *wname = nameLenZ <= countof(tmp) ? tmp : (wchar_t *)alloca(nameLenZ * sizeof(wchar_t));
    mbstowcs(wname, tname, nameLenZ);
    pSetThreadDescription(GetCurrentThread(), wname);
  }
#if !defined(SEH_DISABLED)
  else
  {
    // https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    struct ThreadNameInfo
    {
      DWORD dwType;     // must be 0x1000
      LPCSTR szName;    // pointer to name (in user addr space)
      DWORD dwThreadID; // thread ID (-1=caller thread)
      DWORD dwFlags;    // reserved for future use, must be zero
    } info{0x1000, tname, ~0u, 0};
#pragma warning(push)
#pragma warning(disable : 6320 6322)
    __try
    {
      RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {}
#pragma warning(pop)
  }
#endif
}
#endif

void push_thread_to_list(DaThread *t)
{
  threads_sl.lock();

  if (threads_list_tail)
  {
    threads_list_tail->nextThread = t;
    threads_list_tail = t;
  }
  else
  {
    G_ASSERT(!threads_list_head);
    threads_list_head = threads_list_tail = t;
  }

  threads_sl.unlock();
}

bool remove_thread_from_list(DaThread *t)
{
  threads_sl.lock();

  bool ret = false;
  for (DaThread *ti = threads_list_head, *tprev = NULL; ti; tprev = ti, ti = ti->nextThread)
  {
    if (ti == t)
    {
      if (tprev)
        tprev->nextThread = t->nextThread;
      if (t == threads_list_head)
        threads_list_head = t->nextThread;
      if (t == threads_list_tail)
        threads_list_tail = tprev;
      ret = true;
      break;
    }
  }

  threads_sl.unlock();

  return ret;
}

enum
{
  DEAD,    // before start or after reaping
  RUNNING, // after start
  ZOMBIE,  // done, waiting to be reaped
};

extern void update_float_exceptions();

void DaThread::doThread()
{
#if !(_TARGET_PC_WIN | _TARGET_XBOX)
  id = pthread_self();
#endif
  applyThreadPriority();
  setCurrentThreadName(name);
  uint64_t setAffinity = affinityMask;
#if _TARGET_C3 | _TARGET_TVOS | _TARGET_IOS
  if (!setAffinity)
  {
    // switch pins to core 0 by default.
    // for some reason ios was doing same without comments
    setAffinity = (1ull << cpujobs::get_core_count()) - 1;
  }
#endif
  applyThisThreadAffinity(setAffinity);

#if DAGOR_DBGLEVEL > 0 || _TARGET_PC
  update_float_exceptions();
#endif

  execute();

  afterThreadExecution();
}

void DaThread::afterThreadExecution()
{
  setCurrentThreadName(NULL);

  // set state only if thread is still alive (it may be already destroyed)
  threads_sl.lock();
  for (DaThread *t = threads_list_head; t; t = t->nextThread)
    if (t == this)
    {
      interlocked_compare_exchange(threadState, ZOMBIE, RUNNING);
      break;
    }
  threads_sl.unlock();
}

void DaThread::applyThreadPriority()
{
#if HAS_THREAD_PRIORITY // It is dangerous to set threads different priorities with many locks/mutex/critsections. It caused Priority
                        // Inversion, and can lead to freezes
#if HAS_THREAD_PRIORITY == PRIORITY_ONLY_HIGHER // We allow to have different priority, but only one step higher. That is resistent to
                                                // Priority Inversion.
  priority = min(max(priority, (int)cpujobs::DEFAULT_THREAD_PRIORITY), cpujobs::DEFAULT_THREAD_PRIORITY + 1);
#endif
  if (priority != 0)
    DaThread::applyThisThreadPriority(priority, name);
#endif
}
bool DaThread::applyThisThreadPriority(int priority, const char *name)
{
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_ANDROID // on android thread is lightweight process, and it need set priority as process.
  const int newPriority = cpujobs::DEFAULT_THREAD_PRIORITY - priority; // Lower value, high priority
                                                                       // https://man7.org/linux/man-pages/man2/setpriority.2.html on
                                                                       // most android range (-10, +10)
  if (setpriority(PRIO_PROCESS, 0, newPriority) != 0)
  {
    logwarn("failed to set thread priority (%d) <%s> error:%d", newPriority, name, errno);
    return false;
  }
#elif defined(HAVE_PTHREAD)
  int policy = 0;
  struct sched_param param;
  if (pthread_getschedparam(pthread_self(), &policy, &param) == 0)
  {
    param.sched_priority += priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
      logwarn("failed to set thread sched params (%d:%d) <%s>", policy, param.sched_priority, name);
  }
  else
  {
    logerr("failed to set thread sched params <%s>", name);
    return false;
  }
#elif _TARGET_PC_WIN | _TARGET_XBOX
  G_STATIC_ASSERT(THREAD_PRIORITY_NORMAL == 0);
  SetThreadPriority(GetCurrentThread(), priority);
#else
  return false;
#endif
  G_UNUSED(name);
  return true;
}


#if HAVE_THREAD_NAMES
static thread_local const char *thread_name_tls = nullptr;
/*static*/ const char *DaThread::getCurrentThreadName() { return thread_name_tls; }
#else
/*static*/ const char *DaThread::getCurrentThreadName() { return nullptr; }
#endif // HAVE_THREAD_NAMES
/*static*/
void DaThread::setCurrentThreadName(const char *tname)
{
  debug_set_thread_name(tname);

#if HAVE_THREAD_NAMES
  thread_name_tls = tname;
  if (!tname)
    return;

#if _TARGET_PC_WIN | _TARGET_XBOX
  win_set_cur_thread_description(tname);
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_LINUX | _TARGET_ANDROID
  prctl(PR_SET_NAME, tname);
#elif _TARGET_C3

#endif

#endif // HAVE_THREAD_NAMES
}

void DaThread::applyThisThreadAffinity(uint64_t affinity)
{
  if (!affinity)
    return;
#if HAVE_THREAD_NAMES
  const char *tname = thread_name_tls;
#else
  const char *tname = "thread";
#endif
  G_UNREFERENCED(tname);
#if _TARGET_PC_WIN || _TARGET_XBOX
  DWORD_PTR processAffinity, systemAffinity;
  if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity))
    affinity &= processAffinity;
  if (affinity)
    SetThreadAffinityMask(GetCurrentThread(), affinity);
#elif _TARGET_TVOS | _TARGET_IOS
  const int cores_mask = (1 << cpujobs::get_core_count()) - 1;
  const int cpuset = affinity & cores_mask;
  if (!affinity)
    return;
  thread_affinity_policy_data_t policy = {cpuset};
  thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
#elif _TARGET_C3












#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_ANDROID
  const int android_online_cores = sysconf(_SC_NPROCESSORS_ONLN);
  affinity &= (1ull << android_online_cores) - 1;
  if (!affinity)
    return;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (uint32_t i = 0; i < android_online_cores; ++i)
    if (affinity & (1ull << i))
      CPU_SET(i, &cpuset);
  if (sched_setaffinity(gettid(), sizeof(cpuset), &cpuset))
    debug("DaThread failed to set affinity 0x%04x for %s errno: %d", affinity, tname, errno);
#else
  // Not implemented.
  G_UNREFERENCED(affinity);
#endif
}

void DaThread::setThreadIdealProcessor(int ideal_processor_no)
{
#if _TARGET_PC_WIN
  SetThreadIdealProcessor((HANDLE)id, ideal_processor_no);
#elif _TARGET_XBOX
  PROCESSOR_NUMBER pnum = {};
  GetCurrentProcessorNumberEx(&pnum);
  pnum.Number = ideal_processor_no;
  SetThreadIdealProcessorEx((HANDLE)id, &pnum, NULL);
#else
  G_UNUSED(ideal_processor_no);
#endif
}

void DaThread::stripStackInMinidump()
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  minidumpSaveStack = false;
#endif
}

#if defined(HAVE_PTHREAD)
void *DaThread::threadEntry(void *arg)
{
  ((DaThread *)arg)->doThread();
  pthread_exit(0);
}
#elif _TARGET_PC_WIN | _TARGET_XBOX
unsigned __stdcall DaThread::threadEntry(void *arg)
{
  ((DaThread *)arg)->doThread();
  return 0;
}
#else
#error "DaThread is not ported on this platform yet"
#endif

DaThread::DaThread(const char *threadName, size_t stack_size, int prio, uint64_t affinity) :
  threadState(DEAD), nextThread(NULL), affinityMask(affinity)
{
  memset(&id, 255, sizeof(id));
#if _TARGET_64BIT
  stackSize = stack_size * 2;
#else
  stackSize = stack_size;
#endif
  priority = prio;

  if (!threadName)
    threadName = "DaThread";

  strncpy(name, threadName, MAX_NAME_LEN);
  name[MAX_NAME_LEN] = '\0';
}

DaThread::~DaThread() { terminate(false); }

bool DaThread::start()
{
  if (interlocked_compare_exchange(threadState, RUNNING, DEAD) != DEAD)
    return false;
  if (affinityMask == WORKER_THREADS_AFFINITY_MASK)
    affinityMask = WORKER_THREADS_AFFINITY_USE;

  nextThread = nullptr;
  push_thread_to_list(this);
  interlocked_release_store(terminating, 0);

  auto *strErr = +[](int e, char *b, size_t bsz) {
#if _TARGET_PC_LINUX
    return strerror_r(e, b, bsz);
#else
    strerror_r(e, b, bsz);
    return b;
#endif
  };
  G_UNUSED(strErr);

#if defined(HAVE_PTHREAD)

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stackSize);

#if _TARGET_C1 | _TARGET_C2

#endif

  int err = pthread_create(&id, &attr, threadEntry, this);

  pthread_attr_destroy(&attr);

#elif _TARGET_PC_WIN | _TARGET_XBOX

  unsigned thrIdent;
#if _TARGET_XBOX // Microsoft extensions for threading are obsolete and have been removed.
  id = (uintptr_t)CreateThread(NULL, stackSize, (LPTHREAD_START_ROUTINE)threadEntry, (void *)this, 0, (LPDWORD)&thrIdent);
  int err = id ? 0 : GetLastError();
  strErr = +[](int e, char *b, size_t bsz) {
    auto langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, e, langid, b, bsz, NULL);
    return b;
  };
#else
  id = _beginthreadex(NULL, (int)stackSize, threadEntry, (void *)this, STACK_SIZE_PARAM_IS_A_RESERVATION, &thrIdent);
  int err = id ? 0 : errno;
#endif

#else
#error "DaThread is not ported on this platform yet"
#endif

  if (DAGOR_UNLIKELY(err != 0)) // start wasn't successfull, rollback
  {
    interlocked_release_store(threadState, DEAD);
    remove_thread_from_list(this);
    char errMsgBuf[128];
    logerr("Failed to start thread <%s> stackSize/flexa=%d/%d KB with error %d: %s", name, stackSize >> 10, GET_FLEXA_KB(), err,
      strErr(err, errMsgBuf, sizeof(errMsgBuf)));
  }

  return err == 0;
}

void DaThread::terminate(bool wait, int timeout_ms, os_event_t *wake_event)
{
  if (interlocked_acquire_load(threadState) == DEAD)
    return;

#if defined(HAVE_PTHREAD)
  if (timeout_ms >= 0 || !wait)
  {
    int ret = pthread_detach(id);
    if (ret != 0)
      debug("thread_detach(%s) failed with %d", name, ret);
  }
#endif

  interlocked_release_store(terminating, 1);
  if (wake_event)
    os_event_set(wake_event);
#if _TARGET_PC_WIN | _TARGET_XBOX
  if (wait)
    WaitForSingleObject((HANDLE)id, timeout_ms < 0 ? INFINITE : timeout_ms);
  CloseHandle((HANDLE)id);
#elif defined(HAVE_PTHREAD)

  if (wait && timeout_ms != 0)
  {
    if (timeout_ms > 0)
    {
      int deadline_ms = get_time_msec() + timeout_ms;
      while (interlocked_acquire_load(threadState) == RUNNING && get_time_msec() < deadline_ms)
        sleep_msec(0);
      // timeout?
    }
    else
    {
      int ret = pthread_join(id, NULL);
      if (ret != 0)
        debug("thread_join(%s) failed with %d", name, ret);
    }
  }

#else
#error "DaThread is not ported on this platform yet"
#endif

  G_VERIFY(remove_thread_from_list(this));
  if (wait && timeout_ms < 0)
    G_ASSERT(threadState == ZOMBIE);
  memset(&id, 0xff, sizeof(id));
  interlocked_release_store(threadState, DEAD);
}

bool DaThread::isThreadStarted() const { return interlocked_acquire_load(threadState) != DEAD; }
bool DaThread::isThreadRunnning() const { return interlocked_acquire_load(threadState) == RUNNING; }

void DaThread::terminate_all(bool wait, int timeout_ms)
{
  if (wait)
  {
    threads_sl.lock();
    for (DaThread *t = threads_list_head; t; t = t->nextThread)
      interlocked_release_store(t->terminating, 1);
    threads_sl.unlock();

    bool found_unfinished = false;
    int start = get_time_msec_qpc();
    while (get_time_msec_qpc() - start < timeout_ms)
    {
      found_unfinished = false;

      threads_sl.lock();
      for (DaThread *t = threads_list_head; t; t = t->nextThread)
        if (t->threadState == RUNNING)
        {
          found_unfinished = true;
          break;
        }
      threads_sl.unlock();

      if (found_unfinished)
        sleep_msec(10);
      else
        break;
    }
  }

  // you can't do it threadsafely with spinlocks (due to lack of recursion)
  // hopefully this function is very special case which won't need true threadsafety
  while (threads_list_head)
    threads_list_head->terminate(false);
}

#if _TARGET_PC_WIN | _TARGET_XBOX
bool DaThread::isDaThreadWinUnsafe(uintptr_t thread_id, bool &minidump_save_stack)
{
  minidump_save_stack = false;
  for (DaThread *t = threads_list_head; t; t = t->nextThread)
  {
    if (GetThreadId((HANDLE)t->id) == thread_id)
    {
      minidump_save_stack = t->minidumpSaveStack;
      return true;
    }
  }
  return false;
}
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_threads
#include <supp/exportPull.h>
