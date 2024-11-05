// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
#include <memory/dag_memStat.h>
#include <memory/dag_dbgMem.h>
#include <osApiWrappers/dag_symHlp.h>
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
#if _TARGET_PC
      // reopen log in special synchronous mode (to avoid leaks on async writer memory)
      if (freopen(get_log_filename(), "at", stdout))
      { // workaround against gcc warn_unused_result warning
      }
      start_classic_debug_system("*"); // use stdout
#endif

      char text[100];
      SNPRINTF(text, sizeof(text), "%lld bytes leaked in %lld chunks.", (long long)dagor_memory_stat::get_memory_allocated(),
        (long long)dagor_memory_stat::get_memchunk_count());

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
