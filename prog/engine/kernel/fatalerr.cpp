#include <supp/_platform.h>

#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_demoMode.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/limBufWriter.h>
#include <util/engineInternals.h>
#include <stdio.h>
#include <stdlib.h>
#include <osApiWrappers/dag_cpuJobs.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
#include <unistd.h>
#endif

#if _TARGET_ANDROID
void (*android_exit_app_ptr)(int code) = NULL;
#endif

#if _TARGET_C3

#endif

#include "debugPrivate.h"

// implicitly this also allows several simultaneous fatals from different threads
#define ALLOW_IN_THREADS_FATALS_DURING_QUIT (1)

volatile int g_debug_is_in_fatal = 0;

struct FatalFlagHandler
{
  FatalFlagHandler() { interlocked_exchange(g_debug_is_in_fatal, 1); }
  ~FatalFlagHandler() { interlocked_exchange(g_debug_is_in_fatal, 0); }
};

struct CallbackGuard
{
#define MAX_CALLBACKS 16
  typedef void *cb_ptr_t;
  CallbackGuard(cb_ptr_t cb) : callback(cb)
  {
    if (!callback)
      return;

    // Normally we should not exceed MAX_CALLBACKS but in case we
    // do, just ignore new ones thus avoiding memory corruption
    if (next < MAX_CALLBACKS)
      callbacks[next] = callback;

    next++;
  }

  ~CallbackGuard()
  {
    if (!callback)
      return;

    size_t current = next - 1;
    if (next < MAX_CALLBACKS && callbacks[current] == callback)
      callbacks[current] = NULL;

    next--;
  }

  bool canExecuteCallback()
  {
    if (!callback)
      return false;

    for (size_t i = 0; i < next - 1 && i < MAX_CALLBACKS; ++i)
      if (callbacks[i] == callback)
        return false;

    return true;
  }

private:
  static cb_ptr_t callbacks[MAX_CALLBACKS];
  static size_t next;
  cb_ptr_t callback;
};
CallbackGuard::cb_ptr_t CallbackGuard::callbacks[] = {NULL};
size_t CallbackGuard::next = 0;

#define CALLBACK_IF_POSSIBLE(f, ...)             \
  {                                              \
    CallbackGuard g((CallbackGuard::cb_ptr_t)f); \
    if (g.canExecuteCallback())                  \
      f(__VA_ARGS__);                            \
  }

static thread_local const char *_fatal_origin_file = "<unknown>";
static thread_local int _fatal_origin_line = 0;
static thread_local stackhelp::CallStackCaptureStore<64> _fatal_origin_stack;
static thread_local stackhelp::ext::CallStackCaptureStore _fatal_origin_ext_stack;

static CritSecStorage quitCritSec;
static bool quitCritSecInited = false;

void init_quit_crit_sec()
{
  if (quitCritSecInited)
    return;
  quitCritSecInited = true;
  ::create_critical_section(quitCritSec, "quitcrit");
}

static void close_all()
{
  flush_debug_file();

  cpujobs::abortJobsRequested = true;

  CALLBACK_IF_POSSIBLE(dgs_shutdown);

  CALLBACK_IF_POSSIBLE(dgs_post_shutdown_handler);

  flush_debug_file();
}

static FATAL_PREFIX void vfatal(bool quit_type, const char *fmt, const void *arg, int anum) FATAL_SUFFIX;

static void enter_quit_critsec()
{
  if (!quitCritSecInited)
    return;
  if (!try_enter_critical_section(quitCritSec))
  {
#if ALLOW_IN_THREADS_FATALS_DURING_QUIT
    debug("%s", __FUNCTION__);
    enter_critical_section(quitCritSec);
#else
    if (!is_main_thread())
    {
      debug("kill thread on attempt to fatal");
#if _TARGET_XBOX
      ExitThread(13);
#else
#error "Not supported on this platform"
#endif
    }
#endif
  }
}

