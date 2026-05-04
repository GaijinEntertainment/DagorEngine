// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqmodules/sqmodules.h>
#include <memory/dag_memStat.h>
#include <workCycle/dag_workCyclePerf.h>

#include <sqrat.h>
#include <util/dag_string.h>
#include <util/dag_console.h>
#include <gui/dag_visualLog.h>
#include <debug/dag_logSys.h>

// Squirrel internals
#include <squirrel/sqpcheader.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqvm.h>
#include <quirrel/quirrelHost/memtrace.h>
#include <stdio.h>

namespace bindquirrel
{
extern void logerr_interceptor_bind_api(Sqrat::Table &nsTbl);

static bool console_output = false;

static void script_debug(const char *msg)
{
  if (console_output)
    printf("[SQ]: %s\n", msg);

  logmessage(_MAKE4C('SQRL'), "%s", msg);
}

static void script_fatal(const char *msg)
{
  if (console_output)
    printf("SQ_FATAL: %s\n", msg);

  DAG_FATAL("[SQ]: Fatal error:\n%s\n", msg);
}

static SQInteger script_logerr(HSQUIRRELVM vm)
{
  const char *msg = nullptr;
  SQRESULT sqres = sq_getstring(vm, 2, &msg);
  G_ASSERT_RETURN(SQ_SUCCEEDED(sqres), 0);

  if (console_output)
    printf("[SQ] Error: %s\n", msg);

  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
  {
    const char *sqcs = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &sqcs)) && sqcs)
    {
      logerr("[SQ]: %s\n%s", msg, sqcs);
      sq_pop(vm, 1);
      return 0;
    }
  }

  logerr("[SQ]: %s", msg);
  return 0;
}

static SQInteger sq_get_memory_allocated_kb() { return dagor_memory_stat::get_memory_allocated_kb(true); }

static SQFloat getAvgCpuOnlyCycleTimeUsec() { return SQFloat(workcycleperf::get_avg_cpu_only_cycle_time_usec()); }

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

static void print_to_debug(HSQUIRRELVM, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  if (console_output)
  {
    vprintf(s, vl);
    printf("\n");
  }
  cvlogmessage(_MAKE4C('SQRL'), s, vl);
  va_end(vl);
}


#endif

static SQInteger debug_dump_callstack(HSQUIRRELVM v)
{
#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

  SQPRINTFUNCTION pf = sq_getprintfunc(v);
  SQPRINTFUNCTION ef = sq_geterrorfunc(v);

  sq_setprintfunc(v, print_to_debug, print_to_debug);
  sqstd_printcallstack(v);
  sq_setprintfunc(v, pf, ef);
#else
  (void)(v);
#endif
  return 0;
}

static void screenlog(const char *msg)
{
  if (console_output)
    printf("SCREENLOG: %s\n", msg);

  visuallog::logmsg(msg);
}

static void console_print(const char *s) { console::print(s); }

void sqrat_bind_dagor_logsys(SqModules *module_mgr, bool console_mode)
{
  G_ASSERT(module_mgr);
  console_output = console_mode;
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Table memTraceTbl(vm);

  ///@module dagor.memtrace
  memTraceTbl //
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        sqmemtrace::reset_statistics_for_current_vm(vm);
        return 0;
      },
      "reset_cur_vm(): null")
    .SquirrelFuncDeclString(sqmemtrace::get_quirrel_object_size, "get_quirrel_object_size(obj: any): table")
    .SquirrelFuncDeclString(sqmemtrace::get_quirrel_object_size_as_string, "get_quirrel_object_size_as_string(obj: any): string")
    .SquirrelFuncDeclString(sqmemtrace::is_quirrel_object_larger_than, "is_quirrel_object_larger_than(obj: any, size: int): bool")
    .Func("reset_all", sqmemtrace::reset_all)
    .Func("set_huge_alloc_threshold", sqmemtrace::set_huge_alloc_threshold)
    /// @return previously allocation threshold
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        SQInteger num_top;
        sq_getinteger(vm, 2, &num_top);
        sqmemtrace::dump_statistics_for_current_vm(vm, num_top);
        return 0;
      },
      "dump_cur_vm(num_top: int): null")
    .Func("dump_all", sqmemtrace::dump_all)
    .Func("get_memory_allocated_kb", sq_get_memory_allocated_kb)
    /// @return i
    ;
  module_mgr->addNativeModule("dagor.memtrace", memTraceTbl);


  Sqrat::Table nsTbl(vm);

  ///@module dagor.debug
  nsTbl //
    .Func("debug", script_debug)
    .Func("fatal", script_fatal)
    .Func("screenlog", screenlog)
    .SquirrelFuncDeclString(script_logerr, "logerr(msg: string): null")
    .Func("console_print", &console_print)
    .SquirrelFuncDeclString(debug_dump_callstack, "debug_dump_stack(): null")
    .Func("get_log_filename", get_log_filename)
    /**/;
  logerr_interceptor_bind_api(nsTbl);

  module_mgr->addNativeModule("dagor.debug", nsTbl);

  Sqrat::Table perfTbl(vm);

  ///@module dagor.perf
  perfTbl //
    .Func("get_avg_cpu_only_cycle_time_usec", getAvgCpuOnlyCycleTimeUsec)
    .Func("reset_summed_cpu_only_cycle_time", []() { workcycleperf::reset_summed_cpu_only_cycle_time(); })
    /**/;

  module_mgr->addNativeModule("dagor.perf", perfTbl);
}

} // namespace bindquirrel
