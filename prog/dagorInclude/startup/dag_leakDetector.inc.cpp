// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
#include <memory/dag_memStat.h>
#include <memory/dag_dbgMem.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_logSys.h>
#include <startup/dag_globalSettings.h> // dgs_execute_quiet
#include <stdio.h>
#include <Windows.h>
#pragma warning(disable : 4074) // initializers put in compiler reserved initialization area
#pragma warning(disable : 4722) // 'OnExit::~OnExit' : destructor never returns, potential memory leak
#pragma init_seg(compiler)

static struct OnExit
{
  OnExit() { g_destroy_init_on_demand_in_dtor = true; }
  ~OnExit()
  {
#if _TARGET_PC
    close_debug_files();
#endif
    if (dagor_memory_stat::get_memory_allocated() || dagor_memory_stat::get_memchunk_count())
    {
      char text[100];
      SNPRINTF(text, sizeof(text), "%lld bytes leaked in %lld chunks.", (long long)dagor_memory_stat::get_memory_allocated(),
        (long long)dagor_memory_stat::get_memchunk_count());

#if _TARGET_PC
      char log_fname[DAGOR_MAX_PATH];
      strncpy(log_fname, get_log_filename(), DAGOR_MAX_PATH - 1);
      log_fname[DAGOR_MAX_PATH - 1] = '\0';

      if (dgs_get_argv("copy_log_to_stdout"))
      {
        // write to debug_console_handle via out_debug_str()
        start_classic_debug_system(nullptr);
        debug("\n!!! MEMORY LEAKS !!!\n\n%s", text);
        DagDbgMem::dump_all_used_ptrs(false);
        debug("\n!!! MEMORY LEAKS !!!");
        close_debug_files();
      }

      // reopen log in special synchronous mode (to avoid leaks on async writer memory)
      if (freopen(log_fname, "at", stdout))
      { // workaround against gcc warn_unused_result warning
      }
      set_debug_console_handle(invalid_console_handle);
      start_classic_debug_system("*"); // use stdout
#endif

      debug("%s", text);
      DagDbgMem::dump_all_used_ptrs(false);
      close_debug_files();

#if _TARGET_PC_WIN
      if (!dgs_execute_quiet)
        ::MessageBoxA(NULL, text, "Memory leak", MB_ICONERROR);
      symhlp_close();
      ExitProcess(113);
#endif
    }

#if _TARGET_PC
    symhlp_close();
#endif
  }
} dagor_on_exit;
#endif
