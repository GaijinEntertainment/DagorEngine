// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_context.h"
#include <3d/dag_lockSbuffer.h>
#include <image/dag_texPixel.h>
#include <memory/dag_framemem.h>
#include <generic/dag_enumerate.h>

const PerInstanceData PerInstanceData::ZERO = {};

namespace bvh
{

struct DeathrowJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "process_bvh_deathrow"; }
  void doJob() override
  {
    static constexpr uint32_t BUFFER_LIMIT = 512;

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

void Object::teardown(ContextId context_id, uint64_t object_id)
{
  blas.reset();
  for (auto &mesh : meshes)
    mesh.teardown(context_id);

  if (metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
    context_id->freeMetaRegion(metaAllocId);

  if (auto iter = context_id->stationaryTreeBuffers.find(object_id); iter != context_id->stationaryTreeBuffers.end())
  {
    if (iter->second.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
      context_id->freeMetaRegion(iter->second.metaAllocId);
    iter->second.blas.reset();
    context_id->stationaryTreeBuffers.erase(iter);
  }
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

  context_id->releaseTexture(albedoTextureId);
  context_id->releaseTexture(alphaTextureId);
  context_id->releaseTexture(normalTextureId);
  context_id->releaseTexture(extraTextureId);
  context_id->releaseTexture(ppPositionTextureId);
  context_id->releaseTexture(ppDirectionTextureId);

  albedoTextureId = BAD_TEXTUREID;
  alphaTextureId = BAD_TEXTUREID;
  normalTextureId = BAD_TEXTUREID;
  extraTextureId = BAD_TEXTUREID;
  ppPositionTextureId = BAD_TEXTUREID;
  ppDirectionTextureId = BAD_TEXTUREID;
  ppPositionBindless = 0xFFFFFFFFU;
  ppDirectionBindless = 0xFFFFFFFFU;

  if (geometry)
  {
    if (geometry.processedVertexBuffer)
      context_id->releaseBuffer(geometry.processedVertexBuffer.get());
    context_id->moveToDeathrow(eastl::move(geometry));
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
  stubTexture = d3d::create_tex(white, 1, 1, TEXFMT_A8R8G8B8, 1, "bvh_stub_tex", RESTAG_BVH);
  memfree(white, framemem_ptr());
}

void Context::teardown()
{
  threadpool::wait(&deathrowJob);

  for (auto &[object_id, object] : objects)
    object.teardown(this, object_id);
  objects.clear();

  meshMeta.close();
  perInstanceData.close();

  for (auto &lod : terrainLods)
    for (auto &patch : lod.patches)
      patch.teardown(this);
  terrainLods.clear();

  for (auto &job : createCompactedBLASJobQueue)
    threadpool::wait(job.get());
  createCompactedBLASJobQueue.clear();

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

  compactedSizeBuffer.reset();
  compactedSizeBufferReadback.reset();
  compactedSizeBufferCursor = 0;
  compactedSizeQuery.reset();
  compactedSizeQueryRunning = false;
  compactedSizeWritesInQueue = 0;

  releaseAllBindlessTexHolders();

  clearDeathrow();
  G_ASSERTF(meshMetaAllocator.allocated() == 0, "meshMetaAllocator still has %d meta allocated!", meshMetaAllocator.allocated());
#if DAGOR_DBGLEVEL > 0
  for (auto [ptr, allocatorName] : bindlessBufferAllocatorNames)
    logwarn("[BVH] Context::teardown: buffer %p (%s) was not released", ptr, allocatorName.c_str());
#endif

  del_d3dres(stubTexture);

#if DAGOR_DBGLEVEL > 0
  for (auto &[alloc, _] : sourceGeometryAllocators)
    G_ASSERT(!alloc.has_value());
#endif
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
  dynamic_decals_atlasBindless.close(this);
  if (gbufferBindlessRange >= 0)
    d3d::free_bindless_resource_range(D3DResourceType::TEX, gbufferBindlessRange, 5);
  gbufferBindlessRange = -1;
  if (fomShadowsBindlessRange >= 0)
    d3d::free_bindless_resource_range(D3DResourceType::TEX, fomShadowsBindlessRange, 2);
  fomShadowsBindlessRange = -1;
}

void Context::moveToDeathrow(UniqueBVHBufferWithOffset &&buf)
{
  if (buf)
  {
    OSSpinlockScopedLock lock(deathrowLock);
    deathrow.emplace_back(eastl::forward<UniqueBVHBuffer>(buf.buffer));
  }
}

void Context::moveToDeathrow(BVHGeometryBufferWithOffset &&buf)
{
  if (buf)
  {
    freeSourceGeometry(buf.heapIndex, buf.bufferRegion);

    if (buf.processedVertexBuffer)
    {
      OSSpinlockScopedLock lock(deathrowLock);
      deathrow.emplace_back(eastl::forward<UniqueBVHBuffer>(buf.processedVertexBuffer));
    }
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

TextureHandle Context::holdTexture(TEXTUREID id, uint32_t &texture_bindless_index, bool forceRefreshSrvsWhenLoaded)
{
  TextureHandle handle(id);

  texture_bindless_index = MeshMeta::INVALID_TEXTURE;
  if (id != BAD_TEXTUREID)
  {
    WinAutoLock lock(bindlessTextureLock);

    auto [iter, inserted] = usedTextures.insert(id);
    auto &[_, bindlessTex] = *iter;
    ++bindlessTex.referenceCount;
    if (inserted)
    {
      // This is the initial allocation of the texture.
      Texture *texture = acquire_managed_tex(id);
      if (!texture)
      {
        logwarn("BVH Context::holdTexture got a nullptr texture from acquire_managed_tex for id: %u", id.index());
      }
      else if (texture->getType() == D3DResourceType::TEX)
      {
        bindlessTex.rangeBase = bindlessTextureAllocator.add(texture);
        bindlessTex.slotIndex = 0;
        bindlessTex.resourceType = D3DResourceType::TEX;
      }
      else if (texture->getType() == D3DResourceType::CUBETEX)
      {
        bindlessTex.rangeBase = bindlessCubeTextureAllocator.add(texture);
        bindlessTex.slotIndex = 0;
        bindlessTex.resourceType = D3DResourceType::CUBETEX;
      }
      else
        G_ASSERT(0 && "Unknown res");

      if (forceRefreshSrvsWhenLoaded && !check_managed_texture_loaded(id))
        texturesWaitingForLoad.insert(id);
    }

    texture_bindless_index = bindlessTex.rangeBase + bindlessTex.slotIndex;

    // Taking an exclusive reference count for the returned texture pointer.
    handle.texture = acquire_managed_tex(id);
  }

  return handle;
}

bool Context::releaseTextureNoLock(TEXTUREID id)
{
  if (id == BAD_TEXTUREID)
    return false;

  auto iter = usedTextures.find(id);
  G_ASSERT_RETURN(iter != usedTextures.end(), false);
  if (--iter->second.referenceCount == 0)
  {
    // SBUF is just to make it unknown
    auto resourceType = iter->second.resourceType.value_or(D3DResourceType::SBUF);

    if (resourceType == D3DResourceType::TEX)
    {
      if (iter->second.slotIndex == 0)
        bindlessTextureAllocator.remove(iter->second.rangeBase);
    }
    else if (resourceType == D3DResourceType::CUBETEX)
    {
      if (iter->second.slotIndex == 0)
        bindlessCubeTextureAllocator.remove(iter->second.rangeBase);
    }
    else
      G_ASSERTF(false, "Unknown texture type");

    usedTextures.erase(iter);
    release_managed_tex(id);
    texturesWaitingForLoad.erase(id);
    return true;
  }

  return false;
}

bool Context::releaseTexture(TEXTUREID id)
{
  WinAutoLock lock(bindlessTextureLock);

  return releaseTextureNoLock(id);
}

bool Context::releaseTexture(uint32_t texture_bindless_indices)
{
  if (texture_bindless_indices == MeshMeta::INVALID_TEXTURE)
    return false;

  WinAutoLock lock(bindlessTextureLock);

  auto tex = bindlessTextureAllocator.get_resource(texture_bindless_indices);

  return tex && releaseTextureNoLock(tex->getTID());
}

void Context::markChangedTextures()
{
  WinAutoLock lock(bindlessTextureLock);

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
      auto texture = acquire_managed_tex(*it);
      G_ASSERT_CONTINUE(texture);

      if (usedTexIt->second.resourceType == D3DResourceType::TEX)
        bindlessTextureAllocator.update(usedTexIt->second.rangeBase, usedTexIt->second.slotIndex, texture);
      else if (usedTexIt->second.resourceType == D3DResourceType::CUBETEX)
        bindlessCubeTextureAllocator.update(usedTexIt->second.rangeBase, usedTexIt->second.slotIndex, texture);
      else
        G_ASSERT(0 && "Unknown texture type");

      release_managed_tex(*it);
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

    auto [iter, inserted] = usedBuffers.insert(buffer);
    auto &[_, bindlessBuf] = *iter;
    ++bindlessBuf.referenceCount;

    if (inserted)
    {
      bindlessBuf.rangeBase = bindlessBufferAllocator.add(buffer);
      bindlessBuf.slotIndex = 0;
#if DAGOR_DBGLEVEL > 0
      bindlessBufferAllocatorNames[buffer] = buffer->getName();
#endif
    }

    bindless_index = bindlessBuf.rangeBase + bindlessBuf.slotIndex;
  }
}

bool Context::releaseBuffer(Sbuffer *buffer)
{
  if (!buffer)
    return false;

  auto iter = usedBuffers.find(buffer);
  if (iter == usedBuffers.end())
    return false;
  if (--iter->second.referenceCount == 0)
  {
    if (iter->second.slotIndex == 0)
      bindlessBufferAllocator.remove(iter->second.rangeBase);
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

#if !_TARGET_C2
  if (!compactedSizeBuffer)
  {
    compactedSizeBuffer.reset(d3d::buffers::create_ua_structured(8, compactedSizeBufferSize, "compacted_size", RESTAG_BVH));
    // use separate buffer to properly handle async readback, i.e. keep writing original buffer on GPU
    // while reading back is completing from other buffer
    compactedSizeBufferReadback.reset(d3d::buffers::create_ua_structured_readback(8, compactedSizeBufferSize,
      "compacted_size_readback", d3d::buffers::Init::No, RESTAG_BVH));
    compactedSizeWritesInQueue = 0;
  }
  HANDLE_LOST_DEVICE_STATE(compactedSizeBuffer, nullptr);
#endif

  if (!compactedSizeQuery) // PS5 doesn't use compactedSizeBuffer but depends on compactedSizeQuery
  {
    compactedSizeQueryRunning = false;
    compactedSizeQuery.reset(d3d::create_event_query());
  }

  G_ASSERT_RETURN(blasCompactionsAccel.count(object_id) == 0, nullptr);

  auto &compaction = blasCompactions.push_back();
  blasCompactionsAccel[object_id] = --blasCompactions.end();

  compaction = BLASCompaction();
  compaction->objectId = object_id;
  compaction->compactedSizeOffset = compactedSizeBufferCursor;
  compaction->query.reset(d3d::create_event_query());
  compaction->stage = Context::BLASCompaction::Stage::Created;

  compactedSizeBufferCursor++;

  if (compactedSizeBufferCursor >= compactedSizeBufferSize)
    compactedSizeBufferCursor = 0;

  return &compaction.value();
}

void Context::cancelCompaction(uint64_t object_id)
{
  TIME_PROFILE(cancelCompaction);

  auto citer = blasCompactionsAccel.find(object_id);
  if (citer != blasCompactionsAccel.end())
  {
    G_ASSERT_RETURN(citer->second->has_value(), );
    citer->second->reset();
    blasCompactionsAccel.erase(citer);
  }
}

static constexpr uint32_t allocator_buffer_size = 2 << 20;

Context::SourceGeometryAllocation Context::allocateSourceGeometry(uint32_t dwordCount, bool force_unique)
{
  G_ASSERT(is_main_thread());

  auto byteCount = dwordCount * 4;

  int newIndex = -1;

  if (!force_unique)
  {
    for (const auto &[ix, alloc] : enumerate(sourceGeometryAllocators))
      if (alloc.first.has_value())
      {
        if (alloc.first->canAllocate(byteCount))
          return {uint32_t(ix), alloc.first->allocateInHeap(byteCount), alloc.second};
      }
      else if (newIndex < 0)
      {
        newIndex = ix;
      }
  }

  if (newIndex < 0)
  {
    newIndex = sourceGeometryAllocators.size();
    sourceGeometryAllocators.push_back();
  }

  static int cnt = 0;
  auto &[alloc, bindless] = sourceGeometryAllocators[newIndex];
  alloc.emplace(SbufferHeapManager(String(32, "bvh_geometry_%d", cnt++), 4, SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW));
  alloc->resize(force_unique ? byteCount : max(allocator_buffer_size, byteCount));
  holdBuffer(alloc->getHeap().getBuf(), bindless);
  return {uint32_t(newIndex), alloc->allocateInHeap(byteCount), bindless};
}

void Context::freeSourceGeometry(int &heapix, LinearHeapAllocatorSbuffer::RegionId region)
{
  G_ASSERT(is_main_thread());

  if (heapix < 0)
    return;

  G_ASSERT_RETURN(sourceGeometryAllocators[heapix].first.has_value(), );

  G_VERIFY(sourceGeometryAllocators[heapix].first->free(region));
  if (sourceGeometryAllocators[heapix].first->allocated() == 0)
  {
    // No more suballocations so the allocator can go
    releaseBuffer(sourceGeometryAllocators[heapix].first->getHeap().getBuf());
    sourceGeometryAllocators[heapix].first.reset();
  }

  heapix = -1;
}

uint32_t Context::getSourceBufferOffset(int heapix, LinearHeapAllocatorSbuffer::RegionId region)
{
  G_ASSERT_RETURN(heapix >= 0, 0);
  G_ASSERT_RETURN(sourceGeometryAllocators[heapix].first.has_value(), 0);

  return sourceGeometryAllocators[heapix].first->get(region).offset;
}

uint32_t Context::getSourceBufferSize(int heapix, LinearHeapAllocatorSbuffer::RegionId region)
{
  G_ASSERT_RETURN(heapix >= 0, 0);
  G_ASSERT_RETURN(sourceGeometryAllocators[heapix].first.has_value(), 0);

  return sourceGeometryAllocators[heapix].first->get(region).size;
}

void TerrainPatch::teardown(ContextId context_id)
{
  context_id->releaseBuffer(vertices.get());
  vertices.reset();

  context_id->freeMetaRegion(metaAllocId);
}

bool Context::RingBuffers::allocate(int struct_size, int elements, int type, const char *name, ContextId context_id)
{
  for (auto [bufIx, buf] : enumerate(buffers))
  {
    eastl::string fullName(eastl::string::CtorSprintf{}, "%s_%s_%d", context_id->name.c_str(), name, bufIx);
    buf = dag::create_sbuffer(struct_size, elements, type, 0, fullName.c_str(), RESTAG_BVH);
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
