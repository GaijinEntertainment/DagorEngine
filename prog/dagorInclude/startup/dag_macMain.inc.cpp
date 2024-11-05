// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include <osApiWrappers/dag_dbgStr.h>
#include <debug/dag_hwExcept.h>
#include <memory/dag_mem.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <unistd.h>
#include <locale.h>
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"
#include <mach-o/dyld.h>

void DagorWinMainInit(int nCmdShow, bool debugmode);
int DagorWinMain(int nCmdShow, bool debugmode);

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void apply_hinstance(void *hInstance, void *hPrevInstance);
extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);

extern void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stack);

int main(int argc, char *argv[])
{
  measure_cpu_freq();
  stderr = freopen("/dev/null", "wt", stderr);
  setlocale(LC_NUMERIC, "C");

  // replace any relative or non-full path with fully qualified module path
  {
    static char exe_path[MAX_PATH];
    char full_exe_path[MAX_PATH];
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0)
    {
      strncpy(exe_path, realpath(exe_path, full_exe_path), sizeof(exe_path));
      exe_path[sizeof(exe_path) - 1] = 0;
      if (strcmp(argv[0], exe_path) != 0)
      {
        out_debug_str_fmt("module argv[0]=%s -> %s", argv[0], exe_path);
        argv[0] = exe_path;
      }
    }
  }

  dgs_init_argv(argc, argv);
  dgs_report_fatal_error = messagebox_report_fatal_error;
  apply_hinstance(NULL, NULL);
  ::dgs_execute_quiet = ::dgs_get_argv("quiet");

#if DAGOR_DBGLEVEL != 0
  char stamp_buf[256];
  printf("%s", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), "\n\n"));
#endif

  DagorHwException::setHandler("main");
  default_crt_init_kernel_lib();

  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));

#if defined(__DEBUG_FILEPATH)
  start_classic_debug_system(__DEBUG_FILEPATH);
#elif defined(__DEBUG_MODERN_PREFIX)
#if defined(__DEBUG_DYNAMIC_LOGSYS)
  if (__DEBUG_USE_CLASSIC_LOGSYS())
    start_classic_debug_system(__DEBUG_CLASSIC_LOGSYS_FILEPATH);
  else
#endif // defined(__DEBUG_DYNAMIC_LOGSYS)
    start_debug_system(argv[0], __DEBUG_MODERN_PREFIX);
#else
  start_debug_system(argv[0]);
#endif

  if (dgs_get_argv("copy_log_to_stdout"))
    set_debug_console_handle((intptr_t)stdout);

  static DagorSettingsBlkHolder stgBlkHolder;
  ::DagorWinMainInit(0, 0);

  default_crt_init_core_lib();

  return ::DagorWinMain(0, 0);
}
