// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_globDef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <osApiWrappers/dag_events.h>
#include <debug/dag_debug.h>
#include <supp/dag_cpuControl.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/bitset.h>
#if _TARGET_APPLE
#include <mach/thread_act.h>
#include <sys/sysctl.h>
#include <unistd.h>
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <process.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#define GET_FLEXA_KB() (-1) // N/a
#if _TARGET_PC_LINUX
#include <sys/prctl.h>
#elif _TARGET_ANDROID
#include <sys/prctl.h>
#include <unistd.h>
#elif _TARGET_C1 | _TARGET_C2




#elif _TARGET_C3

#endif

#ifdef INVALID_HANDLE_VALUE
#elif defined(PTHREAD_NULL)
#define INVALID_HANDLE_VALUE PTHREAD_NULL
#else
#define INVALID_HANDLE_VALUE pthread_t()
#endif

#define DEBUG_JOB_LEV 0

#if DEBUG_JOB_LEV > 1
#include <osApiWrappers/dag_dbgStr.h>
#define LOGMSG0 out_debug_str_fmt
#define LOGMSG1 out_debug_str_fmt

#elif DEBUG_JOB_LEV > 0
#include <osApiWrappers/dag_dbgStr.h>
#define LOGMSG0 out_debug_str_fmt
inline void LOGMSG1(const char *, ...) {}

#else
inline void LOGMSG0(const char *, ...) {}
inline void LOGMSG1(const char *, ...) {}

#endif


#if _TARGET_PC
#define MAX_VIRT_JOB_MGR_COUNT 88
#else
#define MAX_VIRT_JOB_MGR_COUNT 32
#endif

#include "threadPriorities.h"

void set_current_cpujobs_thread_name(const char *tname) { DaThread::setCurrentThreadName(tname); }

#if _TARGET_PC_WIN
static unsigned __stdcall job_mgr_thread(void *p); // _beginthreadex
#elif _TARGET_XBOX
static DWORD __stdcall job_mgr_thread(void *p);
#else
static void *__cdecl job_mgr_thread(void *p);
#endif

namespace cpujobs
{
struct JobQueueList
{
  IJob *volatile first;
  IJob *last;

  IJob *getFirst() { return interlocked_acquire_load_ptr(first); }

  void reset() { first = last = NULL; }

  void addJobInternal(IJob *j)
  {
    if (!first)
    {
      last = j;
      interlocked_release_store_ptr(first, j);
    }
    else
    {
      last->next = j;
      last = j;
    }
  }

  void add(IJob *j)
  {
    j->next = NULL;
    addJobInternal(j);
  }
  void addJobList(IJob *j)
  {
    addJobInternal(j);
    while (last->next && last->next != last)
      last = last->next;
  }

  void prepend(IJob *j)
  {
    if (!first)
    {
      j->next = NULL;
      last = j;
      interlocked_release_store_ptr(first, j);
    }
    else
    {
      j->next = first->next;
      first->next = j;
      if (!j->next)
        last = j;
    }
  }

  // Returns job if we need add this job to doneJobs queue;
  // otherwise return nullptr
  IJob *advance(IJob *&next)
  {
    IJob *j = first;
    next = first->next;
    j->next = NULL;
    // NOTE: if needRelease is false, worker thread can't read/write 'IJob *j'
    // after removing it from queue (store_ptr to 'first').
    // Because main thread can deallocate 'j' right after removing it from queue
    if (!j->needRelease)
      j = nullptr;
    interlocked_release_store_ptr(first, next);
    if (!next)
      last = NULL;
    return j;
  }

  void resetButCurrent(JobQueueList &done)
  {
    if (first && first->next)
    {
      IJob *j = first->next;
      first->next = NULL;
      last = first;
      done.addJobList(j);
    }
  }

  int removeByTag(JobQueueList &done, unsigned tag)
  {
    int jobsRemovedCount = 0;
    if (first && first->next)
    {
      IJob *j = first;
      IJob *n = j->next;
      while (n)
      {
        if (n->getJobTag() == tag)
        {
          j->next = n->next;
          n->next = NULL;
          done.add(n);
          n = j->next;
          jobsRemovedCount++;
        }
        else
        {
          j = n;
          n = j->next;
        }
        if (!n)
          last = j;
      }
    }
    return jobsRemovedCount;
  }

