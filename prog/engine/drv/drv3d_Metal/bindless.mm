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
#include <drv_assert_defs.h>

#include "free_list_utils.h"

using namespace drv3d_metal;

static void resizeBindlessArrays(D3DResourceType type, uint32_t count)
{
  if (D3DResourceType::SBUF == type)
  {
    render.bindlessBuffers.resize(count);
    G_ASSERTF(count <= Render::BINDLESS_BUFFER_COUNT, "bindless buffer slot out of range: %d (max slot id: %d)", count, Render::BINDLESS_BUFFER_COUNT);
  }
  else
  {
    if (type == D3DResourceType::TEX)
      render.bindlessTextures2D.resize(count);
    else if (type == D3DResourceType::CUBETEX)
      render.bindlessTexturesCube.resize(count);
    else if (type == D3DResourceType::ARRTEX)
      render.bindlessTextures2DArray.resize(count);
    else
      G_ASSERTF(0, "Unknown bindless array type %d", int(type));
    G_ASSERTF(count <= Render::BINDLESS_TEXTURE_COUNT, "bindless texture slot out of range: %d (max slot id: %d)", count, Render::BINDLESS_TEXTURE_COUNT);
  }
}

uint32_t BindlessManager::allocateBindlessResourceRange(D3DResourceType type, uint32_t count)
{
  G_ASSERT(D3DResourceType::TEX == type || D3DResourceType::CUBETEX == type || D3DResourceType::ARRTEX == type || D3DResourceType::SBUF == type);

  ResourceArray &res = getArray(type);

  auto range = free_list_allocate_smallest_fit<uint32_t>(res.freeSlotRanges, count);
  if (range.empty())
  {
    auto r = res.size;
    res.size += count;
    resizeBindlessArrays(type, res.size);
    return r;
  }
  else
  {
    return range.front();
  }
}

uint32_t BindlessManager::resizeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t currentCount, uint32_t newCount)
{
  G_ASSERT(D3DResourceType::TEX == type || D3DResourceType::CUBETEX == type || D3DResourceType::ARRTEX == type || D3DResourceType::SBUF == type);

  ResourceArray &res = getArray(type);

  if (newCount == currentCount)
  {
    return index;
  }

  uint32_t rangeEnd = index + currentCount;
  if (rangeEnd == res.size)
  {
    // the range is in the end of the heap, so we just update the heap size
    res.size = index + newCount;
    resizeBindlessArrays(type, res.size);
    return index;
  }
  if (free_list_try_resize(res.freeSlotRanges, make_value_range(index, currentCount), newCount))
  {
    return index;
  }
  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the existing descriptors :/
  uint32_t newIndex = allocateBindlessResourceRange(type, newCount);

  render.copyBindlessResources(type, index, newIndex, currentCount);

  freeBindlessResourceRange(type, index, currentCount);
  return newIndex;
}

void BindlessManager::freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count)
{
  G_ASSERT(D3DResourceType::TEX == type || D3DResourceType::CUBETEX == type || D3DResourceType::ARRTEX == type || D3DResourceType::SBUF == type);

  ResourceArray &res = getArray(type);

  G_ASSERTF_RETURN(index + count <= res.size, ,
    "metal: freeBindlessResourceRange tried to free out of range slots, range %u - "
    "%u, bindless count %u",
    index, index + count, res.size);

  render.acquireOwnership();
  render.updateBindlessResourcesToNull(type, index, count);
  render.releaseOwnership();

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

uint32_t BindlessManager::registerBindlessSampler(int index, float bias)
{
  D3D_CONTRACT_ASSERTF_RETURN(index >= 0, 0, "Trying to register an invalid bindless sampler");

  id<MTLSamplerState> sampler = render.sampler_states[index].sampler;

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
  if (render.bindlessSamplers.size() < newIndex + 1)
  {
    render.bindlessSamplers.resize(newIndex + 1);
    render.bindlessSamplersCache.resize(newIndex + 1);
  }
  render.bindlessSamplers[newIndex] = sampler;
  if (@available(iOS 16, macOS 13.0, *))
  {
    MTLResourceID res = sampler.gpuResourceID;
    memcpy(&render.bindlessSamplersCache[newIndex], &res, sizeof(res));
  }
  render.bindless_resources_bound &= ~BindlessTypeSampler;

  return newIndex;
}
