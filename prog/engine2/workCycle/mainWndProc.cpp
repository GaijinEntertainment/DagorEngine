#include "workCyclePriv.h"
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_genGuiMgr.h>
#include <workCycle/dag_wcHooks.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_atomic.h>
#include <humanInput/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_drv3d.h>
#include <util/dag_globDef.h>
#include <supp/_platform.h>
#include <supp/dag_cpuControl.h>
#include <debug/dag_logSys.h>
#include <util/dag_watchdog.h>


using namespace workcycle_internal;

int workcycle_internal::inInternalWinLoop = 0;

#if _TARGET_PC_WIN
static void enterInternalWinLoop(void *wnd)
{
  if (is_window_in_thread || interlocked_relaxed_load(enable_idle_priority))
    return;

  if (inInternalWinLoop == 0)
    SetTimer((HWND)wnd, 777, 30, NULL);
  inInternalWinLoop++;
  debug("enterInternalWinLoop: %d", inInternalWinLoop);
}

static void leaveInternalWinLoop(void *wnd)
{
  if (is_window_in_thread || interlocked_relaxed_load(enable_idle_priority) || !inInternalWinLoop)
    return;

  inInternalWinLoop--;
  if (inInternalWinLoop == 0)
    KillTimer((HWND)wnd, 777);
  debug("enterInternalWinLoop: %d", inInternalWinLoop);
}

void workcycle_internal::set_priority(bool foreground)
{
  SetPriorityClass(GetCurrentProcess(),
    foreground ? (dgs_higher_active_app_priority ? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS) : IDLE_PRIORITY_CLASS);
}
#else
void workcycle_internal::set_priority(bool) {}
#endif

namespace workcycle_internal
{
#if _TARGET_PC_WIN
namespace windows
{
eastl::pair<bool, intptr_t> main_wnd_proc(void *hwnd, unsigned message, uintptr_t wParam, [[maybe_unused]] intptr_t lParam)
{
  static bool shouldquit = false;

  switch (message)
  {
    case WM_ACTIVATE:
    {
      if (wParam != WA_INACTIVE)
        ::acquire_all_inpdev();

      int fActive = IsIconic((HWND)hwnd) ? WA_INACTIVE : LOWORD(wParam); // activation flag

      if (fActive == WA_INACTIVE)
        ::reset_all_inpdev();

      if (!window_initing)
      {
        if (!application_active && (fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE))
        {
          set_priority(true);
          application_active = true;
          dgs_app_active = true;
          if (!is_float_exceptions_enabled())
            _fpreset();
        }
        else if (application_active && (fActive == WA_INACTIVE))
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
          application_active = false;
          dgs_app_active = false;
          debug_flush(false);
        }
      }
      break;
    }

    case WM_ACTIVATEAPP:
      if (!window_initing)
      {
        BOOL n_app_active = IsIconic((HWND)hwnd) ? false : LOWORD(wParam);
        if (!application_active && n_app_active)
        {
          set_priority(true);
          if (::global_cls_drv_pnt && ::global_cls_drv_pnt->isMouseCursorHidden())
            SetCursor((HCURSOR)win32_empty_mouse_cursor);
          application_active = true;
          dgs_app_active = true;
          if (!is_float_exceptions_enabled())
            _fpreset();
        }
        else if (application_active && !n_app_active)
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
          application_active = false;
          dgs_app_active = false;
        }
      }
      break;

    case WM_SETFOCUS:
    {
      ::dagor_reset_spent_work_time();
      set_priority(true);
      break;
    }

    case WM_TIMER:
      // special timer to be used during move/resize loop
      if (wParam == 777 && inInternalWinLoop)
      {
        dagor_work_cycle();
        if (dwc_hook_inside_internal_winloop)
          dwc_hook_inside_internal_winloop();
      }
      break;

    case WM_CLOSE:
    case WM_DAGOR_CLOSING:
      if (dagor_gui_manager && !dagor_gui_manager->canCloseNow())
        return {true, 0};
      shouldquit = true;
      d3d::prepare_for_destroy();
      PostMessageW((HWND)hwnd, WM_DAGOR_CLOSED, wParam, lParam);
      break;

    case WM_DESTROY:
    case WM_DAGOR_DESTROY:
      win32_set_main_wnd(NULL);
      d3d::window_destroyed(hwnd);
      if (shouldquit)
        quit_game(0);
      break;

