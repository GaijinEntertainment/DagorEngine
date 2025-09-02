// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasInterop.h"
#include "dasScripts.h"
#include "guiScene.h"
#include "elementRef.h"

#include <sqModules/sqModules.h>
#include <sqstdaux.h>

#include <dasModules/dasSystem.h>
#include <daScript/ast/ast.h>

using namespace das;


namespace darg
{


SQInteger DasFunction::script_ctor(HSQUIRRELVM vm)
{
  HSQOBJECT hScript;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &hScript)));
  Sqrat::Object scriptObj(hScript, vm);

  if (sq_type(hScript) != OT_INSTANCE || !Sqrat::ClassType<DasScript>::IsClassInstance(hScript, false))
    return sq_throwerror(vm, "Invalid 'script' argument for DasFunction constructor");

  const char *funcName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &funcName)));

  {
    GuiScene *scene = GuiScene::get_from_sqvm(vm);
    das::daScriptEnvironmentGuard envGuard(scene->dasScriptsData->dasEnv, scene->dasScriptsData->dasEnv);

    if (!scene->dasScriptsData.get())
      return sq_throwerror(vm, "daScript is not initialized");

    DasScript *script = scriptObj.Cast<DasScript *>();
    das::Context *ctx = script->ctx.get();

    bool isUnique = false;
    das::SimFunction *dasFunc = ctx->findFunction(funcName, isUnique);
    if (!dasFunc)
      return sqstd_throwerrorf(vm, "Function '%s' not found in das script", funcName);

    if (!isUnique)
      return sqstd_throwerrorf(vm, "Function '%s' is not unique", funcName);

    DasFunction *inst = new DasFunction();

    inst->dasScriptObj = scriptObj;
    inst->dasCtx = ctx;
    inst->dasFunc = dasFunc;

    Sqrat::ClassType<DasFunction>::SetManagedInstance(vm, 1, inst);
    return 0;
  }
}

SQInteger DasFunction::script_call(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<DasFunction *>(vm, 1))
    return SQ_ERROR;

  DasFunction *inst = Sqrat::Var<DasFunction *>(vm, 1).value;
  if (!inst || !inst->dasFunc || !inst->dasCtx)
    return sq_throwerror(vm, "Invalid DasFunction instance");

  GuiScene *scene = GuiScene::get_from_sqvm(vm);
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
    ctx->unlock();
    ctx->restartHeaps();
    return sqstd_throwerrorf(vm, "das exception: %s", ex);
  }

  ctx->unlock();
  ctx->restartHeaps();

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
