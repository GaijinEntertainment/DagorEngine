//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <string.h>
#include <osApiWrappers/dag_miscApi.h>

#include <supp/dag_define_KRNLIMP.h>

enum WatchdogFlags
{
  WATCHDOG_IGNORE_BACKGROUND = 1, // keep working in background mode (i.e. if window in background)
  WATCHDOG_IGNORE_DEBUGGER = 2,   // keep working even if debugger active
  WATCHDOG_DISABLED = 4,          // do not perform watchdog checks
};

enum WatchdogOptions
{
  WATCHDOG_OPTION_TRIG_THRESHOLD,       // if 0 - disable watchdog, return old value on set
  WATCHDOG_OPTION_CALLSTACKS_THRESHOLD, // set callstacks dump threshold, return old value on set
  WATCHDOG_OPTION_SLEEP,                // set watchdog thread sleep duration, return old value on set
  WATCHDOG_OPTION_DUMP_THREADS // if p1 is 1 then add p0 as interested thread id, if p1 is 0 remove the p0 from interested list
};

static constexpr int WATCHDOG_DISABLE = 0;

void KRNLIMP on_freeze_detected_default(unsigned time, int64_t user_data);

struct WatchdogConfig
{
  bool (*keep_sleeping_cb)() = nullptr;
  void (*on_freeze_cb)(unsigned time, int64_t user_data) = &on_freeze_detected_default;
  int64_t user_data = 0;
  int flags = 0;
  int triggering_threshold_ms = 0;
  int dump_threads_threshold_ms = 0;
  int sleep_time_ms = 0;
};

class IWatchDog

{
public:
  // use watchdog_init_instance to create an instance

  // kick() must be called periodically to reset watchdog timer
  // if watchdog is enabled, it will call on_freeze_cb() if freeze detected
  virtual void kick() = 0; // call this to reset watchdog timer

  // use setOption to change watchdog options like timeouts, thread dumps, etc.
  // returns old value on success, -1 on error
  virtual intptr_t setOption(int option, intptr_t p0 = 0, intptr_t p1 = 0) = 0;

  // virtual destructor to ensure proper cleanup of derived classes
  // destructor actually terminate the watchdog thread
  virtual ~IWatchDog() = default; // ensure proper destruction of derived classes
};

KRNLIMP void watchdog_init(WatchdogConfig *cfg = nullptr);
KRNLIMP void watchdog_shutdown();
KRNLIMP void watchdog_kick();
KRNLIMP intptr_t watchdog_set_option(int option, intptr_t p0 = 0, intptr_t p1 = 0);

// create an instance of IWatchDog
// if cfg is nullptr, default watchdog config will be used
// watchdog thread will be created and started by this function
// use delete to destroy the instance
KRNLIMP IWatchDog *watchdog_init_instance(WatchdogConfig *cfg = nullptr);

#if _TARGET_PC_WIN
KRNLIMP bool is_watchdog_thread(uintptr_t thread_id);
#endif
#include <supp/dag_undef_KRNLIMP.h>

class ScopeSetWatchdogCurrentThreadDump
{
public:
  ScopeSetWatchdogCurrentThreadDump() { watchdog_set_option(WATCHDOG_OPTION_DUMP_THREADS, get_current_thread_id(), 1); };
  ~ScopeSetWatchdogCurrentThreadDump() { watchdog_set_option(WATCHDOG_OPTION_DUMP_THREADS, get_current_thread_id(), 0); };
};

class ScopeSetWatchdogTimeout
{
  int prevTmt;

public:
  ScopeSetWatchdogTimeout(int newtm) { prevTmt = watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, newtm); }
  ~ScopeSetWatchdogTimeout() { watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, prevTmt); }
};
