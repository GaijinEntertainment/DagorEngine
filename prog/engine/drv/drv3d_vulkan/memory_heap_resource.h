// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "device_resource.h"

namespace drv3d_vulkan
{

struct MemoryHeapResourceDescription
{
  VkDeviceSize size;
  DeviceMemoryClass memClass;
  bool useDedicated;

  MemoryHeapResourceDescription() = default;

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

  void tooLongCleanupDelayComplain();
  uint32_t cleanupTries = 0;
  static constexpr uint32_t MAX_CLEANUP_RETRIES = GPU_TIMELINE_HISTORY_SIZE * 2;

public:
  // external handlers
  static constexpr int CLEANUP_DESTROY = 0;

  template <int Tag>
  void onDelayedCleanupBackend(){};

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

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

  bool nonResidentCreation() { return true; }

  void bindMemory() {}

  void reuseHandle() { DAG_FATAL("vulkan: no handle reuse for heaps"); }
  void shutdown() {}
  void releaseSharedHandle() { DAG_FATAL("vulkan: no handle reuse for heaps"); }
};

} // namespace drv3d_vulkan
