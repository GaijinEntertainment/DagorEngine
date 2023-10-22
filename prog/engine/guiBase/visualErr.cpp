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
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_miscApi.h>
#include <stdio.h>


static bool enable_red_window = true;

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

static debug_log_callback_t orig_debug_log = nullptr;


struct PutsToConsoleDelayed : public DelayedAction
{
  String message;
  bool precondition() override { return is_main_thread(); }
  void performAction() override { console::con_vprintf(console::CONSOLE_ERROR, false, message, nullptr, 0); }
};


static int visuallog_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  static const int D3DE_TAG = _MAKE4C('D3DE');
  if (lev_tag != LOGLEVEL_ERR && lev_tag != LOGLEVEL_FATAL && lev_tag != D3DE_TAG)
    return 1;

  static thread_local int entered = 0;
  if (entered)
    return 1;
  entered = 1;
  bool isError = lev_tag != LOGLEVEL_FATAL;

  String s(framemem_ptr());
  if (ctx_file)
    s.printf(0, "[%c] %s,%d: ", isError ? 'E' : 'F', ctx_file, ctx_line);
  else
    s.printf(0, "[%c] ", isError ? 'E' : 'F');
  s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);
  visuallog::logmsg(s, &visuallog_upd_proc, NULL, E3DCOLOR(isError ? 255 : 128, 32, 32), isError ? 1000 : 2000);

  if (console::print_logerr_to_console && lev_tag <= LOGLEVEL_ERR && strncmp(fmt, "CON: ", 5) != 0)
  {
    if (!is_main_thread())
    {
      PutsToConsoleDelayed *p = new PutsToConsoleDelayed;
      p->message = s;
      ::add_delayed_action(p);
    }
    else
      console::con_vprintf(console::CONSOLE_ERROR, false, s, nullptr, 0);
  }

  if (isError && ::dgs_get_argv("logerr_to_stderr"))
    fprintf(stderr, "%s\n", s.str());

  if (isError)
  {
    stackhelp::CallStackCaptureStore<48> stack;
    stackhelp::ext::CallStackCaptureStore extStack;
    stack.capture();
    extStack.capture();
    debug("BP=%p %s", stackhlp_get_bp(), get_call_stack_str(stack, extStack));
    total_logerr++;
    visual_err_log_update_window_background();
  }
  else
    total_fatalerr++;

  entered = 0;

  if (orig_debug_log)
    return orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line);

  return 1;
}

void visual_err_log_setup(bool red_window)
{
  enable_red_window = red_window;
  orig_debug_log = debug_set_log_callback(&visuallog_callback);
}

void visual_err_log_check_any()
{
  G_ASSERT_LOG(!total_logerr && !total_fatalerr, "%d error(s) and %d fatal error(s). See the exact list of errors in the logger file!",
    total_logerr, total_fatalerr);
}
