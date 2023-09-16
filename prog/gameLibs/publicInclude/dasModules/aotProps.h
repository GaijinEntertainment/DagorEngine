//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daECS/core/entityId.h>
#include <dasModules/aotEcs.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{
template <typename Props>
inline bool das_get_props(int id, const das::TBlock<void, const Props> &block, das::Context *context, das::LineInfoArg *at)
{
  const Props *props = Props::get_props(id);
  if (props)
  {
    vec4f arg = das::cast<const Props *const>::from(props);
    context->invoke(block, &arg, nullptr, at);
    return true;
  }

  return false;
}

template <typename Props>
inline const Props *das_try_get_props(int id)
{
  return Props::try_get_props(id);
}
} // namespace bind_dascript
