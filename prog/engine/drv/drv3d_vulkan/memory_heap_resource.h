// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "device_resource.h"
#include "cleanup_queue_tags.h"

namespace drv3d_vulkan
{

union MemoryHeapMaskedClass
{
  uintptr_t uptr;
  struct
  {
#if _TARGET_64BIT
    uint32_t mask;
    DeviceMemoryClass _class;
#else
    uint32_t mask : 28;
    DeviceMemoryClass _class : 4;
#endif
  };

  MemoryHeapMaskedClass(DeviceMemoryClass __class, uint32_t _mask) : mask(_mask), _class(__class) {}
  MemoryHeapMaskedClass(uintptr_t v) : uptr(v) {}
};
static_assert(sizeof(MemoryHeapMaskedClass) == sizeof(uintptr_t));

struct MemoryHeapResourceDescription
{
  VkDeviceSize size;
  MemoryHeapMaskedClass memType;
  bool useDedicated;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<MemoryHeapResourceDescription, VulkanDeviceMemoryHandle, ResourceType::HEAP> MemoryHeapResourceImplBase;

class MemoryHeapResource : public MemoryHeapResourceImplBase
{
#if DAGOR_DBGLEVEL > 0
private:
  uint32_t placedResources = 0;

public:
  void onResourcePlace() { ++placedResources; }
  void onResourceRemove() { --placedResources; }
#endif
  struct ResourceRecreateInfo
  {
    Resource *res;
    VkDeviceSize offset;
    VkDeviceSize size;
  };
  dag::Vector<ResourceRecreateInfo> recreations;

  template <typename ResType>
  void onDeviceResetInHeapCb(const ResourceMemory &heap_mem, ResType *res);
  template <typename ResType>
  void afterDeviceResetInHeapCb(ResType *res, VkDeviceSize offset, VkDeviceSize size);

  void tooLongCleanupDelayComplain();
  uint32_t cleanupTries = 0;
  static constexpr uint32_t MAX_CLEANUP_RETRIES = GPU_TIMELINE_HISTORY_SIZE * 2;

public:
  template <CleanupTag Tag>
  void onDelayedCleanupBackend() {};

  template <CleanupTag Tag>
  void onDelayedCleanupFinish();

  void onDeviceReset();
  void afterDeviceReset();

  MemoryHeapResource(MemoryHeapResourceDescription heap_desc, bool manage = true) : ResourceImplBase(heap_desc, manage) {}

  void createVulkanObject();
  void destroyVulkanObject();
  bool hasPlacedResources();

  MemoryRequirementInfo getMemoryReq()
  {
    MemoryRequirementInfo ret{};
    ret.requirements.alignment = 1;
    return ret;
  }

  VkMemoryRequirements getSharedHandleMemoryReq()
  {
    DAG_FATAL("vulkan: no handle reuse for heaps");
    return {};
  }

  bool nonResidentCreation() { return false; }

  void bindMemory() {}

  void reuseHandle() { DAG_FATAL("vulkan: no handle reuse for heaps"); }
  void shutdown() {}
  void releaseSharedHandle() { DAG_FATAL("vulkan: no handle reuse for heaps"); }

  template <typename ResType>
  void place(ResType *res, VkDeviceSize offset, VkDeviceSize size);
};

} // namespace drv3d_vulkan
