#pragma once
#include "device_resource.h"
#include "descriptor_set.h"
namespace drv3d_vulkan
{

class ExecutionContext;

enum BufferMemoryFlags
{
  NONE = 0x0,
  DEDICATED = 0x1,
  TEMP = 0x2,
  FAILABLE = 0x4
};

struct BufferDescription
{
  uint32_t blockSize;
  DeviceMemoryClass memoryClass;
  uint32_t discardCount;
  BufferMemoryFlags memFlags;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<BufferDescription, VulkanBufferHandle, ResourceType::BUFFER> BufferImplBase;

class Buffer : public BufferImplBase, public ResourceExecutionSyncableExtend
{
public:
  static constexpr int CLEANUP_DESTROY = 0;

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

  template <int Tag>
  void onDelayedCleanupBackend(ContextBackend &)
  {}

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

private:
  eastl::unique_ptr<VulkanBufferViewHandle[]> views;
  eastl::unique_ptr<uint32_t[]> discardAvailableFrames;
  uint32_t currentDiscardIndex = 0;
  uint8_t *mappedPtr = nullptr;
  VkDeviceSize sharedOffset = 0;
  VkDeviceSize memoryOffset = 0;
  VulkanDeviceMemoryHandle nonCoherentMemoryHandle{};

  uint32_t getCurrentDiscardOffset() const
  {
    G_ASSERT(currentDiscardIndex < desc.discardCount);
    return desc.blockSize * currentDiscardIndex;
  }

  void reportToTQL(bool is_allocating);
  void fillPointers(const ResourceMemory &mem);

public:
  void markNonCoherentRangeAbs(uint32_t offset, uint32_t size, bool flush);
  void markNonCoherentRangeLoc(uint32_t offset, uint32_t size, bool flush);

  inline bool onDiscard(uint32_t frontFrameIdx)
  {
    G_ASSERT(currentDiscardIndex < desc.discardCount);

    // check if next discard element is available now
    uint32_t nextDiscardIndex = (currentDiscardIndex + 1) % desc.discardCount;
    if (!discardAvailableFrames)
      discardAvailableFrames.reset(new uint32_t[desc.discardCount]());

    if (discardAvailableFrames[nextDiscardIndex] < frontFrameIdx)
    {
      // use element and mark on what frame it can be used again
      // discard element will be in use in replay thread & gpu for FRAME_FRAME_BACKLOG_LENGTH frames starting from frontend observed
      // frame and backend frame index can lag behing current frame by MAX_PENDING_WORK_ITEMS
      discardAvailableFrames[currentDiscardIndex] = frontFrameIdx + FRAME_FRAME_BACKLOG_LENGTH + MAX_PENDING_WORK_ITEMS;

      currentDiscardIndex = nextDiscardIndex;
      return true;
    }

    return false;
  }

  inline uint32_t getCurrentDiscardIndex() const { return currentDiscardIndex; }
  inline uint32_t getMaxDiscardLimit() const { return desc.discardCount; }
  inline void setViews(eastl::unique_ptr<VulkanBufferViewHandle[]> v) { views = std::move(v); }
  inline VulkanBufferViewHandle getViewOfDiscardIndex(uint32_t i) const { return views[i]; }
  inline VulkanBufferViewHandle getView() const
  {
    if (views)
      return views[currentDiscardIndex];
    return VulkanBufferViewHandle();
  }
  inline bool hasView() const { return nullptr != views; }
  bool hasMappedMemory() const;

private:
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

  VkDeviceSize getBlockSize() { return desc.blockSize; }
  VkDeviceSize getTotalSize() { return desc.blockSize * desc.discardCount; }

  Buffer(const Description &in_desc, bool managed = true) : BufferImplBase(in_desc, managed), currentDiscardIndex(0) {}
  static VkBufferUsageFlags getUsage(VulkanDevice &device, DeviceMemoryClass memClass);
};

struct BufferSubAllocation
{
  Buffer *buffer = nullptr;
  VkDeviceSize offset = 0;
  VkDeviceSize size = 0;

  inline explicit operator bool() { return buffer != nullptr; }
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

// references a buffer with a fixed discard index
// provides a buffer like interface but returns values for the fixed discard index
struct BufferRef
{
  Buffer *buffer = nullptr;
  uint32_t discardIndex = 0;
  uint32_t offset = 0;
  VkDeviceSize visibleDataSize = 0;
  BufferRef() = default;
  ~BufferRef() = default;
  BufferRef(const BufferRef &) = default;
  BufferRef &operator=(const BufferRef &) = default;
  // make this explicit to make it clear that we grab and hold on to the current
  // discard index
  explicit BufferRef(Buffer *bfr, uint32_t visible_data_size = 0);
  explicit BufferRef(Buffer *bfr, uint32_t visible_data_size, uint32_t in_offset);
  explicit operator bool() const;
  VulkanBufferHandle getHandle() const;
  VulkanBufferViewHandle getView() const;
  VkDeviceSize bufOffset(VkDeviceSize ofs) const;
  VkDeviceSize memOffset(VkDeviceSize ofs) const;
  VkDeviceSize totalSize() const;
  void clear()
  {
    buffer = nullptr;
    discardIndex = 0;
    visibleDataSize = 0;
    offset = 0;
  }
};

inline bool operator==(const BufferRef &l, const BufferRef &r)
{
  return (l.buffer == r.buffer) && (l.discardIndex == r.discardIndex) && (l.offset == r.offset);
}

inline bool operator!=(const BufferRef &l, const BufferRef &r) { return !(l == r); }

inline BufferRef::BufferRef(Buffer *bfr, uint32_t visible_data_size) :
  buffer(bfr),
  discardIndex(bfr ? bfr->getCurrentDiscardIndex() : 0),
  visibleDataSize(bfr && !visible_data_size ? bfr->getBlockSize() : visible_data_size),
  offset(0)
{}

inline BufferRef::BufferRef(Buffer *bfr, uint32_t visible_data_size, uint32_t in_offset) :
  buffer(bfr),
  discardIndex(bfr ? bfr->getCurrentDiscardIndex() : 0),
  visibleDataSize(bfr && !visible_data_size ? bfr->getBlockSize() : visible_data_size),
  offset(in_offset)
{}


inline BufferRef::operator bool() const { return nullptr != buffer; }

inline VulkanBufferHandle BufferRef::getHandle() const { return buffer->getHandle(); }

inline VulkanBufferViewHandle BufferRef::getView() const
{
  return buffer->hasView() ? buffer->getViewOfDiscardIndex(discardIndex) : VulkanBufferViewHandle{};
}

inline VkDeviceSize BufferRef::bufOffset(VkDeviceSize ofs) const
{
  return buffer->getBlockSize() * discardIndex + buffer->bufOffsetAbs(ofs) + offset;
}
inline VkDeviceSize BufferRef::memOffset(VkDeviceSize ofs) const
{
  return buffer->getBlockSize() * discardIndex + buffer->memOffsetAbs(ofs) + offset;
}
inline VkDeviceSize BufferRef::totalSize() const { return buffer->getBlockSize(); }


} // namespace drv3d_vulkan