  bool isInQueue(IJob *j)
  {
    IJob *p = first;
    while (p)
      if (p == j)
        return true;
      else
        p = p->next;
    return false;
  }

  static void releaseJobList(IJob *j)
  {
    while (j)
    {
      IJob *next = j->next;
      j->next = NULL;
      j->releaseJob();
      j = next;
    }
  }
};

static JobQueueList globDoneJobs;
volatile bool abortJobsRequested = false;
enum
{
  ZOMBIE = -1,
  NOT_USED = 0,
  USED,
  THREAD_STARTED,
  EXIT_REQUESTED,
  ABORT_REQUESTED
};

struct JobMgrCtx // Note: zero inited
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  static constexpr int THREAD_NAME_LEN = 32;
#else
  static constexpr int THREAD_NAME_LEN = 16;
#endif
  JobQueueList job, doneJobs;
  volatile int state;
  int stackSz;
  int idleTime; // in msec
  int prio;
  uint64_t affinity;
  os_event_t cmdEvent;
#if _TARGET_PC_WIN | _TARGET_XBOX
  HANDLE tId, handle;
#else
  pthread_t tId, handle;
#endif
  CritSecStorage critSec;

  char threadName[THREAD_NAME_LEN];
  const char *getName() const { return threadName; }

  void setEvent() { os_event_set(&cmdEvent); }
  int waitEvent() { return os_event_wait(&cmdEvent, idleTime ? idleTime : OS_WAIT_INFINITE); }

  void lock() { ::enter_critical_section(critSec); }
  void unlock() { ::leave_critical_section(critSec); }

  bool isUsed() const
  {
    int js = interlocked_acquire_load(state);
    return js == THREAD_STARTED || js == USED;
  }
  bool isBusy() const { return interlocked_acquire_load(state) >= USED; }

  void detach()
  {
    if (handle != INVALID_HANDLE_VALUE)
#if _TARGET_PC_WIN | _TARGET_XBOX
      G_VERIFY(CloseHandle(eastl::exchange(handle, INVALID_HANDLE_VALUE)));
#else
      G_VERIFY(pthread_detach(eastl::exchange(handle, INVALID_HANDLE_VALUE)) == 0);
#endif
  }

  void init(int stk_sz, const char *name, int jmidx, uint64_t affinity_, int prio_, int idle_time_sec = 0)
  {
    if (name)
      strncpy(threadName, name, sizeof(threadName) - 1);
    else
      _snprintf(threadName, sizeof(threadName) - 1, "JobMgr#%d", jmidx);
    threadName[sizeof(threadName) - 1] = '\0';

    ::create_critical_section(critSec, "jobcrit");
    job.reset();
    doneJobs.reset();
    stackSz = stk_sz;
    affinity = affinity_ == WORKER_THREADS_AFFINITY_MASK ? WORKER_THREADS_AFFINITY_USE : affinity_;
    prio = prio_;
    idleTime = idle_time_sec * 1000;
    tId = handle = INVALID_HANDLE_VALUE;
    os_event_create(&cmdEvent);
    interlocked_release_store(state, USED);
  }
  int startThread() // Returns 0 on success or errorcode otherwise
  {
    detach(); // Detach previous in case of non first start

#if _TARGET_PC_WIN | _TARGET_XBOX
    // To consider: start paused and change prio/affinity and then resume
    tId = 0;     // Zero high bits
#if _TARGET_XBOX // Microsoft extensions for threading are obsolete and have been removed.
    handle = CreateThread(NULL, stackSz, job_mgr_thread, this, 0, (DWORD *)&tId);
#else
    handle = (HANDLE)_beginthreadex(NULL, stackSz, job_mgr_thread, this, STACK_SIZE_PARAM_IS_A_RESERVATION, (unsigned *)&tId);
#endif
    if (DAGOR_UNLIKELY(!handle))
    {
      tId = INVALID_HANDLE_VALUE;
      return GetLastError();
    }
#if HAS_THREAD_PRIORITY // It is dangerous to set threads different priorities with many locks/mutex/critsections. It caused Priority
                        // Inversion, and can lead to freezes
#if HAS_THREAD_PRIORITY == PRIORITY_ONLY_HIGHER // We allow to have different priority, but only one step higher. That is resistent to
    // Priority Inversion.
    int prio = min(max(this->prio, (int)DEFAULT_THREAD_PRIORITY), DEFAULT_THREAD_PRIORITY + 1);
#endif
    if (prio != THREAD_PRIORITY_NORMAL)
      if (!::SetThreadPriority(handle, prio))
        logwarn("%s SetThreadPriority failed with error %d", __FUNCTION__, GetLastError());
#endif
#else

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stackSz);

#if _TARGET_C1

#endif

