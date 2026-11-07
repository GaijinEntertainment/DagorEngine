// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_visualErrLog.h>
#include <gui/dag_visualLog.h>
#include <perfMon/dag_cpuFreq.h>
#include <memory/dag_framemem.h>
#include <debug/dag_logSys.h>
#include <debug/dag_assert.h>
#include <util/dag_string.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <stdio.h>


static bool enable_red_window = true;
static bool dump_stacks_on_logerr = false, copy_logerr_to_stderr = false;

#if _TARGET_PC_WIN
#include <Windows.h>
#include <osApiWrappers/dag_progGlobals.h>

static bool background_is_already_red = false;

void visual_err_log_update_window_background()
{
  if (background_is_already_red || !enable_red_window)
    return;

  background_is_already_red = true;
  HWND hwnd = (HWND)win32_get_main_wnd();
  HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
  SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
  InvalidateRect(hwnd, NULL, TRUE);
}
#else
void visual_err_log_update_window_background() {}
#endif
#if DAGOR_DBGLEVEL > 0
static uint32_t save_first_logerrs_count = 3;
static OSSpinlock logerrsLock;
static String first_logerrs;
#endif

static int total_logerr = 0, total_fatalerr = 0;

static bool visuallog_upd_proc(int event, struct visuallog::LogItem *item)
{
  if (event == visuallog::EVT_REFRESH)
  {
    if (!item->param)
    {
      item->param = (void *)(intptr_t)get_time_msec();
      return true;
    }
    int t0 = (intptr_t)item->param;
    return get_time_msec() < t0 + 15000;
  }
  return true;
}

static LogHandlerId visuallog_handler_id = LogHandlerId::Invalid;
static volatile bool show_errors_on_screen = true;
static thread_local bool checking_for_logged_errors = false;


struct PutsToConsoleDelayed : public DelayedAction
{
  String message;
  bool precondition() override { return is_main_thread(); }
  void performAction() override { console::con_vprintf(console::CONSOLE_ERROR, false, message, nullptr, 0); }
};


static void visuallog_handler(void *, LogMessage &msg)
{
  const int lev_tag = msg.levTag;
  const char *fmt = msg.fmt;
  const void *arg = msg.arg;
  const int anum = msg.anum;

  static thread_local int entered = 0;
  if (entered)
    return;
  bool isError = lev_tag != LOGLEVEL_FATAL;
  if (!isError)
    if (auto sarg = anum > 2 ? &((const DagorSafeArg *)arg)[2] : nullptr; sarg && sarg->varType == sarg->TYPE_STR)
      if (strstr(sarg->varValue.s, "Not enough memory")) // Low memory conditions - don't go further
        return;

  entered = 1;

  // NOTE: our logging infra CAN be called from foreign DLLs and arbitrary threads, e.g. from
  // a dx12 debug layer callback, in which case we cannot have an initialized framemem.
  IMemAlloc *fmem = framemem_ptr();
  String s(fmem ? fmem : strmem);
  if (msg.ctxFile)
    s.printf(0, "[%c] %s,%d: ", isError ? 'E' : 'F', msg.ctxFile, msg.ctxLine);
  else
    s.printf(0, "[%c] ", isError ? 'E' : 'F');
  s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);

  const bool showOnScreen = interlocked_relaxed_load(show_errors_on_screen) && !(msg.flags & LOGMSG_SHOWN_TO_USER);
  if (showOnScreen)
  {
    visuallog::logmsg(s, &visuallog_upd_proc, NULL, E3DCOLOR(isError ? 255 : 128, 32, 32), isError ? 1000 : 2000);
    msg.flags |= LOGMSG_SHOWN_TO_USER;
  }

  if (console::print_logerr_to_console && lev_tag <= LOGLEVEL_ERR && strncmp(fmt, "CON: ", 5) != 0)
  {
    if (!is_main_thread())
    {
      PutsToConsoleDelayed *p = new PutsToConsoleDelayed();
      p->message = s;
      ::add_delayed_action(p);
    }
    else
      console::con_vprintf(console::CONSOLE_ERROR, false, s, nullptr, 0);
  }

  if (isError && copy_logerr_to_stderr)
    fprintf(stderr, "%s\n", s.str());

  if (isError && dump_stacks_on_logerr)
  {
    stackhelp::CallStackCaptureStore<48> stack;
    stackhelp::ext::CallStackCaptureStore extStack;
    stack.capture();
    extStack.capture();
    debug("BP=%p %s", stackhlp_get_bp(), get_call_stack_str(stack, extStack).c_str());
#if DAGOR_DBGLEVEL > 0
    if (total_logerr < save_first_logerrs_count)
    {
      OSSpinlockScopedLock lock(logerrsLock);
      first_logerrs.aprintf(256, "%s\n", s.str());
    }
#endif
    interlocked_increment(total_logerr);
    if (showOnScreen)
      visual_err_log_update_window_background();
  }
  else
    interlocked_increment(total_fatalerr);

  entered = 0;
}

static void mute_log_handlers_while_checking(void *, LogMessage &msg)
{
  if (checking_for_logged_errors)
    msg.flags |= LOGMSG_STOP_HANDLERS;
}

void visual_err_log_show_on_screen(bool show) { interlocked_relaxed_store(show_errors_on_screen, show); }

void visual_err_log_setup(bool red_window)
{
  enable_red_window = red_window;
  dump_stacks_on_logerr = ::dgs_get_settings()->getBool("logerr_stack", true);
  copy_logerr_to_stderr = ::dgs_get_argv("logerr_to_stderr") != nullptr;
#if DAGOR_DBGLEVEL > 0
  save_first_logerrs_count = ::dgs_get_settings()->getInt("save_first_logerrs_count", 3);
#endif
  if (visuallog_handler_id == LogHandlerId::Invalid)
    visuallog_handler_id = debug_add_log_handler(&visuallog_handler, nullptr,
      {.levelMask = LOGMASK_ERRORS, .tags = {LOGTAG_D3D_ERROR}, .order = LOGORDER_SHOW_FALLBACK});
}

void visual_err_log_check_any()
{
  checking_for_logged_errors = true;
  ScopedLogHandler muteLogHandlers(&mute_log_handlers_while_checking, nullptr, {.levelMask = LOGMASK_ALL, .order = LOGORDER_FILTER});
#if DAGOR_DBGLEVEL > 0
  if (total_logerr && save_first_logerrs_count != 0)
  {
    OSSpinlockScopedLock lock(logerrsLock);
    G_ASSERT_LOG(!total_logerr,
      "%d error(s) and %d fatal error(s). First %d errors are:\n%sSee the exact list of errors in the logger file!", total_logerr,
      total_fatalerr, min<int>(total_logerr, save_first_logerrs_count), first_logerrs.c_str());
  }
  else
    G_ASSERT_LOG(!total_logerr && !total_fatalerr,
      "%d error(s) and %d fatal error(s). See the exact list of errors in the logger file!", total_logerr, total_fatalerr);
#else
  G_ASSERT_LOG(!total_logerr && !total_fatalerr, "%d error(s) and %d fatal error(s). See the exact list of errors in the logger file!",
    total_logerr, total_fatalerr);
#endif
  checking_for_logged_errors = false;
}
