#include <util/dag_watchdog.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_fatal.h>
#include <util/dag_console.h>
#include <util/dag_stdint.h>
#include <3d/dag_drv3dCmd.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <atomic>
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
#define WATCHDOG_SLEEP_TIME_MS (500)
#define ACTIVE_SWAP_THRESHOLD  (1000)

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

class WatchDogThread : public DaThread // freezes detector
{
public:
  WatchdogConfig cfg;
  std::atomic_uint lastUpdateTimeMs;
  os_event_t sleepEvent;
  uint32_t lastPageFaultCount = 0;
  uint32_t pageFaultCountResetAt = 0;
  uint32_t activeSwapHandicapMs = 0;
  eastl::fixed_vector<intptr_t, 4> dumpThreadIds;
  OSSpinlock dumpThreadIdMutex;

  WatchDogThread(WatchdogConfig *cfg_) : DaThread("WatchDogThread", 128 << 10), lastUpdateTimeMs(0u)
  {
    os_event_create(&sleepEvent);
    memset(&cfg, 0, sizeof(cfg));
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
  ~WatchDogThread() { os_event_destroy(&sleepEvent); }

  void kick() { lastUpdateTimeMs.store(get_cur_time_ms(), std::memory_order_relaxed); }

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
      if (interlocked_acquire_load(terminating))
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
      unsigned curTime = get_cur_time_ms(), lastTime = lastUpdateTimeMs.load(std::memory_order_relaxed);
      if (curTime < lastTime) // wrap-around?
        goto keep_sleeping;

      int e_time = curTime - lastTime;
      bool freeze = e_time >= triggering_threshold_ms + (cfg.flags & WATCHDOG_SWAP_HANDICAP ? activeSwapHandicapMs : 0);
      if (e_time > interlocked_acquire_load(cfg.dump_threads_threshold_ms))
      {
        if (dgs_last_suspend_at != 0 && dgs_last_resume_at != 0 &&
            ((int)(dgs_last_resume_at - dgs_last_suspend_at) < 0 ||
              (int)(get_cur_time_ms() - dgs_last_resume_at) < WATCHDOG_DUMP_CALLSTACK_THRESHOLD))
        {
          debug("WatchDogThread: system just resumed from sleep");
          goto keep_sleeping;
        }

        if (freeze)
          dump_all_thread_callstacks();
        else
        {
          logwarn("WatchDog: no kick in %d ms", e_time);

          for (intptr_t thread_id : [&] {
                 OSSpinlockScopedLock lock{dumpThreadIdMutex};
                 return dumpThreadIds;
               }())
            dump_thread_callstack(thread_id);
        }

#if _TARGET_PC_WIN
        // Note: we actually want report only hard page faults here, but there is no easy way to do it on Windows
        // (standard API return both soft & hard altogether)
        // For a really adventurous - you probably need to use either ETW or undocumented SYSTEM_PERFORMANCE_INFORMATION
        // See
        // http://blogs.msdn.com/b/greggm/archive/2004/01/21/61237.aspx
        // http://stackoverflow.com/questions/6416005/programatically-read-programs-page-fault-count-on-windows
        PROCESS_MEMORY_COUNTERS memc = {sizeof(PROCESS_MEMORY_COUNTERS)};
        memc.PageFaultCount = 0;
        GetProcessMemoryInfo(GetCurrentProcess(), &memc, sizeof(memc));
        debug("%s PageFaultCount=%d", __FUNCTION__, (int)memc.PageFaultCount);
        if (lastPageFaultCount == 0)
        {
          lastPageFaultCount = memc.PageFaultCount;
          pageFaultCountResetAt = curTime;
        }
        else if (memc.PageFaultCount - lastPageFaultCount > ACTIVE_SWAP_THRESHOLD * (curTime - pageFaultCountResetAt) / 1000)
          activeSwapHandicapMs += cfg.dump_threads_threshold_ms * 2 / 3;
#endif
      }
      else
        lastPageFaultCount = pageFaultCountResetAt = activeSwapHandicapMs = 0;

      if (!freeze)
        continue;

      kick();

      logerr("Watchdog: freeze detected, timeout %d >= %d msec, quant = %d msec, swap = %d msec", e_time, triggering_threshold_ms,
        cfg.sleep_time_ms, activeSwapHandicapMs);

      debug_flush(false);
      sleep_msec(100);

      if (!(cfg.flags & WATCHDOG_NO_FATAL))
      {
#if _TARGET_APPLE && DAGOR_DBGLEVEL > 0
        __builtin_trap();
#else
        fatal("Freeze detected");
#endif
      }
    }
  }
};

static WatchDogThread *watchdog_thread = NULL;

void watchdog_init(WatchdogConfig *cfg)
{
  G_ASSERT(!interlocked_acquire_load_ptr(watchdog_thread));
  WatchDogThread *thread = new WatchDogThread(cfg);
  thread->start();
  thread->setAffinity(WORKER_THREADS_AFFINITY_MASK); // Exclude core where the main thread runs to avoid scheduling it out.
  interlocked_release_store_ptr(watchdog_thread, thread);
}

void watchdog_shutdown()
{
  if (auto *thread = interlocked_exchange_ptr(watchdog_thread, (WatchDogThread *)nullptr))
  {
    thread->terminate(true, -1, &thread->sleepEvent);
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
  WatchdogConfig *cfg = &thread->cfg;
  switch (option)
  {
    case WATCHDOG_OPTION_TRIG_THRESHOLD:
    {
      int ott = interlocked_exchange(cfg->triggering_threshold_ms, (p0 >= 0) ? p0 : WATCHDOG_FREEZE_THRESHOLD);
      if (!p1)
        debug("set watchdog threshold %lld ms", (int64_t)p0);
      thread->kick();
      return ott;
    }
    case WATCHDOG_OPTION_CALLSTACKS_THRESHOLD:
    {
      int old = interlocked_exchange(cfg->dump_threads_threshold_ms, p0);
      if (!p1)
        debug("set watchdog callstacks threshold %lld ms", (int64_t)p0);
      thread->kick();
      return old;
    }
    case WATCHDOG_OPTION_SLEEP:
    {
      int old = interlocked_exchange(cfg->sleep_time_ms, p0);
      if (!p1)
        debug("set watchdog sleep time %lld ms", (int64_t)p0);
      thread->kick();
      return old;
    }
    case WATCHDOG_OPTION_DUMP_THREADS:
    {
      if (p1)
      {
        debug("add thread id %lld to watchdog dump list", (int64_t)p0);
        OSSpinlockScopedLock lock{thread->dumpThreadIdMutex};
        thread->dumpThreadIds.emplace_back(p0);
      }
      else
      {
        debug("remove thread id %lld from watchdog dump list", (int64_t)p0);
        OSSpinlockScopedLock lock{thread->dumpThreadIdMutex};
        thread->dumpThreadIds.erase(eastl::remove(thread->dumpThreadIds.begin(), thread->dumpThreadIds.end(), p0),
          thread->dumpThreadIds.end());
      }
      thread->kick();

      return 0;
    }
  }
  return -1;
}

#define EXPORT_PULL dll_pull_baseutil_watchdog
#include <supp/exportPull.h>
