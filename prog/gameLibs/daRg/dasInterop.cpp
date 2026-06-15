// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasInterop.h"
#include "dasScripts.h"
#include "guiScene.h"
#include "elementRef.h"

#include <sqmodules/sqmodules.h>
#include <sqstdaux.h>

#include <dasModules/dasSystem.h>
#include <daScript/ast/ast.h>

using namespace das;


namespace darg
{


// Resolve the SimFunction from the (possibly still-loading) DasScript. Returns 0 on success
// (inst->dasCtx/dasFunc set), or a thrown SQ error. Synchronous fallback: if the script is still
// compiling on the worker we drain it here (main thread) - async stays a pure latency win, a _call
// before the compile finished just pays the remaining compile time once. Permanent errors
// (not found / not unique / failed to load) throw loudly, as the synchronous loader did.
static SQInteger resolve_das_function(HSQUIRRELVM vm, GuiScene *scene, DasFunction *inst)
{
  if (inst->dasFunc)
    return 0;
  if (inst->resolveFailed)
    return sqstd_throwerrorf(vm, "DasFunction '%s': script failed to load", inst->funcName.c_str());

  if (inst->dasScriptObj.GetType() != OT_INSTANCE || !Sqrat::ClassType<DasScript>::IsClassInstance(inst->dasScriptObj.GetObject()))
  {
    inst->resolveFailed = true;
    return sq_throwerror(vm, "Invalid 'script' for DasFunction");
  }

  DasScript *script = inst->dasScriptObj.Cast<DasScript *>();

  if (script->isLoading())
    scene->dasScriptsData->waitScriptResolved(script);

  if (!script->isReady() || !script->ctx)
  {
    inst->resolveFailed = true;
    return sqstd_throwerrorf(vm, "DasFunction '%s': script failed to load", inst->funcName.c_str());
  }

  das::daScriptEnvironmentGuard envGuard(scene->dasScriptsData->dasEnv, scene->dasScriptsData->dasEnv);
  das::Context *ctx = script->ctx.get();
  bool isUnique = false;
  das::SimFunction *dasFunc = ctx->findFunction(inst->funcName.c_str(), isUnique);
  if (!dasFunc)
  {
    inst->resolveFailed = true;
    return sqstd_throwerrorf(vm, "Function '%s' not found in das script", inst->funcName.c_str());
  }
  if (!isUnique)
  {
    inst->resolveFailed = true;
    return sqstd_throwerrorf(vm, "Function '%s' is not unique", inst->funcName.c_str());
  }

  inst->dasCtx = ctx;
  inst->dasFunc = dasFunc;
  return 0;
}


SQInteger DasFunction::script_ctor(HSQUIRRELVM vm)
{
  HSQOBJECT hScript;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &hScript)));
  Sqrat::Object scriptObj(hScript, vm);

  if (sq_type(hScript) != OT_INSTANCE || !Sqrat::ClassType<DasScript>::IsClassInstance(hScript))
    return sq_throwerror(vm, "Invalid 'script' argument for DasFunction constructor");

  const char *funcName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &funcName)));

  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  if (!scene->dasScriptsData.get())
    return sq_throwerror(vm, "daScript is not initialized");

  DasFunction *inst = new DasFunction();
  inst->dasScriptObj = scriptObj;
  inst->funcName = funcName;

  // If the script already finished loading, resolve eagerly so a genuine "function not found"
  // is reported at construction time (preserves the old loud behaviour for ready scripts).
  // While the script is still LOADING we defer resolution to the first _call - no block here.
  DasScript *script = scriptObj.Cast<DasScript *>();
  if (script->isReady())
  {
    SQInteger r = resolve_das_function(vm, scene, inst);
    if (SQ_FAILED(r))
    {
      delete inst;
      return r;
    }
  }

  Sqrat::ClassType<DasFunction>::SetManagedInstance(vm, 1, inst);
  return 0;
}

