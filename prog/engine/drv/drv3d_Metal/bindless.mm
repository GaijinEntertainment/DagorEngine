// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bindless.h"
#include "shader.h"

#include <basetexture.h>
#include "texture.h"
#include "render.h"
#include "buffBindPoints.h"
#include "buffers.h"

#include <drv/3d/dag_bindless.h>
#include <util/dag_globDef.h>
#include <debug/dag_assert.h>
#include <debug/dag_fatal.h>
#include <drv_assert_defs.h>
#include "drv_log_defs.h"
#include <osApiWrappers/dag_critSec.h>

#include "free_list_utils.h"

using namespace drv3d_metal;

// d3d:: calls that allocate/resize/free bindless ranges have no external sync requirement, so do sync internally.
static WinCritSec rangesMutex;

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
  WinAutoLock lock(rangesMutex);
  return allocateBindlessResourceRangeNoLock(type, count);
}

uint32_t BindlessManager::allocateBindlessResourceRangeNoLock(D3DResourceType type, uint32_t count)
{
  G_ASSERT(D3DResourceType::TEX == type || D3DResourceType::CUBETEX == type || D3DResourceType::ARRTEX == type || D3DResourceType::SBUF == type);

  ResourceArray &res = getArray(type);

  auto range = free_list_allocate_smallest_fit<uint32_t>(res.freeSlotRanges, count);
  if (range.empty())
  {
    auto r = res.size;
    if (r + count > ::bindless::MAX_RESOURCE_INDEX_COUNT)
    {
      D3D_CONTRACT_ERROR("Metal: Out of bindless slots, asked for %u while limit is %u and %u is already allocated", count,
        ::bindless::MAX_RESOURCE_INDEX_COUNT, r);
      DAG_FATAL("Metal: Critical D3D contract violation, out of bindless slots, can not continue");
      return 0;
    }
    res.size += count;
    resizeBindlessArrays(type, res.size);
    return r;
  }
  else
  {
    return range.front();
  }
}

void BindlessManager::freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count)
{
  WinAutoLock lock(rangesMutex);
  freeBindlessResourceRangeNoLock(type, index, count);
}

void BindlessManager::freeBindlessResourceRangeNoLock(D3DResourceType type, uint32_t index, uint32_t count)
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
  if (render.bindlessSamplerBiases.size() < newIndex + 1)
  {
    render.bindlessSamplerBiases.resize(newIndex + 1);
    render.bindlessSamplersCache.resize(newIndex + 1);
  }
  render.bindlessSamplerBiases[newIndex] = bias;
  if (@available(iOS 16, macOS 13.0, *))
  {
    MTLResourceID res = sampler.gpuResourceID;
    memcpy(&render.bindlessSamplersCache[newIndex], &res, sizeof(res));
  }
  render.bindless_resources_bound &= ~BindlessTypeSampler;

  return newIndex;
}
