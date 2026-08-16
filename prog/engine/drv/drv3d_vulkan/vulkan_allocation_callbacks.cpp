// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vulkan_allocation_callbacks.h"
#include <memory/dag_memBase.h>
#include <debug/dag_log.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/memory.h>
#include <string.h>
#include <stddef.h>
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

// ---- guarded layout ----------------------------------------------------------
// [ leading pad | GuardedHeader (ends exactly at ptr) | user data | back redzone ]
// frontGuard is the last header member, so it sits immediately before ptr and
// catches underruns; the back redzone catches overruns; magic catches
// double/wild frees. realAlignment is recomputed on free from the stored fields.

static constexpr uint64_t GUARD_MAGIC_LIVE = 0xA5A5C0DEF00DFACEull;
static constexpr uint64_t GUARD_MAGIC_FREED = 0xDEADBEEFDEADBEEFull;
static constexpr uint32_t GUARD_FRONT_MAGIC = 0xFEEDFACEu;
static constexpr uint8_t GUARD_REDZONE_BYTE = 0xAB;
static constexpr uint8_t GUARD_FREED_FILL = 0xDD;
static constexpr size_t GUARD_REDZONE_SIZE = 32;
static constexpr size_t GUARD_HEADER_RESERVE = 64; // power-of-two, >= sizeof(GuardedHeader)

struct GuardedHeader
{
  uint64_t magic;
#if VULKAN_GUARDED_ALLOC_REGISTRY
  GuardedHeader *prev;
  GuardedHeader *next;
#endif
  size_t size;
  size_t alignment;
  uint32_t typeIndex;
  uint32_t frontGuard; // keep last: must be adjacent to the returned pointer
};
static_assert(sizeof(GuardedHeader) <= GUARD_HEADER_RESERVE, "grow GUARD_HEADER_RESERVE");
static_assert(offsetof(GuardedHeader, frontGuard) == sizeof(GuardedHeader) - sizeof(uint32_t),
  "frontGuard must be the last member so it sits right before the user pointer");