SQInteger DasFunction::script_call(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<DasFunction *>(vm, 1))
    return SQ_ERROR;

  DasFunction *inst = Sqrat::Var<DasFunction *>(vm, 1).value;
  if (!inst)
    return sq_throwerror(vm, "Invalid DasFunction instance");

  GuiScene *scene = GuiScene::get_from_sqvm(vm);

  // Lazy resolution: the script may have been LOADING at ctor time. Resolve now (with a
  // synchronous fallback if it is still compiling). Permanent errors throw.
  if (!inst->dasFunc)
  {
    SQInteger r = resolve_das_function(vm, scene, inst);
    if (SQ_FAILED(r))
      return r;
  }
  if (!inst->dasFunc || !inst->dasCtx)
    return sq_throwerror(vm, "Invalid DasFunction instance");

  das::daScriptEnvironmentGuard envGuard(scene->dasScriptsData->dasEnv, scene->dasScriptsData->dasEnv);

  das::SimFunction *dasFunc = inst->dasFunc;
  das::Context *ctx = inst->dasCtx;
  das::FuncInfo *info = dasFunc->debugInfo;

  int numArgs = sq_gettop(vm) - 2;
  int dasArgCount = info ? info->count : 0;

  if (numArgs != dasArgCount)
    return sqstd_throwerrorf(vm, "Argument count mismatch: expected %d, got %d", dasArgCount, numArgs);

  constexpr int maxArgs = 8;
  vec4f args[maxArgs];
  if (dasArgCount > maxArgs)
    return sqstd_throwerrorf(vm, "Too many arguments for das function: %d (max %d)", dasArgCount, maxArgs);

  for (int i = 0; i < dasArgCount; ++i)
  {
    das::TypeInfo *argType = info->fields[i];
    // Simple POD types only for now

    SQInteger sqIdx = i + 3;

    if (isValidArgumentType(argType, scene->dasScriptsData->typeConstElemRef))
    {
      ElementRef *eref = ElementRef::get_from_stack(vm, sqIdx);
      if (!eref)
        return sqstd_throwerrorf(vm, "Argument %d: expected Element type", i + 1);
      if (!eref->elem)
        return sqstd_throwerrorf(vm, "Argument %d: invalid Element (null ref)", i + 1);

      args[i] = das::cast<const Element &>::from(*eref->elem);
    }
    else
    {
      switch (argType->type)
      {
        case das::Type::tInt:
        {
          SQInteger v;
          if (SQ_FAILED(sq_getinteger(vm, sqIdx, &v)))
            return sqstd_throwerrorf(vm, "Argument %d: expected int", i + 1);
          args[i] = das::cast<int>::from((int)v);
          break;
        }
        case das::Type::tFloat:
        {
          SQFloat v;
          if (SQ_FAILED(sq_getfloat(vm, sqIdx, &v)))
            return sqstd_throwerrorf(vm, "Argument %d: expected float", i + 1);
          args[i] = das::cast<float>::from((float)v);
          break;
        }
        case das::Type::tBool:
        {
          SQBool v;
          if (SQ_FAILED(sq_getbool(vm, sqIdx, &v)))
            return sqstd_throwerrorf(vm, "Argument %d: expected bool", i + 1);
          args[i] = das::cast<bool>::from(v != 0);
          break;
        }
        default: return sqstd_throwerrorf(vm, "Argument %d: unsupported type for cross-call", i + 1);
      }
    }
  }

  ctx->tryRestartAndLock();
  bind_dascript::RAIIStackwalkOnLogerr stackwalkOnLogerr(ctx);

  vec4f ret = ctx->evalWithCatch(dasFunc, args, nullptr);

  if (const char *ex = ctx->getException())
  {
    // Breadcrumb: the sq error may be caught by script, so logerr is the only durable record;
    // emitted only on actual eval/compile overlap.
    if (darg_das_compile_in_flight())
      logerr("DasFunction '%s': das exception while a worker das compile was in flight"
             " (eval/compile overlap, see daRg async-load): %s",
        inst->funcName.c_str(), ex);
    ctx->unlock();
    return sqstd_throwerrorf(vm, "das exception: %s", ex);
  }

  ctx->unlock();

  das::TypeInfo *retType = info ? info->result : nullptr;
  if (!retType || retType->type == das::Type::tVoid)
    return 0;

  // Simple POD types only for now
  switch (retType->type)
  {
    case das::Type::tInt: sq_pushinteger(vm, das::cast<int>::to(ret)); return 1;
    case das::Type::tFloat: sq_pushfloat(vm, das::cast<float>::to(ret)); return 1;
    case das::Type::tBool: sq_pushbool(vm, das::cast<bool>::to(ret)); return 1;
    default: return sq_throwerror(vm, "Unsupported return type for cross-call");
  }
}

void DasFunction::register_script_class(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();

  Sqrat::Class<DasFunction, Sqrat::NoCopy<DasFunction>> clsDasFunc(vm, "DasFunction");
  clsDasFunc //
    .SquirrelCtor(script_ctor, 3, ".xs")
    .SquirrelFunc("_call", script_call, -1, "x", "Call the function with the given arguments")
    /**/;

  exports.Bind("DasFunction", clsDasFunc);
}

} // namespace darg