    int ret = pthread_create(&thread, &attr, job_mgr_thread, this);

    pthread_attr_destroy(&attr);

    if (DAGOR_UNLIKELY(ret))
      return ret;

    handle = tId = thread;

#if HAS_THREAD_PRIORITY // It is dangerous to set threads different priorities with many locks/mutex/critsections. It caused Priority
                        // Inversion, and can lead to freezes
#if HAS_THREAD_PRIORITY == PRIORITY_ONLY_HIGHER // We allow to have different priority, but only one step higher. That is resistent to
                                                // Priority Inversion.
    int prio = min(max(this->prio, (int)DEFAULT_THREAD_PRIORITY), DEFAULT_THREAD_PRIORITY + 1);
#endif
    if (prio != DEFAULT_THREAD_PRIORITY)
    {
      int policy = 0;
      struct sched_param param;
      if (pthread_getschedparam(handle, &policy, &param) == 0)
      {
        param.sched_priority += prio - DEFAULT_THREAD_PRIORITY;
        if (pthread_setschedparam(handle, policy, &param) != 0)
          logerr("failed to set thread sched params (%d:%d) for jobMgr M%s>", policy, param.sched_priority, threadName);
      }
      else
        logerr("failed to set thread sched params for jobMgr <%s>", threadName);
    }
#endif

#endif
    return 0;
  }
  void term(IJob *&jout)
  {
    ::destroy_critical_section(critSec);
    os_event_destroy(&cmdEvent);
    if (IJob *j = doneJobs.getFirst())
    {
      globDoneJobs.addJobList(j);
      if (!jout)
        jout = j;
    }
    doneJobs.reset();
    detach();
    interlocked_release_store(state, NOT_USED);
  }
  void addJob(IJob *j, bool prepend, bool need_release)
  {
    lock();

    G_ASSERT(!job.isInQueue(j));
    G_ASSERT(!doneJobs.isInQueue(j));

    j->needRelease = need_release;
    !prepend ? job.add(j) : job.prepend(j);

    int threadStarted = 0;
    if (DAGOR_UNLIKELY(interlocked_relaxed_load(state) == USED))
      if (interlocked_compare_exchange(state, THREAD_STARTED, USED) == USED)
      {
        threadStarted = startThread();
        if (DAGOR_UNLIKELY(threadStarted))
          interlocked_release_store(state, USED);
      }

    unlock();

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
    if (DAGOR_UNLIKELY(threadStarted != 0))
    {
      char errMsgBuf[128], *errMsg = errMsgBuf;
#if _TARGET_PC_WIN | _TARGET_XBOX
      auto fmf = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
      auto langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
      FormatMessageA(fmf, NULL, threadStarted, langid, errMsgBuf, sizeof(errMsgBuf), NULL);
#elif _TARGET_PC_LINUX
      errMsg = strerror_r(threadStarted, errMsgBuf, sizeof(errMsgBuf));
#else
      strerror_r(threadStarted, errMsgBuf, sizeof(errMsgBuf));
#endif
      logerr("Failed to start thread for jobMgr <%s> stackSize/flexa=%d/%d KB with error %d: %s", threadName, stackSz >> 10,
        GET_FLEXA_KB(), threadStarted, errMsg);
    }
#endif
  }
  IJob *nextJob()
  {
    lock();
    IJob *next;
    IJob *grabbedTask = job.advance(next);
    if (grabbedTask)
      doneJobs.add(grabbedTask);
    unlock();
    return next;
  }
  void resetJobs()
  {
    lock();
    job.resetButCurrent(doneJobs);
    unlock();
  }
  int removeJobs(unsigned tag)
  {
    lock();
    const int jobsRemovedCount = job.removeByTag(doneJobs, tag);
    unlock();
    return jobsRemovedCount;
  }
  void releaseDoneQueue()
  {
    lock();
    // Use interlocked even under lock to avoid technical race on checking `first` ptr in `release_done_jobs`
    IJob *j = interlocked_exchange_ptr(doneJobs.first, (IJob *)nullptr);
    if (!j)
      return unlock();
    doneJobs.last = nullptr; // Reset
    unlock();
    JobQueueList::releaseJobList(j);
  }
};

