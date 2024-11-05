// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bindless.h"
#include "shader.h"

#include <basetexture.h>
#include "texture.h"
#include "render.h"
#include "buffBindPoints.h"
#include "buffers.h"

#include <util/dag_globDef.h>
#include <debug/dag_assert.h>

#include "free_list_utils.h"

using namespace drv3d_metal;

uint32_t BindlessManager::allocateBindlessResourceRange(uint32_t resourceType, uint32_t count)
{
  ResourceArray &res = RES3D_SBUF != resourceType ? textures : buffers;

  auto range = free_list_allocate_smallest_fit<uint32_t>(res.freeSlotRanges, count);
  if (range.empty())
  {
    auto r = res.size;
    res.size += count;
    return r;
  }
  else
  {
    return range.front();
  }
}

uint32_t BindlessManager::resizeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t currentCount, uint32_t newCount)
{
  ResourceArray &res = RES3D_SBUF != resourceType ? textures : buffers;

  if (newCount == currentCount)
  {
    return index;
  }

  uint32_t rangeEnd = index + currentCount;
  if (rangeEnd == res.size)
  {
    // the range is in the end of the heap, so we just update the heap size
    res.size = index + newCount;
    return index;
  }
  if (free_list_try_resize(res.freeSlotRanges, make_value_range(index, currentCount), newCount))
  {
    return index;
  }
  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the existing descriptors :/
  uint32_t newIndex = allocateBindlessResourceRange(resourceType, newCount);

  render.copyBindlessResources(resourceType, index, newIndex, currentCount);

  freeBindlessResourceRange(resourceType, index, currentCount);
  return newIndex;
}

void BindlessManager::freeBindlessResourceRange(uint32_t resourceType, uint32_t index, uint32_t count)
{
  ResourceArray &res = RES3D_SBUF != resourceType ? textures : buffers;

  G_ASSERTF_RETURN(index + count <= res.size, ,
    "metal: freeBindlessResourceRange tried to free out of range slots, range %u - "
    "%u, bindless count %u",
    index, index + count, res.size);

  render.updateBindlessResourcesToNull(resourceType, index, count);

  if (index + count != res.size)
  {
    free_list_insert_and_coalesce(res.freeSlotRanges, make_value_range(index, count));
    return;
  }
  res.size = index;
  if (!res.freeSlotRanges.empty() && (res.freeSlotRanges.back().stop == res.size))
  {
    res.size = res.freeSlotRanges.back().start;
    res.freeSlotRanges.pop_back();
  }
}

uint32_t BindlessManager::registerBindlessSampler(id<MTLSamplerState> sampler)
{
  uint32_t newIndex;
  {
    auto ref = eastl::find(begin(samplerTable), end(samplerTable), sampler);
    if (ref != end(samplerTable))
    {
      return static_cast<uint32_t>(ref - begin(samplerTable));
    }

    newIndex = static_cast<uint32_t>(samplerTable.size());
    samplerTable.push_back(sampler);
  }

  G_ASSERTF(newIndex < Render::BINDLESS_SAMPLER_COUNT, "bindless sampler slot out of range: %d (max slot id: %d)", newIndex, Render::BINDLESS_SAMPLER_COUNT);
  render.bindlessSamplers[newIndex] = sampler;

  if (render.maxBindlessSamplerId < newIndex + 1)
    render.maxBindlessSamplerId = newIndex + 1;

  return newIndex;
}
