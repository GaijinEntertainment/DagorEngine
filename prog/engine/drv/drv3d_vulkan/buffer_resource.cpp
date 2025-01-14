// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "buffer_alignment.h"
#include "backend.h"
#include "execution_context.h"
#include "bindless.h"

using namespace drv3d_vulkan;

static constexpr VkBufferUsageFlags default_buffer_usage =
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
static constexpr uint8_t buffer_tail_filler = 0xA7;
#endif

DeviceMemoryClass BufferDescription::getDeviceReadDynamicBuffersMemoryClass()
{
  return Globals::Mem::pool.getMemoryConfiguration() == DeviceMemoryConfiguration::DEDICATED_DEVICE_MEMORY
           ? DeviceMemoryClass::DEVICE_RESIDENT_BUFFER
           : DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
}

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
VkDeviceAddress Buffer::getDeviceAddress(VulkanDevice &device) const
{
  G_ASSERTF(desc.memoryClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
    "vulkan: trying to get device address for non device resident buffer %p:%s", this, getDebugName());
  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  info.buffer = getHandle();
  return device.vkGetBufferDeviceAddressKHR(device.get(), &info);
}

VkDeviceAddress Buffer::devOffsetAbs(VkDeviceSize ofs) const { return getDeviceAddress(Globals::VK::dev) + sharedOffset + ofs; }

#endif