static struct CpuJobsData //-V730
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  int numLogicalCores = 0;
  int numPhysicalCores = 0;
  int numLogicalCoresPerLLC = 0;
#elif _TARGET_C1 | _TARGET_C2



#elif _TARGET_C3





#else
  union
  {
    int numLogicalCores = 0;
    int numPhysicalCores;
  };
#endif
  uint16_t ctxCount = 0;
  uint16_t firstVirtJobMgrId;
  volatile int maxVirtJobMgrId = -1;
  CritSecStorage globCritSec;
#if CONSTEXPR_CORE_COUNT
  JobMgrCtx ctx[numLogicalCores + MAX_VIRT_JOB_MGR_COUNT];
#else
  JobMgrCtx *ctx; // array of ctxCount dim
#endif
  void lock() { ::enter_critical_section(&globCritSec); }
  void unlock() { ::leave_critical_section(&globCritSec); }
  bool isInited() const { return ctxCount != 0; }
  void reset() { memset(this, 0, offsetof(CpuJobsData, globCritSec)); }
} cpujobs_data;
} // namespace cpujobs

extern void update_float_exceptions();

#if _TARGET_PC_WIN
static unsigned __stdcall job_mgr_thread(void *p) // _beginthreadex
#elif _TARGET_XBOX
static DWORD __stdcall job_mgr_thread(void *p)
#else
static void *__cdecl job_mgr_thread(void *p)
#endif
{
  using namespace cpujobs;

  JobMgrCtx &ctx = *(JobMgrCtx *)p;

  set_current_cpujobs_thread_name(ctx.getName());
  uint64_t setAffinity = ctx.affinity;
#if _TARGET_TVOS | _TARGET_IOS | _TARGET_C3
  if (!setAffinity)
    setAffinity = (1ull << cpujobs::get_core_count()) - 1;
#endif
  DaThread::applyThisThreadAffinity(setAffinity);

#if DAGOR_DBGLEVEL > 0 || _TARGET_PC
  update_float_exceptions();
#endif

  bool timeouted = false;
  for (;;) // infinite cycle with explicit break
  {
    if (DAGOR_UNLIKELY(ctx.waitEvent() == OS_WAIT_TIMEOUTED))
    {
      ctx.lock();
      if (!ctx.job.getFirst() && interlocked_compare_exchange(ctx.state, USED, THREAD_STARTED) == THREAD_STARTED)
      {
        ctx.tId = INVALID_HANDLE_VALUE;
        ctx.unlock();
        timeouted = true;
        break;
      }
      ctx.unlock();
    }

    int curState = interlocked_acquire_load(ctx.state);
    if (curState == ABORT_REQUESTED || (curState == EXIT_REQUESTED && !ctx.job.getFirst()))
      break;

    IJob *job = ctx.job.getFirst();
    while (job)
    {
      job->doJob();
      LOGMSG1("cpuobjs: job %p on %d done\n", job, &ctx - &cpujobs_data.ctx[0]);
      job = ctx.nextJob();

      if (interlocked_acquire_load(ctx.state) == ABORT_REQUESTED)
        break;
    }

    curState = interlocked_acquire_load(ctx.state);
    if (curState == ABORT_REQUESTED || (curState == EXIT_REQUESTED && !ctx.job.getFirst()))
      break;
  }

  if (!timeouted)
    interlocked_release_store(ctx.state, ZOMBIE); // waiting to be reaped

  LOGMSG0("cpuobjs: job mgr %d thread exited\n", &ctx - &cpujobs_data.ctx[0]);

  return 0;
}

int cpujobs::is_inited() { return cpujobs_data.isInited(); }

