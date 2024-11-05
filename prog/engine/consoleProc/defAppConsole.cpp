// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_miscApi.h>
#include <memory/dag_dbgMem.h>
#include <debug/dag_memReport.h>
#include <perfMon/dag_perfTimer.h>

using namespace console;

static bool def_app_console_handler(const char *argv[], int argc) // use it only for app like console processors. move eveyrthing else
                                                                  // elsewhere
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("app", "fatal", 1, 1) { DAG_FATAL("Manual fatal from console"); }
  CONSOLE_CHECK_NAME("app", "dflush", 1, 2) { debug_flush(argc > 1 ? to_bool(argv[1]) : false); }
  CONSOLE_CHECK_NAME("app", "crash", 1, 1) { *(volatile int *)0 = 0; }
  CONSOLE_CHECK_NAME("app", "sleep_msec_once", 2, 2) { sleep_msec(atoi(argv[1])); }
  CONSOLE_CHECK_NAME("app", "busy_loop_for_ms", 1, 2)
  {
    const int64_t us = min(atoi(argv[1]), 1000) * 1000ll;
    int64_t ref = profile_ref_ticks();
    size_t i = 0, timePassedUs = 0;
    double sinRet = 0;
    for (; timePassedUs < us; ++i)
    {
      sinRet += sin(i);
      timePassedUs = profile_time_usec(ref);
    }
    console::print_d("performed %d sins ret= %g (%g sin per us)", i, sinRet, double(i) / timePassedUs);
  }
  CONSOLE_CHECK_NAME("mem", "alloc_10mb", 1, 2)
  {
    for (unsigned int blockNo = 0; blockNo < 1024 * (argc > 1 ? to_int(argv[1]) : 1); blockNo++)
      new char[10 * 1024];
  }
  CONSOLE_CHECK_NAME("mem", "stats", 1, 1)
  {
    char mstatsbuf[4 << 10];
    get_memory_stats(mstatsbuf, sizeof(mstatsbuf));
    console::print_d("%s", mstatsbuf);
  }
  CONSOLE_CHECK_NAME("mem", "leak", 1, 1) { (void)new char[1]; }
  CONSOLE_CHECK_NAME("mem", "dumprawheap", 1, 2) { DagDbgMem::dump_raw_heap(argc > 1 ? argv[1] : "heap.bin"); }
  CONSOLE_CHECK_NAME("mem", "dump_ptrs", 1, 1) { DagDbgMem::dump_all_used_ptrs(/*only_cur_gen*/ true); }
  CONSOLE_CHECK_NAME("mem", "next_gen", 1, 1) { DagDbgMem::next_generation(); }
  CONSOLE_CHECK_NAME("mem", "set_sysmem_ref", 1, 1) { memreport::set_sysmem_reference(); }
  CONSOLE_CHECK_NAME("mem", "reset_sysmem_ref", 1, 1) { memreport::reset_sysmem_reference(); }
  CONSOLE_CHECK_NAME("app", "logerr", 1, 2) { logerr("%s", argc > 1 ? argv[1] : nullptr); }
  CONSOLE_CHECK_NAME("app", "abort", 1, 1) { exit(0); }
  return found;
}

REGISTER_CONSOLE_HANDLER(def_app_console_handler);
