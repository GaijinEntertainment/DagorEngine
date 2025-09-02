// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "badResolutionTracker.h"

#include <drv/3d/dag_info.h>
#include <common/genericPoint.h>
#include <common/dynamicResolution.h>

namespace dafg
{

static bool is_autoresolution_resource(const intermediate::Resource &res)
{
  if (!res.isScheduled())
    return false;
  if (res.asScheduled().resourceType != ResourceType::Texture)
    return false;
  if (!res.asScheduled().resolutionType.has_value())
    return false;

  return true;
}

void BadResolutionTracker::filterOutBadResolutions(DynamicResolutionUpdates &updates, int current_frame)
{
  if (!d3d::get_driver_desc().caps.hasResourceHeaps)
    return;

  const auto nextFrame = (current_frame + 1) % SCHEDULE_FRAME_WINDOW;

  // Corrections are applied in 2 steps:
  // - First, only to resources created in the upcoming frame, which happens
  //   below and allows us not to resize preserved history.
  // - Second, we propagate corrections to the "next next" frame here.
  for (auto &[_, correction] : correctedSizes)
    correction.forFrame[nextFrame] = max(correction.forFrame[nextFrame], correction.forFrame[current_frame]);

  for (auto [idx, res] : graph.resources.enumerate())
  {
    if (!is_autoresolution_resource(res))
      continue;

    const auto &resolutionType = res.asScheduled().resolutionType;

    if (!updates.isMapped(resolutionType->id) || eastl::holds_alternative<NoDynamicResolutionUpdate>(updates[resolutionType->id]))
      continue;

    const auto &desc = res.asScheduled().getGpuDescription();
    G_ASSERT(desc.type != D3DResourceType::SBUF);
    const bool automip = res.asScheduled().autoMipCount;

    const auto [dynamicDesc, staticDesc] = eastl::visit(
      [&](const auto &reso) -> eastl::pair<ResourceDescription, ResourceDescription> {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(reso)>, NoDynamicResolutionUpdate>)
          return {
            set_tex_desc_resolution(desc, scale_by(reso.dynamicRes, resolutionType->multiplier), automip),
            set_tex_desc_resolution(desc, scale_by(reso.staticRes, resolutionType->multiplier), automip),
          };
        G_ASSERT(false); // impossible situation
        return {};
      },
      updates[resolutionType->id]);

    const size_t dynamicSize = sizeFor(dynamicDesc);
    const auto it = correctedSizes.find(staticDesc);
    const size_t staticSize = it != correctedSizes.end() ? it->second.forFrame[nextFrame] : sizeFor(staticDesc);

    if (dynamicSize <= staticSize)
      continue;

    debug("daFG: skipping dynamic resolution update for auto-res ID %d"
          " as it would overflow the size from %zu (static) to %zu (dynamic) for resource '%s'",
      eastl::to_underlying(resolutionType->id), staticSize, dynamicSize, graph.resourceNames[idx].c_str());

    // At first, corrections are only applied to the upcoming frame's resources.
    auto &correction = correctedSizes[staticDesc];
    correction.forFrame[nextFrame] = dynamicSize;
    updates[resolutionType->id] = NoDynamicResolutionUpdate{};
    reschedulingCountdown = SCHEDULE_FRAME_WINDOW;
  }
}

size_t BadResolutionTracker::sizeFor(const ResourceDescription &desc)
{
  const auto driverGetter = [&desc]() { return d3d::get_resource_allocation_properties(desc).sizeInBytes; };
  return texSizeCache.access(desc, driverGetter);
}

BadResolutionTracker::Corrections BadResolutionTracker::getTexSizeCorrections() const
{
  Corrections result;
  for (auto &arr : result)
    arr.resize(graph.resources.size(), 0);

  if (!d3d::get_driver_desc().caps.hasResourceHeaps)
    return result;

  for (auto [idx, res] : graph.resources.enumerate())
  {
    if (!is_autoresolution_resource(res))
      continue;

    if (auto it = correctedSizes.find(res.asScheduled().getGpuDescription()); it != correctedSizes.end())
      for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
        result[f][idx] = it->second.forFrame[f];
  }

  return result;
}


} // namespace dafg