void cpujobs::init(int force_core_count, bool reserve_jobmgr_cores)
{
  G_ASSERT_RETURN(!cpujobs_data.isInited(), );

#ifndef CONSTEXPR_CORE_COUNT
  if (force_core_count <= 0)
  {
#if _TARGET_PC_WIN | _TARGET_XBOX
    DWORD bufLen = 0;
    G_VERIFY(!GetLogicalProcessorInformation(NULL, &bufLen) && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *sysInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)alloca(bufLen);
    G_VERIFYF(GetLogicalProcessorInformation(sysInfo, &bufLen), "%d", GetLastError());
    int numCores = 0, numProcs = 0;
    size_t maxCoresPerCacheL[4] = {0, 0, 0, 0};
    for (int i = 0, n = bufLen / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i < n; ++i)
    {
      const SYSTEM_LOGICAL_PROCESSOR_INFORMATION &inf = sysInfo[i];
      switch (inf.Relationship)
      {
        case RelationCache:
        {
          int cli = inf.Cache.Level - 1;
          G_FAST_ASSERT(cli >= 0 && cli < countof(maxCoresPerCacheL));
          maxCoresPerCacheL[cli] = eastl::max(maxCoresPerCacheL[cli], eastl::bitset<64>(inf.ProcessorMask).count());
        }
        break;

        case RelationProcessorCore:
        {
          numCores++;
          numProcs += eastl::bitset<64>(inf.ProcessorMask).count();
        }
        break;

        default: continue;
      }
    }
    int llcIndex = 0;
    for (; llcIndex < countof(maxCoresPerCacheL) && maxCoresPerCacheL[llcIndex]; ++llcIndex)
      ;
    cpujobs_data.numLogicalCores = numProcs;
    cpujobs_data.numPhysicalCores = numCores;
    cpujobs_data.numLogicalCoresPerLLC = maxCoresPerCacheL[llcIndex - 1];
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_MACOSX | _TARGET_PC_LINUX | _TARGET_ANDROID
    cpujobs_data.numLogicalCores = sysconf(_SC_NPROCESSORS_ONLN);
#elif _TARGET_TVOS | _TARGET_IOS
    cpujobs_data.numLogicalCores = 1;
    uint32_t sysctl_value;
    size_t length = sizeof(sysctl_value);
    /*if (!sysctlbyname("hw.physicalcpu_max", &sysctl_value, &length, NULL, 0)) {
      physical_cpus_ = static_cast<int>(sysctl_value);
    }*/
    length = sizeof(sysctl_value);
    if (!sysctlbyname("hw.logicalcpu_max", &sysctl_value, &length, NULL, 0))
      cpujobs_data.numLogicalCores = static_cast<int>(sysctl_value);
#else
#error : unsupported target!
#endif
  }
  else // force_core_count > 0
  {
    cpujobs_data.numLogicalCores = force_core_count;
  }
#else
  G_UNUSED(force_core_count);
#endif

  cpujobs_data.firstVirtJobMgrId = reserve_jobmgr_cores ? cpujobs_data.numLogicalCores : 0;

  ::create_critical_section(cpujobs_data.globCritSec, "cpujobs");
  cpujobs_data.ctxCount = cpujobs_data.numLogicalCores + MAX_VIRT_JOB_MGR_COUNT;
#if CONSTEXPR_CORE_COUNT
  G_ASSERT(cpujobs_data.ctxCount == countof(cpujobs_data.ctx));
#else
  cpujobs_data.ctx = (JobMgrCtx *)calloc(cpujobs_data.ctxCount, sizeof(JobMgrCtx));
#endif
  if (INVALID_HANDLE_VALUE != decltype(cpujobs_data.ctx[0].handle)())
    for (int i = 0; i < cpujobs_data.ctxCount; ++i)
      cpujobs_data.ctx[i].handle = INVALID_HANDLE_VALUE; //-V522

  LOGMSG0("cpuobjs inited: %d Cores\n", cpujobs_data.numLogicalCores);
}

