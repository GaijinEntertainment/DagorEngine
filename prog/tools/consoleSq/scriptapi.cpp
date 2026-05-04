// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scriptapi.h"

#include <sqmodules/sqmodules.h>
#include <sqstdaux.h>
#include <sqrat.h>

#include <workCycle/dag_workCycle.h>


void register_csq_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  // clang-format off
  exports
    .Func("dagor_work_cycle", dagor_work_cycle)
  ;
  // clang-format on

  module_mgr->addNativeModule("csq", exports);
}
