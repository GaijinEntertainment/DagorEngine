// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "memory_heap_resource.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "backend.h"
#include "pipeline_state_pending_references.h"

using namespace drv3d_vulkan;

namespace drv3d_vulkan
{

template <>
void MemoryHeapResource::onDelayedCleanupFinish<CleanupTag::DESTROY>()
{
  // delay destruction if some resources are not yet freed
  if (hasPlacedResources())
  {
    ++cleanupTries;
    Backend::State::pendingCleanups.removeReferenced(this);
    if ((cleanupTries % MAX_CLEANUP_RETRIES) == 0)
      tooLongCleanupDelayComplain();
    return;
  }

  ResourceManager &rm = Globals::Mem::res;
  rm.free(this);
}

template void MemoryHeapResource::place<Image>(Image *res, VkDeviceSize offset, VkDeviceSize size);
template void MemoryHeapResource::place<Buffer>(Buffer *res, VkDeviceSize offset, VkDeviceSize size);

} // namespace drv3d_vulkan

void MemoryHeapResource::onDeviceReset()
{
  const ResourceMemory &heapMem = getMemory();
  Globals::Mem::res.iterateAllocated<Image>([&heapMem, this](Image *i) { onDeviceResetInHeapCb(heapMem, i); });
  Globals::Mem::res.iterateAllocated<Buffer>([&heapMem, this](Buffer *i) { onDeviceResetInHeapCb(heapMem, i); });
}

void MemoryHeapResource::afterDeviceReset()
{
  for (ResourceRecreateInfo &i : recreations)
  {
    switch (i.res->getResType())
    {
      case ResourceType::IMAGE: afterDeviceResetInHeapCb((Image *)i.res, i.offset, i.size); break;
      case ResourceType::BUFFER: afterDeviceResetInHeapCb((Buffer *)i.res, i.offset, i.size); break;
      default:
        G_ASSERTF(0, "vulkan: heap trying to recreate unknown res %p:%s with type %u", i.res, i.res->getDebugName(),
          (int)i.res->getResType());
        break;
    }
  }
  recreations.clear();
}

void MemoryHeapResourceDescription::fillAllocationDesc(AllocationDesc &alloc_desc) const
{
  alloc_desc.reqs = {};
  alloc_desc.canUseSharedHandle = 0;
  alloc_desc.forceDedicated = useDedicated ? 1 : 0;
  alloc_desc.memClass = memType._class;
  alloc_desc.temporary = 0;
  alloc_desc.objectBaked = 0;
  alloc_desc.reqs.requirements.size = size;
  alloc_desc.reqs.requirements.alignment = Globals::VK::phy.properties.limits.bufferImageGranularity;
  alloc_desc.reqs.requirements.memoryTypeBits = memType.mask;
}

template <typename ResType>
void MemoryHeapResource::onDeviceResetInHeapCb(const ResourceMemory &heap_mem, ResType *res)
{
  if (!(res->getMemoryId() != -1 && res->getMemory().intersects(heap_mem)))
    return;

  recreations.push_back({res, res->getMemory().offset, res->getMemory().size});
  res->markDeviceReset();
  res->onDeviceReset();
  res->destroyVulkanObject();
  res->freeMemory();
  res->releaseHeap();
}

template <typename ResType>
void MemoryHeapResource::afterDeviceResetInHeapCb(ResType *res, VkDeviceSize offset, VkDeviceSize size)
{
  place(res, offset, size);
  // short cut to backend here, as everything is in locked&paused state
  Backend::aliasedMemory.update(res->getMemoryId(), AliasedResourceMemory(res->getMemory()));
  res->afterDeviceReset();
}

void MemoryHeapResource::createVulkanObject()
{
  AllocationDesc allocDesc = {*this};
  desc.fillAllocationDesc(allocDesc);
  if (tryAllocMemory(allocDesc))
  {
    setHandle(generalize(getMemory().deviceMemory()));
    reportToTQL(true);
  }
}

void MemoryHeapResource::tooLongCleanupDelayComplain()
{
  const ResourceMemory &heapMem = getMemory();
  uint32_t aliveReferences = 0;
  auto iterCb = [&heapMem, &aliveReferences](const Resource *i) {
    if (i->getMemoryId() != -1 && i->getMemory().intersects(heapMem))
    {
      ++aliveReferences;
      D3D_ERROR("vulkan: %s %p:%s still alive in destroyed heap", i->resTypeString(), i, i->getDebugName());
    }
  };
  Globals::Mem::res.iterateAllocated<Image>(iterCb);
  Globals::Mem::res.iterateAllocated<Buffer>(iterCb);

#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(aliveReferences == placedResources,
    "vulkan: placed resources counter for heap %p:%s is broken (counted by iter %u saved in field %u)", this, getDebugName(),
    aliveReferences, placedResources);
#endif

  D3D_ERROR("vulkan: heap %p:%s is used by %u resources on %u cleanup retries, remove resources placed in heap!", this, getDebugName(),
    aliveReferences, cleanupTries);
}

void MemoryHeapResource::destroyVulkanObject()
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(placedResources == 0, "vulkan: heap %p:%s is used by %u resources, remove resources placed in heap!", this, getDebugName(),
    placedResources);
#endif

  if (isResident())
    reportToTQL(false);
  setHandle(0);
}

bool MemoryHeapResource::hasPlacedResources()
{
#if DAGOR_DBGLEVEL > 0
  return placedResources > 0;
#else
  // heap destroy is rare event, loop here instead of wasting memory footprint per object (to keep references) on rel build
  const ResourceMemory &heapMem = getMemory();
  bool ret = false;
  auto iterCb = [&heapMem, &ret](const Resource *i) {
    if (i->getMemoryId() != -1 && i->getMemory().intersects(heapMem))
    {
      ret |= true;
      return false;
    }
    return true;
  };
  Globals::Mem::res.iterateAllocatedBreakable<Image>(iterCb);
  if (ret)
    return true;
  Globals::Mem::res.iterateAllocatedBreakable<Buffer>(iterCb);
  return ret;
#endif
}

template <typename ResType>
void MemoryHeapResource::place(ResType *res, VkDeviceSize offset, VkDeviceSize size)
{
  res->createVulkanObject();
  ResourceMemoryId memId = Globals::Mem::res.allocAliasedMemory(getMemoryId(), size, offset);
  res->bindMemoryFromHeap(this, memId);
}