void cpujobs::term(bool cancel_jobs, int timeout_msec)
{
  if (!cpujobs_data.isInited())
    return;

  for (int i = 0; i < cpujobs_data.firstVirtJobMgrId; i++)
    if (cpujobs_data.ctx[i].isUsed())
      stop_job_manager(i, !cancel_jobs);

  for (int i = cpujobs_data.firstVirtJobMgrId; i < cpujobs_data.ctxCount; i++)
    if (cpujobs_data.ctx[i].isUsed())
      destroy_virtual_job_manager(i, !cancel_jobs);

  bool all_done = true;
  do
  {
    all_done = true;
    for (int i = 0; i < cpujobs_data.ctxCount; i++)
      if (cpujobs_data.ctx[i].isBusy())
        all_done = false;
      else if (auto &th = cpujobs_data.ctx[i].handle; th != INVALID_HANDLE_VALUE)
      {
        // Wait to ensure that TLSes are actually destroyed
#if _TARGET_PC_WIN | _TARGET_XBOX
        G_VERIFY(WaitForSingleObject(th, INFINITE) == WAIT_OBJECT_0);
        G_VERIFY(CloseHandle(eastl::exchange(th, INVALID_HANDLE_VALUE)));
#else
        G_VERIFY(pthread_join(eastl::exchange(th, INVALID_HANDLE_VALUE), NULL) == 0);
#endif
      }

    if (all_done || timeout_msec-- <= 0)
      break;

    abortJobsRequested = true;
    sleep_msec(1);
  } while (1);

  if (!cancel_jobs)
  {
    G_ASSERT(all_done);
  }
  release_done_jobs();

  if (all_done)
  {
    ::destroy_critical_section(cpujobs_data.globCritSec);
#ifndef CONSTEXPR_CORE_COUNT
    free(cpujobs_data.ctx);
    cpujobs_data.ctx = NULL;
#endif
    cpujobs_data.reset();
  }

  LOGMSG0("cpuobjs terminated\n");
}

int cpujobs::get_logical_core_count()
{
#ifndef CONSTEXPR_CORE_COUNT
  G_ASSERT(cpujobs_data.isInited());
#endif
  return cpujobs_data.numLogicalCores;
}

int cpujobs::get_physical_core_count()
{
#ifndef CONSTEXPR_CORE_COUNT
  G_ASSERT(cpujobs_data.isInited());
#endif
  return cpujobs_data.numPhysicalCores;
}

int cpujobs::get_core_count_per_llc()
{
  G_ASSERT(cpujobs_data.isInited());
#if _TARGET_PC_WIN | _TARGET_XBOX
  return cpujobs_data.numLogicalCoresPerLLC;
#else
  return 1;
#endif
}

bool cpujobs::start_job_manager(int core_id, int stk_sz, const char *threadName, int thread_priority)
{
  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(core_id >= 0 && core_id < cpujobs_data.numLogicalCores);
  G_ASSERT(cpujobs_data.firstVirtJobMgrId > 0); // init wasn't called with reserve_jobmgr_cores = true?

#if _TARGET_64BIT
  stk_sz *= 2;
#endif

  cpujobs_data.lock();
  JobMgrCtx &ctx = cpujobs_data.ctx[core_id];
  int jstate = interlocked_acquire_load(ctx.state);
  if (jstate != NOT_USED)
  {
    bool ret = jstate >= USED && stk_sz <= ctx.stackSz;
    cpujobs_data.unlock();
    return ret;
  }

  LOGMSG0("cpuobjs: start job mgr %d, stk_sz=%d\n", core_id, stk_sz);
  ctx.init(stk_sz, threadName, core_id, (uint64_t)1 << core_id, thread_priority);
  interlocked_release_store(cpujobs_data.maxVirtJobMgrId, max(cpujobs_data.maxVirtJobMgrId, core_id));

  interlocked_release_store(ctx.state, THREAD_STARTED);
  int threadStarted = ctx.startThread();
  if (DAGOR_UNLIKELY(threadStarted))
    interlocked_release_store(ctx.state, USED);

  cpujobs_data.unlock();

  if (DAGOR_UNLIKELY(threadStarted))
    logerr("Failed to start thread for jobMgr <%s> stackSize=%dK with error %d", threadName, stk_sz >> 10, threadStarted);

  return threadStarted == 0;
}

bool cpujobs::stop_job_manager(int core_id, bool finish_jobs)
{
  G_ASSERT(cpujobs_data.isInited());

  cpujobs_data.lock();
  JobMgrCtx &ctx = cpujobs_data.ctx[core_id];
  int jstate = interlocked_acquire_load(ctx.state);
  if (jstate < USED)
  {
    cpujobs_data.unlock();
    return false;
  }

  if (jstate == THREAD_STARTED)
    jstate = interlocked_compare_exchange(ctx.state, finish_jobs ? EXIT_REQUESTED : ABORT_REQUESTED, THREAD_STARTED);
  // Thread not started or just exited
  if (jstate == USED && (jstate = interlocked_compare_exchange(ctx.state, ZOMBIE, USED)) == USED)
    ; // zombie transition successed
  else if (jstate == EXIT_REQUESTED && !finish_jobs)
    interlocked_compare_exchange(ctx.state, ABORT_REQUESTED, EXIT_REQUESTED);

  ctx.setEvent();
  cpujobs_data.unlock();
  LOGMSG0("cpuobjs: request stop job mgr %d\n", core_id);
  return true;
}


