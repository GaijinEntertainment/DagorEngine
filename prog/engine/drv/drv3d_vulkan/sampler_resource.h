#pragma once

#include "driver.h"
#include "device_resource.h"


namespace drv3d_vulkan
{

struct SamplerDescription
{
  SamplerState state;

  SamplerDescription() = default;
  SamplerDescription(SamplerState state) : state(state) {}

  void fillAllocationDesc(AllocationDesc &alloc_desc) const
  {
    alloc_desc.reqs = {};
    alloc_desc.canUseSharedHandle = 0;
    alloc_desc.forceDedicated = 0;
    alloc_desc.memClass = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
    alloc_desc.temporary = 0;
    alloc_desc.objectBaked = 1;
  }
};

typedef ResourceImplBase<SamplerDescription, VulkanSamplerHandle, ResourceType::SAMPLER> SamplerResourceImplBase;

class SamplerResource : public SamplerResourceImplBase
{
public:
  // external handlers
  static constexpr int CLEANUP_DESTROY = 0;

  SamplerInfo samplerInfo;

  SamplerResource(SamplerDescription samplerDesc, bool manage = true) : SamplerResourceImplBase(samplerDesc, manage) {}

  void createVulkanObject();
  void destroyVulkanObject();

  MemoryRequirementInfo getMemoryReq()
  {
    MemoryRequirementInfo ret{};
    ret.requirements.alignment = 1;
    return ret;
  }

  VkMemoryRequirements getSharedHandleMemoryReq()
  {
    DAG_FATAL("vulkan: no handle reuse for samplers");
    return {};
  }

  bool nonResidentCreation() { return false; }

  void bindMemory() {}

  void reuseHandle() { DAG_FATAL("vulkan: no handle reuse for samplers"); }
};

} // namespace drv3d_vulkan
