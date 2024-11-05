// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>
#include "sqRegExp.h"
#include "sqUtf8.h"


namespace bindquirrel
{
static SQInteger utf8_slice(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Utf8 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Utf8 *> utf8(vm, 1);
  sq_pushstring(vm, utf8.value->slice(vm), -1);
  return 1;
}

static SQInteger utf8_indexof(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Utf8 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Utf8 *> utf8(vm, 1);
  int res = utf8.value->indexof(vm);
  if (res >= 0)
    sq_pushinteger(vm, res);
  else
    sq_pushnull(vm);
  return 1;
}

void register_reg_exp(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Class<RegExp> regexp2(vm, "regexp2");
  ///@class regexp2=regexp2
  regexp2 //
    .Ctor<const char *>()
    ///@param regexp_string s
    .Func("match", &RegExp::match)
    .Func("fullmatch", &RegExp::fullMatch)
    .Func("replace", &RegExp::replace)
    .SquirrelFunc("multiExtract", &RegExp::sqMultiExtract, 3, nullptr)
    .Func("pattern", &RegExp::pattern)
    /**/;

  module_mgr->addNativeModule("regexp2", Sqrat::Object(regexp2.GetObject(), vm));
}

void register_utf8(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Class<Utf8> utf8(vm, "utf8");
  ///@class utf8=utf8
  utf8 //
    .Ctor<const char *>()
    ///@param text s
    .Func("_tostring", &Utf8::str)
    .Func("str", &Utf8::str)
    .Func("charCount", &Utf8::charCount)
    .Func("strtr", &Utf8::strtr)
    .SquirrelFunc("slice", utf8_slice, -2, nullptr)
    .SquirrelFunc("indexof", utf8_indexof, -2, nullptr)
    /**/;

  module_mgr->addNativeModule("utf8", Sqrat::Object(utf8.GetObject(), vm));
}
} // namespace bindquirrel
