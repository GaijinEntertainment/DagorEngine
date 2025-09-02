// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_context.h"
#include <3d/dag_lockSbuffer.h>
#include <image/dag_texPixel.h>
#include <memory/dag_framemem.h>
#include <generic/dag_enumerate.h>

namespace bvh
{

struct DeathrowJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "process_bvh_deathrow"; }
  void doJob() override
  {
#if _TARGET_C2

#else
    // Delete only a few buffers at most to avoid stuttering, as deleting a buffer
    // is actually a very slow operation.
    static constexpr uint32_t BUFFER_LIMIT = 15;
#endif

    decltype(contextId->deathrow) workingSet;

    // Deleting the buffers is slow, so under the lock, lets move the items to be deleted into a separate container.
    // Then after unlocking the main container, the clear will actually delete the buffers without holding the lock.
    {
      OSSpinlockScopedLock lock(contextId->deathrowLock);
      int delCount = min(BUFFER_LIMIT, contextId->deathrow.size());
      DA_PROFILE_TAG_DESC(getJobNameProfDesc(), "%d of %d", delCount, contextId->deathrow.size());

      if (contextId->deathrow.empty())
        return;

      workingSet.resize(delCount);
      for (int i = 0; i < delCount; ++i)
      {
        workingSet[i] = eastl::move(contextId->deathrow.back());
        contextId->deathrow.pop_back();
      }
    }

    workingSet.clear();
  }

  void start(ContextId context_id)
  {
    threadpool::wait(this);
    contextId = context_id;
    threadpool::add(this, threadpool::JobPriority::PRIO_LOW);
  }
} deathrowJob;

