// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <3d/dag_lockSbuffer.h>
#include "placer.h"
#include "../shaders/dagdp_common.hlsli"

using TmpName = eastl::fixed_string<char, 128>;

namespace dagdp
{

CONSOLE_FLOAT_VAL("dagdp", frustum_culling_bias, 0.0f);

struct Transition
{
  PlaceableId pId;
  RenderableId rId;
  float drawDistance;
};

void calculate_draw_ranges(CommonPlacerBufferInit &init, dag::ConstSpan<PlacerObjectGroup> object_groups)
{
  size_t maxTransitions = 0;
  for (const auto &objectGroup : object_groups)
    for (const auto &placeable : objectGroup.info->placeables)
      maxTransitions += placeable.ranges.size();

  dag::VectorSet<float, eastl::less<float>, framemem_allocator> transitionPoints;
  dag::Vector<Transition, framemem_allocator> transitions;
  transitionPoints.reserve(maxTransitions);
  transitions.reserve(maxTransitions);

  uint32_t pId = init.numPlaceables;
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

  init.drawRangesFmem.insert(init.drawRangesFmem.end(), transitionPoints.begin(), transitionPoints.end());

  const uint32_t numPlaceables = pId - init.numPlaceables;
  const uint32_t numDrawRanges = transitionPoints.size();
  const uint32_t renderableIndexStart = init.renderableIndicesFmem.size();

  init.renderableIndicesFmem.resize(renderableIndexStart + numDrawRanges * numPlaceables);
  for (uint32_t i = renderableIndexStart; i < init.renderableIndicesFmem.size(); ++i)
    init.renderableIndicesFmem[i] = ~0u;

  dag::Vector<uint32_t, framemem_allocator> currentRangeIndices(numPlaceables);

  for (const auto &transition : transitions)
  {
    const uint32_t pIdOffset = transition.pId - init.numPlaceables;
    while (
      currentRangeIndices[pIdOffset] < numDrawRanges && transition.drawDistance >= transitionPoints[currentRangeIndices[pIdOffset]])
    {
      init.renderableIndicesFmem[renderableIndexStart + currentRangeIndices[pIdOffset] * numPlaceables + pIdOffset] = transition.rId;
      ++currentRangeIndices[pIdOffset];
    }
  }
}

void add_variant(
  CommonPlacerBufferInit &init, dag::ConstSpan<PlacerObjectGroup> object_groups, float density, float placeableWeightEmpty)
{
  auto &gpuData = init.variantsFmem.push_back();

  uint32_t numVariantPlaceables = 0;
  for (const auto &objectGroup : object_groups)
  {
    numVariantPlaceables += objectGroup.info->placeables.size();
    auto &item = init.objectGroupsFmem.push_back();
    item.info = objectGroup.info;
    item.weightFactor = (1.0 - placeableWeightEmpty) * (objectGroup.effectiveDensity / density);
  }

  // TODO: rename placeableWeightEmpty.
  gpuData.placeableWeightEmpty = placeableWeightEmpty;
  gpuData.placeableStartIndex = init.numPlaceables;
  gpuData.placeableCount = numVariantPlaceables;
  gpuData.placeableEndIndex = init.numPlaceables + numVariantPlaceables;
  gpuData.drawRangeStartIndex = init.drawRangesFmem.size();
  gpuData.renderableIndicesStartIndex = init.renderableIndicesFmem.size();

  calculate_draw_ranges(init, object_groups);
  gpuData.drawRangeEndIndex = init.drawRangesFmem.size();

  init.numPlaceables += numVariantPlaceables;
}

bool init_common_placer_buffers(const CommonPlacerBufferInit &init, eastl::string_view buffer_name_prefix, CommonPlacerBuffers &output)
{
  {
    TmpName bufferName(TmpName::CtorSprintf(), "%.*s_draw_ranges", buffer_name_prefix.size(), buffer_name_prefix.data());
    output.drawRangesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(float), init.drawRangesFmem.size(), bufferName.c_str());

    bool updated =
      output.drawRangesBuffer->updateData(0, data_size(init.drawRangesFmem), init.drawRangesFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return false;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%.*s_renderable_indices", buffer_name_prefix.size(), buffer_name_prefix.data());
    output.renderableIndicesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), init.renderableIndicesFmem.size(), bufferName.c_str());

    bool updated = output.renderableIndicesBuffer->updateData(0, data_size(init.renderableIndicesFmem),
      init.renderableIndicesFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return false;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%.*s_variants", buffer_name_prefix.size(), buffer_name_prefix.data());
    output.variantsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(VariantGpuData), init.variantsFmem.size(), bufferName.c_str());

    bool updated = output.variantsBuffer->updateData(0, data_size(init.variantsFmem), init.variantsFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return false;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%.*s_placeables", buffer_name_prefix.size(), buffer_name_prefix.data());
    output.placeablesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(PlaceableGpuData), init.numPlaceables, bufferName.c_str());

    auto lockedBuffer = lock_sbuffer<PlaceableGpuData>(output.placeablesBuffer.getBuf(), 0, init.numPlaceables, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return false;
    }

    uint32_t i = 0;
    for (const auto &objectGroup : init.objectGroupsFmem)
      for (const auto &placeable : objectGroup.info->placeables)
      {
        auto &item = lockedBuffer[i++];
        const auto &params = placeable.params;

        item.yawRadiansMin = md_min(params.yawRadiansMidDev);
        item.yawRadiansMax = md_max(params.yawRadiansMidDev);
        item.pitchRadiansMin = md_min(params.pitchRadiansMidDev);
        item.pitchRadiansMax = md_max(params.pitchRadiansMidDev);
        item.rollRadiansMin = md_min(params.rollRadiansMidDev);
        item.rollRadiansMax = md_max(params.rollRadiansMidDev);
        item.scaleMin = md_min(params.scaleMidDev);
        item.scaleMax = md_max(params.scaleMidDev);
        item.maxBaseDrawDistance = placeable.ranges.back().baseDrawDistance;
        item.slopeFactor = params.slopeFactor;
        item.flags = params.flags;
        item.riPoolOffset = params.riPoolOffset;
      }

    G_ASSERT(i == init.numPlaceables);
  }

  {
    // TODO: these are not weights, but probabilities, and should be renamed for clarity.
    TmpName bufferName(TmpName::CtorSprintf(), "%.*s_placeable_weights", buffer_name_prefix.size(), buffer_name_prefix.data());
    output.placeableWeightsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(float), init.numPlaceables, bufferName.c_str());

    auto lockedBuffer = lock_sbuffer<float>(output.placeableWeightsBuffer.getBuf(), 0, init.numPlaceables, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return false;
    }

    uint32_t i = 0;
    for (const auto &objectGroup : init.objectGroupsFmem)
      for (const auto &placeable : objectGroup.info->placeables)
        lockedBuffer[i++] = placeable.params.weight * objectGroup.weightFactor;

    G_ASSERT(i == init.numPlaceables);
  }

  return true;
}

float get_frustum_culling_bias() { return frustum_culling_bias.get(); }

} // namespace dagdp
