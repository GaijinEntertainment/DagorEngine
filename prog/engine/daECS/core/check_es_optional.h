// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entitySystem.h>
#include <EASTL/algorithm.h>

namespace ecs
{

inline bool allCompsAreOptional(const EntitySystemDesc *es)
{
  auto isOptPred = [](const ecs::ComponentDesc &cd) { return (cd.flags & CDF_OPTIONAL) != 0 || cd.name == ECS_HASH("eid").hash; };
  return es->componentsRQ.empty() && eastl::all_of(es->componentsRW.begin(), es->componentsRW.end(), isOptPred) &&
         eastl::all_of(es->componentsRO.begin(), es->componentsRO.end(), isOptPred);
}

} // namespace ecs