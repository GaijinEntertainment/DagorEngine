// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicResolution.h"
#include <math/integer/dag_IPoint3.h>
#include <frontend/internalRegistry.h>

using namespace dafg;

DynamicResolutionUpdates dafg::collect_dynamic_resolution_updates(const InternalRegistry &registry)
{
  DynamicResolutionUpdates result;
  for (auto [id, dynResType] : registry.autoResTypes.enumerate())
    if (dynResType.dynamicResolutionCountdown > 0)
    {
      eastl::visit(
        [&, id = id](const auto &values) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
            result.set(id, ConcreteDynamicResolutionUpdate{values.dynamicResolution, values.staticResolution});
        },
        dynResType.values);
    }
  return result;
}

void dafg::track_applied_dynamic_resolution_updates(InternalRegistry &registry, const DynamicResolutionUpdates &updates)
{
  for (auto [id, val] : updates.enumerate())
    if (!eastl::holds_alternative<NoDynamicResolutionUpdate>(val))
    {
      --registry.autoResTypes[id].dynamicResolutionCountdown;
      eastl::visit(
        [](auto &values) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
            values.lastAppliedDynamicResolution = values.dynamicResolution;
        },
        registry.autoResTypes[id].values);
    }
}

DynamicResolutions dafg::collect_applied_dynamic_resolutions(const InternalRegistry &registry)
{
  DynamicResolutions dynResolutions;
  for (auto [id, dynResType] : registry.autoResTypes.enumerate())
    eastl::visit(
      [&, id = id](const auto &values) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
          dynResolutions.set(id, values.lastAppliedDynamicResolution);
      },
      dynResType.values);
  return dynResolutions;
}
