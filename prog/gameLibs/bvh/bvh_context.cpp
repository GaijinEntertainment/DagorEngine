// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_context.h"
#include <3d/dag_lockSbuffer.h>
#include <image/dag_texPixel.h>
#include <memory/dag_framemem.h>

namespace bvh
{

void Mesh::teardown(ContextId context_id)
{
  context_id->releaseTexure(albedoTextureId);
  context_id->releaseTexure(alphaTextureId);
  context_id->releaseTexure(normalTextureId);
  context_id->releaseTexure(extraTextureId);
  albedoTextureId = BAD_TEXTUREID;
  alphaTextureId = BAD_TEXTUREID;
  normalTextureId = BAD_TEXTUREID;
  extraTextureId = BAD_TEXTUREID;

  blas.reset();

  if (processedIndices)
  {
    context_id->releaseBuffer(processedIndices.get());
    context_id->moveToDeathrow(eastl::move(processedIndices));
  }
  if (processedVertices)
  {
    context_id->releaseBuffer(processedVertices.get());
    context_id->moveToDeathrow(eastl::move(processedVertices));
  }

  if (metaAllocId >= 0)
    context_id->freeMetaRegion(metaAllocId);
}

MeshMeta &Mesh::createAndGetMeta(ContextId context_id)
{
  if (metaAllocId < 0)
    metaAllocId = context_id->allocateMetaRegion();

  return context_id->meshMetaAllocator.get(metaAllocId);
}

Context::Context()
{
  auto white = TexImage32::create(1, 1, framemem_ptr());
  white->getPixels()->u = 0xFFFFFFFF;
  stubTexture = d3d::create_tex(white, 1, 1, TEXFMT_A8R8G8B8, 1, "bvh_stub_tex");
  memfree(white, framemem_ptr());
}

void Context::teardown()
{
  for (auto &mesh : meshes)
    mesh.second.teardown(this);
  meshes.clear();

  meshMeta.close();
  perInstanceData.close();

  for (auto &lod : terrainLods)
    for (auto &patch : lod.patches)
      patch.teardown(this);
  terrainLods.clear();

  tlasMain.reset();
  tlasTerrain.reset();
  tlasParticles.reset();
  tlasUploadMain.close();
  tlasUploadTerrain.close();
  tlasUploadParticles.close();

  cableVertices.close();
  cableIndices.close();
  cableBLASes.clear();

  atmosphereTexture.close();

  releasePaintTex();
  releaseLandColorTex();

  clearDeathrow();

  G_ASSERT(meshMetaAllocator.allocated() == 0);
  G_ASSERT(bindlessTextureAllocator.allocated() == 0);
  G_ASSERT(bindlessBufferAllocator.allocated() == 0);

  del_d3dres(stubTexture);
}

void Context::releasePaintTex()
{
  if (registeredPaintTexId != BAD_TEXTUREID)
  {
    releaseTexure(registeredPaintTexId);
    registeredPaintTexId = BAD_TEXTUREID;
    registeredPaintTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  }
}

void Context::releaseLandColorTex()
{
  if (registeredLandColorTexId != BAD_TEXTUREID)
  {
    releaseTexure(registeredLandColorTexId);
    registeredLandColorTexId = BAD_TEXTUREID;
    registeredLandColorTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  }
}

void Context::moveToDeathrow(UniqueBVHBufferWithOffset &&buf)
{
  if (buf)
  {
    OSSpinlockScopedLock lock(deathrowLock);
    deathrow.emplace_back(eastl::forward<UniqueBVHBuffer>(buf.buffer));
  }
}

void Context::clearDeathrow()
{
  OSSpinlockScopedLock lock(deathrowLock);
  deathrow.clear();
}

void Context::processDeathrow()
{
  // Delete 10 buffers at most to avoid stuttering, as deleting a buffer
  // is actually a very slow operation.

  OSSpinlockScopedLock lock(deathrowLock);
  for (int i = 0; i < 10 && !deathrow.empty(); ++i)
    deathrow.pop_back();
}

void Context::getDeathRowStats(int &count, int &size)
{
  OSSpinlockScopedLock lock(deathrowLock);
  count = deathrow.size();
  size = 0;
  for (const auto &buf : deathrow)
    size += buf->ressize();
}

MeshMetaAllocator::AllocId Context::allocateMetaRegion()
{
  OSSpinlockScopedLock lock(meshMetaAllocatorLock);
  int id = meshMetaAllocator.allocate();
  auto &meta = meshMetaAllocator.get(id);
  meta = MeshMeta();
  return id;
}

void Context::freeMetaRegion(MeshMetaAllocator::AllocId &id)
{
  OSSpinlockScopedLock lock(meshMetaAllocatorLock);
  meshMetaAllocator.free(id);
  id = -1;
}

Texture *Context::holdTexture(TEXTUREID id, uint32_t &texture_and_sampler_bindless_index, d3d::SamplerHandle sampler,
  bool forceRefreshSrvsWhenLoaded)
{
  if (id != BAD_TEXTUREID)
  {
    auto result = usedTextures.insert(id);
    ++result.first->second.referenceCount;
    if (result.second)
    {
      // This is the initial allocation of the texture.
      result.first->second.texture = acquire_managed_tex(id);
      if (!result.first->second.texture)
      {
        logwarn("BVH Context::holdTexture got a nullptr texture from acquire_managed_tex for id: %u", id.index());
        result.first->second.texture = stubTexture;
      }
      result.first->second.allocId = bindlessTextureAllocator.allocate(result.first->second.texture);
      if (sampler == d3d::INVALID_SAMPLER_HANDLE)
        result.first->second.samplerIndex = d3d::register_bindless_sampler(result.first->second.texture);
      else
        result.first->second.samplerIndex = d3d::register_bindless_sampler(sampler);

      if (forceRefreshSrvsWhenLoaded && !check_managed_texture_loaded(id))
        texturesWaitingForLoad.insert(id);
    }

    texture_and_sampler_bindless_index = (uint32_t(result.first->second.allocId) << 16) | result.first->second.samplerIndex;

    return result.first->second.texture;
  }

  return nullptr;
}

bool Context::releaseTexure(TEXTUREID id)
{
  if (id == BAD_TEXTUREID)
    return false;

  auto iter = usedTextures.find(id);
  G_ASSERT_RETURN(iter != usedTextures.end(), false);
  if (--iter->second.referenceCount == 0)
  {
    bindlessTextureAllocator.free(iter->second.allocId);
    usedTextures.erase(iter);
    release_managed_tex(id);
    texturesWaitingForLoad.erase(id);
    return true;
  }

  return false;
}

void Context::markChangedTextures()
{
  for (auto it = texturesWaitingForLoad.begin(); it != texturesWaitingForLoad.end();)
  {
    if (!check_managed_texture_loaded(*it, true))
    {
      // using the texture ensures it will eventually be fully loaded
      mark_managed_tex_lfu(*it);
      it++;
      continue;
    }

    // the texture loaded new mip levels so its srv must be updated
    if (auto usedTexIt = usedTextures.find(*it); usedTexIt != usedTextures.end())
      bindlessTextureAllocator.getHeap().set(usedTexIt->second.allocId, usedTexIt->second.texture);

    it = texturesWaitingForLoad.erase(it);
  }
}

void Context::holdBuffer(Sbuffer *buffer, uint32_t &bindless_index)
{
  if (buffer)
  {
    G_ASSERT(buffer->getFlags() & SBCF_BIND_SHADER_RES);
    G_ASSERT(!strstr(buffer->getResName(), "united"));

    auto result = usedBuffers.insert(buffer);
    ++result.first->second.referenceCount;

    if (result.second)
      result.first->second.allocId = bindlessBufferAllocator.allocate(buffer);

    bindless_index = result.first->second.allocId;
  }
}

bool Context::releaseBuffer(Sbuffer *buffer)
{
  if (!buffer)
    return false;

  auto iter = usedBuffers.find(buffer);
  G_ASSERT_RETURN(iter != usedBuffers.end(), false);
  if (--iter->second.referenceCount == 0)
  {
    bindlessBufferAllocator.free(iter->second.allocId);
    usedBuffers.erase(iter);
    return true;
  }

  return false;
}

Context::BLASCompaction *Context::beginBLASCompaction()
{
  if (!ENABLE_BLAS_COMPACTION)
    return nullptr;

  auto buffer = d3d::buffers::create_ua_structured_readback(8, 1, "compacted_size");
  HANDLE_LOST_DEVICE_STATE(buffer, nullptr);

  Context::BLASCompaction &compaction = blasCompactions.push_back();
  compaction.compactedSize = buffer;
  compaction.query.reset(d3d::create_event_query());
  compaction.stage = Context::BLASCompaction::Stage::Prepared;
  return &compaction;
}

Context::BLASCompaction *Context::beginBLASCompaction(uint64_t mesh_id)
{
  auto compaction = beginBLASCompaction();
  if (compaction)
    compaction->meshId = mesh_id;
  return compaction;
}

void Context::removeFromCompaction(uint64_t mesh_id)
{
  auto citer = eastl::find_if(blasCompactions.begin(), blasCompactions.end(),
    [mesh_id](const auto &compaction) { return compaction.meshId == mesh_id; });
  if (citer != blasCompactions.end())
  {
    del_d3dres(citer->compactedSize);
    blasCompactions.erase(citer);
  }
}

void TerrainPatch::teardown(ContextId context_id)
{
  G_VERIFY(context_id->releaseBuffer(vertices.get()));
  vertices.reset();

  context_id->freeMetaRegion(metaAllocId);
}

} // namespace bvh