    case WM_SETCURSOR:
      if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
        if (LOWORD(lParam) != HTCLIENT)
          break;
      if (::global_cls_drv_pnt)
      {
        HCURSOR cursor = ::global_cls_drv_pnt->isMouseCursorHidden() ? (HCURSOR)win32_empty_mouse_cursor : LoadCursor(NULL, IDC_ARROW);
        if (is_window_in_thread)
          PostMessageW((HWND)hwnd, WM_DAGOR_SETCURSOR, 0, (LPARAM)cursor);
        else
          SetCursor(cursor);
      }
      return {true, TRUE};
  }

  return {false, 0};
}

eastl::pair<bool, intptr_t> default_wnd_proc(void *hwnd, unsigned message, uintptr_t wParam, intptr_t lParam)
{
  switch (message)
  {
    case WM_DAGOR_SETCURSOR:
    {
      SetCursor((HCURSOR)lParam);
      return {true, TRUE};
    }

    case WM_CLOSE:
    {
      if (!is_window_in_thread)
        break;
      PostMessageW((HWND)hwnd, WM_DAGOR_CLOSING, wParam, lParam);
      return {true, 0};
    }

    case WM_SYSCOMMAND:
    {
      long uCmdType = wParam & 0xFFF0;
      if (uCmdType == SC_KEYMENU || uCmdType == SC_MOUSEMENU)
        return {true, 0};
      if ((uCmdType == SC_MOVE || uCmdType == SC_SIZE) && !interlocked_relaxed_load(enable_idle_priority))
      {
        enterInternalWinLoop(hwnd);
        if (uCmdType == SC_MOVE)
          wParam = SC_MOVE;
        auto ret = DefWindowProc((HWND)hwnd, message, wParam, lParam);
        leaveInternalWinLoop(hwnd);
        return {true, ret};
      }
      break;
    }

    case WM_ENTERMENULOOP:
    {
      enterInternalWinLoop(hwnd);
      break;
    }
    case WM_EXITMENULOOP:
    {
      leaveInternalWinLoop(hwnd);
      break;
    }

    case WM_WINDOWPOSCHANGING:
      if (::dgs_immovable_window)
        ((WINDOWPOS *)lParam)->flags |= SWP_NOMOVE;
      return {true, 0};

    case WM_POWERBROADCAST:
    {
      static intptr_t prevTmt = 0;
      if (wParam == PBT_APMSUSPEND)
      {
        dgs_last_suspend_at = timeGetTime();
        debug("Windows suspended @ %ums", dgs_last_suspend_at);
        prevTmt = watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, 0);
      }
      if (wParam == PBT_APMRESUMEAUTOMATIC) // The system always sends a PBT_APMRESUMEAUTOMATIC message whenever the system resumes.
      {
        dgs_last_resume_at = timeGetTime();
        debug("Windows resumed @ %ums", dgs_last_resume_at);
        watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, prevTmt);
      }
      return {true, TRUE};
    }
  }

  return {false, 0};
}
} // namespace windows
#else
namespace non_windows
{
static intptr_t wnd_proc([[maybe_unused]] void *hwnd, unsigned message, uintptr_t wParam, [[maybe_unused]] intptr_t lParam)
{
  switch (message)
  {
    case GPCM_Activate:
    {
      const int fActive = wParam; // activation flag
      if (fActive == GPCMP1_Inactivate)
        ::reset_all_inpdev();
      else
        ::acquire_all_inpdev();

      if (!window_initing)
      {
        if (!application_active && (fActive == GPCMP1_Activate || fActive == GPCMP1_ClickActivate))
        {
          set_priority(true);
          application_active = true;
          dgs_app_active = true;
          debug("activate");
          // if (!is_float_exceptions_enabled())
          //   _fpreset();
        }
        else if (application_active && (fActive == GPCMP1_Inactivate))
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
          application_active = false;
          dgs_app_active = false;
          debug("activate-");
          debug_flush(false);
        }
      }
    }
    break;
  }

  return 0;
}
} // namespace non_windows
#endif

intptr_t main_window_proc(void *hwnd, unsigned message, uintptr_t wParam, intptr_t lParam)
{
  if (intptr_t result; ::perform_wnd_proc_components(hwnd, message, wParam, lParam, result))
    return result;

#if _TARGET_PC_WIN
  if (auto [handled, result] = windows::main_wnd_proc(hwnd, message, wParam, lParam); handled)
    return result;

  if (auto [handled, result] = windows::default_wnd_proc(hwnd, message, wParam, lParam); handled)
    return result;

  return DefWindowProcW((HWND)hwnd, message, wParam, lParam);
#else
  return non_windows::wnd_proc(hwnd, message, wParam, lParam);
#endif
}

} // namespace workcycle_internal
