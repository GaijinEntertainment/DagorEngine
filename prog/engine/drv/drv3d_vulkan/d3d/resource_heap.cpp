// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_driver.h>
#include <EASTL/vector.h>
#include "device_memory.h"
#include "memory_heap_resource.h"
#include "texture.h"
#include "buffer.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "buffer_alignment.h"
#include "driver_config.h"
#include "device_context.h"

using namespace drv3d_vulkan;

// global fields accessor to reduce code footprint
namespace
{
struct LocalAccessor
{
  VulkanDevice &dev;
  DeviceContext &ctx;

  LocalAccessor() : dev(Globals::VK::dev), ctx(Globals::ctx) {}
};

DeviceMemoryClass heapGroupToMemoryClass(ResourceHeapGroup *heap_group)
{
  return static_cast<DeviceMemoryClass>(reinterpret_cast<uintptr_t>(heap_group));
}

ResourceHeapGroup *memoryClassToHeapGroup(DeviceMemoryClass memory_class)
{
  return reinterpret_cast<ResourceHeapGroup *>(static_cast<uintptr_t>(memory_class));
}

size_t useSafeAligment(size_t in_aligment)
{
  return max<size_t>(in_aligment, (size_t)Globals::VK::phy.properties.limits.bufferImageGranularity);
}

ResourceAllocationProperties getAllocPropsForImage(const ImageCreateInfo &ici)
{
  Image::Description::TrimmedCreateInfo tici;
  tici.fillFromImageCreate(ici);
  Image tempImage({tici, ici.residencyFlags, ici.format, VK_IMAGE_LAYOUT_UNDEFINED}, true);
  AllocationDesc allocDesc{tempImage};
  tempImage.getDescription().fillAllocationDesc(allocDesc);
  tempImage.createVulkanObject();
  allocDesc.reqs = tempImage.getMemoryReq();
  tempImage.destroyVulkanObject();

  ResourceAllocationProperties ret;
  ret.heapGroup = memoryClassToHeapGroup(allocDesc.memClass);
  ret.offsetAlignment = useSafeAligment(allocDesc.reqs.requirements.alignment);
  ret.sizeInBytes = allocDesc.reqs.requirements.size;
  return ret;
}

void resourceDescriptionToImageCreateInfo(const ResourceDescription &desc, ImageCreateInfo &ici)
{
  switch (desc.resType)
  {
    case RES3D_TEX:
      ici.type = VK_IMAGE_TYPE_2D;
      ici.size.width = desc.asTexRes.width;
      ici.size.height = desc.asTexRes.height;
      ici.size.depth = 1;
      ici.mips = desc.asTexRes.mipLevels;
      ici.arrays = 1;
      ici.initByTexCreate(desc.asTexRes.cFlags, false /*cube_tex*/);
      break;
    case RES3D_CUBETEX:
      ici.type = VK_IMAGE_TYPE_2D;
      ici.size.width = desc.asCubeTexRes.extent;
      ici.size.height = desc.asCubeTexRes.extent;
      ici.size.depth = 1;
      ici.mips = desc.asCubeTexRes.mipLevels;
      ici.arrays = 6;
      ici.initByTexCreate(desc.asCubeTexRes.cFlags, true /*cube_tex*/);
      break;
    case RES3D_VOLTEX:
      ici.type = VK_IMAGE_TYPE_3D;
      ici.size.width = desc.asVolTexRes.width;
      ici.size.height = desc.asVolTexRes.height;
      ici.size.depth = desc.asVolTexRes.depth;
      ici.mips = desc.asVolTexRes.mipLevels;
      ici.arrays = 1;
      ici.initByTexCreate(desc.asVolTexRes.cFlags, false /*cube_tex*/);
      break;
    case RES3D_ARRTEX:
      ici.type = VK_IMAGE_TYPE_2D;
      ici.size.width = desc.asArrayTexRes.width;
      ici.size.height = desc.asArrayTexRes.height;
      ici.size.depth = 1;
      ici.mips = desc.asArrayTexRes.mipLevels;
      ici.arrays = desc.asArrayTexRes.arrayLayers;
      ici.initByTexCreate(desc.asArrayTexRes.cFlags, false /*cube_tex*/);
      break;
    case RES3D_CUBEARRTEX:
      ici.type = VK_IMAGE_TYPE_2D;
      ici.size.width = desc.asArrayCubeTexRes.extent;
      ici.size.height = desc.asArrayCubeTexRes.extent;
      ici.size.depth = 1;
      ici.mips = desc.asArrayCubeTexRes.mipLevels;
      ici.arrays = desc.asArrayCubeTexRes.cubes * 6;
      ici.initByTexCreate(desc.asArrayCubeTexRes.cFlags, true /*cube_tex*/);
      break;
    default: G_ASSERTF(0, "vulkan: non image resource desc is used to generate image create info"); break;
  }
}

void verifyAllocProps(const ResourceAllocationProperties &alloc_info, const MemoryHeapResource *heap, size_t offset)
{
  G_UNUSED(alloc_info);
  G_UNUSED(heap);
  G_UNUSED(offset);
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(heap, "vulkan: trying to place resource in non-existing heap");
  const MemoryHeapResourceDescription &heapDesc = heap->getDescription();
  G_ASSERTF((alloc_info.sizeInBytes + offset) <= heapDesc.size,
    "vulkan: resource of size %u with offset %u does not fit in heap %p:%s with size %u", alloc_info.sizeInBytes, offset, heap,
    heap->getDebugName(), heapDesc.size);
  G_ASSERTF(((heap->getMemory().offset + offset) % alloc_info.offsetAlignment) == 0,
    "vulkan: trying to place resource at wrongly aligned offset %u - requires aligment %u", offset, alloc_info.offsetAlignment);
  G_ASSERTF(heapGroupToMemoryClass(alloc_info.heapGroup) == heapDesc.memClass,
    "vulkan: trying to place resource into heap %p:%s with memory class %u when allocation properties listed memory class %u", heap,
    heap->getDebugName(), (uint32_t)heapDesc.memClass, (uint32_t)heapGroupToMemoryClass(alloc_info.heapGroup));
#endif
}

} // namespace

