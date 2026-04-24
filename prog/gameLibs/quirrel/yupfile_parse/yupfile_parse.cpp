// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>
#include <sqrat.h>
#include <sqmodules/sqmodules.h>
#include <yup_parse/yup.h>


static SQInteger get_str(HSQUIRRELVM vm)
{
  const char *yupfilename, *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &yupfilename)));
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &key)));
  be_node *yup = yup_load(yupfilename);
  const char *value = yup_get_str(yup, key);
  sq_pushstring(vm, value, -1);
  yup_free(yup);
  return 1;
}


static SQInteger get_int(HSQUIRRELVM vm)
{
  const char *yupfilename, *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &yupfilename)));
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &key)));
  be_node *yup = yup_load(yupfilename);
  const long long *value = yup_get_int(yup, key);
  sq_pushinteger(vm, *value);
  yup_free(yup);
  return 1;
}


namespace bindquirrel
{

void register_yupfile_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module yupfile_parse
  exports //
    .SquirrelFuncDeclString(get_str, "getStr(yupfilename: string, key: string): string")
    .SquirrelFuncDeclString(get_int, "getInt(yupfilename: string, key: string): int")
    /**/;
  module_mgr->addNativeModule("yupfile_parse", exports);
}

} // namespace bindquirrel