void Object::teardown(ContextId context_id)
{
  blas.reset();
  for (auto &mesh : meshes)
    mesh.teardown(context_id);

  if (metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
    context_id->freeMetaRegion(metaAllocId);
}

dag::Span<MeshMeta> Object::createAndGetMeta(ContextId context_id, int size)
{
  if (metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
    metaAllocId = context_id->allocateMetaRegion(size);

  return context_id->meshMetaAllocator.get(metaAllocId);
}

void Mesh::teardown(ContextId context_id)
{
  TIME_PROFILE(bvh::Mesh::teardown);

  context_id->releaseTexure(albedoTextureId);
  context_id->releaseTexure(alphaTextureId);
  context_id->releaseTexure(normalTextureId);
  context_id->releaseTexure(extraTextureId);
  albedoTextureId = BAD_TEXTUREID;
  alphaTextureId = BAD_TEXTUREID;
  normalTextureId = BAD_TEXTUREID;
  extraTextureId = BAD_TEXTUREID;

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
  if (ahsVertices)
  {
    context_id->releaseBuffer(ahsVertices.get());
    context_id->moveToDeathrow(eastl::move(ahsVertices));
  }
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
  threadpool::wait(&deathrowJob);

  for (auto &object : objects)
    object.second.teardown(this);
  objects.clear();

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

  instanceDescsCpu.clear();
  instanceDescsCpu.shrink_to_fit();
  perInstanceDataCpu.clear();
  perInstanceDataCpu.shrink_to_fit();

  compactedSizeBufferCache.clear();
  pendingCompactedSizeBuffersCache.clear();

  releaseAllBindlessTexHolders();

  clearDeathrow();
  G_ASSERT(meshMetaAllocator.allocated() == 0);
#if DAGOR_DBGLEVEL > 0
  for (auto [ptr, allocatorName] : bindlessBufferAllocatorNames)
    logwarn("[BVH] Context::teardown: buffer %p (%s) was not released", ptr, allocatorName.c_str());
  for (size_t ix = 0; ix < bindlessBufferAllocator.size(); ++ix)
    if (auto elem = bindlessBufferAllocator.get(ix))
    {
      logwarn("[BVH] Context::teardown: buffer %p was not released", elem);
      logwarn("[BVH] Context::teardown: name: %s, size: %u", elem->getName(), elem->getSize());
    }
  for (size_t ix = 0; ix < bindlessTextureAllocator.size(); ++ix)
    if (auto elem = bindlessTextureAllocator.get(ix))
    {
      logwarn("[BVH] Context::teardown: texture %p was not released", elem);
      logwarn("[BVH] Context::teardown: name: %s, size: %u", elem->getName(), elem->getSize());
    }
  for (size_t ix = 0; ix < bindlessCubeTextureAllocator.size(); ++ix)
    if (auto elem = bindlessCubeTextureAllocator.get(ix))
    {
      logwarn("[BVH] Context::teardown: cube texture %p was not released", elem);
      logwarn("[BVH] Context::teardown: name: %s, size: %u", elem->getName(), elem->getSize());
    }
#endif

  del_d3dres(stubTexture);
}

void Context::releaseAllBindlessTexHolders()
{
  paint_details_texBindless.close(this);
  grass_land_color_maskBindless.close(this);
  dynamic_mfd_texBindless.close(this);
  cache_tex0Bindless.close(this);
  indirection_texBindless.close(this);
  cache_tex1Bindless.close(this);
  cache_tex2Bindless.close(this);
  last_clip_texBindless.close(this);
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

void Context::processDeathrow() { deathrowJob.start(this); }

void Context::getDeathRowStats(int &count, int64_t &size)
{
  OSSpinlockScopedLock lock(deathrowLock);
  count = deathrow.size();
  size = 0;
  for (const auto &buf : deathrow)
    size += buf->getSize();
}

MeshMetaAllocator::AllocId Context::allocateMetaRegion(int size)
{
  OSSpinlockScopedLock lock(meshMetaAllocatorLock);
  auto id = meshMetaAllocator.allocate(size);
  for (auto &meta : meshMetaAllocator.get(id))
    meta = MeshMeta();
  return id;
}

void Context::freeMetaRegion(MeshMetaAllocator::AllocId &id)
{
  OSSpinlockScopedLock lock(meshMetaAllocatorLock);
  meshMetaAllocator.free(id);
  id = MeshMetaAllocator::INVALID_ALLOC_ID;
}

Texture *Context::holdTexture(TEXTUREID id, uint32_t &texture_bindless_index, uint32_t &sampler_bindless_index,
  d3d::SamplerHandle sampler, bool forceRefreshSrvsWhenLoaded)
{
  texture_bindless_index = MeshMeta::INVALID_TEXTURE;
  sampler_bindless_index = MeshMeta::INVALID_SAMPLER;
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
      if (result.first->second.texture->getType() == D3DResourceType::TEX)
        result.first->second.allocId = bindlessTextureAllocator.allocate(result.first->second.texture);
      else if (result.first->second.texture->getType() == D3DResourceType::CUBETEX)
        result.first->second.allocId = bindlessCubeTextureAllocator.allocate(result.first->second.texture);
      else
        G_ASSERT(0 && "Unknown res");
      // @TODO: try replace with some validation instead of silent stubbing
      if (sampler == d3d::INVALID_SAMPLER_HANDLE)
        sampler = d3d::request_sampler({});

      result.first->second.samplerIndex = d3d::register_bindless_sampler(sampler);

      if (forceRefreshSrvsWhenLoaded && !check_managed_texture_loaded(id))
        texturesWaitingForLoad.insert(id);
    }

    texture_bindless_index = uint32_t(BindlessTextureAllocator::decode(result.first->second.allocId));
    sampler_bindless_index = result.first->second.samplerIndex;

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
    G_ASSERT(iter->second.texture);
    if (iter->second.texture->getType() == D3DResourceType::TEX)
      bindlessTextureAllocator.free(iter->second.allocId);
    else if (iter->second.texture->getType() == D3DResourceType::CUBETEX)
      bindlessCubeTextureAllocator.free(iter->second.allocId);
    else
      G_ASSERT(0 && "Unknown texture type");
    usedTextures.erase(iter);
    release_managed_tex(id);
    texturesWaitingForLoad.erase(id);
    return true;
  }

  return false;
}

bool Context::releaseTextureFromPackedIndices(uint32_t texture_bindless_indices)
{
  if (texture_bindless_indices == MeshMeta::INVALID_TEXTURE)
    return false;

  const uint16_t allocIndex = texture_bindless_indices;
  auto tex = bindlessTextureAllocator.get(allocIndex);

  return tex && releaseTexure(tex->getTID());
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
    {
      if (usedTexIt->second.texture->getType() == D3DResourceType::TEX)
        bindlessTextureAllocator.getHeap().set(BindlessTextureAllocator::decode(usedTexIt->second.allocId), usedTexIt->second.texture);
      else if (usedTexIt->second.texture->getType() == D3DResourceType::CUBETEX)
        bindlessCubeTextureAllocator.getHeap().set(BindlessCubeTextureAllocator::decode(usedTexIt->second.allocId),
          usedTexIt->second.texture);
      else
        G_ASSERT(0 && "Unknown texture type");
    }

    it = texturesWaitingForLoad.erase(it);
  }
}