ResourceAllocationProperties d3d::get_resource_allocation_properties(const ResourceDescription &desc)
{
  LocalAccessor la;
  ResourceAllocationProperties ret;

  switch (desc.resType)
  {
    case RES3D_TEX:
    case RES3D_CUBETEX:
    case RES3D_VOLTEX:
    case RES3D_ARRTEX:
    case RES3D_CUBEARRTEX:
    {
      ImageCreateInfo ici;
      resourceDescriptionToImageCreateInfo(desc, ici);
      ret = getAllocPropsForImage(ici);
    }
    break;
    case RES3D_SBUF:
    {
      const DeviceMemoryClass bufDMC = BufferDescription::memoryClassFromCflags(desc.asBufferRes.cFlags);
      ret = {desc.asBufferRes.elementSizeInBytes * desc.asBufferRes.elementCount,
        useSafeAligment(Globals::VK::bufAlign.getForUsageAndFlags(0, Buffer::getUsage(la.dev, bufDMC))),
        memoryClassToHeapGroup(bufDMC)};
    }
    break;
    default:
      // unreachable code
      ret = {};
      G_ASSERTF(0, "vulkan: unhandled/unknown resource type (%u)", desc.resType);
      break;
  }
  VkDeviceSize invAlign = ret.offsetAlignment - 1;
  ret.sizeInBytes = (ret.sizeInBytes + invAlign) & ~invAlign;
  return ret;
}

ResourceHeap *d3d::create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  if (!size)
    return nullptr;

  // gather callstack outside of lock
  String debugName;
  if (Globals::cfg.debugLevel)
    debugName = backtrace::get_stack();

  LocalAccessor la;
  WinAutoLock lk(Globals::Mem::mutex);
  MemoryHeapResource *ret = Globals::Mem::res.alloc<MemoryHeapResource>(
    {size, heapGroupToMemoryClass(heap_group), flags & ResourceHeapCreateFlag::RHCF_REQUIRES_DEDICATED_HEAP ? true : false});
  if (ret->isResident())
  {
    ret->setDebugName(debugName);
    return reinterpret_cast<ResourceHeap *>(ret);
  }
  else
    Globals::Mem::res.free(ret);

  return nullptr;
}

void d3d::destroy_resource_heap(ResourceHeap *heap)
{
  G_ASSERTF(heap, "vulkan: trying to free non existing heap");
  LocalAccessor la;
  la.ctx.dispatchCommandWithStateSet<CmdDestroyHeap>({reinterpret_cast<MemoryHeapResource *>(heap)});
}