void Buffer::destroyVulkanObject()
{
  G_ASSERT(!isHandleShared());

  VulkanDevice &dev = Globals::VK::dev;

  if (isManaged())
    reportToTQL(false);

  Globals::VK::dev.vkDestroyBuffer(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

namespace drv3d_vulkan
{

template <>
void Buffer::onDelayedCleanupBackend<Buffer::CLEANUP_DESTROY>()
{
  for (uint32_t i : bindlessSlots)
    Backend::bindless.cleanupBindlessBuffer(i, this);
  bindlessSlots.clear();
}

template <>
void Buffer::onDelayedCleanupFinish<Buffer::CLEANUP_DESTROY>()
{
  G_ASSERTF(!isFrameMemReferencedAtRemoval(), "vulkan: framemem buffer %p:%s is still referenced (%u refs) while being destroyed",
    this, getDebugName(), discardIndex);

  if (desc.memFlags & BufferMemoryFlags::IN_PLACED_HEAP)
  {
    destroyVulkanObject();
    freeMemory();
    releaseHeap();
  }

  Globals::Mem::res.free(this);
}

} // namespace drv3d_vulkan

void Buffer::createVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;

  VkBufferCreateInfo bci;
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = NULL;
  bci.flags = 0;
  bci.size = desc.blockSize * desc.discardBlocks;
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

  VulkanDevice &dev = Globals::VK::dev;

  // TODO: can be moved from vulkan device here
  MemoryRequirementInfo ret = get_memory_requirements(dev, getHandle());
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
  ret.requirements.size *= 2;
#endif
  return ret;
}

VkMemoryRequirements Buffer::getSharedHandleMemoryReq()
{
  VulkanDevice &vkDev = Globals::VK::dev;

  VkMemoryRequirements ret;
  ret.alignment = Globals::VK::bufAlign.getForUsageAndFlags(0, Buffer::getUsage(vkDev, desc.memoryClass));
  ret.size = ret.alignment > 0 ? getTotalSize() : 0;
  ret.memoryTypeBits = Globals::Mem::pool.getMemoryTypeMaskForClass(desc.memoryClass);
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
  ret.size *= 2;
#endif
  return ret;
}

void Buffer::bindAssignedMemoryId()
{
  G_ASSERT(getBaseHandle());
  G_ASSERT(getMemoryId() != -1);

  const ResourceMemory &mem = getMemory();
  G_ASSERT(mem.isDeviceMemory());
  memoryOffset = mem.offset;
  sharedOffset = 0;
  fillPointers(mem);

  // we are binding object to memory offset
  VulkanDevice &dev = Globals::VK::dev;
  VULKAN_EXIT_ON_FAIL(dev.vkBindBufferMemory(dev.get(), getHandle(), mem.deviceMemory(), mem.offset));
}

void Buffer::bindMemory()
{
  bindAssignedMemoryId();
  reportToTQL(true);
}

void Buffer::bindMemoryFromHeap(MemoryHeapResource *in_heap, ResourceMemoryId heap_mem_id)
{
  setMemoryId(heap_mem_id);
  bindAssignedMemoryId();
  setHeap(in_heap);
}

void Buffer::reuseHandle()
{
  G_ASSERT(!getBaseHandle());
  G_ASSERT(getMemoryId() != -1);

  const ResourceMemory &mem = getMemory();
  G_ASSERT(!mem.isDeviceMemory());
  memoryOffset = 0;
  sharedOffset = mem.offset;
  fillPointers(mem);

  // we are reusing handle
  setHandle(mem.handle);
  reportToTQL(true);
}

void Buffer::releaseSharedHandle()
{
  G_ASSERT(!getMemory().isDeviceMemory());
  G_ASSERT(isHandleShared());
  reportToTQL(false);
  setHandle(generalize(Handle()));
}

void Buffer::fillPointers(const ResourceMemory &mem)
{
  if (mem.mappedPointer)
  {
    mappedPtr = mem.mappedPtrOffset(0);
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
    memset(mappedPtr + mem.size / 2, buffer_tail_filler, mem.size / 2);
#endif
  }
  else
    // surely avoid corrupting other buffer memory / force crash if buffer usages/creation are broken
    mappedPtr = nullptr;
  if (!Globals::cfg.bits.useCoherentMemory || !Globals::Mem::pool.isCoherentMemoryType(mem.memType))
    nonCoherentMemoryHandle = mem.isDeviceMemory() ? mem.deviceMemory() : mem.deviceMemorySlow();
}

void Buffer::evict() { DAG_FATAL("vulkan: buffers are not evictable"); }

void Buffer::restoreFromSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: buffers are not evictable"); }

bool Buffer::nonResidentCreation() { return (desc.memFlags & BufferMemoryFlags::FAILABLE) != 0; }

void Buffer::makeSysCopy(ExecutionContext &) { DAG_FATAL("vulkan: buffers are not evictable"); }

bool Buffer::isEvictable() { return false; }

void Buffer::shutdown()
{
  VulkanDevice &dev = Globals::VK::dev;

  if (views)
  {
    for (uint32_t i = 0; i < desc.discardBlocks; ++i)
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
        D3D_ERROR("buffer is corrupted at shutdown: name %s pos %u", getDebugName(), i);
      ++tailPtr;
    }
  }
#endif
}

bool Buffer::hasMappedMemory() const { return mappedPtr != nullptr; }

bool Buffer::isFakeFrameMem() { return Globals::cfg.bits.debugFrameMemUsage > 0; }

void Buffer::verifyFrameMem()
{
  ExecutionContext &ctx = Backend::State::exec.getExecutionContext();
  uint32_t discardedAt = interlocked_relaxed_load(lastDiscardFrame);
  bool passing = false;
  if (Globals::cfg.bits.debugFrameMemUsage)
    // with debug, we using normal discard approach, index may go over current frame for every frame discard
    // may fail to detect edge-error cases, but gives resilence for wast amount of false triggers
    passing = discardedAt >= ctx.data.frontFrameIndex;
  else
    passing = discardedAt == ctx.data.frontFrameIndex;

  if (!passing)
  {
    // enable Globals::cfg.bits.debugFrameMemUsage to see proper buffer name
    // but framemem logic will be not used to full extent!
    D3D_ERROR("vulkan: missed discard on framemem buffer %p:%s, expected frame %u got %u at %s", this, getDebugName(),
      ctx.data.frontFrameIndex, discardedAt, Backend::State::exec.getExecutionContext().getCurrentCmdCaller());
  }
}