alignas(64) static int64_t allocationSizes[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {};
alignas(64) static int64_t notifiedAllocationSizes[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {};

#if VULKAN_GUARDED_ALLOC_REGISTRY
static OSSpinlock guardListLock;
static GuardedHeader *guardListHead = nullptr;

static void guardListInsert(GuardedHeader *hdr)
{
  OSSpinlockScopedLock lk(guardListLock);
  hdr->prev = nullptr;
  hdr->next = guardListHead;
  if (guardListHead)
    guardListHead->prev = hdr;
  guardListHead = hdr;
}

static void guardListRemove(GuardedHeader *hdr)
{
  OSSpinlockScopedLock lk(guardListLock);
  if (hdr->prev)
    hdr->prev->next = hdr->next;
  else
    guardListHead = hdr->next;
  if (hdr->next)
    hdr->next->prev = hdr->prev;
}
#endif

static void reportGuardFailure(const GuardedHeader *hdr, const void *ptr, const char *what, const char *context)
{
  const uint32_t ti = hdr ? hdr->typeIndex : ~0u;
  const char *typeName = ti < (uint32_t)drv3d_vulkan::AllocationType::COUNT ? typeNames[ti] : "<corrupt-type>";
  DAG_FATAL("vulkan: host allocation guard tripped: %s [%s] ptr %p type %s size %llu", what, context, ptr, typeName,
    hdr ? (unsigned long long)hdr->size : 0ull);
}

// checks a block expected to be live; fatals on the first inconsistency
static void checkGuardedBlock(const GuardedHeader *hdr, const void *ptr, const char *context)
{
  if (hdr->magic == GUARD_MAGIC_FREED)
    reportGuardFailure(hdr, ptr, "double free / use-after-free (header marked freed)", context);
  else if (hdr->magic != GUARD_MAGIC_LIVE)
    reportGuardFailure(hdr, ptr, "corrupt header (wild pointer or adjacent-block overwrite)", context);
  if (hdr->frontGuard != GUARD_FRONT_MAGIC)
    reportGuardFailure(hdr, ptr, "underrun (write just before allocation)", context);
  const uint8_t *rz = (const uint8_t *)ptr + hdr->size;
  for (size_t i = 0; i < GUARD_REDZONE_SIZE; ++i)
    if (rz[i] != GUARD_REDZONE_BYTE)
      reportGuardFailure(hdr, ptr, "overrun (write past allocation)", context);
}

static void *guardedAlloc(uint32_t typeIndex, size_t size, size_t alignment)
{
  const size_t realAlignment = max(alignment, GUARD_HEADER_RESERVE);
  uint8_t *base = (uint8_t *)midmem->allocAligned(realAlignment + size + GUARD_REDZONE_SIZE, realAlignment);
  uint8_t *ptr = base + realAlignment;

  GuardedHeader *hdr = (GuardedHeader *)(ptr - sizeof(GuardedHeader));
  hdr->magic = GUARD_MAGIC_LIVE;
  hdr->size = size;
  hdr->alignment = alignment;
  hdr->typeIndex = typeIndex;
  hdr->frontGuard = GUARD_FRONT_MAGIC;
  memset(ptr + size, GUARD_REDZONE_BYTE, GUARD_REDZONE_SIZE);

  interlocked_add(allocationSizes[typeIndex], (int64_t)size);
#if VULKAN_GUARDED_ALLOC_REGISTRY
  guardListInsert(hdr);
#endif
  return ptr;
}

static void guardedFree(void *ptr)
{
  if (ptr == nullptr)
    return;
  GuardedHeader *hdr = (GuardedHeader *)((uint8_t *)ptr - sizeof(GuardedHeader));
  checkGuardedBlock(hdr, ptr, "free");

#if VULKAN_GUARDED_ALLOC_REGISTRY
  guardListRemove(hdr);
#endif
  interlocked_add(allocationSizes[hdr->typeIndex], -(int64_t)hdr->size);

  const size_t realAlignment = max(hdr->alignment, GUARD_HEADER_RESERVE);
  hdr->magic = GUARD_MAGIC_FREED;
  // poison the user region so a use-after-free reads obvious garbage
  memset(ptr, GUARD_FREED_FILL, hdr->size);
  midmem->freeAligned((uint8_t *)ptr - realAlignment);
}

// ---- plain callbacks (midmem-backed, used for the memory-tracking feature) ----

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

// ---- guarded callbacks (raw-heap-backed, redzone/double-free detection) --------

void *VKAPI_PTR guardedAllocationCallback(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationScope);
  return guardedAlloc(*(uint32_t *)pUserData, size, alignment);
}

void VKAPI_PTR guardedFreeCallback(void *pUserData, void *pMemory)
{
  G_UNUSED(pUserData);
  guardedFree(pMemory);
}

void *VKAPI_PTR guardedReallocationCallback(void *pUserData, void *pOriginal, size_t size, size_t alignment,
  VkSystemAllocationScope allocationScope)
{
  G_UNUSED(allocationScope);
  const uint32_t typeIndex = *(uint32_t *)pUserData;
  if (size == 0 && pOriginal == nullptr)
    return nullptr;
  if (pOriginal == nullptr)
    return guardedAlloc(typeIndex, size, alignment);

  GuardedHeader *hdr = (GuardedHeader *)((uint8_t *)pOriginal - sizeof(GuardedHeader));
  checkGuardedBlock(hdr, pOriginal, "realloc");
  if (size == 0)
  {
    guardedFree(pOriginal);
    return nullptr;
  }
  // always move so both redzones are re-placed around the new size
  const size_t oldSize = hdr->size;
  void *newPtr = guardedAlloc(typeIndex, size, alignment);
  if (!newPtr)
    return nullptr; // per spec the original block must be left intact on failure
  memcpy(newPtr, pOriginal, min(oldSize, size));
  guardedFree(pOriginal);
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

static VkAllocationCallbacks plainCallbacks[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {
#define ALLOCATION_TYPE(var)                                                                                          \
  {&typeIndices[(unsigned)drv3d_vulkan::AllocationType::var], allocationCallback, reallocationCallback, freeCallback, \
    internalAllocationCallback, internalFreeCallback},
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
};

static VkAllocationCallbacks guardedCallbacks[(unsigned)drv3d_vulkan::AllocationType::COUNT] = {
#define ALLOCATION_TYPE(var)                                                                                          \
  {&typeIndices[(unsigned)drv3d_vulkan::AllocationType::var], guardedAllocationCallback, guardedReallocationCallback, \
    guardedFreeCallback, internalAllocationCallback, internalFreeCallback},
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
};

namespace drv3d_vulkan
{

VkAllocationCallbacks *getAllocationCallbacksPtr(AllocationType type)
{
  // guard vs plain is decided by which callback set allocated the block, not by any
  // shared flag - so a block's layout and free path can never disagree
  if (DAGOR_UNLIKELY(Globals::cfg.bits.guardVulkanAllocations))
    return &guardedCallbacks[(unsigned)type];
  if (DAGOR_UNLIKELY(Globals::cfg.bits.useCustomAllocationCallbacks))
    return &plainCallbacks[(unsigned)type];
  return nullptr;
}

void checkVulkanAllocationGuards(const char *when)
{
#if VULKAN_GUARDED_ALLOC_REGISTRY
  OSSpinlockScopedLock lk(guardListLock);
  size_t liveCount = 0;
  for (GuardedHeader *hdr = guardListHead; hdr; hdr = hdr->next, ++liveCount)
    checkGuardedBlock(hdr, (uint8_t *)hdr + sizeof(GuardedHeader), when);
  debug("vulkan: host allocation guard sweep ok at [%s], %u live", when, (uint32_t)liveCount);
#else
  G_UNUSED(when);
#endif
}

void printAllocationCallbacksStatistics()
{
  if (DAGOR_UNLIKELY(Globals::cfg.bits.useCustomAllocationCallbacks || Globals::cfg.bits.guardVulkanAllocations))
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