int cpujobs::create_virtual_job_manager(int stk_sz, uint64_t affinity_mask, const char *thread_name, int thread_priority,
  int idle_time_sec)
{
  G_ASSERT(cpujobs_data.isInited());
  cpujobs_data.lock();

  int vmgr_id = -1;
  for (int i = cpujobs_data.firstVirtJobMgrId; i < cpujobs_data.ctxCount; i++)
    if (interlocked_acquire_load(cpujobs_data.ctx[i].state) == NOT_USED)
    {
      vmgr_id = i;
      break;
    }
  if (vmgr_id == -1)
  {
    LOGMSG0("cpuobjs: no unused managers left\n");
    cpujobs_data.unlock();
    return -1;
  }

  JobMgrCtx &ctx = cpujobs_data.ctx[vmgr_id];

#if _TARGET_64BIT
  stk_sz *= 2;
#endif
  LOGMSG0("cpuobjs: start job mgr %d, stk_sz=%d, aff=%08X:%08X\n", vmgr_id, stk_sz, affinity_mask);

  ctx.init(stk_sz, thread_name, vmgr_id, affinity_mask, thread_priority, idle_time_sec);
  interlocked_release_store(cpujobs_data.maxVirtJobMgrId, max(cpujobs_data.maxVirtJobMgrId, vmgr_id));
  cpujobs_data.unlock();
  return vmgr_id;
}

void cpujobs::destroy_virtual_job_manager(int vmgr_id, bool finish_jobs)
{
  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(vmgr_id >= cpujobs_data.firstVirtJobMgrId && vmgr_id < cpujobs_data.ctxCount);
  stop_job_manager(vmgr_id, finish_jobs);
}


bool cpujobs::add_job(int core_or_vmgr_id, cpujobs::IJob *job, bool prepend, bool need_release)
{
  G_ASSERT(job != NULL);
  if (core_or_vmgr_id == COREID_IMMEDIATE)
  {
    job->doJob();
    job->releaseJob();
    return true;
  }

  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);

  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  cpujobs_data.lock();
  if (!ctx.isUsed())
  {
    cpujobs_data.unlock();
    return false;
  }

  if (!need_release) // wait until this job is done
  {
    cpujobs_data.unlock();
    bool in_queue;
    do
    {
      cpujobs_data.lock();
      ctx.lock();

      in_queue = ctx.job.isInQueue(job);

      ctx.unlock();
      cpujobs_data.unlock();

      if (in_queue)
        sleep_msec(0);
      else
        break;
    } while (1);
    cpujobs_data.lock();
  }

  ctx.addJob(job, prepend, need_release);
  cpujobs_data.unlock();
  ctx.setEvent();
  LOGMSG1("cpuobjs: add job %p to %d\n", job, core_or_vmgr_id);
  return true;
}

void cpujobs::reset_job_queue(int core_or_vmgr_id, bool auto_release_jobs)
{
  G_ASSERT(cpujobs_data.isInited());
  if (core_or_vmgr_id == COREID_IMMEDIATE)
    return;
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);

  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  cpujobs_data.lock();
  if (!ctx.isUsed())
  {
    cpujobs_data.unlock();
    return;
  }
  ctx.resetJobs();

  if (auto_release_jobs)
    ctx.releaseDoneQueue();
  cpujobs_data.unlock();
  LOGMSG1("cpuobjs: reset job queue on %d\n", core_or_vmgr_id);
}

int cpujobs::remove_jobs_by_tag(int core_or_vmgr_id, unsigned tag, bool auto_release_jobs)
{
  G_ASSERT(cpujobs_data.isInited());
  if (core_or_vmgr_id == COREID_IMMEDIATE)
    return 0;
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);

  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  cpujobs_data.lock();
  if (!ctx.isUsed())
  {
    cpujobs_data.unlock();
    return 0;
  }
  const int jobsRemovedCount = ctx.removeJobs(tag);

  if (auto_release_jobs)
    ctx.releaseDoneQueue();
  cpujobs_data.unlock();
  LOGMSG1("cpuobjs: remove job queue on %d, tag=%f\n", core_or_vmgr_id, tag);
  return jobsRemovedCount;
}

