// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"

namespace dagdp
{

struct PlacerObjectGroup
{
  // This pointer should be safe while RulesBuilder (and anything recursively contained in it, including ObjectGroupInfo) does not
  // change. And it should not change from the point rules were rebuilt, up to a point of rules invalidation, but then we invalidate
  // views as well, so this pointer should not be dereferenced anymore.
  const ObjectGroupInfo *info = nullptr;

  float effectiveDensity = 0.0f;
};

using DrawRangesFmem = dag::RelocatableFixedVector<float, 64, true, framemem_allocator>;
using RenderableIndicesFmem = dag::RelocatableFixedVector<uint32_t, 64, true, framemem_allocator>;

// Should add elements to `draw_ranges` and `renderable_indices`.
// Number of elements added must be `R` and `R * P` respectively, where R is the resulting number of ranges, and P is the number of
// placeables.
void calculate_draw_ranges(uint32_t p_id_start,
  dag::ConstSpan<PlacerObjectGroup> object_groups,
  DrawRangesFmem &draw_ranges,
  RenderableIndicesFmem &renderable_indices);

} // namespace dagdp
