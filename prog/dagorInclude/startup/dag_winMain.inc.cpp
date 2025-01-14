// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include "dag_winCommons.h"
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <direct.h>
#include <eh.h>

#if _TARGET_GDK
#include <osApiWrappers/gdk/app.h>
#endif

#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"


void DagorWinMainInit(int nCmdShow, bool debugmode);
int DagorWinMain(int nCmdShow, bool debugmode);

#if !_TARGET_STATIC_LIB
extern int pull_rtlOverride_stubRtl;
int pull_rtl_mem_override_to_dll = pull_rtlOverride_stubRtl;

#include <startup/dag_fs_utf8.inc.cpp>
#endif

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void apply_hinstance(void *hInstance, void *hPrevInstance);

extern void messagebox_win_report_fatal_error(const char *title, const char *msg, const char *call_stack);
static int dagor_program_exec(int nCmdShow, int debugmode, WinDeferredStartupLogs &deferredLogs);

static void __cdecl abort_handler(int) { DAG_FATAL("SIGABRT"); }

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR /*lpCmdLine*/, int nCmdShow)
{
  int retcode = 0;
  WinDeferredStartupLogs deferredLogs{};

  win_set_process_dpi_aware(deferredLogs);
  win_recover_systemroot_env();
  setlocale(LC_NUMERIC, "C");

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  dgs_init_argv(__argc, __argv);
  dgs_report_fatal_error = messagebox_win_report_fatal_error;
  apply_hinstance(hInstance, hPrevInstance);
  ::dgs_execute_quiet = ::dgs_get_argv("quiet");

  char noeh = ::dgs_get_argv("noeh") ? 1 : 0;
  char debugmode = ::dgs_get_argv("debug") ? 1 : 0;
  if (debugmode)
    noeh = 1;

  if (::dgs_get_argv("no_level_file"))
    debug_allow_level_files(false);

  symhlp_init_default();

  signal(SIGABRT, &abort_handler);

  if (!noeh)
  {
    DagorHwException::setHandler("main");

    DAGOR_TRY { retcode = dagor_program_exec(nCmdShow, debugmode, deferredLogs); }
    DAGOR_CATCH(DagorException e)
    {
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, true);
#endif
    }

    DagorHwException::cleanup();
  }
  else
  {
#pragma warning(push, 0) // to avoid C4297 in VC8
    DAGOR_TRY { retcode = dagor_program_exec(nCmdShow, debugmode, deferredLogs); }
    DAGOR_CATCH(...)
    {
      DEBUG_CP();
      flush_debug_file();
      DAGOR_RETHROW();
    }
#pragma warning(pop)
  }

  return retcode;
}

static int dagor_program_exec(int nCmdShow, int debugmode, WinDeferredStartupLogs &deferredLogs)
{
  default_crt_init_kernel_lib();

  // do this dummy new/delete to force linker to use OWR implementation of these (instead of RTL ones)
  delete[] new String[1];

  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));

#if _TARGET_GDK
  gdk::initialize_runtime();
#endif

#if defined(__DEBUG_FILEPATH)
  start_classic_debug_system(__DEBUG_FILEPATH);
#elif defined(__DEBUG_MODERN_PREFIX)
#if defined(__DEBUG_DYNAMIC_LOGSYS)
  if (__DEBUG_USE_CLASSIC_LOGSYS())
    start_classic_debug_system(__DEBUG_CLASSIC_LOGSYS_FILEPATH);
  else
#endif // defined(__DEBUG_DYNAMIC_LOGSYS)
    start_debug_system(__argv[0], __DEBUG_MODERN_PREFIX);
#else
  start_debug_system(__argv[0]);
#endif

  if (dgs_get_argv("copy_log_to_stdout"))
  {
    if (dgs_get_argv("attach_parent_console"))
      AttachConsole(ATTACH_PARENT_PROCESS); // Optional, because `>` won't work in cmd.exe/PowerShell
    set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE));
  }

#if _TARGET_GDK
  if (!gdk::app_initialize())
  {
    logerr("Failed to initialize GDK application");
    return 1;
  }
#endif

  if (deferredLogs.failed_SetProcessDPIAware)
    logwarn("SetProcessDPIAware failed");
  if (deferredLogs.failed_SetProcessDpiAwarenessContext)
    logwarn("SetProcessDpiAwarenessContext failed");
  if (deferredLogs.failed_SetThreadDpiAwarenessContext)
    logwarn("SetThreadDpiAwarenessContext failed");

  if (dgs_get_argv("rdp_mode"))
    win32_rdp_compatible_mode = true;
  else if (const char *var = getenv("DAGOR_RDP_MODE"))
    win32_rdp_compatible_mode = atoi(var) ? true : false;

  if (win32_rdp_compatible_mode)
    debug("started with win32_rdp_compatible_mode=%d", win32_rdp_compatible_mode);

  static DagorSettingsBlkHolder stgBlkHolder;
  ::DagorWinMainInit(nCmdShow, debugmode);

  default_crt_init_core_lib();

  int ret = ::DagorWinMain(nCmdShow, debugmode);

  close_debug_files();

  return ret;
}
