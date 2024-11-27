// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_cpuJobsQueue.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_globDef.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math/dag_mathBase.h>
#include <math/dag_adjpow2.h>
#include <memory/dag_framemem.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/type_traits.h>
#include <atomic>
#include <condition_variable>

#if _TARGET_XBOX
#include <windows.h>
#pragma comment(lib, "Synchronization.lib")
#define HAVE_FUTEX true
#elif _TARGET_PC_WIN
#include <windows.h>
#define HAVE_FUTEX (threadpool::WaitOnAddress != NULL) // WaitOnAddress supported since Win8
#else
#define HAVE_FUTEX false
#define INFINITE   0
static inline void WaitOnAddress(...) {}
static inline void WakeByAddressSingle(...) {}
static inline void WakeByAddressAll(...) {}
#endif

#if _TARGET_C1

#endif

extern const size_t DEFAULT_FRAMEMEM_SIZE;

namespace threadpool
{
static thread_local int currentWorkerId = -1;

#if _TARGET_XBOXONE
#define PER_WORKER_MAX_PRIO 1
static uint8_t min_per_worker_prio = PRIO_HIGH;
#endif

#if _TARGET_PC_WIN // WaitOnAddress supported since Win8
static eastl::add_pointer_t<decltype(::WaitOnAddress)> WaitOnAddress;
static eastl::add_pointer_t<decltype(::WakeByAddressAll)> WakeByAddressAll;
static eastl::add_pointer_t<decltype(::WakeByAddressSingle)> WakeByAddressSingle;
static void init_futex_impl()
{
  if (HAVE_FUTEX)
    return;
  if (auto futexLib = LoadLibraryA("API-MS-Win-Core-Synch-l1-2-0.dll"))
  {
    reinterpret_cast<FARPROC &>(threadpool::WaitOnAddress) = GetProcAddress(futexLib, "WaitOnAddress");
    reinterpret_cast<FARPROC &>(threadpool::WakeByAddressAll) = GetProcAddress(futexLib, "WakeByAddressAll");
    reinterpret_cast<FARPROC &>(threadpool::WakeByAddressSingle) = GetProcAddress(futexLib, "WakeByAddressSingle");
  }
}
#else
static inline void init_futex_impl() {}
#endif

struct JobDoneEvent
{
  JobDoneEvent()
  {
    init_futex_impl();
    if (!HAVE_FUTEX)
    {
      os_event_create(&event, "jobDoneEvent");
    }
  }
  ~JobDoneEvent()
  {
    if (!HAVE_FUTEX)
      os_event_destroy(&event);
  }
  void done(volatile int &var)
  {
    if (HAVE_FUTEX)
    {
      WakeByAddressAll((void *)&var);
    }
    else
    {
      // probably better use conditional variables
      // in c++ 20 we can use std::atomic wait/wake as well
      os_event_set(&event);
    }
  }
  void wait(volatile int &var, intptr_t &spins)
  {
    while (!interlocked_acquire_load(var))
      if (--spins >= 0)
        cpu_yield();
      else
      {
        if (HAVE_FUTEX)
        {
          int value = 0;
          WaitOnAddress(&var, &value, sizeof(var), 1); // we don't care about spurious wake ups
        }
        else
        {
          os_event_wait(&event, 1); // To consider: we can use pointer indexed hash table of condvars (see e.g. msvc fallback impl of
                                    // atomic<>::wait())
        }
        spins = 0;
        break;
      }
  }

private:
  os_event_t event;
};

static struct TPCtx *tp_instance = NULL;
static char *tp_instance_memory = NULL;
static InitOnDemand<JobEnvironment, false> jenv_direct; // used if no workers allocated

static std::atomic<int> num_wakes;
#if _TARGET_IOS | _TARGET_ANDROID | _TARGET_C3 | /*steamdeck*/ _TARGET_PC_LINUX
static constexpr int BUSY_WAIT_ITERATIONS = 32;
#else
static constexpr int BUSY_WAIT_ITERATIONS = 128; // worker threads will iterate this amount of iterations polling the queue, before
                                                 // going to sleep if there is no job to do
#endif

struct JobQueueElem
{
  cpujobs::IJob *job = NULL;
  uint32_t index = 0;
  uint16_t count = 0;
  // barrierMode  - allow_all, allow_only_this_job

