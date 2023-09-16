//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

using Sqrat_AbstractStaticClassData = Sqrat::AbstractStaticClassData;
MAKE_TYPE_FACTORY(AbstractStaticClassData, Sqrat_AbstractStaticClassData)

namespace bind_dascript
{
inline bool find_AbstractStaticClassData(const das::TBlock<bool, const das::TTemporary<const Sqrat::AbstractStaticClassData>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  vec4f arg;
  bool found = false;
  context->invokeEx(
    block, &arg, nullptr,
    [&](das::SimNode *code) {
      for (auto &classData : Sqrat::_ClassType_helper<>::all_classes)
      {
        arg = das::cast<const Sqrat::AbstractStaticClassData *>::from(classData);
        found = code->evalBool(*context);
        if (found)
          break;
      }
    },
    at);
  return found;
}

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
