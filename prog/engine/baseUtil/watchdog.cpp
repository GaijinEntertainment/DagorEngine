// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_watchdog.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_fatal.h>
#include <util/dag_console.h>
#include <util/dag_stdint.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/fixed_vector.h>
#if _TARGET_PC_WIN
#include <windows.h>
#include <MMSystem.h>
#include <Psapi.h>
#include <time.h>
#include <stdio.h>
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_LINUX
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#if _TARGET_PC
#define WATCHDOG_FREEZE_THRESHOLD         (20000)
#define WATCHDOG_DUMP_CALLSTACK_THRESHOLD (5000)
#else
#define WATCHDOG_FREEZE_THRESHOLD         (7000)
#define WATCHDOG_DUMP_CALLSTACK_THRESHOLD (WATCHDOG_FREEZE_THRESHOLD / 2)
#endif
#define WATCHDOG_SLEEP_TIME_MS         (500)
#define WATCHDOG_ACTIVE_SWAP_THRESHOLD (1000)
// Assume that OS is resumed from sleep/hibernation if more then this time passed
#define WATCHDOG_OS_RESUMED_THRESHOLD  (10 * 60 * 1000)

static inline unsigned get_cur_time_ms()
{
#if _TARGET_PC_WIN
  return timeGetTime(); // QPC (which used for implementation of get_time_msec()) is known to be buggy on some hardware
#else
  return get_time_msec();
#endif
}

#if _TARGET_PC_WIN
#pragma comment(lib, "psapi.lib")
#endif

void on_freeze_detected_default(unsigned e_time, int64_t)
{
  // Note: intentionally not logerr to avoid trigerring various error hooks (as it might have various time-sensitive side effects)
  logwarn("Watchdog: freeze detected, timeout %u msec", e_time);

#if _TARGET_APPLE && DAGOR_DBGLEVEL > 0
  __builtin_trap();
#else
  DAG_FATAL("Freeze detected");
#endif
}

class WatchDogThread final : public DaThread, public IWatchDog // freezes detector
{
public:
  WatchdogConfig cfg;
  dag::AtomicInteger<unsigned int> lastUpdateTimeMs;
  os_event_t sleepEvent;
  eastl::fixed_vector<intptr_t, 4> dumpThreadIds;
  OSSpinlock dumpThreadIdMutex;

  WatchDogThread(WatchdogConfig *cfg_) : DaThread("WatchDogThread", 128 << 10, 0, WORKER_THREADS_AFFINITY_MASK), lastUpdateTimeMs(0u)
  {
    os_event_create(&sleepEvent);
    if (cfg_)
      cfg = *cfg_;

    // default values
    if (!cfg.triggering_threshold_ms)
      cfg.triggering_threshold_ms = WATCHDOG_FREEZE_THRESHOLD;
    if (!cfg.dump_threads_threshold_ms)
      cfg.dump_threads_threshold_ms = WATCHDOG_DUMP_CALLSTACK_THRESHOLD;
    if (!cfg.sleep_time_ms)
      cfg.sleep_time_ms = WATCHDOG_SLEEP_TIME_MS;
    dumpThreadIds.emplace_back(get_main_thread_id());

    kick();
  }

  ~WatchDogThread() override
  {
    terminate(true, -1, &sleepEvent);
    os_event_destroy(&sleepEvent); // -V779
  }

  intptr_t setOption(int option, intptr_t p0, intptr_t p1) override
  {
    switch (option)
    {
      case WATCHDOG_OPTION_TRIG_THRESHOLD:
      {
        int ott = interlocked_exchange(cfg.triggering_threshold_ms, (p0 >= 0) ? p0 : WATCHDOG_FREEZE_THRESHOLD);
        if (!p1)
          debug("set watchdog threshold %lld ms", (int64_t)p0);
        kick();
        return ott;
      }
      case WATCHDOG_OPTION_CALLSTACKS_THRESHOLD:
      {
        int old = interlocked_exchange(cfg.dump_threads_threshold_ms, p0);
        if (!p1)
          debug("set watchdog callstacks threshold %lld ms", (int64_t)p0);
        kick();
        return old;
      }
      case WATCHDOG_OPTION_SLEEP:
      {
        int old = interlocked_exchange(cfg.sleep_time_ms, p0);
        if (!p1)
          debug("set watchdog sleep time %lld ms", (int64_t)p0);
        kick();
        return old;
      }
      case WATCHDOG_OPTION_DUMP_THREADS:
      {
        if (p1)
        {
          debug("add thread id %lld to watchdog dump list", (int64_t)p0);
          OSSpinlockScopedLock lock{dumpThreadIdMutex};
          dumpThreadIds.emplace_back(p0);
        }
        else
        {
          debug("remove thread id %lld from watchdog dump list", (int64_t)p0);
          OSSpinlockScopedLock lock{dumpThreadIdMutex};
          dumpThreadIds.erase(eastl::remove(dumpThreadIds.begin(), dumpThreadIds.end(), p0), dumpThreadIds.end());
        }
        kick();

        return 0;
      }
    }
    return -1;
  }

