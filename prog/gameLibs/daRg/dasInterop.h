// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <EASTL/string.h>

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

  // Async loading: at ctor time the DasScript may still be LOADING (ctx == null), so the
  // SimFunction cannot be resolved yet. We keep the requested name and resolve lazily on the
  // first _call (synchronous fallback if the script is still compiling - see dasInterop.cpp).
  eastl::string funcName;
  bool resolveFailed = false;

  static void register_script_class(Sqrat::Table &exports);

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger script_call(HSQUIRRELVM vm);
};

} // namespace darg