  JobQueueElem() {}
  JobQueueElem(cpujobs::IJob *j, uint32_t idx, uint32_t cnt) : job(j), index(idx), count(cnt) {}
};

typedef cpujobs::JobQueue<JobQueueElem, true> TPQueue;

static bool has_queue_items(TPQueue *pools, int lowest_prio = NUM_PRIO - 1) // return true if queue is not empty
{
  for (int prio = lowest_prio; prio >= PRIO_HIGH; --prio)
    if (pools[prio].size() > 0)
      return true;
  return false;
}

// return true if job is done
static __forceinline bool perform_queue_elem(const threadpool::JobQueueElem &jelem, JobEnvironment &jenv, JobDoneEvent &doneEvent)
{
  if (!jelem.job)
    return false;

  if (DAGOR_LIKELY(jelem.count == 0))
    jelem.job->doJob();
  else
  {
    threadpool::IJobNode *jn = (threadpool::IJobNode *)jelem.job;
    for (uint32_t i = jelem.index; i < (jelem.index + jelem.count); i++)
      jn->doJob(&jenv, ((uint8_t *)(jn->jobDataPtr)) + jn->jobDataSize * i, i);

    // decrement jobcount and set completion flag
    int oldCount = interlocked_add(jn->jobCount, -jelem.count) + jelem.count;
    if (oldCount != jelem.count)
      return true;
  }

  G_ASSERT(interlocked_acquire_load(jelem.job->done) == 0); // it must not be done in queue and must not change during job either
  interlocked_release_store(jelem.job->done, 1);
  doneEvent.done(jelem.job->done);

  return true;
}

static threadpool::JobQueueElem pop_job(TPQueue *pools, int prio_lowest, int prio = PRIO_HIGH)
{
  threadpool::JobQueueElem jelem;
  for (; prio <= prio_lowest && !jelem.job; ++prio)
    jelem = pools[prio].pop();
  return jelem;
}

// return true if job is done
static bool perform_job(TPQueue *pools, int prio_lowest, JobEnvironment &jenv, JobDoneEvent &doneEvent, int prio = PRIO_HIGH)
{
  return perform_queue_elem(pop_job(pools, prio_lowest, prio), jenv, doneEvent);
}

struct TPCtxBase
{
  TPQueue taskQueue[NUM_PRIO];
  std::mutex condVarMtx;
  std::condition_variable condVar;
  JobDoneEvent jobDoneEvent;
};

struct TPWorkerThread final : public DaThread
{
  TPCtxBase &tpctx; // To consider: we can alloc instance with e.g. 64K alignment and mask 'this' pointer instead
  uint8_t id;
#if PER_WORKER_MAX_PRIO
  const uint8_t maxPrio = PRIO_HIGH;
#else
  constexpr static uint8_t maxPrio = PRIO_HIGH;
#endif
  int nwakeslocal = 0; // intentionally stored in instance (as opposed to stack) to prevent threads snooping each other stacks

  JobEnvironment jenv;

  TPWorkerThread(); // not defined
  TPWorkerThread(const char *name, int id_, TPCtxBase &tpctx_, size_t stack_size, int thread_priority, uint8_t max_prio,
    uint64_t affinity) :
    DaThread(name, stack_size, thread_priority, affinity),
    tpctx(tpctx_),
    id(uint8_t(id_))
#if PER_WORKER_MAX_PRIO
    ,
    maxPrio(max_prio)
#endif
  {
    G_UNUSED(max_prio);
    G_FAST_ASSERT(max_prio < NUM_PRIO);
  }

  void execute() override
  {
#if _TARGET_C1



#elif _TARGET_XBOX
    // Disable thread priority boost (*) to disallow workers schedule each other out on wake ups
    // (*) https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts
    G_VERIFYF(SetThreadPriorityBoost(GetCurrentThread(), /*bDisablePriorityBoost*/ TRUE), "%d", GetLastError());
#endif
    TIME_PROFILE_THREAD(getCurrentThreadName());
    jenv.coreId = id;
    currentWorkerId = id;

    bool haveFutex = HAVE_FUTEX;
    RAIIThreadFramememAllocator framememAllocator(DEFAULT_FRAMEMEM_SIZE / 2);
    do
    {
      threadpool::JobQueueElem jelem;
      // To consider: evaluate actual rate of spurious wakeups on different systems/OSes
#if _TARGET_PC_WIN
      if (DAGOR_LIKELY(haveFutex)) // Note: win7 (which doesn't have it) market share is declining
#else
      if (haveFutex)
#endif
      {
        WaitOnAddress(&num_wakes, &nwakeslocal, sizeof(num_wakes), INFINITE);
        nwakeslocal = num_wakes.load(std::memory_order_relaxed);
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW, maxPrio);
      }
      else
      {
        std::unique_lock<std::mutex> lock(tpctx.condVarMtx);
        tpctx.condVar.wait(lock, [&, this]() { return num_wakes.load() != nwakeslocal; }); // Predicate prevents lost wake ups
        nwakeslocal = num_wakes.load(std::memory_order_relaxed);
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW, maxPrio); // Try to pop job under mutex since we already holding it at this point
      }

