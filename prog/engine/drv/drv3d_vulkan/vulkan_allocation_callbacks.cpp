// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vulkan_allocation_callbacks.h"
#include <memory/dag_memBase.h>
#include <debug/dag_log.h>
#include <osApiWrappers/dag_atomic.h>
#include <EASTL/memory.h>
#include "globals.h"
#include "driver_config.h"

#if ALLOW_VULKAN_ALLOCATION_CALLBACKS

struct AllocationHeader
{
  size_t size;
  size_t alignment;
};
#define ALLOCATION_HEADER_SIZE sizeof(AllocationHeader)

static uint32_t typeIndices[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {
#define ALLOCATION_TYPE(var) (uint32_t) drv3d_vulkan::AllocationType::var,
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
};

static const char *typeNames[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {
#define ALLOCATION_TYPE(var) #var,
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
};


void *VKAPI_PTR allocationCallback(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
void VKAPI_PTR freeCallback(void *pUserData, void *pMemory);
void *VKAPI_PTR reallocationCallback(void *pUserData, void *pOriginal, size_t size, size_t alignment,
  VkSystemAllocationScope allocationScope);
void VKAPI_PTR internalAllocationCallback(void *pUserData, size_t size, VkInternalAllocationType allocationType,
  VkSystemAllocationScope allocationScope);
void VKAPI_PTR internalFreeCallback(void *pUserData, size_t size, VkInternalAllocationType allocationType,
  VkSystemAllocationScope allocationScope);

static VkAllocationCallbacks callbacks[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {
#define ALLOCATION_TYPE(var)                                                                                          \
  {&typeIndices[(unsigned)drv3d_vulkan::AllocationType::var], allocationCallback, reallocationCallback, freeCallback, \
    internalAllocationCallback, internalFreeCallback},
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
};

alignas(64) static int64_t allocationSizes[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {};
alignas(64) static int64_t notifiedAllocationSizes[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {};

void *VKAPI_PTR allocationCallback(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationScope);
  size_t realAlignment = max(alignment, ALLOCATION_HEADER_SIZE);
  void *allocation = midmem->allocAligned(realAlignment + size, realAlignment);
  void *ptr = (uint8_t *)allocation + realAlignment;
  AllocationHeader *hdr = (AllocationHeader *)((uint8_t *)ptr - ALLOCATION_HEADER_SIZE);
  hdr->alignment = alignment;
  hdr->size = size;
  interlocked_add(allocationSizes[*(uint32_t *)pUserData], (int64_t)size);
  return ptr;
}

void VKAPI_PTR freeCallback(void *pUserData, void *pMemory)
{
  if (pMemory == nullptr)
    return;
  AllocationHeader *hdr = (AllocationHeader *)((uint8_t *)pMemory - ALLOCATION_HEADER_SIZE);
  size_t realAlignment = max(hdr->alignment, ALLOCATION_HEADER_SIZE);
  void *ptr = (uint8_t *)pMemory - realAlignment;
  interlocked_add(allocationSizes[*(uint32_t *)pUserData], -(int64_t)hdr->size);
  midmem->freeAligned(ptr);
}

void *VKAPI_PTR reallocationCallback(void *pUserData, void *pOriginal, size_t size, size_t alignment,
  VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationScope);
  if (size == 0 && pOriginal == nullptr)
    return nullptr;
  if (pOriginal == nullptr)
    return allocationCallback(pUserData, size, alignment, allocationScope);
  AllocationHeader *hdr = (AllocationHeader *)((uint8_t *)pOriginal - ALLOCATION_HEADER_SIZE);
  bool tryInplace = true;
  if (hdr->alignment != alignment)
  {
    logwarn("vulkan: reallocate callback called with different alignment than original allocation: %d requested now and %d originally "
            "specified",
      alignment, hdr->alignment);
    tryInplace = false;
  }
  if (size == 0)
  {
    freeCallback(pUserData, pOriginal);
    return nullptr;
  }
  if (hdr->size == size && hdr->alignment == alignment)
    return pOriginal;

  size_t newSize = size + max(hdr->alignment, ALLOCATION_HEADER_SIZE);
  if (tryInplace && midmem->resizeInplace((uint8_t *)pOriginal - max(hdr->alignment, ALLOCATION_HEADER_SIZE), newSize))
  {
    interlocked_add(allocationSizes[*(uint32_t *)pUserData], (int64_t)size - (int64_t)hdr->size);
    hdr->size = size;
    return pOriginal;
  }
  else if (hdr->size >= size)
    return pOriginal;
  void *newPtr = allocationCallback(pUserData, size, alignment, allocationScope);
  memcpy(newPtr, pOriginal, hdr->size);
  freeCallback(pUserData, pOriginal);
  return newPtr;
}

void VKAPI_PTR internalAllocationCallback(void *pUserData, size_t size, VkInternalAllocationType allocationType,
  VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationType);
  G_UNUSED(allocationScope);
  interlocked_add(notifiedAllocationSizes[*(uint32_t *)pUserData], (int64_t)size);
}

void VKAPI_PTR internalFreeCallback(void *pUserData, size_t size, VkInternalAllocationType allocationType,
  VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationType);
  G_UNUSED(allocationScope);
  interlocked_add(notifiedAllocationSizes[*(uint32_t *)pUserData], -(int64_t)size);
}

namespace drv3d_vulkan
{

VkAllocationCallbacks *getAllocationCallbacksPtr(AllocationType type)
{
  if (DAGOR_UNLIKELY(Globals::cfg.bits.useCustomAllocationCallbacks))
    return &callbacks[(unsigned)type];
  return nullptr;
}

void printAllocationCallbacksStatistics()
{
  if (DAGOR_UNLIKELY(Globals::cfg.bits.useCustomAllocationCallbacks))
  {
    debug("vulkan: cpu memory allocations report (allocated with provided callbacks | internally allocated by driver and reported "
          "through callback)");
    float totalAllocated = 0.f, totalNotified = 0.f;
    for (int i = 0; i < (int)drv3d_vulkan::AllocationType::COUNT; i++)
    {
      float allocated = allocationSizes[i] / (1024.f * 1024.f);
      float notified = notifiedAllocationSizes[i] / (1024.f * 1024.f);
      totalAllocated += allocated;
      totalNotified += notified;
      debug(" %s: %f Mb | %f Mb", typeNames[i], allocated, notified);
    }
    debug(" Total: %f Mb | %f Mb", totalAllocated, totalNotified);
  }
}

} // namespace drv3d_vulkan

#endif