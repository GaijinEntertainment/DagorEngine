// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
#include <memory/dag_memStat.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <generic/dag_initOnDemand.h>
#include <perfMon/dag_daProfileMemory.h>
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
    if (dagor_memory_stat::get_memchunk_count() ||
        (dagor_memory_stat::get_memory_allocated() &&
          dagor_memory_stat::get_memory_allocated() != dagor_memory_stat::get_memory_allocated(true)))
    {
      // doesn't return amount of memory allocated by profiler, but is very fast function
      char text[100];
      SNPRINTF(text, sizeof(text), "%lld bytes leaked in %lld chunks.", (long long)dagor_memory_stat::get_memory_allocated(),
        (long long)dagor_memory_stat::get_memchunk_count());

      size_t profiler_chunks, profiled_entries;
      da_profiler::profile_memory_profiler_allocations(profiler_chunks, profiled_entries);
      size_t threshold = 0;
      if (profiled_entries > 128) // too many, let's only focus on biggest
      {
        size_t profiled_memory, max_entry_sz;
        da_profiler::profile_memory_allocated(profiler_chunks, profiled_entries, profiled_memory, max_entry_sz);
        threshold = max_entry_sz >> 3; // 12.5% of max entry
      }
      class LeakMemDumpCb final : public da_profiler::MemDumpCb
      {
      public:
        size_t threshold = 0;
        LeakMemDumpCb(size_t t) : threshold(t) {}
        virtual ~LeakMemDumpCb() {}
        void dump(da_profiler::profile_mem_data_t, const uintptr_t *stack, uint32_t stack_size, size_t allocated,
          size_t allocations) override
        {
          if (!stack_size || allocated < threshold)
            return;
          char stk[8192];
          const char *cs = stackhlp_get_call_stack(stk, sizeof(stk), (const void *const *)stack, stack_size);
          debug(">>>> allocated %llu bytes in %d pointers", allocated, allocations);
          debug("\n  %s", cs);
        }
      };
#if _TARGET_PC
      char log_fname[DAGOR_MAX_PATH];
      strncpy(log_fname, get_log_filename(), DAGOR_MAX_PATH - 1);
      log_fname[DAGOR_MAX_PATH - 1] = '\0';

      if (get_debug_console_handle() != invalid_console_handle)
      {
        // write to debug_console_handle via out_debug_str()
        start_classic_debug_system(nullptr);
        debug("\n!!! MEMORY LEAKS !!!\n\n%s", text);
        LeakMemDumpCb dump(threshold);
        da_profiler::dump_memory_map(dump);
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
      LeakMemDumpCb dump(threshold);
      da_profiler::dump_memory_map(dump);

      close_debug_files();
      if (dgs_on_memory_leak_detected_handler)
        dgs_on_memory_leak_detected_handler(text);
      dgs_on_memory_leak_detected_handler = nullptr;
    }

#if _TARGET_PC && !DAGOR_HOSTED_INTERNAL_SERVER
    symhlp_close();
#endif
  }
} dagor_on_exit;
#endif