static void vfatal(bool quit_type, const char *fmt, const void *args, int anum)
{
  FatalFlagHandler _flag_handler;

  enter_quit_critsec();
  char stackmsg[8192];

  flush_debug_file();

  CALLBACK_IF_POSSIBLE(dgs_pre_shutdown_handler);

  char buf[4 << 10];
  {
    LimitedBufferWriter lbw(buf, sizeof(buf));

    lbw.mixed_avprintf(fmt, args, anum);
    if (dgs_fill_fatal_context)
      lbw.incPos(dgs_fill_fatal_context(lbw.getBufPos(), lbw.getBufLeft(), false));
    lbw.done();
  }

#if _TARGET_PC
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191) // silence on conversion to void(*)()
                                // it's safe to use in CallbackGuard
#endif                          // #ifdef _MSC_VER
  get_call_stack(stackmsg, sizeof(stackmsg), _fatal_origin_stack, _fatal_origin_ext_stack);
  logmessage(LOGLEVEL_FATAL, "FATAL ERROR:\n%s,%d:\n    %s\n%s", _fatal_origin_file, _fatal_origin_line, buf, stackmsg);
  flush_debug_file();

  {
    CallbackGuard g((CallbackGuard::cb_ptr_t)dgs_fatal_handler);
    if (g.canExecuteCallback() && !dgs_fatal_handler(buf, stackmsg, _fatal_origin_file, _fatal_origin_line))
    {
      if (quitCritSecInited)
        leave_critical_section(quitCritSec);
#if DAGOR_DBGLEVEL > 0
      return;
#endif
    }
  }

  if (quit_type)
    close_all();

  flush_debug_file();
  CALLBACK_IF_POSSIBLE(dgs_report_fatal_error, "FATAL ERROR", buf, stackmsg);

  quit_game(!quit_type ? -1 : 1);

#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER
#else  // !_TARGET_PC

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#if _TARGET_IOS | _TARGET_TVOS | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3 | _TARGET_ANDROID
  logmessage(LOGLEVEL_FATAL, "FATAL ERROR:\n%s,%d:\n  %s\n%s", _fatal_origin_file, _fatal_origin_line, buf,
    get_call_stack(stackmsg, sizeof(stackmsg), _fatal_origin_stack, _fatal_origin_ext_stack));
#else
  const char *fmt_str = "FATAL ERROR:\n%s\n";
  out_debug_str_fmt(fmt_str, buf);
  out_debug_str(get_call_stack(stackmsg, sizeof(stackmsg), _fatal_origin_stack, _fatal_origin_ext_stack));
#endif
  flush_debug_file();

  (void)quit_type;
  {
    CallbackGuard g((CallbackGuard::cb_ptr_t)dgs_fatal_handler);
    if (g.canExecuteCallback() &&
        !dgs_fatal_handler(buf, get_call_stack(stackmsg, sizeof(stackmsg), _fatal_origin_stack, _fatal_origin_ext_stack),
          _fatal_origin_file, _fatal_origin_line))
    {
      if (quitCritSecInited)
        leave_critical_section(quitCritSec);
#if DAGOR_DBGLEVEL > 0
      return;
#endif
    }
  }

#else // DAGOR_DBGLEVEL <= 0
  (void)quit_type;
  CALLBACK_IF_POSSIBLE(dgs_fatal_handler, buf, NULL, _fatal_origin_file, _fatal_origin_line);
#endif

  out_debug_str("[F] ---DEBUG BREAK---");
  printf("---DEBUG BREAK---\n");
  debug_flush(false);
  fflush(stdout);
  fflush(stderr);

#if defined(__clang__) || defined(__GNUC__)
  __builtin_trap();
#elif _TARGET_XBOX
  if (IsDebuggerPresent())
    __debugbreak();
  else
    __ud2();
#elif defined(_MSC_VER)
  __ud2();
#else // unknown platform
  // it is actually UB to write to nullptr, and modern clang is capable of removing such instructions.
  *(volatile int *)0 = NULL; // in release much more convenient crash in fatal (to get feedback from QA)