void Context::holdBuffer(Sbuffer *buffer, uint32_t &bindless_index)
{
  if (buffer)
  {
    G_ASSERT(buffer->getFlags() & SBCF_BIND_SHADER_RES);
    G_ASSERT(!strstr(buffer->getName(), "united"));

    auto result = usedBuffers.insert(buffer);
    ++result.first->second.referenceCount;

    if (result.second)
    {
      result.first->second.allocId = bindlessBufferAllocator.allocate(buffer);
#if DAGOR_DBGLEVEL > 0
      bindlessBufferAllocatorNames[buffer] = buffer->getName();
#endif
    }

    bindless_index = BindlessBufferAllocator::decode(result.first->second.allocId);
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

#if DAGOR_DBGLEVEL > 0
    bindlessBufferAllocatorNames.erase(buffer);
#endif

    return true;
  }

  return false;
}

Context::BLASCompaction *Context::beginBLASCompaction(uint64_t object_id)
{
  if (!is_blas_compaction_enabled())
    return nullptr;

  Sbuffer *buffer = nullptr;
#if !_TARGET_C2
  if (compactedSizeBufferCache.empty())
  {
    for (PendingCompactSizeBuffer &i : pendingCompactedSizeBuffersCache)
    {
      if (d3d::get_event_query_status(i.query.get(), false))
      {
        compactedSizeBufferCache.push_back();
        compactedSizeBufferCache.back().swap(i.buf);
        i.query.reset();
      }
      else
      {
        // once we hit still non completed buffers, following ones will be not completed too
        pendingCompactedSizeBuffersCache.erase(pendingCompactedSizeBuffersCache.begin(), &i);
        break;
      }
    }
  }

  if (!compactedSizeBufferCache.empty())
  {
    buffer = compactedSizeBufferCache.back().release();
    compactedSizeBufferCache.pop_back();
  }
  if (!buffer)
    buffer = d3d::buffers::create_ua_structured_readback(8, 1, "compacted_size");
  HANDLE_LOST_DEVICE_STATE(buffer, nullptr);
#endif

  G_ASSERT_RETURN(blasCompactionsAccel.count(object_id) == 0, nullptr);

  auto &compaction = blasCompactions.push_back();
  blasCompactionsAccel[object_id] = --blasCompactions.end();

  compaction = BLASCompaction();
  compaction->objectId = object_id;
  compaction->compactedSize.reset(buffer);
  compaction->query.reset(d3d::create_event_query());
  compaction->stage = Context::BLASCompaction::Stage::Prepared;
  return &compaction.value();
}

void Context::cancelCompaction(uint64_t object_id)
{
  TIME_PROFILE(cancelCompaction);

  auto citer = blasCompactionsAccel.find(object_id);
  if (citer != blasCompactionsAccel.end())
  {
    G_ASSERT_RETURN(citer->second->has_value(), );
    if (citer->second->value().compactedSize)
    {
      // don't recycle buffer right away if readback on it was not completed
      // this allows driver level async readback
      if (!citer->second->value().query || d3d::get_event_query_status(citer->second->value().query.get(), false))
      {
        compactedSizeBufferCache.push_back();
        compactedSizeBufferCache.back().swap(citer->second->value().compactedSize);
      }
      else
      {
        pendingCompactedSizeBuffersCache.push_back();
        pendingCompactedSizeBuffersCache.back().buf.swap(citer->second->value().compactedSize);
        pendingCompactedSizeBuffersCache.back().query.swap(citer->second->value().query);
      }
    }
    citer->second->reset();
    blasCompactionsAccel.erase(citer);
  }
}

void TerrainPatch::teardown(ContextId context_id)
{
  G_VERIFY(context_id->releaseBuffer(vertices.get()));
  vertices.reset();

  context_id->freeMetaRegion(metaAllocId);
}

bool Context::RingBuffers::allocate(int struct_size, int elements, int type, const char *name, ContextId context_id)
{
  for (auto [bufIx, buf] : enumerate(buffers))
  {
    eastl::string fullName(eastl::string::CtorSprintf{}, "%s_%s_%d", context_id->name.c_str(), name, bufIx);
    buf = dag::create_sbuffer(struct_size, elements, type | SBCF_CPU_ACCESS_WRITE, 0, fullName.c_str());
    if (!buf)
      return false;
  }
  logdbg("%s resized to %u", name, elements);
  return true;
}

} // namespace bvh

#if _TARGET_SCARLETT
#include <windows.h>
#endif

namespace bvh
{

void bvh_yield()
{
#if _TARGET_SCARLETT
  // Sleep(0) is used explicitly. sleep_msec would call SwitchToThread with 0, which is not good here.
  // SwitchToThread will always wait a scheduler slice, which would cause the job to be slower. Also it
  // switch to lower prio threads which which we also don't want, they can run when the workers are idle
  // later.
  // Yielding itself is necessary as the BVH saturates all available threads with work, and as a result,
  // when a high prio thread comes, the schedular can stop the main thread and cause stuttering.
  Sleep(0);
#endif
}

} // namespace bvh
