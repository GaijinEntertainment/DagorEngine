// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

//! install fatal handler; get_ctx() may return string to describe current game state when fatal occurs
extern void dagor_install_dev_fatal_handler(const char *(*get_ctx)());


//
// development-time fatal handler implementation
//
#if DAGOR_DBGLEVEL > 0

#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <drv/3d/dag_driver.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_messageBox.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <util/dag_string.h>
#include <stdio.h>

#if _TARGET_PC_WIN
#include <windows.h>
#elif _TARGET_C1 | _TARGET_C2


#endif

static bool (*main_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line) = NULL;
static String fatal_context_str;
static const char *get_fatal_ctx_def() { return "?unknown?"; }
static const char *(*get_fatal_ctx)() = NULL;

static bool ask_user_fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  if (::dgs_get_argv("fatals_to_stderr"))
  {
    fprintf(stderr, "FATAL ERROR:\n%s:%d:\n%s\n%s", file, line, msg, call_stack);
    fflush(stderr);
  }

  flush_debug_file();

  bool close = true;

#if _TARGET_PC_WIN
  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    logerr("Engine in fullscreen mode, we have to shutdown on fatal");
    shutdown_game(RESTART_ALL);
  }

  if (::dgs_execute_quiet)
    ::ExitProcess(1);
#endif // _TARGET_PC_WIN

#if DAGOR_DBGLEVEL != 0

  if (!::dgs_execute_quiet)
  {
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s\n%s", msg, call_stack);
    buf[sizeof(buf) - 1] = 0;
    ScopeDetachAllWndComponents wndCompsGuard; // stop handling windows input events during fatal message box
#if _TARGET_PC
    int osmb_flags = dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE ? GUI_MB_RETRY_CANCEL : GUI_MB_ABORT_RETRY_IGNORE;
#else
    int osmb_flags = GUI_MB_ABORT_RETRY_IGNORE;
#endif
    int result = os_message_box(buf, "FATAL ERROR", osmb_flags | GUI_MB_ICON_ERROR | GUI_MB_FOREGROUND | GUI_MB_NATIVE_DLG);
    flush_debug_file();

#if _TARGET_PC
    if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
      result = (result == GUI_MB_BUTTON_1) ? GUI_MB_BUTTON_2 : GUI_MB_BUTTON_1;
#endif

    if (result == GUI_MB_BUTTON_1 || result == GUI_MB_FAIL)
    {
      terminate_process(1);
    }
    else if (result == GUI_MB_BUTTON_2)
    {
      G_DEBUG_BREAK;
    }
    else
    {
      // continue execution
      close = false;
    }
  }
#endif // DAGOR_DBGLEVEL

  return close;
}


//! install fatal handler
void dagor_install_dev_fatal_handler(const char *(*get_ctx)())
{
  main_fatal_handler = ::dgs_fatal_handler;
  ::dgs_fatal_handler = ask_user_fatal_handler;
  get_fatal_ctx = get_ctx ? get_ctx : &get_fatal_ctx_def;
}

#else

//! in release, API will do nothing
void dagor_install_dev_fatal_handler(const char *(*get_ctx)())
{
  if (get_ctx)
    get_ctx();
}

#endif
