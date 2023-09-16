#pragma once
#include "device_resource.h"
#include "descriptor_set.h"
namespace drv3d_vulkan
{

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

class Buffer : public BufferImplBase
{
public:
  static constexpr int CLEANUP_DESTROY = 0;

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
  void onDelayedCleanupBackend(ContextBackend &)
  {}

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

private:
  uint32_t currentDiscardIndex = 0;
  eastl::unique_ptr<VulkanBufferViewHandle[]> views;
  eastl::unique_ptr<uint32_t[]> discardAvailableFrames;
  VkDeviceSize baseMemoryOffset = 0;
  uint8_t *mappedPtr = nullptr;
  VkDeviceSize nonCoherentMemoryOffset = 0;
  VulkanDeviceMemoryHandle nonCoherentMemoryHandle{};

  uint32_t getDiscardBlockSize() const { return desc.blockSize; }
  uint32_t getCurrentDiscardOffset() const
  {
    G_ASSERT(currentDiscardIndex < desc.discardCount);
    return getDiscardBlockSize() * currentDiscardIndex;
  }

  void reportToTQL(bool is_allocating);

public:
  void markNonCoherentRange(uint32_t offset, uint32_t size, bool flush);

  VkDeviceSize getBaseOffsetIntoMemory(VkDeviceSize offset) const { return offset + baseMemoryOffset; }
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

  uint8_t *getMappedOffset(VkDeviceSize ofs) const;

  inline uint8_t *dataPointer(VkDeviceSize ofs) const { return getMappedOffset(ofs + getCurrentDiscardOffset()); }
  inline VkDeviceSize dataOffset(VkDeviceSize ofs) const { return baseMemoryOffset + ofs + getCurrentDiscardOffset(); }
  inline VkDeviceSize dataOffsetWithDiscardIndex(VkDeviceSize ofs, uint32_t discard_index) const
  {
    return baseMemoryOffset + ofs + (getDiscardBlockSize() * discard_index);
  }
  inline VkDeviceSize offsetOfLastDiscardIndex(VkDeviceSize ofs) const
  {
    G_ASSERT(currentDiscardIndex > 0);
    return baseMemoryOffset + ofs + getCurrentDiscardOffset() - getDiscardBlockSize();
  }
  inline VkDeviceSize offset(VkDeviceSize ofs) const { return baseMemoryOffset + ofs + getCurrentDiscardOffset(); }
  inline VkDeviceSize offsetWithDiscardIndex(VkDeviceSize ofs, uint32_t discard_index) const
  {
    return baseMemoryOffset + ofs + (getDiscardBlockSize() * discard_index);
  }
  inline VkDeviceSize dataSize() const { return totalSize(); }

  Buffer(const Description &in_desc, bool managed = true) : BufferImplBase(in_desc, managed), currentDiscardIndex(0) {}

  inline VkDeviceSize totalSize() const { return getDiscardBlockSize(); }

#if VK_KHR_buffer_device_address
  VkDeviceAddress getDeviceAddress(VulkanDevice &device);
#endif

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
  VkDeviceSize visibleDataSize = 0;
  BufferRef() = default;
  ~BufferRef() = default;
  BufferRef(const BufferRef &) = default;
  BufferRef &operator=(const BufferRef &) = default;
  // all defined in device.h
  // make this explicit to make it clear that we grab and hold on to the current
  // discard index
  explicit BufferRef(Buffer *bfr, uint32_t visible_data_size = 0);
  explicit operator bool() const;
  VulkanBufferHandle getHandle() const;
  VulkanBufferViewHandle getView() const;
  VkDeviceSize dataOffset(VkDeviceSize ofs) const;
  VkDeviceSize dataSize() const;
  VkDeviceSize offset(VkDeviceSize ofs) const;
  VkDeviceSize totalSize() const;
  VkDeviceSize offsetOfLastDiscardIndex(VkDeviceSize ofs) const;
};

inline bool operator==(const BufferRef &l, const BufferRef &r) { return (l.buffer == r.buffer) && (l.discardIndex == r.discardIndex); }

inline bool operator!=(const BufferRef &l, const BufferRef &r) { return !(l == r); }

inline BufferRef::BufferRef(Buffer *bfr, uint32_t visible_data_size) :
  buffer(bfr),
  discardIndex(bfr ? bfr->getCurrentDiscardIndex() : 0),
  visibleDataSize(bfr && !visible_data_size ? bfr->dataSize() : visible_data_size)
{}

inline BufferRef::operator bool() const { return nullptr != buffer; }

inline VulkanBufferHandle BufferRef::getHandle() const { return buffer->getHandle(); }

inline VulkanBufferViewHandle BufferRef::getView() const
{
  return buffer->hasView() ? buffer->getViewOfDiscardIndex(discardIndex) : VulkanBufferViewHandle{};
}

inline VkDeviceSize BufferRef::dataOffset(VkDeviceSize ofs) const { return buffer->dataOffsetWithDiscardIndex(ofs, discardIndex); }

inline VkDeviceSize BufferRef::dataSize() const { return visibleDataSize; }

inline VkDeviceSize BufferRef::offset(VkDeviceSize ofs) const { return buffer->offsetWithDiscardIndex(ofs, discardIndex); }

inline VkDeviceSize BufferRef::totalSize() const { return buffer->totalSize(); }

inline VkDeviceSize BufferRef::offsetOfLastDiscardIndex(VkDeviceSize ofs) const
{
  G_ASSERT(discardIndex > 0);
  return buffer->offsetWithDiscardIndex(ofs, discardIndex - 1);
}

} // namespace drv3d_vulkan