      if (interlocked_acquire_load(terminating))
        break;

      int busyIterations = BUSY_WAIT_ITERATIONS / (maxPrio + 1); // if worker woke up, there is(was) something to do.
      // Let's just-in case busy wait for few iterations, if new job appears, it won't have to wait for worker
      do // do while jobs exist
      {
        if (perform_queue_elem(jelem, jenv, tpctx.jobDoneEvent))
        {
          busyIterations = BUSY_WAIT_ITERATIONS / (maxPrio + 1);
        }
        else
        {
          if (--busyIterations <= 0)
            break;     // no more jobs in queue
          cpu_yield(); // just for HT
        }
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW, maxPrio);
      } while (1);

      reset_framemem();

    } while (!interlocked_acquire_load(terminating));
  }
};

struct TPCtx : public TPCtxBase
{
  const unsigned numWorkers;
#ifdef _MSC_VER
#pragma warning(disable : 4200) // non standard extention used (zero array)
#endif
  TPWorkerThread workers[0];

  TPCtx(int nw) : numWorkers(nw) { G_FAST_ASSERT(nw > 0); }

  static void on_queue_full(void *pThis) { ((TPCtx *)pThis)->wakeUpAll(); }

  uint32_t add(cpujobs::IJob *j, JobPriority prio)
  {
    threadpool::JobQueueElem jelem(j, 0, 0);
    return taskQueue[prio].push(jelem, &on_queue_full, this);
  }

  void addNode(threadpool::IJobNode *j, uint32_t num_jobs, uint32_t start_index, JobPriority prio)
  {
    j->jobCount = num_jobs;

    uint32_t scale = 1;
    const int targetSwitches = 4;
    if (num_jobs > numWorkers * targetSwitches * 2)
      scale = num_jobs / (numWorkers * targetSwitches);

    for (uint32_t i = 0; i < num_jobs; i += scale)
    {
      uint32_t remaining = min(num_jobs - i, scale);
      threadpool::JobQueueElem jelem(j, i + start_index, remaining);
      taskQueue[prio].push(jelem, &on_queue_full, this);
    }
  }

  void waitFlag(volatile int &var, int lowest_prio, uint32_t desc)
  {
    lock_start_t interval = 0;
    intptr_t spins = SPINS_BEFORE_SLEEP << int(!HAVE_FUTEX); // sleep more if we don't have futex impl (to avoid long 1ms event waits)
    if (lowest_prio < 0 || !has_queue_items(taskQueue, lowest_prio))
    {
      lock_start_t reft = dagor_lock_profiler_start();
      if (lowest_prio >= 0)
        spins /= 8; // This number reflects on how much we sure that "no work" state can be relied upon
      while (--spins >= 0)
      {
        if (interlocked_acquire_load(var) != 0)
          return;
        cpu_yield();
      }
      interval += dagor_lock_profiler_start() - reft;
    }
    do
    {
      wakeUpAll(); // this (at least first) wake is imporant - no wake up might be called at this point (so calling code might rely on
                   // it)
      if (lowest_prio >= 0 && perform_job(taskQueue, min(lowest_prio, (int)PRIO_LOW), *jenv_direct, jobDoneEvent))
        ;
      else
      {
        lock_start_t reft = dagor_lock_profiler_start();
        jobDoneEvent.wait(var, spins);
        interval += dagor_lock_profiler_start() - reft;
      }
    } while (interlocked_acquire_load(var) == 0);
    dagor_lock_profiler_interval(interval, desc ? desc : da_profiler::DescThreadPool);
  }

