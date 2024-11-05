// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scriptapi.h"

#include <sqModules/sqModules.h>
#include <sqstdaux.h>
#include <sqrat.h>

#include <workCycle/dag_workCycle.h>


static SQInteger get_type_delegates(HSQUIRRELVM v)
{
  const SQChar *type_name;
  sq_getstring(v, 2, &type_name);

  SQObjectType t;
  if (!strcmp(type_name, "table"))
    t = OT_TABLE;
  else if (!strcmp(type_name, "array"))
    t = OT_ARRAY;
  else if (!strcmp(type_name, "string"))
    t = OT_STRING;
  else if (!strcmp(type_name, "integer"))
    t = OT_INTEGER;
  else if (!strcmp(type_name, "generator"))
    t = OT_GENERATOR;
  else if (!strcmp(type_name, "closure"))
    t = OT_CLOSURE;
  else if (!strcmp(type_name, "thread"))
    t = OT_THREAD;
  else if (!strcmp(type_name, "class"))
    t = OT_CLASS;
  else if (!strcmp(type_name, "instance"))
    t = OT_INSTANCE;
  else if (!strcmp(type_name, "weakref"))
    t = OT_WEAKREF;
  else
    return sqstd_throwerrorf(v, "no default delegate table for type %s or it is an unknown type", type_name);

  SQRESULT res = sq_getdefaultdelegate(v, t);
  if (SQ_FAILED(res))
    return SQ_ERROR;

  return 1;
}


void register_csq_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  // clang-format off
  exports
    .SquirrelFunc("get_type_delegates", get_type_delegates, 2, ".s")
    .Func("dagor_work_cycle", dagor_work_cycle)
  ;
  // clang-format on

  module_mgr->addNativeModule("csq", exports);
}
