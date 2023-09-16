#pragma once
#include "device_resource.h"

namespace drv3d_vulkan
{

struct ImageCreateInfo
{
  VkImageType type;
  VkExtent3D size;
  VkImageUsageFlags usage;
  uint16_t arrays;
  uint8_t mips;
  FormatStore format;
  VkImageCreateFlags flags;
  uint32_t residencyFlags;
  VkSampleCountFlagBits samples;
};

struct ImageDescription
{
  // trimmed image create info
  struct TrimmedCreateInfo
  {
    VkImageCreateFlags flags;
    VkImageType imageType;
    VkExtent3D extent;
    uint8_t mipLevels;
    uint16_t arrayLayers;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits samples;

    VkImageCreateInfo toVk() const
    {
      return {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, flags, imageType, VK_FORMAT_UNDEFINED, extent, mipLevels, arrayLayers,
        samples, VkImageTiling::VK_IMAGE_TILING_OPTIMAL, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED};
    }
  };

  TrimmedCreateInfo ici;
  uint8_t memFlags;
  FormatStore format;
  ImageState initialState;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<ImageDescription, VulkanImageHandle, ResourceType::IMAGE> ImageImplBase;

class DeviceContext;

class Image : public ImageImplBase
{
  bool useFallbackFormat(VkImageCreateInfo &ici);

public:
  enum
  {
    CLEANUP_DESTROY = 0,
    CLEANUP_EVICT = 1,
    CLEANUP_DELAYED_DESTROY = 2
  };
  static constexpr int MAX_VIEW_FORMAT = 4;
  using ViewFormatList = StaticTab<VkFormat, MAX_VIEW_FORMAT>;

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
  void onDelayedCleanupBackend(ContextBackend &);

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

public:
  BEGIN_BITFIELD_TYPE(StateLookupEntry, uint64_t)
    ADD_BITFIELD_MEMBER(access, 0, 16)
    ADD_BITFIELD_ARRAY(layouts, 16, 8, 2)
    ADD_BITFIELD_ARRAY(stages, 32, 16, 2)
    StateLookupEntry(VkAccessFlags flags, VkImageLayout layout_0, VkImageLayout layout_1, VkPipelineStageFlags stage_0,
      VkPipelineStageFlags stage_1)
    {
      access = flags;
      layouts[0] = layout_0;
      layouts[1] = layout_1;
      stages[0] = stage_0;
      stages[1] = stage_1;
    }
  END_BITFIELD_TYPE()
  typedef ImageState State;
  struct ViewInfo
  {
    VulkanImageViewHandle view;
    ImageViewState state;
  };

private:
  BEGIN_BITFIELD_TYPE(Flags, uint8_t)
    ADD_BITFIELD_MEMBER(isSealed, 0, 1)
  END_BITFIELD_TYPE()

  Flags flags{0};
  eastl::vector<ViewInfo> views;
  eastl::vector<uint32_t> bindlessSlots;
  Tab<uint8_t> state;
  Buffer *hostCopy = nullptr;
  SamplerState cachedSamplerState; //-V730_NOINIT
  SamplerInfo *cachedSampler = nullptr;

public:
  enum
  {
    MEM_NOT_EVICTABLE = 0x1,
    MEM_LAZY_ALLOCATION = 0x2,
    MEM_DEDICATE_ALLOC = 0x4
  };

  SamplerInfo *getSampler(SamplerState sampler_state);

  void setDerivedHandle(VulkanImageHandle new_handle)
  {
    G_ASSERT(!getBaseHandle());
    G_ASSERT(!isManaged());
    setHandle(generalize(new_handle));
  }
  void doResidencyRestore(DeviceContext &context);
  Buffer *getHostCopyBuffer();
  void fillImage2BufferCopyData(carray<VkBufferImageCopy, MAX_MIPMAPS> &copies, Buffer *targetBuffer);
  VkBufferImageCopy makeBufferCopyInfo(uint8_t mip, uint16_t arr, VkDeviceSize buf_offset);

