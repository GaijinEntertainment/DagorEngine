// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_hlsl_floatx.h>
#include "common.h"
#include "../shaders/dagdp_common_placer.hlsli"

namespace dagdp
{

struct PlacerObjectGroup
{
  // This pointer should be safe while RulesBuilder (and anything recursively contained in it, including ObjectGroupInfo) does not
  // change. And it should not change from the point rules were rebuilt, up to a point of rules invalidation, but then we invalidate
  // views as well, so this pointer should not be dereferenced anymore.
  const ObjectGroupInfo *info = nullptr;

  float effectiveDensity = 0.0f;
  Point4 densityMaskChannelWeights = Point4::ZERO;
};

float get_frustum_culling_bias();

struct CommonPlacerBufferInit
{
  // Draw ranges are concatenated across variants, with `drawRangeStartIndex` marking the start of entries for the variant.
  // For a single variant, R float distances are stored, where:
  // - R = number of ranges in the variant.
  dag::RelocatableFixedVector<float, 64, true, framemem_allocator> drawRangesFmem;

  // Renderable indices are concatenated across variants, with `renderableIndicesStartIndex` marking the start of entries for the
  // variant. For a single variant, (P * R) renderable indices (IDs) are stored, where:
  // - P = number of placeables in the variant.
  // - R = number of ranges in the variant.
  dag::RelocatableFixedVector<uint32_t, 64, true, framemem_allocator> renderableIndicesFmem;

  dag::RelocatableFixedVector<VariantGpuData, 16, true, framemem_allocator> variantsFmem;

  struct Group
  {
    const ObjectGroupInfo *info = nullptr;
    float weightFactor = 0.0f;
    Point4 densityMaskChannelWeights = Point4::ZERO;
  };
  dag::RelocatableFixedVector<Group, 64, true, framemem_allocator> objectGroupsFmem;

  uint32_t numPlaceables = 0;
};

struct CommonPlacerBuffers
{
  UniqueBuf drawRangesBuffer;
  UniqueBuf renderableIndicesBuffer;
  UniqueBuf variantsBuffer;
  UniqueBuf placeablesBuffer;
  UniqueBuf placeableWeightsBuffer;
};

void add_variant(
  CommonPlacerBufferInit &init, dag::ConstSpan<PlacerObjectGroup> object_groups, float density, float placeableWeightEmpty);

[[nodiscard]] bool init_common_placer_buffers(
  const CommonPlacerBufferInit &init, eastl::string_view buffer_name_prefix, CommonPlacerBuffers &output);

template <typename T>
bool update_frame_mem(dag::ConstSpan<T> items, Sbuffer *buffer, uint32_t max_size, const char *what)
{
  if (items.size() == 0)
  {
    // frame mem buffers must be updated every frame if shader specifies binding,
    // regardless of logic inside of shader
    T dummyItem = {};
    buffer->updateData(0, sizeof(T), &dummyItem, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    return true;
  }

  if (items.size() > max_size)
  {
    logerr("daGdp: placement exceeded max. number of %s (%u > %u).", what, items.size(), max_size);
    return false;
  }

  if (!buffer->updateData(0, sizeof(T) * items.size(), items.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    logerr("daGdp: %s staging buffer update error.", what);
    return false;
  }

  return true;
}

void debug_draw_volume(const TMatrix &transform, float scale, vec4f extent2, int volume_type);

} // namespace dagdp
