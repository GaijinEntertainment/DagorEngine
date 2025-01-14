// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include "device_resource.h"
#include "descriptor_set.h"
#include "debug_naming.h"
#include "timeline_latency.h"

namespace drv3d_vulkan
{

class ExecutionContext;
class MemoryHeapResource;

enum BufferMemoryFlags
{
  NONE = 0x0,
  DEDICATED = 0x1,
  TEMP = 0x2,
  FAILABLE = 0x4,
  IN_PLACED_HEAP = 0x8,
  FRAMEMEM = 0x10
};

struct BufferDescription
{
  uint32_t blockSize;
  DeviceMemoryClass memoryClass;
  uint32_t discardBlocks;
  BufferMemoryFlags memFlags;

  static DeviceMemoryClass getDeviceReadDynamicBuffersMemoryClass();

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
  static DeviceMemoryClass memoryClassFromCflags(uint32_t cflag)
  {
    if (cflag & SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE)
      return DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;

    // SBCF_CPU_* is for staging buffer logic, but when only both this RW bits are set
    // use HOST resident heap because we intendedly treat this buffer as CPU staging
    if (cflag == SBCF_STAGING_BUFFER)
      return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;

    if (cflag & SBCF_DYNAMIC)
    {
      if (cflag & SBCF_CPU_ACCESS_READ)
        return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;

      // CB and indirect buffers are surprisingly slower to read on GPUs from shared / CPU memory
      // batch copy from temp/staging memory is faster
      // in case of dedicated memory, otherwise using shared memory class is better
      if (cflag & (SBCF_BIND_CONSTANT | SBCF_INDIRECT))
        return getDeviceReadDynamicBuffersMemoryClass();

      return DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;
    }
    return DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
  }
};

typedef ResourceImplBase<BufferDescription, VulkanBufferHandle, ResourceType::BUFFER> BufferImplBase;

class Buffer : public BufferImplBase,
               public ResourceExecutionSyncableExtend,
               public ResourcePlaceableExtend,
               public ResourceBindlessExtend
{
public:
  static constexpr int CLEANUP_DESTROY = 0;
  static constexpr uint32_t DISCARD_INDEX_FRAMEMEM = ~0;

  void destroyVulkanObject();
  void createVulkanObject();
  MemoryRequirementInfo getMemoryReq();
  VkMemoryRequirements getSharedHandleMemoryReq();
  void bindMemory();
  void bindMemoryFromHeap(MemoryHeapResource *in_heap, ResourceMemoryId heap_mem_id);
  void reuseHandle();
  void releaseSharedHandle();
  void evict();
  bool isEvictable();
  void shutdown();
  bool nonResidentCreation();
  void restoreFromSysCopy(ExecutionContext &ctx);
  void makeSysCopy(ExecutionContext &ctx);
  bool mayAlias() { return desc.memFlags & IN_PLACED_HEAP; }

  template <int Tag>
  void onDelayedCleanupBackend();

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

private:
  eastl::unique_ptr<VulkanBufferViewHandle[]> views;

  uint32_t discardIndex = 0;
  uint32_t lastDiscardFrame = 0;
  uint32_t currentDiscardOffset = 0;
  uint32_t currentDiscardSize = 0;
  uint8_t *mappedPtr = nullptr;
  VkDeviceSize sharedOffset = 0;
  VkDeviceSize memoryOffset = 0;
  VulkanDeviceMemoryHandle nonCoherentMemoryHandle{};
  void bindAssignedMemoryId();

  void fillPointers(const ResourceMemory &mem);

public:
  void markNonCoherentRangeAbs(uint32_t offset, uint32_t size, bool flush);
  void markNonCoherentRangeLoc(uint32_t offset, uint32_t size, bool flush);

  // additional 1 frame for nooverwrite accesses, that may happen before discard on same work item
  // (so ring item usage on GPU may complete one frame later, than discard)
  static constexpr uint32_t pending_discard_frames = TimelineLatency::replayToGPUCompletionRingBufferSize + 1;

#if DAGOR_DBGLEVEL > 0
  uint32_t discardRingAvailabilityFrame[pending_discard_frames] = {};
  void changeAndVerifyDiscardRingIndex(uint32_t front_frame_idx)
  {
    uint32_t oldRingIdx = discardIndex % pending_discard_frames;
    uint32_t newRingIdx = (discardIndex + 1) % pending_discard_frames;
    G_ASSERTF(TimelineLatency::isCompleted(discardRingAvailabilityFrame[newRingIdx], front_frame_idx),
      "vulkan: discard ring index %u of buffer %p:%s still in use while it is about to be reused, expected after frame %u but got "
      "on frame %u",
      newRingIdx, this, getDebugName(), discardRingAvailabilityFrame[newRingIdx], front_frame_idx);
    // old ring will be available after N frames starting from current frame
    discardRingAvailabilityFrame[oldRingIdx] = TimelineLatency::getNextRingedReplay(front_frame_idx);
  }
#else
  void changeAndVerifyDiscardRingIndex(uint32_t) {}
#endif


  inline bool onDiscard(uint32_t frontFrameIdx, uint32_t size)
  {
    G_ASSERT(currentDiscardOffset + currentDiscardSize <= getTotalSize());

    if (desc.discardBlocks == 1)
      return false;

    uint32_t frameDiscardSize = getTotalSize() / pending_discard_frames;
    if (size > frameDiscardSize)
      return false;

    bool indexChanged = lastDiscardFrame != frontFrameIdx;
    if (indexChanged)
    {
      changeAndVerifyDiscardRingIndex(frontFrameIdx);
      lastDiscardFrame = frontFrameIdx;
      ++discardIndex;
    }

    uint32_t ringIdx = discardIndex % pending_discard_frames;
    if (indexChanged)
      currentDiscardOffset = frameDiscardSize * ringIdx;
    else
      currentDiscardOffset += currentDiscardSize;

    currentDiscardSize = size;
    return (currentDiscardOffset + currentDiscardSize) <= frameDiscardSize * (ringIdx + 1);
  }

  inline bool onDiscardFramemem(uint32_t frontFrameIdx, uint32_t size)
  {
    G_ASSERT(currentDiscardOffset + currentDiscardSize <= getTotalSize());

    uint32_t frameDiscardSize = getTotalSize();
    if (size > frameDiscardSize)
      return false;

    if (frontFrameIdx != lastDiscardFrame)
      currentDiscardOffset = 0;
    else
      currentDiscardOffset += currentDiscardSize;

    markDiscardUsageRange(frontFrameIdx, size);

    return (currentDiscardOffset + currentDiscardSize) <= frameDiscardSize;
  }

  uint32_t getCurrentDiscardVisibleSize() const { return currentDiscardSize ? currentDiscardSize : getBlockSize(); }
  uint32_t getCurrentDiscardOffset() const
  {
    G_ASSERT(currentDiscardOffset < getTotalSize());
    return currentDiscardOffset;
  }
  void addFrameMemRef() { ++discardIndex; }
  void removeFrameMemRef()
  {
    G_ASSERTF(discardIndex > 0, "vulkan: too much reference removals on framemem buffer");
    --discardIndex;
  }
  bool isFrameMemReferencedAtRemoval()
  {
#if DAGOR_DBGLEVEL > 0
    if (isFakeFrameMem())
      return false;
#endif
    if (desc.memFlags & BufferMemoryFlags::FRAMEMEM)
      return discardIndex > 0;
    return false;
  }
  void checkFrameMemAccess()
  {
#if DAGOR_DBGLEVEL > 0
    if (desc.memFlags & BufferMemoryFlags::FRAMEMEM)
      verifyFrameMem();
#endif
  }
  bool isFrameMem() const { return (desc.memFlags & BufferMemoryFlags::FRAMEMEM) > 0; }
  inline uint32_t getDiscardBlocks() const { return desc.discardBlocks; }
  inline VulkanBufferViewHandle getViewOfDiscardIndex(uint32_t i) const { return views[i]; }
  inline bool hasView() const { return nullptr != views; }
  bool hasMappedMemory() const;

private:
  void verifyFrameMem();
  bool isFakeFrameMem();

public:
  // <....*..................> - memory
  //      ^ memory offset / device address
  //      <...*..............> - shared buffer object (buf suballoc)
  //          ^ shared buffer offset
  //          <...|...|...>    - buffer object
  //              ^ discard block offset
  //              <...>        - discard block
  //                ^ offset

  // 4 type of offsets with different bases
  // memory
  //   Abs - memory offset + shared buffer offset + X
  //   Loc - memory offset + shared buffer offset + discard block offset + X
  // buffer
  //   Abs - shared buffer offset + X
  //   Loc - shared buffer offset + discard block offset + X
  // device
  //   Abs - device address + shared buffer offset + X
  //   Loc - device address + shared buffer offset + discard block offset + X
  // ptr (mapped)
  //   Abs - mappedPtr + X
  //   Loc - mappedPtr + discard block offset + X

  VkDeviceSize memOffsetAbs(VkDeviceSize ofs) const { return memoryOffset + sharedOffset + ofs; }
  VkDeviceSize bufOffsetAbs(VkDeviceSize ofs) const { return sharedOffset + ofs; }
  uint8_t *ptrOffsetAbs(VkDeviceSize ofs) const { return mappedPtr + ofs; }

  VkDeviceSize memOffsetLoc(VkDeviceSize ofs) const { return memOffsetAbs(ofs) + getCurrentDiscardOffset(); }
  VkDeviceSize bufOffsetLoc(VkDeviceSize ofs) const { return bufOffsetAbs(ofs) + getCurrentDiscardOffset(); }
  uint8_t *ptrOffsetLoc(VkDeviceSize ofs) const { return ptrOffsetAbs(ofs) + getCurrentDiscardOffset(); }

#if VK_KHR_buffer_device_address
  VkDeviceAddress getDeviceAddress(VulkanDevice &device) const;
  VkDeviceAddress devOffsetAbs(VkDeviceSize ofs) const;
  VkDeviceAddress devOffsetLoc(VkDeviceSize ofs) const { return devOffsetAbs(ofs) + getCurrentDiscardOffset(); }
#endif

  VkDeviceSize getBlockSize() const { return desc.blockSize; }
  VkDeviceSize getTotalSize() const { return desc.blockSize * desc.discardBlocks; }
  void markDiscardUsageRange(uint32_t frontFrameIdx, uint32_t sz)
  {
    currentDiscardSize = sz;
    lastDiscardFrame = frontFrameIdx;
    G_ASSERT(currentDiscardSize <= getTotalSize());
  }

  Buffer(const Description &in_desc, bool managed = true) : BufferImplBase(in_desc, managed)
  {
    // set discard index to 1 for framemem buffers to avoid sending command to backend
    // in place where lock ordering goes wrong
    if (in_desc.memFlags & BufferMemoryFlags::FRAMEMEM)
      discardIndex = 1;
  }
  static VkBufferUsageFlags getUsage(VulkanDevice &device, DeviceMemoryClass memClass);

  static Buffer *create(uint32_t size, DeviceMemoryClass memory_class, uint32_t discard_count, BufferMemoryFlags mem_flags);
  void addBufferView(FormatStore format);
};

inline VkFormat VSDTToVulkanFormat(uint32_t val)
{
  switch (val)
  {
    case VSDT_FLOAT1: return VK_FORMAT_R32_SFLOAT;
    case VSDT_FLOAT2: return VK_FORMAT_R32G32_SFLOAT;
    case VSDT_FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
    case VSDT_FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case VSDT_E3DCOLOR: return VK_FORMAT_B8G8R8A8_UNORM;
    case VSDT_UBYTE4: return VK_FORMAT_R8G8B8A8_UINT;
    case VSDT_SHORT2: return VK_FORMAT_R16G16_SINT;
    case VSDT_SHORT4: return VK_FORMAT_R16G16B16A16_SINT;
    case VSDT_SHORT2N: return VK_FORMAT_R16G16_SNORM;
    case VSDT_SHORT4N: return VK_FORMAT_R16G16B16A16_SNORM;
    case VSDT_USHORT2N: return VK_FORMAT_R16G16_UNORM;
    case VSDT_USHORT4N: return VK_FORMAT_R16G16B16A16_UNORM;
    case VSDT_UDEC3: return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case VSDT_DEC3N: return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
    case VSDT_HALF2: return VK_FORMAT_R16G16_SFLOAT;
    case VSDT_HALF4: return VK_FORMAT_R16G16B16A16_SFLOAT;
    default: G_ASSERTF(false, "invalid vertex declaration type"); return VK_FORMAT_UNDEFINED;
  }
};

} // namespace drv3d_vulkan
