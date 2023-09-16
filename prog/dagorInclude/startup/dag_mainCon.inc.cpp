// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)

#pragma once

#include "dag_winCommons.h"
#if _TARGET_PC_WIN
#include <direct.h>
#include <eh.h>
#endif //_TARGET_PC_WIN

#include <stdlib.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_symHlp.h>
#ifdef __SUPPRESS_BLK_VALIDATION
#include <ioSys/dag_dataBlock.h>
#endif
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"

int DagorWinMain(bool debugmode);

#if !_TARGET_STATIC_LIB
extern int pull_rtlOverride_stubRtl;
int pull_rtl_mem_override_to_dll = pull_rtlOverride_stubRtl;

#include <startup/dag_fs_utf8.inc.cpp>
#endif

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();

extern void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stk);
static int dagor_program_exec(int argc, char **argv, int debugmode);

int __argc;
char **__argv;

int __cdecl main(int argc, char **argv)
{
  __argc = argc;
  __argv = (char **)malloc(sizeof(char *) * argc);
  for (int i = 0; i < argc; ++i)
    __argv[i] = argv[i];
  win_recover_systemroot_env();

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

#ifdef __SUPPRESS_BLK_VALIDATION
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;
#endif

  dgs_init_argv(argc, argv);
  dgs_report_fatal_error = messagebox_report_fatal_error;
  ::dgs_execute_quiet = ::dgs_get_argv("quiet");

  static DagorSettingsBlkHolder stgBlkHolder;
  char noeh = ::dgs_get_argv("noeh") ? 1 : 0;
  char debugmode = ::dgs_get_argv("debug") ? 1 : 0;
  if (debugmode)
    noeh = 1;

  symhlp_init_default();

  int retcode = 0;
  if (!noeh)
  {
    DagorHwException::setHandler("main");
#if !_TARGET_STATIC_LIB && _TARGET_PC_WIN
    _set_se_translator((_se_translator_function)DagorHwException::getHandler());
#endif
    DAGOR_TRY { retcode = dagor_program_exec(argc, argv, debugmode); }
    DAGOR_CATCH(DagorException e)
    {
      retcode = 13;
      debug_cp();
      flush_debug_file();
#ifdef DAGOR_EXCEPTIONS_ENABLED
      DagorHwException::reportException(e, true);
#endif
    }
    DagorHwException::cleanup();
  }
  else
  {
#pragma warning(push, 0) // to avoid C4297 in VC8
    DAGOR_TRY { retcode = dagor_program_exec(argc, argv, debugmode); }
    DAGOR_CATCH(...)
    {
      retcode = 13;
      debug_cp();
      flush_debug_file();
      DAGOR_RETHROW();
    }
#pragma warning(pop)
  }

  flush_debug_file();
  return retcode;
}

static int dagor_program_exec(int argc, char **argv, int debugmode)
{
  G_UNUSED(argc);
  G_UNUSED(argv);
  default_crt_init_kernel_lib();

  // do this dummy new/delete to force linker to use OWR implementation of these (instead of RTL ones)
  delete[] new String[1];

  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));

#if defined(__DEBUG_FILEPATH)
  start_classic_debug_system(__DEBUG_FILEPATH);
#else
  start_classic_debug_system("debug");
#endif

  default_crt_init_core_lib();

  long res = DagorWinMain(debugmode);
  return res;
}

#if _TARGET_PC_MACOSX
class NSWindow;
class NSView;
class CGRect
{};
NSWindow *macosx_create_dagor_window(const char * /*title*/, int /*scr_w*/, int /*scr_h*/, NSView * /*drawView*/, CGRect /*rect*/)
{
  return nullptr;
}
#endif
