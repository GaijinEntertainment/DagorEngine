#pragma once
#if !D3D_HAS_RAY_TRACING
#error raytrace acceleration structure resource header should not be included when RT is off
#endif

#include "device_resource.h"

namespace drv3d_vulkan
{

struct RaytraceASDescription
{
  RaytraceGeometryDescription *geometry;
  uint32_t count;
  RaytraceBuildFlags flags;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<RaytraceASDescription, VulkanAccelerationStructureKHR, ResourceType::AS> RaytraceASImplBase;

class RaytraceAccelerationStructure : public RaytraceASImplBase
{
public:
  uint64_t deviceHandle = 0;
  VulkanBufferHandle bufHandle = {};
  enum
  {
    CLEANUP_DESTROY_BOTTOM = 0,
    CLEANUP_DESTROY_TOP = 1
  };

  void destroyVulkanObject();
  void createVulkanObject();
  MemoryRequirementInfo getMemoryReq();
  void bindMemory();
  void reuseHandle();
  void evict();
  void restoreFromSysCopy();
  bool isEvictable();
  void shutdown();
  bool nonResidentCreation();
  void makeSysCopy();

  template <int Tag>
  void onDelayedCleanupBackend(ContextBackend &){};

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

  RaytraceAccelerationStructure(const Description &in_desc, bool managed = true) : RaytraceASImplBase(in_desc, managed) {}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  VkDeviceAddress getDeviceAddress(VulkanDevice &device);
#endif
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

VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(VulkanDevice &vkDev,
  const RaytraceGeometryDescription &desc);

} // namespace drv3d_vulkan