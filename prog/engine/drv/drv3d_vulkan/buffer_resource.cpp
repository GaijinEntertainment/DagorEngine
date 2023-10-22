#include "device.h"
#include "pipeline.h"
#include <3d/tql.h>

using namespace drv3d_vulkan;

static constexpr VkBufferUsageFlags default_buffer_usage =
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
static constexpr uint8_t buffer_tail_filler = 0xA7;
#endif

VkBufferUsageFlags Buffer::getUsage(VulkanDevice &device, DeviceMemoryClass memClass)
{
  G_UNUSED(device); // make it as used here, makes preprocessor stuff simpler
  auto usage = default_buffer_usage;
#if VK_EXT_conditional_rendering
  if (device.hasExtension<ConditionalRenderingEXT>())
  {
    usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
  }
#endif
#if VK_KHR_buffer_device_address
  if (device.hasExtension<BufferDeviceAddressKHR>() && memClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER)
  {
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  }
#endif
#if VK_KHR_acceleration_structure
  if (device.hasExtension<AccelerationStructureKHR>())
  {
    usage |=
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
  }
#endif
  return usage;
}

#if VK_KHR_buffer_device_address
VkDeviceAddress Buffer::getDeviceAddress(VulkanDevice &device)
{
  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  info.buffer = getHandle();
  return device.vkGetBufferDeviceAddressKHR(device.get(), &info);
}
#endif

void Buffer::reportToTQL(bool is_allocating)
{
  int kbz = tql::sizeInKb(getMemory().size);
  tql::on_buf_changed(is_allocating, is_allocating ? kbz : -kbz);
}

void Buffer::destroyVulkanObject()
{
  G_ASSERT(getMemory().isDeviceMemory());
  G_ASSERT(!isHandleShared());

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  reportToTQL(false);
  get_device().getVkDevice().vkDestroyBuffer(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

namespace drv3d_vulkan
{

template <>
void Buffer::onDelayedCleanupFinish<Buffer::CLEANUP_DESTROY>()
{
  Device &drvDev = get_device();
  drvDev.resources.free(this);
}

} // namespace drv3d_vulkan

void Buffer::createVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  VkBufferCreateInfo bci;
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = NULL;
  bci.flags = 0;
  bci.size = desc.blockSize * desc.discardCount;
  bci.usage = getUsage(dev, desc.memoryClass);
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bci.queueFamilyIndexCount = 0;
  bci.pQueueFamilyIndices = NULL;

  VulkanBufferHandle ret{};
  VULKAN_EXIT_ON_FAIL(dev.vkCreateBuffer(dev.get(), &bci, NULL, ptr(ret)));
  setHandle(generalize(ret));
}

MemoryRequirementInfo Buffer::getMemoryReq()
{
  G_ASSERT(getBaseHandle());

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  // TODO: can be moved from vulkan device here
  MemoryRequirementInfo ret = get_memory_requirements(dev, getHandle());
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
  ret.requirements.size *= 2;
#endif
  return ret;
}

void Buffer::bindMemory()
{
  G_ASSERT(getBaseHandle());
  G_ASSERT(getMemoryId() != -1);

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  const ResourceMemory &mem = getMemory();
  // we are binding object to memory offset
  baseMemoryOffset = 0;
  if (mem.mappedPointer)
  {
    mappedPtr = mem.mappedPtrOffset(baseMemoryOffset);
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
    memset(mappedPtr + mem.size / 2, buffer_tail_filler, mem.size / 2);
#endif
  }
  if (!drvDev.isCoherencyAllowedFor(mem.memType))
  {
    nonCoherentMemoryHandle = mem.deviceMemory();
    nonCoherentMemoryOffset = mem.offset;
  }
  VULKAN_EXIT_ON_FAIL(dev.vkBindBufferMemory(dev.get(), getHandle(), mem.deviceMemory(), mem.offset));

  reportToTQL(true);
}

void Buffer::reuseHandle()
{
  G_ASSERT(getMemoryId() != -1);

  const ResourceMemory &mem = getMemory();
  G_ASSERT(!mem.isDeviceMemory());

  baseMemoryOffset = mem.offset;
  setHandle(mem.handle);
}

void Buffer::evict() { fatal("vulkan: buffers are not evictable"); }

void Buffer::restoreFromSysCopy() { fatal("vulkan: buffers are not evictable"); }

bool Buffer::nonResidentCreation() { return (desc.memFlags & BufferMemoryFlags::FAILABLE) != 0; }

void Buffer::makeSysCopy() { fatal("vulkan: buffers are not evictable"); }

bool Buffer::isEvictable() { return false; }

void Buffer::shutdown()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  if (views)
  {
    for (uint32_t i = 0; i < desc.discardCount; ++i)
      dev.vkDestroyBufferView(dev.get(), views[i], nullptr);
    views.reset();
  }

#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
  if (mappedPtr)
  {
    const ResourceMemory &mem = getMemory();
    uint8_t *tailPtr = mappedPtr + mem.size / 2;
    for (int i = 0; i < mem.size / 2; ++i)
    {
      if (*tailPtr != buffer_tail_filler)
        logerr("buffer is corrupted at shutdown: name %s pos %u", getDebugName(), i);
      ++tailPtr;
    }
  }
#endif

  discardAvailableFrames.reset();
}

uint8_t *Buffer::getMappedOffset(VkDeviceSize ofs) const { return mappedPtr + ofs; }

bool Buffer::hasMappedMemory() const { return mappedPtr != nullptr; }

void Buffer::markNonCoherentRange(uint32_t offset, uint32_t size, bool flush)
{
  // coherent memory does not need this
  if (is_null(nonCoherentMemoryHandle))
    return;

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr};
  range.memory = nonCoherentMemoryHandle;
  range.offset = dataOffset(offset) + nonCoherentMemoryOffset;
  range.size = size;

  // Conform it to VUID-VkMappedMemoryRange-size-01390
  // Assume that the underlying buffer memory is properly aligned.
  VkDeviceSize alignmentMask = get_device().getMinimalBufferAlignment() - 1;
  range.size += range.offset & alignmentMask;
  range.offset = range.offset & ~alignmentMask;
  range.size = (range.size + alignmentMask) & ~alignmentMask;

  if (flush)
  {
    VULKAN_EXIT_ON_FAIL(dev.vkFlushMappedMemoryRanges(dev.get(), 1, &range));
  }
  else
  {
    VULKAN_EXIT_ON_FAIL(dev.vkInvalidateMappedMemoryRanges(dev.get(), 1, &range));
  }
}

void Buffer::Description::fillAllocationDesc(AllocationDesc &alloc_desc) const
{
  alloc_desc.reqs = {};
  bool noSuballoc = (memFlags & BufferMemoryFlags::DEDICATED) != 0;
  alloc_desc.canUseSharedHandle = !noSuballoc;
  alloc_desc.forceDedicated = noSuballoc;
  alloc_desc.reqs.requirements.alignment = get_device().getMinimalBufferAlignment();
  alloc_desc.reqs.requirements.size = blockSize * discardCount;
  alloc_desc.memClass = memoryClass;
  alloc_desc.temporary = (memFlags & BufferMemoryFlags::TEMP) != 0;
  alloc_desc.objectBaked = 0;
}