#endif

  abort(); // this should not be called, but it ensures function won't return
#endif // !_TARGET_PC
}

void quit_game(int c, bool restart, const char **args)
{
#if !ALLOW_IN_THREADS_FATALS_DURING_QUIT
  enter_quit_critsec();
#endif

#if 0 && DAGOR_DBGLEVEL > 0
  if (c==0 && logWasWritten)
    fatal("some error messages were written to log file, please examine them and take measures");
#endif

  if (c != -1)
    close_all();

  if (restart)
  {
#if _TARGET_PC_WIN
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    wchar_t cmd[DAGOR_MAX_PATH] = {L'\0'};
    wchar_t *pos = cmd;
    int len = _snwprintf(cmd, countof(cmd), L"%s", GetCommandLineW());
    int spaceLeft = countof(cmd) - len;
    pos += len;
    for (; args && *args && spaceLeft > 0; ++args)
    {
      wchar_t buf[255] = {L'\0'};
      int written = _snwprintf(pos, spaceLeft, L" %s", utf8_to_wcs(*args, buf, countof(buf)));
      pos += written;
      spaceLeft -= written;
    }

    char utf8cmd[DAGOR_MAX_PATH] = {0};
    wcs_to_utf8(cmd, utf8cmd, countof(utf8cmd));
    logmessage(LOGLEVEL_DEBUG, "CMD: %s", utf8cmd);

    ::CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

#elif _TARGET_PC_MACOSX | _TARGET_PC_LINUX
    extern char **environ;
    static constexpr int MAX_ARGC = 31;
    char *argv[MAX_ARGC + 1];

    memset(argv, 0, sizeof(argv));
    int i = 0;
    for (; i < dgs_argc && i < MAX_ARGC; i++)
      argv[i] = dgs_argv[i];
    for (; args && *args && i < MAX_ARGC; ++args, ++i)
      argv[i] = (char *)*args;
    ::execve(argv[0], argv, environ);
#else
    debug("Restart asked, but not supported on platform");
    G_UNREFERENCED(args);
#endif
  }

  logmessage(LOGLEVEL_DEBUG, "Exit with code %d\n", c);
  flush_debug_file();

  fflush(stdout);
  fflush(stdin);
  fflush(stderr);
  close_debug_files();

#if _TARGET_APPLE | _TARGET_ANDROID
#if _TARGET_ANDROID
  if (android_exit_app_ptr)
    android_exit_app_ptr(c);
#endif
  _exit(c);
#elif _TARGET_PC | _TARGET_XBOX
  exit(c);
#elif _TARGET_C3













#else
  fflush(stdout);
  _Exit(c); // _Exit is C99, _exit is POSIX
#endif
}

void _core_set_fatal_ctx(const char *fn, int ln)
{
  if (fn && ln >= 0)
  {
    while (*fn == '.' || *fn == '\\' || *fn == '/')
      fn++;
    _fatal_origin_file = fn;
    _fatal_origin_line = ln;
  }
  _fatal_origin_stack.capture();
  _fatal_origin_ext_stack.capture();
}


void _core_cfatal(const char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  vfatal(true, s, &ap, -1);
  va_end(ap);
}


void _core_cvfatal(const char *s, va_list ap) { vfatal(true, s, &ap, -1); }

void _core_cfatal_x(const char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  vfatal(false, s, &ap, -1);
  va_end(ap);
}

void _core_cvfatal_x(const char *s, va_list ap) { vfatal(false, s, &ap, -1); }

void _core_fatal_fmt(const char *fn, int ln, bool qterm, const char *fmt, const DagorSafeArg *arg, int anum)
{
  _core_set_fatal_ctx(fn, ln);
  vfatal(qterm, fmt, arg, anum);
}

#define EXPORT_PULL dll_pull_kernel_fatalerr
#include <supp/exportPull.h>
