// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_cpuJobsQueue.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_addressWait.h>
#if _TARGET_PC_WIN || _TARGET_XBOX
#include <windows.h> // SetThreadPriorityBoost
#endif
#include <osApiWrappers/dag_lockProfiler.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_globDef.h>
#include <util/dag_finally.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math/dag_mathBase.h>
#include <math/dag_adjpow2.h>
#include <memory/dag_framemem.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/memory.h>
#include <EASTL/type_traits.h>
#include <condition_variable>

#if _TARGET_PC_WIN
#define HAVE_FUTEX has_futex_impl // for Win7 compat
#define HAVE_FUTEX_CONSTEXPR
#else
#define HAVE_FUTEX           true
#define HAVE_FUTEX_CONSTEXPR constexpr
#endif

#if _TARGET_XBOX || _TARGET_C2
#define HAVE_NATIVE_FUTEX           true
#define HAVE_NATIVE_FUTEX_CONSTEXPR constexpr
#elif _TARGET_PC_WIN
#define HAVE_NATIVE_FUTEX has_futex_impl
#define HAVE_NATIVE_FUTEX_CONSTEXPR
#else // no native futex/WaitOnAddress; os_wait_on_address uses cv-bucket fallback
#define HAVE_NATIVE_FUTEX           false
#define HAVE_NATIVE_FUTEX_CONSTEXPR constexpr
#endif

#if _TARGET_C1

#endif