  void kick() override { lastUpdateTimeMs.store(get_cur_time_ms(), dag::memory_order_relaxed); }

  void execute()
  {
    kick();

    volatile int triggering_threshold_ms;
    while (1)
    {
      int sleep_time_ms = interlocked_acquire_load(cfg.sleep_time_ms);
      int ret = os_event_wait(&sleepEvent, sleep_time_ms);
      (void)ret;
      G_ASSERTF(ret == OS_WAIT_OK || ret == OS_WAIT_TIMEOUTED, "%d", ret);
      if (isThreadTerminating())
        break;
      triggering_threshold_ms = interlocked_acquire_load(cfg.triggering_threshold_ms);

      if ((cfg.flags & WATCHDOG_DISABLED) || triggering_threshold_ms == WATCHDOG_DISABLE ||
          (interlocked_relaxed_load(g_debug_is_in_fatal) > 0) || (!(cfg.flags & WATCHDOG_IGNORE_DEBUGGER) && is_debugger_present()) ||
          (cfg.keep_sleeping_cb && cfg.keep_sleeping_cb()))
      {
      keep_sleeping:
        kick();
        continue;
      }

#if !_TARGET_STATIC_LIB
      if (!(cfg.flags & WATCHDOG_IGNORE_BACKGROUND))
        goto keep_sleeping;
#elif _TARGET_PC_WIN | _TARGET_IOS
      if (!(cfg.flags & WATCHDOG_IGNORE_BACKGROUND) && !dgs_app_active)
        goto keep_sleeping;
#endif
      unsigned curTime = get_cur_time_ms(), lastTime = lastUpdateTimeMs.load(dag::memory_order_relaxed);
      unsigned e_time = curTime - lastTime;
      if (curTime < lastTime || // timeGetTime wraps around happens each ~49.71 days since system start
          e_time > WATCHDOG_OS_RESUMED_THRESHOLD)
        goto keep_sleeping;

      const bool freeze = e_time >= triggering_threshold_ms;
      if (e_time > interlocked_acquire_load(cfg.dump_threads_threshold_ms))
      {
        if (dgs_last_suspend_at != 0 && dgs_last_resume_at != 0 &&
            ((int)(dgs_last_resume_at - dgs_last_suspend_at) < 0 ||
              (int)(get_cur_time_ms() - dgs_last_resume_at) < WATCHDOG_DUMP_CALLSTACK_THRESHOLD))
        {
          debug("WatchDogThread: system just resumed from sleep");
          goto keep_sleeping;
        }

        // Intentionally don't dump in release as it could take quite a bit of time skewing
        // crashserver's hang signature. To consider: add cfg flag that forces this?
#if DAGOR_DBGLEVEL > 0
        if (freeze)
          dump_all_thread_callstacks();
        else
        {
          logwarn("WatchDog: no kick in %d ms", e_time);
#else
        if (!freeze) // To consider: add flag to force dump even in release
        {
#endif
          // we can't dump random thread callstacks so its pointless
          // also it seems it takes forever on intel macs which is weird
#if !_TARGET_APPLE
          for (intptr_t thread_id : [&, this] {
                 OSSpinlockScopedLock lock{dumpThreadIdMutex};
                 return dumpThreadIds;
               }())
            dump_thread_callstack(thread_id);
#endif
        }
      }

      if (!freeze)
        continue;

      if (cfg.on_freeze_cb)
        cfg.on_freeze_cb(e_time, cfg.user_data);

      kick();
    }
  }
};

static WatchDogThread *watchdog_thread = NULL;

IWatchDog *watchdog_init_instance(WatchdogConfig *cfg)
{
  WatchDogThread *thread = new WatchDogThread(cfg);
  thread->start();
  return thread;
}

void watchdog_init(WatchdogConfig *cfg)
{
  G_ASSERT(!interlocked_acquire_load_ptr(watchdog_thread));
  WatchDogThread *thread = new WatchDogThread(cfg);
  thread->start();
  interlocked_release_store_ptr(watchdog_thread, thread);
}

void watchdog_shutdown()
{
  if (auto *thread = interlocked_exchange_ptr(watchdog_thread, (WatchDogThread *)nullptr))
  {
    del_it(thread);
  }
}

void watchdog_kick()
{
  if (auto *thread = interlocked_acquire_load_ptr(watchdog_thread))
    thread->kick();
}

intptr_t watchdog_set_option(int option, intptr_t p0, intptr_t p1)
{
  auto *thread = interlocked_acquire_load_ptr(watchdog_thread);
  if (!thread)
    return -1;
  return thread->setOption(option, p0, p1);
}

#if _TARGET_PC_WIN
bool is_watchdog_thread(uintptr_t thread_id)
{
  if (!watchdog_thread)
    return false;
  return GetThreadId(*(HANDLE *)watchdog_thread->getCurrentThreadIdPtr()) == thread_id;
}
#endif

#define EXPORT_PULL dll_pull_baseutil_watchdog
#include <supp/exportPull.h>