bool cpujobs::is_job_manager_busy(int core_or_vmgr_id)
{
  G_ASSERT(cpujobs_data.isInited());
  if (core_or_vmgr_id == COREID_IMMEDIATE)
    return false;
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);
  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  return ctx.isBusy() && ctx.job.getFirst();
}

bool cpujobs::is_job_running(int core_or_vmgr_id, IJob *job)
{
  G_ASSERT(cpujobs_data.isInited());
  if (core_or_vmgr_id == COREID_IMMEDIATE)
    return false;
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);

  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  bool found = false;
  cpujobs_data.lock();
  if (ctx.isBusy() && ctx.job.getFirst())
  {
    ctx.lock();
    found = ctx.job.isInQueue(job);
    ctx.unlock();
  }
  cpujobs_data.unlock();
  return found;
}

bool cpujobs::is_job_done(int core_or_vmgr_id, IJob *job)
{
  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);

  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  bool found = false;
  cpujobs_data.lock();
  if (ctx.isBusy() && ctx.doneJobs.getFirst())
  {
    ctx.lock();
    found = ctx.doneJobs.isInQueue(job);
    ctx.unlock();
  }
  cpujobs_data.unlock();
  return found;
}

int64_t cpujobs::get_job_manager_thread_id(int vmgr_id)
{
  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(vmgr_id >= 0 && vmgr_id < cpujobs_data.ctxCount);

  // Maybe it is not exactly correct to just cast into int64_t, but we do same in get_current_thread_id()
  return (int64_t)cpujobs_data.ctx[vmgr_id].tId;
}

bool cpujobs::is_in_job()
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  HANDLE thread_id = (HANDLE)GetCurrentThreadId();
#else
  pthread_t thread_id = pthread_self();
#endif

  for (int i = 0; i < cpujobs_data.ctxCount; i++)
#if _TARGET_PC_WIN | _TARGET_XBOX
    if (cpujobs_data.ctx[i].isBusy() && thread_id == cpujobs_data.ctx[i].tId)
#else
    if (cpujobs_data.ctx[i].isBusy() && pthread_equal(thread_id, cpujobs_data.ctx[i].tId))
#endif
      return true;
  return false;
}

bool cpujobs::is_exit_requested(int core_or_vmgr_id)
{
  G_ASSERT(cpujobs_data.isInited());
  G_ASSERT(core_or_vmgr_id >= 0 && core_or_vmgr_id < cpujobs_data.ctxCount);
  JobMgrCtx &ctx = cpujobs_data.ctx[core_or_vmgr_id];
  int jstate = interlocked_acquire_load(ctx.state);
  return jstate >= EXIT_REQUESTED || jstate == ZOMBIE;
}


void cpujobs::release_done_jobs()
{
  for (int i = 0, n = interlocked_acquire_load(cpujobs_data.maxVirtJobMgrId); i <= n; i++)
  {
    int jstate = interlocked_acquire_load(cpujobs_data.ctx[i].state);
    if (jstate == USED || jstate == THREAD_STARTED)
    {
      if (!cpujobs_data.ctx[i].doneJobs.getFirst())
        continue;
    }
    else if (jstate != ZOMBIE)
      continue;

#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
    DA_PROFILE_EVENT("release_done_cpujobs")
#endif

    IJob *globDoneJobsFirst = nullptr;
    do
    {
      if (jstate == USED || jstate == THREAD_STARTED)
      {
        if (cpujobs_data.ctx[i].doneJobs.getFirst())
          cpujobs_data.ctx[i].releaseDoneQueue();
      }
      else if (jstate == ZOMBIE)
      {
        cpujobs_data.lock();
        cpujobs_data.ctx[i].term(globDoneJobsFirst);
        cpujobs_data.unlock();
      }
      if (++i <= n)
        jstate = interlocked_acquire_load(cpujobs_data.ctx[i].state);
      else
        break;
    } while (1);

    if (globDoneJobsFirst)
    {
      globDoneJobs.reset(); // Note: doesn't need to be locked since addition to it done in this function (in JobMgrCtx::term())
      JobQueueList::releaseJobList(globDoneJobsFirst);
    }

    break;
  }
}

#define EXPORT_PULL dll_pull_osapiwrappers_cpuJobs
#include <supp/exportPull.h>
