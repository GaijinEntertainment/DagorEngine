//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_statDrv.h>
#include <EASTL/tuple.h>
#include <sqrat.h>
#include <sqext.h>
#include <memory/dag_framemem.h>


namespace darg
{


class BaseScriptHandler
{
public:
  virtual ~BaseScriptHandler() {}
  virtual bool call() = 0;

  bool allowOnShutdown = false;

#if DA_PROFILER_ENABLED
  uint32_t dapDescription = 0;
#endif
};


template <typename... Args>
class ScriptHandlerSqFunc : public BaseScriptHandler
{
public:
  template <typename... Args_>
  ScriptHandlerSqFunc(const Sqrat::Function &f, Args_ &&...a) : func(f), args(eastl::make_tuple<Args_...>(a...))
  {
#if DA_PROFILER_ENABLED
    String dapFuncName(framemem_ptr());
    SQFunctionInfo fi;
    if (SQ_SUCCEEDED(sq_ext_getfuncinfo(func.GetFunc(), &fi)))
    {
      const char *fnSlash = ::max(strrchr(fi.source, '/'), strrchr(fi.source, _SC('\\')));
      dapFuncName.printf(0, "%s(%s:%d)", fi.name, fnSlash ? (fnSlash + 1) : fi.source, fi.source);
      dapDescription = DA_PROFILE_ADD_DESCRIPTION(fi.source, fi.line, dapFuncName.c_str());
    }
#endif
  }

  virtual bool call() override { return eastl::apply(func, args); }

private:
  Sqrat::Function func;
  eastl::tuple<Args...> args;
};


} // namespace darg
