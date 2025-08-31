// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h>

#include "device_resource.h"
#include "cleanup_queue_tags.h"

#if !VULKAN_HAS_RAYTRACING
#error raytrace acceleration structure resource header should not be included when RT is off
#endif

namespace drv3d_vulkan
{

class ExecutionContext;

struct RaytraceASDescription
{
  bool isTopLevel;
  VkDeviceSize size;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<RaytraceASDescription, VulkanAccelerationStructureKHR, ResourceType::AS> RaytraceASImplBase;

class RaytraceAccelerationStructure : public RaytraceASImplBase, public ResourceExecutionSyncableExtend
{
public:
  uint64_t deviceHandle = 0;
  VulkanBufferHandle bufHandle = {};

  void destroyVulkanObject();
  void createVulkanObject();
  MemoryRequirementInfo getMemoryReq();
  VkMemoryRequirements getSharedHandleMemoryReq();
  void bindMemory();
  void reuseHandle();
  void releaseSharedHandle();
  void evict();
  bool isEvictable();
  void shutdown();
  bool nonResidentCreation();
  void restoreFromSysCopy(ExecutionContext &ctx);
  void makeSysCopy(ExecutionContext &ctx);
  void onDeviceReset();
  void afterDeviceReset();

  template <CleanupTag Tag>
  void onDelayedCleanupBackend() {};

  template <CleanupTag Tag>
  void onDelayedCleanupFinish();

  RaytraceAccelerationStructure(const Description &in_desc, bool managed = true) : RaytraceASImplBase(in_desc, managed) {}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  VkDeviceAddress getDeviceAddress();
#endif

  static RaytraceAccelerationStructure *create(bool top_level, VkDeviceSize size);
  void destroyPrimaryVulkanObject();
};

inline VkBuildAccelerationStructureFlagsKHR ToVkBuildAccelerationStructureFlagsKHR(RaytraceBuildFlags flags)
{
  VkBuildAccelerationStructureFlagsKHR result = 0;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::ALLOW_UPDATE))
    result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::ALLOW_COMPACTION))
    result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::FAST_TRACE))
    result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::FAST_BUILD))
    result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::LOW_MEMORY))
    result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_NV;
  return result;
}

VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(
  const RaytraceGeometryDescription &desc);

} // namespace drv3d_vulkan