void Buffer::markNonCoherentRangeLoc(uint32_t offset, uint32_t size, bool flush)
{
  markNonCoherentRangeAbs(offset + getCurrentDiscardOffset(), size, flush);
}

void Buffer::markNonCoherentRangeAbs(uint32_t offset, uint32_t size, bool flush)
{
  // coherent memory does not need this
  if (is_null(nonCoherentMemoryHandle))
    return;

  VulkanDevice &dev = Globals::VK::dev;

  VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr};
  range.memory = nonCoherentMemoryHandle;
  range.offset = memOffsetAbs(offset);
  range.size = size;

  // Conform it to VUID-VkMappedMemoryRange-size-01390
  // Assume that the underlying buffer memory is properly aligned.
  VkDeviceSize alignmentMask = Globals::VK::bufAlign.minimal - 1;
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
  alloc_desc.reqs.requirements.alignment = Globals::VK::bufAlign.minimal;
  alloc_desc.reqs.requirements.size = blockSize * discardBlocks;
  alloc_desc.memClass = memoryClass;
  alloc_desc.temporary = (memFlags & BufferMemoryFlags::TEMP) != 0;
  alloc_desc.objectBaked = 0;
}

Buffer *Buffer::create(uint32_t size, DeviceMemoryClass memory_class, uint32_t discard_count, BufferMemoryFlags mem_flags)
{
  G_ASSERTF(discard_count > 0, "discard count has to be at least one");
  uint32_t blockSize = Globals::VK::bufAlign.alignSize(size);

  WinAutoLock lk(Globals::Mem::mutex);
  return Globals::Mem::res.alloc<Buffer>({blockSize, memory_class, discard_count, mem_flags}, true);
}

void Buffer::addBufferView(FormatStore format)
{
  G_ASSERTF((getBlockSize() % format.getBytesPerPixelBlock()) == 0,
    "invalid view of buffer, buffer has a size of %u, format (%s) element size is %u"
    " which leaves %u dangling bytes",
    getBlockSize(), format.getNameString(), format.getBytesPerPixelBlock(), getBlockSize() % format.getBytesPerPixelBlock());

  uint32_t viewCount = desc.discardBlocks;
  views.reset(new VulkanBufferViewHandle[viewCount]);

  VkBufferViewCreateInfo bvci;
  bvci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
  bvci.pNext = NULL;
  bvci.flags = 0;
  bvci.buffer = getHandle();
  bvci.format = format.asVkFormat();
  bvci.range = getBlockSize();

  {
    uint64_t sizeLimit = Globals::VK::phy.properties.limits.maxStorageBufferRange;
    sizeLimit = min(sizeLimit, (uint64_t)Globals::VK::phy.properties.limits.maxTexelBufferElements * format.getBytesPerPixelBlock());

    // should be assert, but some GPUs have broken limit values, working fine with bigger buffers
    if (sizeLimit < bvci.range)
    {
      debug("maxStorageBufferRange=%d, maxTexelBufferElements=%d, getBytesPerPixelBlock=%d",
        Globals::VK::phy.properties.limits.maxStorageBufferRange, Globals::VK::phy.properties.limits.maxTexelBufferElements,
        format.getBytesPerPixelBlock());
      D3D_ERROR("vulkan: too big buffer view (%llu bytes max while asking for %u) for %llX:%s", sizeLimit, bvci.range,
        generalize(getHandle()), getDebugName());
      bvci.range = VK_WHOLE_SIZE;
    }
  }

  for (uint32_t d = 0; d < viewCount; ++d)
  {
    bvci.offset = bufOffsetAbs(d * getBlockSize());

    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateBufferView(Globals::VK::dev.get(), &bvci, NULL, ptr(views[d])));
  }
}
