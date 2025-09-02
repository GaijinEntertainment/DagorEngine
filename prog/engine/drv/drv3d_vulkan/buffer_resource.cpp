// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "buffer_alignment.h"
#include "backend.h"
#include "execution_context.h"
#include "bindless.h"
#include "wrapped_command_buffer.h"
#include "execution_sync.h"
#include "vulkan_allocation_callbacks.h"

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
  return Globals::Mem::pool.hasDedicatedMemory() ? DeviceMemoryClass::DEVICE_RESIDENT_BUFFER
                                                 : DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
}

VkBufferUsageFlags Buffer::getUsage(DeviceMemoryClass memClass)
{
  auto usage = default_buffer_usage;
#if VK_EXT_conditional_rendering
  if (Globals::VK::dev.hasExtension<ConditionalRenderingEXT>())
  {
    usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
  }
#endif
#if VK_KHR_buffer_device_address
  if (Globals::VK::dev.hasExtension<BufferDeviceAddressKHR>() && memClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER)
  {
    usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  }
#endif
#if VK_KHR_acceleration_structure
  if (Globals::VK::dev.hasExtension<AccelerationStructureKHR>())
  {
    usage |=
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
  }
#endif
  return usage;
}

#if VK_KHR_buffer_device_address
VkDeviceAddress Buffer::getDeviceAddress() const
{
  if (!isResident())
    return 0;
  G_ASSERTF(desc.memoryClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
    "vulkan: trying to get device address for non device resident buffer %p:%s", this, getDebugName());
  VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  info.buffer = getHandle();
  return Globals::VK::dev.vkGetBufferDeviceAddressKHR(Globals::VK::dev.get(), &info);
}

VkDeviceAddress Buffer::devOffsetAbs(VkDeviceSize ofs) const { return getDeviceAddress() + sharedOffset + ofs; }
#endif

void Buffer::destroyVulkanObject()
{
  G_ASSERT(!isHandleShared());

  if (isManaged() && isResident() && !isMemoryClassHostResident(desc.memoryClass))
    reportToTQL(false);

  Globals::VK::dev.vkDestroyBuffer(Globals::VK::dev.get(), getHandle(), VKALLOC(buffer));
  setHandle(generalize(Handle()));
}

namespace drv3d_vulkan
{

template <>
void Buffer::onDelayedCleanupBackend<CleanupTag::DESTROY>()
{
  for (uint32_t i : bindlessSlots)
    Backend::bindless.cleanupBindlessBuffer(i, this);
  bindlessSlots.clear();
}

template <>
void Buffer::onDelayedCleanupFinish<CleanupTag::DESTROY>()
{
  G_ASSERTF(bindlessSlots.empty(), "vulkan: bindless slots are not free at destruction of buffer %p:%s", this, getDebugName());
  G_ASSERTF(!isFrameMemReferencedAtRemoval(), "vulkan: framemem buffer %p:%s is still referenced (%u refs) while being destroyed",
    this, getDebugName(), discardIndex);

  if (desc.memFlags & BufferMemoryFlags::IN_PLACED_HEAP)
  {
    destroyVulkanObject();
    freeMemory();
    releaseHeap();
  }

  ResourceManager &rm = Globals::Mem::res;
  if (hostCopy)
  {
    rm.free(hostCopy);
    hostCopy = nullptr;
  }
  rm.free(this);
}

} // namespace drv3d_vulkan

void Buffer::createVulkanObject()
{
  VkBufferCreateInfo bci;
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = NULL;
  bci.flags = 0;
  bci.size = desc.blockSize * desc.discardBlocks;
  bci.usage = getUsage(desc.memoryClass);
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bci.queueFamilyIndexCount = 0;
  bci.pQueueFamilyIndices = NULL;

  VulkanBufferHandle ret{};
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateBuffer(Globals::VK::dev.get(), &bci, VKALLOC(buffer), ptr(ret)));
  setHandle(generalize(ret));
}

MemoryRequirementInfo Buffer::getMemoryReq()
{
  G_ASSERT(getBaseHandle());

  MemoryRequirementInfo ret = get_memory_requirements(getHandle());
#if VULKAN_MAPPED_BUFFER_OVERRUN_WRITE_CHECK > 0
  ret.requirements.size *= 2;
#endif
  return ret;
}

VkMemoryRequirements Buffer::getSharedHandleMemoryReq()
{
  VkMemoryRequirements ret;
  ret.alignment = Globals::VK::bufAlign.getForUsageAndFlags(0, Buffer::getUsage(desc.memoryClass));
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
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkBindBufferMemory(Globals::VK::dev.get(), getHandle(), mem.deviceMemory(), mem.offset));
}

void Buffer::bindMemory()
{
  bindAssignedMemoryId();
  if (!isMemoryClassHostResident(desc.memoryClass))
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
  if (!isMemoryClassHostResident(desc.memoryClass))
    reportToTQL(true);
}