  inline void activeWaitBarrier(volatile int &var, JobPriority prio, uint32_t queue_pos, uint32_t desc)
  {
    // Note: assume that if we get here then `var` is likely 0
    while (taskQueue[(int)prio].getCurrentReadIndex() <= queue_pos) // We have not started performing our job yet
    {
      if (!perform_queue_elem(taskQueue[prio].pop(), *jenv_direct, jobDoneEvent))
        break; // Empty queue
      if (interlocked_acquire_load(var))
        return;
    }
    if (interlocked_acquire_load(var))
      return;
    ScopeLockProfiler<da_profiler::NoDesc> lp(desc ? desc : da_profiler::DescThreadPool);
    G_UNUSED(lp);
    intptr_t spins = SPINS_BEFORE_SLEEP << int(!HAVE_FUTEX);
    do
      jobDoneEvent.wait(var, spins);
    while (!interlocked_acquire_load(var));
  }

  void wakeUpAll()
  {
    TIME_PROFILE_DEV(tpool_wake_all);
    ++num_wakes;
    if (HAVE_FUTEX)
      WakeByAddressAll(&num_wakes);
    else
      condVar.notify_all();
  }

  void wakeUpOne()
  {
    TIME_PROFILE_DEV(tpool_wake_one);
    ++num_wakes;
    if (HAVE_FUTEX)
      WakeByAddressSingle(&num_wakes);
    else
      condVar.notify_one();
  }
};

void init_ex(int num_workers, int queue_sizes[NUM_PRIO], size_t stack_size, uint64_t max_workers_prio)
{
  G_UNUSED(max_workers_prio);
  if (tp_instance)
  {
    logwarn("Ignore dup attempt to init threadpool");
    G_ASSERTF(num_workers == tp_instance->numWorkers, "%d != %d", num_workers, tp_instance->numWorkers);
    return;
  }
  if (num_workers <= 0)
  {
    G_ASSERT(!tp_instance);
    return;
  }

  int total_queue_bytes = 0;
  for (int i = 0; i < NUM_PRIO; ++i)
    total_queue_bytes += queue_sizes[i] * sizeof(threadpool::JobQueueElem);

  num_wakes = 0;
  tp_instance_memory =
    (char *)memcalloc_default(sizeof(TPCtx) + num_workers * sizeof(TPWorkerThread) + alignof(TPCtx) + total_queue_bytes, 1); // guard
                                                                                                                             // page?

  tp_instance = (TPCtx *)(tp_instance_memory + (alignof(TPCtx) - uintptr_t(tp_instance_memory) % alignof(TPCtx)) % alignof(TPCtx));
  new (tp_instance, _NEW_INPLACE) TPCtx(num_workers);

  threadpool::JobQueueElem *q = (threadpool::JobQueueElem *)&tp_instance->workers[num_workers];
  for (int i = 0; i < NUM_PRIO; ++i)
  {
    tp_instance->taskQueue[i].initQueue(queue_sizes[i] ? q : NULL, queue_sizes[i]);
    q += queue_sizes[i];
  }

  jenv_direct.demandInit();
  init_futex_impl();

  char thread_name[32];
  // only boost priority if there is enough cores
  const int coreCount = cpujobs::get_core_count();
  const int workerThreadPriority = coreCount < 4 ? cpujobs::DEFAULT_THREAD_PRIORITY : cpujobs::DEFAULT_THREAD_PRIORITY + 1;
  for (int i = 0; i < num_workers; ++i, max_workers_prio >>= 2)
  {
    SNPRINTF(thread_name, sizeof(thread_name), "TPWorker #%d", i);
    uint8_t wrkPrio = max_workers_prio & 3;
#if PER_WORKER_MAX_PRIO
    min_per_worker_prio = max(min_per_worker_prio, wrkPrio);
#endif
    new (&tp_instance->workers[i], _NEW_INPLACE)
      TPWorkerThread(thread_name, i, *tp_instance, stack_size, workerThreadPriority, wrkPrio, WORKER_THREADS_AFFINITY_MASK);
    tp_instance->workers[i].start();
  }

#if _TARGET_XBOXONE
  G_UNUSED(coreCount);
  for (int workerNo = 0; workerNo < num_workers; workerNo++)
  {
    if (workerNo < 4) // #5th core is shared with fmod, #6th core is shared (time sliced) with OS, so allow it to run on any core
                      // except main one
      tp_instance->workers[workerNo].setThreadIdealProcessor(workerNo + 1);
  }
#elif _TARGET_C3 | _TARGET_C1 | _TARGET_ANDROID | _TARGET_IOS
  G_UNUSED(coreCount);
#elif _TARGET_PC_WIN | _TARGET_C2
  int idealCore = 0;
  for (int workerNo = 0; workerNo < num_workers; workerNo++)
  {
    tp_instance->workers[workerNo].setThreadIdealProcessor(idealCore); // Just a hint for OS to not run workers on one core.
                                                                       // scheduling it out.

    idealCore += 2; // Skip HT threads.
    while (((1ull << idealCore) & WORKER_THREADS_AFFINITY_MASK) == 0 && idealCore < coreCount)
      idealCore += 2;

    if (idealCore >= coreCount)
      idealCore = 1; // Use HT threads.
  }
#endif
}

