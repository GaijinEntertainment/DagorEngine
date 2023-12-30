#include <supp/_platform.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_traceInpDev.h>
#include <startup/dag_demoMode.h>
#include <util/dag_loadingProgress.h>
#include <debug/dag_debug.h>
#if __GNUC__
#include <pthread.h>
#endif
#include <stdio.h>
#include <string.h>
#include <supp/dag_define_COREIMP.h>


bool dgs_execute_quiet = false;
void (*dgs_post_shutdown_handler)() = NULL;
void (*dgs_pre_shutdown_handler)() = NULL;
bool (*dgs_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line) = NULL;
void (*dgs_shutdown)() = NULL;
void (*dgs_fatal_report)(const char *msg, const char *call_stack) = NULL;
int (*dgs_fill_fatal_context)(char *buff, int sz, bool terse) = NULL;
void (*dgs_report_fatal_error)(const char *title, const char *msg, const char *call_stack) = NULL;
void (*loading_progress_point_cb)() = NULL;
void (*dgs_on_swap_callback)() = NULL;
void (*dgs_on_dagor_cycle_start)() = NULL;
void (*dgs_on_promoted_log_tag)(int tag, const char *fmt, const void *arg, int anum) = NULL;
void (*dgs_on_thread_enter_cb)(const char *) = nullptr;
void (*dgs_on_thread_exit_cb)() = nullptr;

static const DataBlock *default_get_settings() { return NULL; }
const DataBlock *(*dgs_get_settings)() = &default_get_settings;

static const DataBlock *default_get_game_params() { return NULL; }
const DataBlock *(*dgs_get_game_params)() = &default_get_game_params;

bool dagor_demo_mode = false;

WindowMode dgs_window_mode = WindowMode::FULLSCREEN_EXCLUSIVE;
int dagor_frame_no_int = 0;

bool dgs_app_active = true;
unsigned int dgs_last_suspend_at = 0;
unsigned int dgs_last_resume_at = 0;

#if _TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_ANDROID
static bool launchedAsDemo = false;
static int idleStartT = 0, demoIdleTimeout = 0;

bool dagor_is_demo_mode() { return launchedAsDemo; }
void dagor_demo_reset_idle_timer() { idleStartT = get_time_msec(); }
void dagor_demo_idle_timer_set_IS(bool) {}
bool dagor_demo_check_idle_timeout()
{
  if (!launchedAsDemo || !demoIdleTimeout || get_time_msec() < idleStartT + demoIdleTimeout)
    return false;
  return true;
}
void dagor_demo_final_quit(const char *) {}

void dagor_force_demo_mode(bool demo, int timeout_ms)
{
  measure_cpu_freq();
  launchedAsDemo = demo;
  demoIdleTimeout = timeout_ms;
  dagor_demo_reset_idle_timer();
  debug("force %s mode, timeout=%d ms, idleStartT=%d", demo ? "DEMO" : "normal", timeout_ms, idleStartT);
}
#endif

static bool default_assertion_handler(bool verify, const char *file, int line, const char *func, const char *cond, const char *fmt,
  const DagorSafeArg *arg, int anum)
{
  char buf[1024];
  int w = snprintf(buf, sizeof(buf), "%s failed in %s:%d,%s() :\n\"%s\"%s%s\n", verify ? "verify" : "assert", file, line, func, cond,
    fmt ? "\n\n" : "", fmt ? fmt : "");
  if ((unsigned)w >= sizeof(buf))
    memcpy(buf + sizeof(buf) - 4, "...", 4);
#if DAGOR_DBGLEVEL < 1
  logmessage_fmt(LOGLEVEL_ERR, buf, arg, anum);
#else
  _core_fatal_fmt(file, line, buf, arg, anum);
#endif
  return false;
}

bool (*dgs_assertion_handler)(bool verify, const char *file, int line, const char *func, const char *cond, const char *fmt,
  const DagorSafeArg *arg, int anum) = &default_assertion_handler;

int dgs_argc = 0;
char **dgs_argv = NULL;
bool dgs_sse_present = false;
char dgs_cpu_name[128] = "n/a";

bool dgs_trace_inpdev_line = false;


// build timestamp processing
#include <osApiWrappers/dag_direct.h>
#include <stdio.h>
#include <string.h>

const char *dagor_get_build_stamp_str_ex(char *buf, size_t bufsz, const char *suffix, const char *dagor_exe_build_date,
  const char *dagor_exe_build_time)
{
  if (strcmp(dagor_exe_build_date, "*") == 0)
  {
    char stamp_fn[512];
    if (const char *fext = dd_get_fname_ext(dgs_argv[0]))
      snprintf(stamp_fn, sizeof(stamp_fn), "%.*s-STAMP", int(fext - dgs_argv[0]), dgs_argv[0]);
    else
      snprintf(stamp_fn, sizeof(stamp_fn), "%s-STAMP", dgs_argv[0]);
    if (FILE *fp = fopen(stamp_fn, "rt"))
    {
      if (fgets(stamp_fn, sizeof(stamp_fn), fp) == nullptr)
        stamp_fn[0] = '\0';
      stamp_fn[sizeof(stamp_fn) - 1] = '\0';
      if (char *p = strchr(stamp_fn, '\n'))
        *p = '\0';
      fclose(fp);
      snprintf(buf, bufsz, "BUILD TIMESTAMP:   %s%s", stamp_fn, suffix);
      return buf;
    }
  }
  snprintf(buf, bufsz, "BUILD TIMESTAMP:   %s %s%s", dagor_exe_build_date, dagor_exe_build_time, suffix);
  return buf;
}

#if _TARGET_STATIC_LIB
extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix)
{
  return dagor_get_build_stamp_str_ex(buf, bufsz, suffix, dagor_exe_build_date, dagor_exe_build_time);
}

#endif

#define EXPORT_PULL dll_pull_kernel_kernelGlobalSetting
#include <supp/exportPull.h>
