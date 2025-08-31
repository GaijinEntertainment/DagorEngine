// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globals.h"
#include "resource_manager.h"
#include "buffer.h"
#include "debug_naming.h"
#include "driver_config.h"
#include "backend.h"
#include "pipeline_state.h"
#include "pipeline_state_pending_references.h"

#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)

namespace drv3d_vulkan
{

template <>
void RaytraceAccelerationStructure::onDelayedCleanupFinish<CleanupTag::DESTROY_TOP>()
{
  markDead();

  if (Backend::State::pipe.isReferenced(this))
  {
    Backend::State::pendingCleanups.removeReferenced(this);
    destroyPrimaryVulkanObject();
    return;
  }
  Globals::Mem::res.free(this);
}

template <>
void RaytraceAccelerationStructure::onDelayedCleanupFinish<CleanupTag::DESTROY_BOTTOM>()
{
  Globals::Mem::res.free(this);
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

namespace
{
VkGeometryFlagsKHR toGeometryFlagsKHR(RaytraceGeometryDescription::Flags flags)
{
  VkGeometryFlagsKHR result = 0;
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::IS_OPAQUE))
  {
    result |= VK_GEOMETRY_OPAQUE_BIT_KHR;
  }
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::NO_DUPLICATE_ANY_HIT_INVOCATION))
  {
    result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  }
  return result;
}
VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRAABBs(
  const RaytraceGeometryDescription::AABBsInfo &info)
{
  auto buf = (GenericBufferInterface *)info.buffer;
  BufferRef devBuf = buf->getBufferRef();
  VkAccelerationStructureGeometryKHR result = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  result.pNext = nullptr;
  result.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
  result.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
  result.geometry.aabbs.pNext = nullptr;
  result.geometry.aabbs.data.deviceAddress = devBuf.devOffset(info.offset);
  result.geometry.aabbs.stride = info.stride;
  result.flags = toGeometryFlagsKHR(info.flags);
  return result;
}
VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRTriangles(
  const RaytraceGeometryDescription::TrianglesInfo &info)
{
  const BufferRef &devVbuf = ((GenericBufferInterface *)info.vertexBuffer)->getBufferRef();

  VkAccelerationStructureGeometryKHR result = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  result.pNext = nullptr;
  result.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  result.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  result.geometry.triangles.pNext = nullptr;
  result.geometry.triangles.vertexData.deviceAddress =
    devVbuf.devOffset(info.vertexOffset * info.vertexStride + info.vertexOffsetExtraBytes);
  result.geometry.triangles.maxVertex = devVbuf.visibleDataSize / info.vertexStride - 1; // assume any vertex can be accessed
  result.geometry.triangles.vertexStride = info.vertexStride;
  result.geometry.triangles.vertexFormat = VSDTToVulkanFormat(info.vertexFormat);
  if (info.indexBuffer)
  {
    auto ibuf = (GenericBufferInterface *)info.indexBuffer;
    const BufferRef &devIbuf = ibuf->getBufferRef();
    result.geometry.triangles.indexData.deviceAddress = devIbuf.devOffset(0);
    result.geometry.triangles.indexType = ibuf->getIndexType();
  }
  if (info.transformBuffer)
  {
    auto tbuf = (GenericBufferInterface *)info.transformBuffer;
    const BufferRef &devTbuf = tbuf->getBufferRef();
    result.geometry.triangles.transformData.deviceAddress = devTbuf.devOffset(info.transformOffset * sizeof(float) * 3 * 4);
  }
  result.flags = toGeometryFlagsKHR(info.flags);
  return result;
}

} // namespace

VkAccelerationStructureGeometryKHR drv3d_vulkan::RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(
  const RaytraceGeometryDescription &desc)
{
  switch (desc.type)
  {
    case RaytraceGeometryDescription::Type::TRIANGLES:
      return RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRTriangles(desc.data.triangles);
    case RaytraceGeometryDescription::Type::AABBS:
      return RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRAABBs(desc.data.aabbs);
  }
  VkAccelerationStructureGeometryKHR def{};
  return def;
}

