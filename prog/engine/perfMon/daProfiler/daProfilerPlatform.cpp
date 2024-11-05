// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilePlatform.h"
#include "daGpuProfiler.h"
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_stackHlpEx.h>
#include <startup/dag_globalSettings.h>
#include <time.h>
#include <debug/dag_debug.h>
#include <stdarg.h>
#include <stdio.h>

#define DA_PROFILE_SYMBOLS_AVAILABLE (DAPROFILER_DEBUGLEVEL > 0) // in release builds there are usually no pdb available

extern char *os_gethostname(char *buf, size_t blen, const char *def = NULL);
extern const char *get_log_directory();

namespace da_profiler
{
void report_debug(const char *s, ...)
{
  va_list arguments;
  va_start(arguments, s);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), s, arguments);
  va_end(arguments);
  debug("%s", buf);
}
void report_logerr(const char *s)
{
  logerr("%s", s);
  G_UNUSED(s);
}
void sleep_msec(int msec) { ::sleep_msec(msec); }
void sleep_usec(int usec) { ::sleep_usec(usec); }
uint32_t get_current_process_id() { return get_process_uid(); }
uint32_t get_logical_cores()
{
  static uint32_t core_count = 0; // we cache core core count, so this operation is valid after term
  uint32_t cached = interlocked_acquire_load(core_count);
  if (cached != 0)
    return cached;
  cached = cpujobs::get_core_count();
  interlocked_release_store(core_count, cached);
  return cached;
}
uint64_t get_current_thread_id() { return ::get_current_thread_id(); }

uint32_t time_since_launch_msec() { return ::get_time_msec(); }
uint64_t global_timestamp()
{
  time_t ct;
  ::time(&ct);
  return (uint64_t)ct;
}

const char *get_current_thread_name() { return DaThread::getCurrentThreadName(); }
const char *get_current_platform_name() { return get_platform_string_id(); }
const char *get_executable_name() { return dgs_argv[0]; }
const char *get_cpu_name() { return dgs_cpu_name; }
#if DAPROFILER_DEBUGLEVEL > 0 || DAGOR_FORCE_LOGS
const char *get_default_file_dir() { return get_log_directory(); }
#else
const char *get_default_file_dir() { return nullptr; }
#endif
#if DAPROFILER_DEBUGLEVEL > 0
const char *get_current_host_name()
{
  static char buf[256];
  return os_gethostname(buf, sizeof(buf), "unknown host");
}
#else
const char *get_current_host_name() { return "unknown host"; }
#endif
bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname, size_t max_symbol_name)
{
  return ::stackhlp_get_symbol(addr, line, filename, max_file_name, symbolname, max_symbol_name);
}
bool stackhlp_enum_modules(const function<bool(const char *, size_t base, size_t size)> &cb) { return ::stackhlp_enum_modules(cb); }
#if (_TARGET_PC_WIN | _TARGET_ANDROID | _TARGET_XBOX | _TARGET_APPLE)
bool support_sample_other_threads() { return true; }
#else
bool support_sample_other_threads() { return ::can_unwind_callstack_for_other_thread(); }
#endif
} // namespace da_profiler

#if _TARGET_PC_WIN | _TARGET_ANDROID | _TARGET_XBOX | _TARGET_APPLE
// sampling helper function
namespace da_profiler
{
bool can_resolve_symbols()
{
#if DA_PROFILE_SYMBOLS_AVAILABLE
  return true;
#endif
  return false;
}
} // namespace da_profiler
#else
namespace da_profiler
{
bool can_resolve_symbols() { return false; }
} // namespace da_profiler
#endif