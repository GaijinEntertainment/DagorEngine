//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <need_dasQUIRREL.h>
#include <dasModules/aotEcs.h>
#include <sqrat.h>
#include <daECS/core/baseComponentTypes/objectType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/baseComponentTypes/arrayType.h>

namespace bind_dascript
{

template <typename C>
inline bool PushInstanceCopy(HSQUIRRELVM vm, const C &val)
{
  return Sqrat::ClassType<C>::PushInstanceCopy(vm, val);
}

template <typename C>
inline void GetInstance(HSQUIRRELVM vm, SQInteger idx, bool nullAllowed, const das::TBlock<void, C *> &block, das::Context *context,
  das::LineInfoArg *at)
{
  C *res = Sqrat::ClassType<C>::GetInstance(vm, idx, nullAllowed);
  vec4f arg = das::cast<C *>::from(res);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript
