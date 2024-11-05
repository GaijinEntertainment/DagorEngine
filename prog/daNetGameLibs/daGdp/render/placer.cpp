// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "placer.h"

namespace dagdp
{

struct Transition
{
  PlaceableId pId;
  RenderableId rId;
  float drawDistance;
};

void calculate_draw_ranges(uint32_t p_id_start,
  dag::ConstSpan<PlacerObjectGroup> object_groups,
  DrawRangesFmem &draw_ranges,
  RenderableIndicesFmem &renderable_indices)
{
  size_t maxTransitions = 0;
  for (const auto &objectGroup : object_groups)
    for (const auto &placeable : objectGroup.info->placeables)
      maxTransitions += placeable.ranges.size();

  dag::VectorSet<float, eastl::less<float>, framemem_allocator> transitionPoints;
  dag::Vector<Transition, framemem_allocator> transitions;
  transitionPoints.reserve(maxTransitions);
  transitions.reserve(maxTransitions);

  uint32_t pId = p_id_start;
  for (const auto &objectGroup : object_groups)
  {
    for (const auto &placeable : objectGroup.info->placeables)
    {
      for (const auto &range : placeable.ranges)
      {
        transitions.push_back({pId, range.rId, range.baseDrawDistance});

        // TODO: We could "cluster" similar draw distance values to prevent
        // excessive amounts of unique ranges from being created...
        transitionPoints.insert(range.baseDrawDistance);
      }

      ++pId;
    }
  }

  stlsort::sort(transitions.begin(), transitions.end(), [](const Transition &a, const Transition &b) {
    if (a.drawDistance != b.drawDistance)
      return a.drawDistance < b.drawDistance;

    if (a.pId != b.pId)
      return a.pId < b.pId;

    G_ASSERTF(false, "Can't have a transition with same distance/pId and different rId.");
    return a.rId < b.rId;
  });

  draw_ranges.insert(draw_ranges.end(), transitionPoints.begin(), transitionPoints.end());

  const uint32_t numPlaceables = pId - p_id_start;
  const uint32_t numDrawRanges = transitionPoints.size();
  const uint32_t renderableIndexStart = renderable_indices.size();

  renderable_indices.resize(renderableIndexStart + numDrawRanges * numPlaceables);
  for (uint32_t i = renderableIndexStart; i < renderable_indices.size(); ++i)
    renderable_indices[i] = ~0u;

  dag::Vector<uint32_t, framemem_allocator> currentRangeIndices(numPlaceables);

  for (const auto &transition : transitions)
  {
    const uint32_t pIdOffset = transition.pId - p_id_start;
    while (
      currentRangeIndices[pIdOffset] < numDrawRanges && transition.drawDistance >= transitionPoints[currentRangeIndices[pIdOffset]])
    {
      renderable_indices[renderableIndexStart + currentRangeIndices[pIdOffset] * numPlaceables + pIdOffset] = transition.rId;
      ++currentRangeIndices[pIdOffset];
    }
  }
}

} // namespace dagdp
