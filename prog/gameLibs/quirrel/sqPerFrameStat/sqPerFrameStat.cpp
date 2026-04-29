// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqPerFrameStat/perFrameStat.h>
#include <perFrameStat/perFrameStat.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>

namespace bindquirrel
{
void register_benchmark_perframe_stat(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table tbl(vm);
  ///@module benchmark_perframe_stat
  tbl.Func("benchmark_perframe_stat_init", benchmark_perframe_stat::init)
    .Func("benchmark_perframe_stat_reset", benchmark_perframe_stat::reset)
    .Func("benchmark_perframe_stat_add_last_frame", benchmark_perframe_stat::add_last_frame)
    .Func("benchmark_perframe_stat_dump_to_file", benchmark_perframe_stat::dump_to_file)
    /**/;
  module_mgr->addNativeModule("benchmark_perframe_stat", tbl);
}
} // namespace bindquirrel