Sbuffer *d3d::place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_ASSERTF(desc.resType == RES3D_SBUF, "vulkan: non buffer description supplied for buffer heap placement");
  MemoryHeapResource *dHeap = reinterpret_cast<MemoryHeapResource *>(heap);
  verifyAllocProps(alloc_info, dHeap, offset);

  GenericBufferInterface *ret = allocate_buffer(desc.asBufferRes.elementSizeInBytes, desc.asBufferRes.elementCount,
    desc.asBufferRes.cFlags, FormatStore::fromCreateFlags(desc.asBufferRes.viewFormat), false, name);

  CmdUpdateAliasedMemoryInfo memInfoUpdate;
  LocalAccessor la;
  {
    WinAutoLock lk(Globals::Mem::mutex);
    Buffer *buf = Globals::Mem::res.alloc<Buffer>(
      {(uint32_t)ret->ressize(), dHeap->getDescription().memClass, 1, BufferMemoryFlags::IN_PLACED_HEAP}, false);
    buf->createVulkanObject();
    ResourceMemoryId memId = Globals::Mem::res.allocAliasedMemory(dHeap->getMemoryId(), alloc_info.sizeInBytes, offset);
    buf->bindMemoryFromHeap(dHeap, memId);
    memInfoUpdate.id = memId;
    memInfoUpdate.info = AliasedResourceMemory(buf->getMemory());

    ret->useExternalResource(buf);
  }
  la.ctx.dispatchCommand(memInfoUpdate);
  return ret;
}

BaseTexture *d3d::place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_ASSERTF((desc.resType == RES3D_TEX) || (desc.resType == RES3D_CUBETEX) || (desc.resType == RES3D_VOLTEX) ||
              (desc.resType == RES3D_ARRTEX) || (desc.resType == RES3D_CUBEARRTEX),
    "vulkan: non texture description supplied for texture heap placement");
  MemoryHeapResource *dHeap = reinterpret_cast<MemoryHeapResource *>(heap);
  verifyAllocProps(alloc_info, dHeap, offset);

  Image::Description::TrimmedCreateInfo ici;
  ImageCreateInfo ii;
  resourceDescriptionToImageCreateInfo(desc, ii);
  ii.residencyFlags |= Image::MEM_NOT_EVICTABLE | Image::MEM_IN_PLACED_HEAP;
  ici.fillFromImageCreate(ii);

  TextureInterfaceBase *tex = allocate_texture(desc.resType, desc.asBasicRes.cFlags);

  CmdUpdateAliasedMemoryInfo memInfoUpdate;
  LocalAccessor la;
  {
    WinAutoLock lk(Globals::Mem::mutex);
    Image *img = Globals::Mem::res.alloc<Image>({ici, ii.residencyFlags, ii.format, VK_IMAGE_LAYOUT_UNDEFINED}, false);
    img->createVulkanObject();
    ResourceMemoryId memId = Globals::Mem::res.allocAliasedMemory(dHeap->getMemoryId(), alloc_info.sizeInBytes, offset);
    img->bindMemoryFromHeap(dHeap, memId);
    memInfoUpdate.id = memId;
    memInfoUpdate.info = AliasedResourceMemory(img->getMemory());
    tex->tex.image = img;
    tex->tex.realMipLevels = ii.mips;
  }

  tex->tex.memSize = tex->ressize();
  tex->setParams(ii.size.width, ii.size.height, ii.size.depth, ii.mips, name);
  la.ctx.dispatchCommand(memInfoUpdate);
  return tex;
}

ResourceHeapGroupProperties d3d::get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  ResourceHeapGroupProperties ret;
  ret.isCPUVisible = false;
  ret.isGPULocal = false;
  ret.isOnChip = false;

  DeviceMemoryClass dmc = heapGroupToMemoryClass(heap_group);
  switch (dmc)
  {
    case DeviceMemoryClass::DEVICE_RESIDENT_IMAGE:
    case DeviceMemoryClass::DEVICE_RESIDENT_BUFFER: ret.isGPULocal = true; break;
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER:
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER:
#if !_TARGET_C3
    case DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER:
#endif
      ret.isCPUVisible = true;
      break;
    case DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER:
      ret.isGPULocal = true;
      ret.isCPUVisible = true;
      break;
    case DeviceMemoryClass::TRANSIENT_IMAGE:
      ret.isGPULocal = true;
      ret.isOnChip = true;
      break;
    default: G_ASSERTF(0, "vulkan: invalid heap_group %u supplied", (uint32_t)dmc); break;
  }

  {
    LocalAccessor la;
    WinAutoLock lk(Globals::Mem::mutex);
    ret.maxHeapSize = Globals::Mem::pool.getMaxAllocatableMemorySizeForClass(dmc);
  }
  ret.optimalMaxHeapSize = 0;
  ret.maxResourceSize = ret.maxHeapSize;

  return ret;
}