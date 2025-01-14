// Copyright (C) Gaijin Games KFT.  All rights reserved.

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
#include <drv/hid/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_driver.h>
#include "drv/3d/dag_resetDevice.h"
#include <util/dag_globDef.h>
#include <supp/_platform.h>
#include <supp/dag_cpuControl.h>
#include <debug/dag_logSys.h>
#include <util/dag_watchdog.h>


using namespace workcycle_internal;

#if _TARGET_PC_WIN
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
        if (!::dgs_app_active && (fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE))
        {
          set_priority(true);
          dgs_app_active = true;
          if (!is_float_exceptions_enabled())
            _fpreset();
        }
        else if (::dgs_app_active && (fActive == WA_INACTIVE))
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
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
        if (!::dgs_app_active && n_app_active)
        {
          set_priority(true);
          if (::global_cls_drv_pnt && ::global_cls_drv_pnt->isMouseCursorHidden())
            SetCursor((HCURSOR)win32_empty_mouse_cursor);
          dgs_app_active = true;
          if (!is_float_exceptions_enabled())
            _fpreset();
        }
        else if (::dgs_app_active && !n_app_active)
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
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
        HCURSOR cursor = ::global_cls_drv_pnt->isMouseCursorHidden()
                           ? (HCURSOR)win32_empty_mouse_cursor
                           : (win32_current_mouse_cursor ? (HCURSOR)win32_current_mouse_cursor : LoadCursor(NULL, IDC_ARROW));
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
      break;
    }

    case WM_WINDOWPOSCHANGING:
      if (::dgs_immovable_window)
        ((WINDOWPOS *)lParam)->flags |= SWP_NOMOVE;
      return {true, 0};

    case WM_POWERBROADCAST:
    {
      static int prevTmt = -1;
      if (wParam == PBT_APMSUSPEND)
      {
        dgs_last_suspend_at = timeGetTime();
        debug("Windows suspended @ %ums", dgs_last_suspend_at);
        prevTmt = (int)watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE);
      }
      if (wParam == PBT_APMRESUMEAUTOMATIC) // The system always sends a PBT_APMRESUMEAUTOMATIC message whenever the system resumes.
      {
        dgs_last_resume_at = timeGetTime();
        debug("Windows resumed @ %ums", dgs_last_resume_at);
        if (prevTmt >= 0) // If suspend code called
          watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, prevTmt);
      }
      return {true, TRUE};
    }

    case WM_EXITSIZEMOVE:
    {
      const eastl::pair<bool, intptr_t> handledResult = {true, 0};
      if (!d3d::is_inited() || !is_window_resizing_by_mouse())
        return handledResult;
      // when device is being reset, we want to reset it again with final resolution
      set_driver_reset_pending_on_exit_sizing();
      return handledResult;
    }

    case WM_SIZE:
    {
      const eastl::pair<bool, intptr_t> handledResult = {true, 0};
      // We assume that noone send WM_SIZE with this params from code
      if (wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED)
        set_window_size_has_been_changed_programmatically(false);
      const bool isSizeChangedProgrammatically = is_window_size_has_been_changed_programmatically();
      set_window_size_has_been_changed_programmatically(false);
      if (!d3d::is_inited() || is_window_resizing_by_mouse() || isSizeChangedProgrammatically)
        return handledResult;
      static bool isMinimized = false;
      if (wParam == SIZE_MINIMIZED)
      {
        isMinimized = true;
        return handledResult;
      }
      // On windows, when the window is minimized, it is "resized" to (0,0),
      // and afterwards, when it is un-minimized, the size is restored to
      // the previous one.
      // But there is no dedicated "un-minimization" event type, and the
      // ambiguously named SIZE_RESTORED event comes for ANY type of resize
      // (except for minimization & maximization), including the "un-minimization",
      // so we have to keep track of the "first" restore event after a minimize
      // to avoid unnecessary resets.
      if (wParam == SIZE_RESTORED && isMinimized)
      {
        isMinimized = false;
        return handledResult;
      }
      on_window_resized_change_reset_request();
      return handledResult;
    }

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO *minMax = (MINMAXINFO *)lParam;
      // Hardcoded minimum size for the window. TODO: It can be made configurable someday.
      minMax->ptMinTrackSize.x = 320;
      minMax->ptMinTrackSize.y = 180;
      return {true, 0};
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
        if (!::dgs_app_active && (fActive == GPCMP1_Activate || fActive == GPCMP1_ClickActivate))
        {
          set_priority(true);
          dgs_app_active = true;
          debug("activate");
          // if (!is_float_exceptions_enabled())
          //   _fpreset();
        }
        else if (::dgs_app_active && (fActive == GPCMP1_Inactivate))
        {
          if (interlocked_relaxed_load(enable_idle_priority))
            set_priority(false);
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