void RaytraceAccelerationStructure::destroyPrimaryVulkanObject()
{
  if (!is_null(getHandle()))
  {
    VulkanDevice &dev = Globals::VK::dev;
    VULKAN_LOG_CALL(dev.vkDestroyAccelerationStructureKHR(dev.get(), getHandle(), VKALLOC(acceleration_structure)));
    setHandle(generalize(Handle()));
  }
}

void RaytraceAccelerationStructure::destroyVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;
  destroyPrimaryVulkanObject();
  VULKAN_LOG_CALL(dev.vkDestroyBuffer(dev.get(), bufHandle, VKALLOC(buffer)));
  reportToTQL(false);
}

void RaytraceAccelerationStructure::createVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;

  // TODO: sub-allocate from fewer buffers
  { // Create buffer
    VkBufferCreateInfo bci;
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext = NULL;
    bci.flags = 0;
    bci.size = desc.size;
    bci.usage = Buffer::getUsage(DeviceMemoryClass::DEVICE_RESIDENT_BUFFER);
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices = NULL;

    VulkanBufferHandle ret{};
    VULKAN_EXIT_ON_FAIL(dev.vkCreateBuffer(dev.get(), &bci, VKALLOC(buffer), ptr(ret)));

    bufHandle = ret;
  }

  { // Allocate memory for a buffer
    AllocationDesc alloc_desc = {*this};
    alloc_desc.memClass = DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
    alloc_desc.reqs = get_memory_requirements(bufHandle);

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
  asci.type = desc.isTopLevel ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  asci.buffer = bufHandle;
  asci.offset = 0;
  asci.size = desc.size;

  Handle ret;
  VULKAN_EXIT_ON_FAIL(dev.vkCreateAccelerationStructureKHR(dev.get(), &asci, VKALLOC(acceleration_structure), ptr(ret)));
  setHandle(generalize(ret));

  if (Globals::cfg.debugLevel)
    Globals::Dbg::naming.setAccelerationStructureName(this,
      String(64, "RTAS %s from %s", desc.isTopLevel ? "top" : "bottom", backtrace::get_stack()));

  reportToTQL(true);
}

MemoryRequirementInfo RaytraceAccelerationStructure::getMemoryReq()
{
  MemoryRequirementInfo ret{};
  ret.requirements.alignment = 1;
  ret.requirements.memoryTypeBits = 0xFFFFFFFF;
  return ret;
}

VkMemoryRequirements RaytraceAccelerationStructure::getSharedHandleMemoryReq()
{
  DAG_FATAL("vulkan: no shared handle mode for RT AS");
  return {};
}

void RaytraceAccelerationStructure::bindMemory() {}

void RaytraceAccelerationStructure::reuseHandle() { DAG_FATAL("vulkan: no shared handle mode for RT AS"); }

void RaytraceAccelerationStructure::releaseSharedHandle() { DAG_FATAL("vulkan: no shared handle mode for RT AS"); }

void RaytraceAccelerationStructure::evict() { DAG_FATAL("vulkan: RT AS is not evictable"); }

void RaytraceAccelerationStructure::restoreFromSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: RT AS is not evictable"); }

bool RaytraceAccelerationStructure::nonResidentCreation() { return false; }

void RaytraceAccelerationStructure::makeSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: RT AS is not evictable"); }

void RaytraceAccelerationStructure::onDeviceReset() {}
void RaytraceAccelerationStructure::afterDeviceReset() {}

bool RaytraceAccelerationStructure::isEvictable() { return false; }

void RaytraceAccelerationStructure::shutdown()
{
  // nothing ?
}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
VkDeviceAddress RaytraceAccelerationStructure::getDeviceAddress()
{
  VkAccelerationStructureDeviceAddressInfoKHR info = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
  info.accelerationStructure = getHandle();
  return Globals::VK::dev.vkGetAccelerationStructureDeviceAddressKHR(Globals::VK::dev.get(), &info);
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

RaytraceAccelerationStructure *RaytraceAccelerationStructure::create(bool top_level, VkDeviceSize size)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  WinAutoLock lk(Globals::Mem::mutex);
  return Globals::Mem::res.alloc<RaytraceAccelerationStructure>({top_level, size});
#else
  G_UNUSED(desc);
  G_UNUSED(count);
  G_UNUSED(flags);
  return nullptr;
#endif
}

#endif // VULKAN_HAS_RAYTRACING