void Buffer::releaseSharedHandle()
{
  G_ASSERT(!getMemory().isDeviceMemory());
  G_ASSERT(isHandleShared());
  if (!isMemoryClassHostResident(desc.memoryClass))
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

void Buffer::evict()
{
  for (uint32_t i : bindlessSlots)
    Backend::bindless.evictBindlessBuffer(i, this);
  shutdown();
}

void Buffer::onDeviceReset()
{
  shutdown();
  disallowAccessesOnGpuWorkId(-1);
}

void Buffer::afterDeviceReset()
{
  if (FormatStore(0) != viewFmt && !views)
    addBufferView(viewFmt);

  for (uint32_t i : bindlessSlots)
    Backend::bindless.restoreBindlessBuffer(i, this);

#if DAGOR_DBGLEVEL > 0
  // there is no backend tracking for host writes, so exclude such buffers from checking
  if (hasMappedMemory())
    checkAccessAfterDeviceReset(true);
#endif
}

void Buffer::restoreFromSysCopy(ExecutionContext &ctx)
{
  if (FormatStore(0) != viewFmt && !views)
    addBufferView(viewFmt);

  // was not evicted, created non-residently
  if (!hostCopy)
    return;
  ctx.queueBufferResidencyRestore(this);
}

bool Buffer::nonResidentCreation()
{
  // all other buffers are required to be CPU mapped, so no point to lazy allocate them
  if (desc.memoryClass != DeviceMemoryClass::DEVICE_RESIDENT_BUFFER)
    return false;
#if VULKAN_HAS_RAYTRACING
  // sadly but this is only way right now, to know that device address buffer is leaked to frontend or other resources
  if (Globals::Mem::res.sizeAllocated<RaytraceAccelerationStructure>() > 1) // 1 for dummy AS
    return false;
#endif
  memoryOffset = 0;
  sharedOffset = 0;
  createVulkanObject();
  // FIXME: right now non resident buffers will not contribute to TQL, biasing streaming
  return true;
}

void Buffer::delayedRestoreFromSysCopy()
{
  G_ASSERT(hostCopy);

  const VkBufferCopy hostRestoreCopy = {hostCopy->bufOffsetLoc(0), bufOffsetLoc(0), getBlockSize()};
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(hostCopy->getHandle(), getHandle(), 1, &hostRestoreCopy));

  Backend::gpuJob.get().cleanups.enqueue<CleanupTag::DESTROY>(*hostCopy);
  hostCopy = nullptr;

  for (uint32_t i : bindlessSlots)
    Backend::bindless.restoreBindlessBuffer(i, this);
}

Buffer *Buffer::getHostCopyBuffer()
{
  if (!hostCopy)
  {
    hostCopy = Buffer::create(getBlockSize(), DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::NONE);
    if (Globals::cfg.debugLevel > 0)
      hostCopy->setDebugName(String(128, "host copy of buf %s", getDebugName()));
  }
  return hostCopy;
}

void Buffer::makeSysCopy(ExecutionContext &)
{
  Buffer *buf = getHostCopyBuffer();

  uint32_t sourceBufOffset = bufOffsetLoc(0);
  uint32_t destBufOffset = buf->bufOffsetLoc(0);

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT}, this,
    {sourceBufOffset, getBlockSize()});
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}, buf, {destBufOffset, getBlockSize()});
  Backend::sync.completeNeeded();

  VkBufferCopy bc;
  bc.srcOffset = sourceBufOffset;
  bc.dstOffset = destBufOffset;
  bc.size = getBlockSize();
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBuffer(getHandle(), buf->getHandle(), 1, &bc));
}

bool Buffer::isEvictable()
{
  // host buffers are rarely a concern
  // framemem is shared and auto cleaned up every frame, cleaning max size for it is better solution
  //
  // for shared handles and device addresses,
  // we must stop work on main thread to avoid corrupting frontend visible data
  // but to stop work we must finish current one and to finish current one we must handle OOM
  // so just disable eviction on them for now
  bool ret = (desc.memoryClass == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER) && !isFrameMem() && !isHandleShared();
  ret &= (desc.memFlags & BufferMemoryFlags::NOT_EVICTABLE) == 0;
#if VULKAN_HAS_RAYTRACING
  // sadly but this is only way right now, to know that device address buffer is leaked to frontend or other resources
  ret &= Globals::Mem::res.sizeAllocated<RaytraceAccelerationStructure>() <= 1; // 1 for dummy AS
#endif
  return ret;
}

void Buffer::shutdown()
{
  if (views)
  {
    for (uint32_t i = 0; i < desc.discardBlocks; ++i)
      Globals::VK::dev.vkDestroyBufferView(Globals::VK::dev.get(), views[i], VKALLOC(buffer_view));
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
    D3D_CONTRACT_ERROR("vulkan: missed discard on framemem buffer %p:%s, expected frame %u got %u at %s", this, getDebugName(),
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
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkFlushMappedMemoryRanges(Globals::VK::dev.get(), 1, &range));
  }
  else
  {
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkInvalidateMappedMemoryRanges(Globals::VK::dev.get(), 1, &range));
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
  D3D_CONTRACT_ASSERTF(discard_count > 0, "discard count has to be at least one");
  uint32_t blockSize = Globals::VK::bufAlign.alignSize(size);

  WinAutoLock lk(Globals::Mem::mutex);
  return Globals::Mem::res.alloc<Buffer>({blockSize, memory_class, discard_count, mem_flags}, true);
}

void Buffer::addBufferView(FormatStore format)
{
  D3D_CONTRACT_ASSERTF((getBlockSize() % format.getBytesPerPixelBlock()) == 0,
    "invalid view of buffer, buffer has a size of %u, format (%s) element size is %u"
    " which leaves %u dangling bytes",
    getBlockSize(), format.getNameString(), format.getBytesPerPixelBlock(), getBlockSize() % format.getBytesPerPixelBlock());

  viewFmt = format;
  if (!isResident())
    return;

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

    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateBufferView(Globals::VK::dev.get(), &bvci, VKALLOC(buffer_view), ptr(views[d])));
  }
}
