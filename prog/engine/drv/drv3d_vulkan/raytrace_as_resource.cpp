#include "device.h"

#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)

namespace drv3d_vulkan
{

template <>
void RaytraceAccelerationStructure::onDelayedCleanupFinish<RaytraceAccelerationStructure::CLEANUP_DESTROY_TOP>()
{
  get_device().resources.free(this);
}

template <>
void RaytraceAccelerationStructure::onDelayedCleanupFinish<RaytraceAccelerationStructure::CLEANUP_DESTROY_BOTTOM>()
{
  get_device().resources.free(this);
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

void RaytraceAccelerationStructure::destroyVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();
  VULKAN_LOG_CALL(dev.vkDestroyBuffer(dev.get(), bufHandle, NULL));
  VULKAN_LOG_CALL(dev.vkDestroyAccelerationStructureKHR(dev.get(), getHandle(), NULL));
  setHandle(generalize(Handle()));
}

void RaytraceAccelerationStructure::createVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  eastl::vector<VkAccelerationStructureGeometryKHR> geometryDef;
  eastl::vector<uint32_t> maxPrimCounts;
  bool isTopAS = (desc.geometry == nullptr);
  if (!isTopAS)
  {
    geometryDef.reserve(desc.count);
    for (uint32_t i = 0; i < desc.count; ++i)
    {
      geometryDef.push_back(RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(dev, desc.geometry[i]));
      maxPrimCounts.push_back(desc.geometry[i].data.triangles.indexCount / 3);
    }
  }
  else
  {
    VkAccelerationStructureGeometryKHR &tlasGeo = geometryDef.emplace_back();
    tlasGeo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeo.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    maxPrimCounts.push_back(desc.count);
  }

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  buildInfo.type = isTopAS ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.flags = ToVkBuildAccelerationStructureFlagsKHR(desc.flags);
  buildInfo.geometryCount = uint32_t(geometryDef.size());
  buildInfo.pGeometries = geometryDef.data();

  VkAccelerationStructureBuildSizesInfoKHR size_info = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  dev.vkGetAccelerationStructureBuildSizesKHR(dev.get(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
    maxPrimCounts.data(), &size_info);

  drvDev.ensureRoomForRaytraceBuildScratchBuffer(size_info.buildScratchSize);

  // TODO: sub-allocate from fewer buffers

  { // Create buffer
    VkBufferCreateInfo bci;
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext = NULL;
    bci.flags = 0;
    bci.size = size_info.accelerationStructureSize;
    bci.usage = Buffer::getUsage(dev, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER);
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices = NULL;

    VulkanBufferHandle ret{};
    VULKAN_EXIT_ON_FAIL(dev.vkCreateBuffer(dev.get(), &bci, NULL, ptr(ret)));

    bufHandle = ret;
  }

  { // Allocate memory for a buffer
    AllocationDesc alloc_desc = {*this};
    alloc_desc.memClass = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
    alloc_desc.reqs = get_memory_requirements(dev, bufHandle);

    if (!tryAllocMemory(alloc_desc))
      return;
  }

  { // Bind memory to buffer
    G_ASSERT(getMemoryId() != -1);

    const ResourceMemory &mem = getMemory();
    VULKAN_EXIT_ON_FAIL(dev.vkBindBufferMemory(dev.get(), bufHandle, mem.deviceMemory(), mem.offset));
  }

  VkAccelerationStructureCreateInfoKHR asci = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
  asci.type = isTopAS ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  asci.buffer = bufHandle;
  asci.offset = 0;
  asci.size = size_info.accelerationStructureSize;

  Handle ret;
  VULKAN_EXIT_ON_FAIL(dev.vkCreateAccelerationStructureKHR(dev.get(), &asci, nullptr, ptr(ret)));
  setHandle(generalize(ret));
}

MemoryRequirementInfo RaytraceAccelerationStructure::getMemoryReq()
{
  MemoryRequirementInfo ret{};
  ret.requirements.alignment = 1;
  return ret;
}

void RaytraceAccelerationStructure::bindMemory() {}

void RaytraceAccelerationStructure::reuseHandle() { fatal("vulkan: no shared handle mode for RT AS"); }

void RaytraceAccelerationStructure::evict() { fatal("vulkan: RT AS is not evictable"); }

void RaytraceAccelerationStructure::restoreFromSysCopy() { fatal("vulkan: RT AS is not evictable"); }

bool RaytraceAccelerationStructure::nonResidentCreation() { return false; }

void RaytraceAccelerationStructure::makeSysCopy() { fatal("vulkan: RT AS is not evictable"); }

bool RaytraceAccelerationStructure::isEvictable() { return false; }

void RaytraceAccelerationStructure::shutdown()
{
  // nothing ?
}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
VkDeviceAddress RaytraceAccelerationStructure::getDeviceAddress(VulkanDevice &device)
{
  VkAccelerationStructureDeviceAddressInfoKHR info = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
  info.accelerationStructure = getHandle();
  return device.vkGetAccelerationStructureDeviceAddressKHR(device.get(), &info);
}
#endif

void RaytraceAccelerationStructure::Description::fillAllocationDesc(AllocationDesc &alloc_desc) const
{
  alloc_desc.reqs = {};
  alloc_desc.canUseSharedHandle = 0;
  alloc_desc.forceDedicated = 0;
  alloc_desc.memClass = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
  alloc_desc.temporary = 0;
  alloc_desc.objectBaked = 1;
}

#endif // D3D_HAS_RAY_TRACING