namespace threadpool
{
static thread_local int currentWorkerId = -1;

#if _TARGET_PC_WIN
static bool has_futex_impl = false;
#endif

struct JobDoneEvent
{
  JobDoneEvent()
  {
    if HAVE_FUTEX_CONSTEXPR (!HAVE_FUTEX)
    {
      os_event_create(&event, "jobDoneEvent");
    }
  }
  ~JobDoneEvent()
  {
    if HAVE_FUTEX_CONSTEXPR (!HAVE_FUTEX)
      os_event_destroy(&event);
  }
  void done([[maybe_unused]] volatile unsigned &var)
  {
    if HAVE_FUTEX_CONSTEXPR (HAVE_FUTEX)
    {
      os_wake_on_address_all(&var);
    }
    else
    {
      // probably better use conditional variables
      // in c++ 20 we can use std::atomic wait/wake as well
      os_event_set(&event);
    }
  }
  void wait([[maybe_unused]] volatile unsigned &var, intptr_t &spins)
  {
    while (!interlocked_acquire_load(var))
      if (--spins >= 0)
        cpu_yield();
      else
      {
        if HAVE_FUTEX_CONSTEXPR (HAVE_FUTEX)
        {
          unsigned notDone = 0;
          os_wait_on_address(&var, &notDone); // Intentionally (default) inf timeout for portability
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

static struct NumWakes
{
  // Align to avoid false sharing with other globals (notably `tp_instance`)
  alignas(128) volatile unsigned value;
} num_wakes;

// Worker threads will iterate this amount of iterations polling the queue, before going to sleep if there is no job to do
#if _TARGET_IOS | _TARGET_ANDROID | _TARGET_C3 | _TARGET_PC_LINUX // Linux for SteamDeck
static constexpr int BUSY_WAIT_ITERATIONS = 32;
#else
static constexpr int BUSY_WAIT_ITERATIONS = 128;
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

#if TIME_PROFILER_ENABLED
  DA_PROFILE_EVENT_DESC(jelem.job->getJobNameProfDesc());
#endif
  if (DAGOR_LIKELY(jelem.count == 0))
    jelem.job->doJob();
  else
  {
    threadpool::IJobNode *jn = static_cast<threadpool::IJobNode *>(jelem.job);
    for (uint32_t i = jelem.index; i < (jelem.index + jelem.count); i++)
      jn->doNodeJob(&jenv, ((uint8_t *)(jn->jobDataPtr)) + jn->jobDataSize * i, i);

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

static threadpool::JobQueueElem pop_job(TPQueue *pools, int prio_lowest)
{
  threadpool::JobQueueElem jelem;
  for (int prio = PRIO_HIGH; prio <= prio_lowest && !jelem.job; ++prio)
    jelem = pools[prio].pop();
  return jelem;
}

// return true if job is done
static bool perform_job(TPQueue *pools, int prio_lowest, JobEnvironment &jenv, JobDoneEvent &doneEvent)
{
  return perform_queue_elem(pop_job(pools, prio_lowest), jenv, doneEvent);
}

EA_DISABLE_VC_WARNING(4582) // ctor is not implicitly called
struct TPCtxBase
{
  TPQueue taskQueue[NUM_PRIO];
  union
  {
    alignas(std::mutex) char condVarMtxStor[sizeof(std::mutex)];
    std::mutex condVarMtx;
  };
  union
  {
    alignas(std::condition_variable) char condVarStor[sizeof(std::condition_variable)];
    std::condition_variable condVar;
  };
  JobDoneEvent jobDoneEvent;
  TPCtxBase()
  {
    if HAVE_NATIVE_FUTEX_CONSTEXPR (!HAVE_NATIVE_FUTEX)
    {
      new (&condVarMtxStor, _NEW_INPLACE) std::mutex;
      new (&condVarStor, _NEW_INPLACE) std::condition_variable;
    }
  }
  ~TPCtxBase()
  {
    if HAVE_NATIVE_FUTEX_CONSTEXPR (!HAVE_NATIVE_FUTEX)
    {
      eastl::destroy_at(&condVar);
      eastl::destroy_at(&condVarMtx);
    }
  }
};
EA_RESTORE_VC_WARNING()

struct TPWorkerThread final : public DaThread
{
  TPCtxBase &tpctx; // To consider: we can alloc instance with e.g. 64K alignment and mask 'this' pointer instead
  uint8_t id;
  uint32_t nwakeslocal = 0; // intentionally stored in instance (as opposed to stack) to prevent threads snooping each other stacks

  JobEnvironment jenv;

  TPWorkerThread(); // not defined
  TPWorkerThread(const char *name, int id_, TPCtxBase &tpctx_, size_t stack_size, int thread_priority, uint64_t affinity) :
    DaThread(name, stack_size, thread_priority, affinity), tpctx(tpctx_), id(uint8_t(id_))
  {}

  void execute() override
  {
#if _TARGET_C1



#elif _TARGET_XBOX
    // Disable thread priority boost (*) to disallow workers schedule each other out on wake ups
    // (*) https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts
    G_VERIFYF(SetThreadPriorityBoost(GetCurrentThread(), /*bDisablePriorityBoost*/ TRUE), "%d", GetLastError());
#elif _TARGET_PC_WIN // Use cached in locals fn pointer to reduce possible CPU cache interference with `num_wakes` globals
    auto waitOnAddr = os_get_native_wait_on_address_impl();
    auto os_wait_on_address = [&](volatile uint32_t *a, const uint32_t *ca) { waitOnAddr(a, ca, (int)INFINITE); };
    G_FAST_ASSERT(!waitOnAddr == !HAVE_NATIVE_FUTEX);
#endif
    TIME_PROFILE_THREAD(getCurrentThreadName());
    jenv.coreId = id;
    currentWorkerId = id;

    if (auto ptmem = get_thread_framemem()) //-V547
#if _TARGET_STATIC_LIB
      alloc_threadpool_framemem();
#else
      *ptmem = nullptr; // Alloc on demand
#endif
    FINALLY([] { free_thread_framemem(); });

    HAVE_NATIVE_FUTEX_CONSTEXPR bool haveNativeFutex = HAVE_NATIVE_FUTEX;
    do
    {
      threadpool::JobQueueElem jelem;
      if HAVE_NATIVE_FUTEX_CONSTEXPR (haveNativeFutex)
      {
        if (auto tmpWakes = interlocked_acquire_load(num_wakes.value); tmpWakes == nwakeslocal)
        {
          os_wait_on_address(&num_wakes.value, &nwakeslocal);
          nwakeslocal = interlocked_acquire_load(num_wakes.value);
        }
        else
          nwakeslocal = tmpWakes;
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW);
      }
      else
      {
        std::unique_lock<std::mutex> lock(tpctx.condVarMtx);
        // Predicate reduces chances of lost wake ups
        tpctx.condVar.wait(lock, [&, this]() { return interlocked_acquire_load(num_wakes.value) != nwakeslocal; });
        nwakeslocal = interlocked_acquire_load(num_wakes.value);
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW); // Try to pop job under mutex since we already holding it at this point
      }

      if (interlocked_acquire_load(terminating))
        break;

      int busyIterations = BUSY_WAIT_ITERATIONS; // if worker woke up, there is(was) something to do.
      // Let's just-in case busy wait for few iterations, if new job appears, it won't have to wait for worker
      do // do while jobs exist
      {
        if (perform_queue_elem(jelem, jenv, tpctx.jobDoneEvent))
          busyIterations = BUSY_WAIT_ITERATIONS;
        else
        {
          if (--busyIterations <= 0)
            break;     // no more jobs in queue
          cpu_yield(); // just for HT
        }
        jelem = pop_job(tpctx.taskQueue, PRIO_LOW);
      } while (1);

      reset_framemem();

    } while (!interlocked_acquire_load(terminating));
  } // execute()
};

struct TPCtx : public TPCtxBase
{
  const unsigned numWorkers;
#ifdef _MSC_VER
#pragma warning(disable : 4200) // non standard extention used (zero array)
#endif
  TPWorkerThread workers[0];

  TPCtx(int nw) : numWorkers(nw) { G_FAST_ASSERT(nw > 0); }

  void onQueueFull(JobPriority prio)
  {
    wakeUpAll();
    if (currentWorkerId >= 0)
    {
      // the worker is stuck adding new jobs, let's finish queued ones
      perform_queue_elem(taskQueue[prio].pop(), *jenv_direct, jobDoneEvent);
    }
  }

  using QueueFullArgs = eastl::pair<TPCtx *, JobPriority>;
  static void on_queue_full(void *pArgs)
  {
    auto args = (QueueFullArgs *)pArgs;
    args->first->onQueueFull(args->second);
  }

  uint32_t add(cpujobs::IJob *j, JobPriority prio)
  {
    threadpool::JobQueueElem jelem(j, 0, 0);
    auto args = QueueFullArgs(this, prio);
    return taskQueue[prio].push(jelem, &on_queue_full, &args);
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
      auto args = QueueFullArgs(this, prio);
      taskQueue[prio].push(jelem, &on_queue_full, &args);
    }
  }

  void waitFlag(volatile unsigned &var, int lowest_prio, uint32_t desc)
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
      // this (at least first) wake is imporant - no wake up might be called at this point (so calling code might rely on it)
      wakeUpOne();

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

  inline void activeWaitBarrier(volatile unsigned &var, JobPriority prio, uint32_t queue_pos, uint32_t desc)
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
    interlocked_increment(num_wakes.value);
    if HAVE_NATIVE_FUTEX_CONSTEXPR (HAVE_NATIVE_FUTEX)
      os_wake_on_address_all(&num_wakes.value);
    else
      condVar.notify_all();
  }

  void wakeUpOne()
  {
    TIME_PROFILE_DEV(tpool_wake_one);
    interlocked_increment(num_wakes.value);
    if HAVE_NATIVE_FUTEX_CONSTEXPR (HAVE_NATIVE_FUTEX)
      os_wake_on_address_one(&num_wakes.value);
    else
      condVar.notify_one();
  }
};

void init_ex(int num_workers, int queue_sizes[NUM_PRIO], size_t stack_size)
{
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

  num_wakes.value = 0;
#if _TARGET_PC_WIN
  has_futex_impl = (bool)os_get_native_wait_on_address_impl();
#endif

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

  char thread_name[32];
  // only boost priority if there is enough cores
  const int coreCount = cpujobs::get_core_count();
  const int workerThreadPriority = coreCount < 4 ? cpujobs::DEFAULT_THREAD_PRIORITY : cpujobs::DEFAULT_THREAD_PRIORITY + 1;
#if _TARGET_C1

#else
  uint64_t waff = WORKER_THREADS_AFFINITY_MASK;
#endif
  for (int i = 0; i < num_workers; ++i)
  {
    SNPRINTF(thread_name, sizeof(thread_name), "TPWorker #%d", i);
    new (&tp_instance->workers[i], _NEW_INPLACE) TPWorkerThread(thread_name, i, *tp_instance, stack_size, workerThreadPriority, waff);
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

  spin_wait([] {
    tp_instance->wakeUpAll();
    if HAVE_NATIVE_FUTEX_CONSTEXPR (!HAVE_NATIVE_FUTEX) // NOTE: wakeUpAll on condVar path can lose a wake
      for (int i = 0; i < tp_instance->numWorkers; ++i)
        if (tp_instance->workers[i].isThreadRunnning())
          return true;
    return false;
  });

  for (int i = 0; i < tp_instance->numWorkers; ++i)
    tp_instance->workers[i].terminate(/*wait*/ true);
  for (int i = 0; i < tp_instance->numWorkers; ++i)
    tp_instance->workers[i].~TPWorkerThread();
  tp_instance->~TPCtx();

  jenv_direct.demandDestroy();

  memfree(tp_instance_memory, defaultmem);
  tp_instance = NULL;
  tp_instance_memory = nullptr;
  num_wakes.value = 0;
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
      tp_instance->wakeUpOne();
  }
  else
  {
    j->doJob();
    if (tp_instance)
    {
      interlocked_release_store(j->done, 1);
      tp_instance->jobDoneEvent.done(j->done);
    }
    else
    {
      j->done = 1;
    }
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
    const char *getJobName(bool &) const override { return "WakeUpAllJob"; }
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
      tp_instance->wakeUpAll();
  }
  else
  {
    for (uint32_t i = start_index; i < (start_index + num_jobs); i++)
      j->doNodeJob(jenv_direct.get(), ((uint8_t *)(j->jobDataPtr)) + j->jobDataSize * i, i);
    j->jobCount = 0;
    if (tp_instance)
    {
      interlocked_release_store(j->done, 1);
      tp_instance->jobDoneEvent.done(j->done);
    }
    else
    {
      j->done = 1;
    }
  }

  return true;
}
int get_current_worker_id() { return currentWorkerId; }
} // namespace threadpool

#define EXPORT_PULL dll_pull_baseutil_threadPool
#include <supp/exportPull.h>
