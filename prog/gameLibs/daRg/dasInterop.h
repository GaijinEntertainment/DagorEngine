// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>

class SqModules;

namespace das
{
struct SimFunction;
struct TypeInfo;
class Context;
} // namespace das

namespace darg
{

class DasFunction
{
public:
  das::SimFunction *dasFunc = nullptr;
  das::Context *dasCtx = nullptr;
  Sqrat::Object dasScriptObj;

  static void register_script_class(Sqrat::Table &exports);

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger script_call(HSQUIRRELVM vm);
};

} // namespace darg