void shutdown()
{
  if (!tp_instance)
    return;

  for (int i = 0; i < tp_instance->numWorkers; ++i)
    interlocked_release_store(tp_instance->workers[i].terminating, 1);
  tp_instance->wakeUpAll();
  for (int i = 0; i < tp_instance->numWorkers; ++i)
    tp_instance->workers[i].terminate(/*wait*/ true);
  for (int i = 0; i < tp_instance->numWorkers; ++i)
    tp_instance->workers[i].~TPWorkerThread();
  tp_instance->~TPCtx();

  jenv_direct.demandDestroy();

  memfree(tp_instance_memory, defaultmem);
  tp_instance = NULL;
  tp_instance_memory = nullptr;
  num_wakes = 0;
}

int get_num_workers() { return tp_instance ? tp_instance->numWorkers : 0; }

int get_queue_size(JobPriority prio) { return tp_instance ? (tp_instance->taskQueue[prio].queueSizeMinusOne + 1) : 0; }

JobEnvironment *get_job_env(int worker_index) { return tp_instance ? &tp_instance->workers[worker_index].jenv : NULL; }

// will actively perform jobs if there are any in queue earlier than target. Allows implementing simple barrier
void barrier_active_wait_for_job_ool(cpujobs::IJob *j, JobPriority prio, uint32_t queue_pos, uint32_t profile_wait_token)
{
  tp_instance->activeWaitBarrier(j->done, prio, queue_pos, profile_wait_token);
}

void wait_ool(cpujobs::IJob *j, uint32_t profile_token, int lowest_prio_to_perform)
{
  tp_instance->waitFlag(j->done, lowest_prio_to_perform, profile_token);
}

bool add(cpujobs::IJob *j, JobPriority prio, uint32_t &queue_pos, AddFlags flags)
{
  G_ASSERT(j);

  if ((flags & AddFlags::IgnoreNotDone) == AddFlags::None)
  {
    if (tp_instance)
      wait(j);
    else
      G_ASSERT(j->done); // when no worker threads exists job must always be "done"
  }

  interlocked_release_store(j->done, 0);

  if (tp_instance && tp_instance->taskQueue[prio].queue)
  {
    queue_pos = tp_instance->add(j, prio);
    if ((flags & AddFlags::WakeOnAdd) != AddFlags::None)
    {
#if PER_WORKER_MAX_PRIO
      if (prio < min_per_worker_prio) // Note: can't wake only one since it might be the one that serves only low prio jobs
        tp_instance->wakeUpAll();
      else
#endif
        tp_instance->wakeUpOne();
    }
  }
  else
  {
    j->doJob();
    j->done = 1;
  }

  return true;
}

void wake_up_all()
{
  if (tp_instance)
    tp_instance->wakeUpAll();
}

void wake_up_one()
{
  if (tp_instance)
    tp_instance->wakeUpOne();
}

void wake_up_all_delayed(JobPriority prio, bool wake)
{
  static struct WakeUpAllJob final : public cpujobs::IJob
  {
    void doJob() override { wake_up_all(); }
  } wake_up_all_job;
  add(&wake_up_all_job, prio, wake);
}

bool add_node(threadpool::IJobNode *j, uint32_t start_index, uint32_t num_jobs, JobPriority prio, bool wake_up)
{
  G_ASSERT(j);

  if (tp_instance)
    wait(j);
  else if (num_jobs == 0)
    return false;

  interlocked_release_store(j->done, 0);

  if (tp_instance && tp_instance->taskQueue[prio].queue)
  {
    tp_instance->addNode(j, max(num_jobs, 1U), start_index, prio);
    if (wake_up)
      tp_instance->wakeUpOne();
  }
  else
  {
    for (uint32_t i = start_index; i < (start_index + num_jobs); i++)
      j->doJob(jenv_direct.get(), ((uint8_t *)(j->jobDataPtr)) + j->jobDataSize * i, i);
    j->jobCount = 0;
    j->done = 1;
  }

  return true;
}
int get_current_worker_id() { return currentWorkerId; }
} // namespace threadpool

#define EXPORT_PULL dll_pull_baseutil_threadPool
#include <supp/exportPull.h>