  bool hasMoreThanOneSubresource() const { return (desc.ici.mipLevels > 1) || (desc.ici.arrayLayers > 1); }
  bool isValidResId(uint32_t res_id) const { return res_id < (desc.ici.mipLevels * desc.ici.arrayLayers); }
  FormatStore getFormat() const { return desc.format; }
  VkImageUsageFlags getUsage() const { return desc.ici.usage; }
  VkImageCreateFlags getCreateFlags() const { return desc.ici.flags; }
  const eastl::vector<ViewInfo> &getViews() const { return views; }
  eastl::vector<ViewInfo>::iterator addView(const ViewInfo &view) { return views.insert(views.end(), view); }
  VkImageType getType() const { return desc.ici.imageType; }
  uint16_t getArrayLayers() const { return desc.ici.arrayLayers; }
  ValueRange<uint16_t> getArrayLayerRange() const { return {0, desc.ici.arrayLayers}; }
  uint8_t getMipLevels() const { return desc.ici.mipLevels; }
  ValueRange<uint8_t> getMipLevelRange() const { return {0, desc.ici.mipLevels}; }
  bool isSealed() const { return 0 != flags.isSealed; }
  VkSampleCountFlagBits getSampleCount() const { return desc.ici.samples; }
  uint32_t getSubresourceMaxSize();

  void seal()
  {
    flags.isSealed = 1;
    for (auto &&slot : state)
      slot = SN_SAMPLED_FROM;
  }

  void unseal(const char * /*why*/, uint32_t /*next_state*/) { flags.isSealed = 0; }

  uint32_t stateIndexToMipIndex(uint32_t id) const { return id % desc.ici.mipLevels; }
  uint32_t stateIndexToArrayIndex(uint32_t id) const { return id / desc.ici.mipLevels; }
  uint32_t mipAndLayerIndexToStateIndex(uint32_t mip, uint32_t layer) const { return mip + layer * desc.ici.mipLevels; }

  uint8_t getState(uint32_t mip, uint32_t layer) { return getStateFromIndex(mipAndLayerIndexToStateIndex(mip, layer)); }

  uint8_t getStateFromIndex(uint32_t index) { return state[index]; }

  void setStateForIndex(uint32_t index, uint8_t s) { state[index] = s; }

  void setState(uint32_t mip, uint32_t layer, uint8_t s)
  {
    if (isSealed())
    {
      debug("vulkan: %p->setState called but this was sealed!", this);
    }
    setStateForIndex(mipAndLayerIndexToStateIndex(mip, layer), s);
  }

  void resetState(uint32_t mip, uint32_t layer, uint8_t s)
  {
    if (isSealed())
    {
      debug("vulkan: %p->resetState called but this was sealed!", this);
    }
    state[mipAndLayerIndexToStateIndex(mip, layer)] = s;
  }

  bool anySubresInState(int in_state)
  {
    ValueRange<uint8_t> mipRange = getMipLevelRange();
    ValueRange<uint16_t> arrRange = getArrayLayerRange();

    for (uint16_t arr : arrRange)
    {
      for (uint8_t mip : mipRange)
      {
        if (getState(mip, arr) == in_state)
          return true;
      }
    }

    return false;
  }

  Image(const Description &in_desc, bool manage = true);

  void wrapImage(VkImage image);

  void cleanupReferences();

  const VkExtent3D &getBaseExtent() const { return desc.ici.extent; }

  inline VkExtent3D getMipExtents(uint32_t level) const
  {
    VkExtent3D me = //
      {max(desc.ici.extent.width >> level, 1u), max(desc.ici.extent.height >> level, 1u), max(desc.ici.extent.depth >> level, 1u)};
    return me;
  }

  inline VkExtent2D getMipExtents2D(uint32_t level) const
  {
    VkExtent2D me = {max(desc.ici.extent.width >> level, 1u), max(desc.ici.extent.height >> level, 1u)};
    return me;
  }

  inline uint32_t getMipHeight(uint32_t level) const { return max(desc.ici.extent.height >> level, 1u); }

  bool isAllowedForFramebuffer()
  {
    return getUsage() & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  void addBindlessSlot(uint32_t slot) { bindlessSlots.push_back(slot); }

  void removeBindlessSlot(uint32_t slot)
  {
    bindlessSlots.erase(eastl::remove_if(begin(bindlessSlots), end(bindlessSlots), [slot](uint32_t r_slot) { return r_slot == slot; }),
      end(bindlessSlots));
  }
};

struct ImageStateRange
{
  Image *image = nullptr;
  uint32_t stateArrayIndex = 0;
  ValueRange<uint16_t> arrayRange;
  ValueRange<uint16_t> mipRange;
};

void viewFormatListFrom(FormatStore format, VkImageUsageFlags usage, Image::ViewFormatList &viewFormats);

} // namespace drv3d_vulkan