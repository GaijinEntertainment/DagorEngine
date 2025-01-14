// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "device_resource.h"
#include "debug_naming.h"
#include "image_view_state.h"

namespace drv3d_vulkan
{

class ExecutionContext;
class MemoryHeapResource;
class Buffer;

struct ImageCreateInfo
{
  VkImageType type;
  VkExtent3D size;
  VkImageUsageFlags usage;
  uint16_t arrays;
  uint8_t mips;
  FormatStore format;
  VkImageCreateFlags flags;
  uint8_t residencyFlags;
  VkSampleCountFlagBits samples;

  void initByTexCreate(uint32_t cflag, bool cube_tex);
  void setUsageBitsFromCflags(uint32_t cflag);
  void setVulkanCreateFlagsFromCflags(uint32_t cflag, bool cube_tex);
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

    void fillFromImageCreate(const ImageCreateInfo &ii)
    {
      flags = ii.flags;
      imageType = ii.type;
      extent = ii.size;
      mipLevels = ii.mips;
      arrayLayers = ii.arrays;
      usage = ii.usage;
      samples = ii.samples;
    }

    VkImageCreateInfo toVk() const
    {
      return {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, flags, imageType, VK_FORMAT_UNDEFINED, extent, mipLevels, arrayLayers,
        samples, VkImageTiling::VK_IMAGE_TILING_OPTIMAL, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED};
    }
  };

  TrimmedCreateInfo ici;
  uint8_t memFlags;
  FormatStore format;
  VkImageLayout initialLayout;

  void fillAllocationDesc(AllocationDesc &alloc_desc) const;
};

typedef ResourceImplBase<ImageDescription, VulkanImageHandle, ResourceType::IMAGE> ImageImplBase;

class DeviceContext;

struct ImageLayoutInfo
{
  uint32_t mipLevels;
  VkImageLayout roSealTargetLayout;
  Tab<VkImageLayout> data;

  uint32_t indexToMipIndex(uint32_t id) const { return id % mipLevels; }
  uint32_t indexToArrayIndex(uint32_t id) const { return id / mipLevels; }
  uint32_t mipAndLayerToIndex(uint32_t mip, uint32_t layer) const { return mip + layer * mipLevels; }

  VkImageLayout get(uint32_t mip, uint32_t layer) { return getFromIndex(mipAndLayerToIndex(mip, layer)); }

  VkImageLayout getFromIndex(uint32_t index) { return data[index]; }

  void setForIndex(uint32_t index, VkImageLayout s) { data[index] = s; }

  void set(uint32_t mip, uint32_t layer, VkImageLayout s) { setForIndex(mipAndLayerToIndex(mip, layer), s); }

  void set(uint32_t mip, uint32_t mipRange, uint32_t layer, uint32_t layerRange, VkImageLayout s)
  {
    for (uint32_t i = mip; i < mip + mipRange; ++i)
      for (uint32_t j = layer; j < layer + layerRange; ++j)
        setForIndex(mipAndLayerToIndex(i, j), s);
  }

  void resetTo(VkImageLayout in_val)
  {
    roSealTargetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    for (VkImageLayout &i : data)
      i = in_val;
  }

  bool anySubresInState(VkImageLayout in_val)
  {
    for (VkImageLayout i : data)
    {
      if (i == in_val)
        return true;
    }

    return false;
  }

  void init(uint32_t mips, uint32_t layers, VkImageLayout initial)
  {
    roSealTargetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mipLevels = mips;
    data.resize(mips * layers);
    for (auto &&s : data)
      s = initial;
  }
};

class Image : public ImageImplBase,
              public ResourceExecutionSyncableExtend,
              public ResourcePlaceableExtend,
              public ResourceBindlessExtend
{
  bool useFallbackFormat(VkImageCreateInfo &ici);
  void bindAssignedMemoryId();

public:
  enum
  {
    CLEANUP_DESTROY = 0,
    CLEANUP_DELAYED_DESTROY = 1
  };
  static constexpr int MAX_VIEW_FORMAT = 4;
  using ViewFormatList = StaticTab<VkFormat, MAX_VIEW_FORMAT>;

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
  bool mayAlias() { return desc.memFlags & MEM_IN_PLACED_HEAP; }
  void delayedRestoreFromSysCopy();

  template <int Tag>
  void onDelayedCleanupBackend();

  template <int Tag>
  void onDelayedCleanupFrontend(){};

  template <int Tag>
  void onDelayedCleanupFinish();

public:
  struct ViewInfo
  {
    VulkanImageViewHandle view;
    ImageViewState state;
  };

private:
  eastl::vector<ViewInfo> views;
  Buffer *hostCopy = nullptr;
  SamplerState cachedSamplerState; //-V730_NOINIT
  SamplerInfo *cachedSampler = nullptr;

public:
  enum
  {
    MEM_NORMAL = 0x0,
    MEM_NOT_EVICTABLE = 0x1,
    MEM_LAZY_ALLOCATION = 0x2,
    MEM_DEDICATE_ALLOC = 0x4,
    MEM_IN_PLACED_HEAP = 0x8
  };
  static bool lazyAllocationFromCflags(uint32_t cflag) { return cflag & (TEXCF_SAMPLECOUNT_MASK | TEXCF_TRANSIENT); }

  ImageLayoutInfo layout;

  SamplerInfo *getSampler(SamplerState sampler_state);

  void setDerivedHandle(VulkanImageHandle new_handle)
  {
    G_ASSERT(!getBaseHandle());
    G_ASSERT(!isManaged());
    setHandle(generalize(new_handle));
  }
  Buffer *getHostCopyBuffer();
  void fillImage2BufferCopyData(carray<VkBufferImageCopy, MAX_MIPMAPS> &copies, Buffer *targetBuffer);
  VkBufferImageCopy makeBufferCopyInfo(uint8_t mip, uint16_t arr, VkDeviceSize buf_offset);

  bool hasMoreThanOneSubresource() const { return (desc.ici.mipLevels > 1) || (desc.ici.arrayLayers > 1); }
  bool isValidResId(uint32_t res_id) const { return res_id < (desc.ici.mipLevels * desc.ici.arrayLayers); }
  FormatStore getFormat() const { return desc.format; }
  VkImageUsageFlags getUsage() const { return desc.ici.usage; }
  VkImageCreateFlags getCreateFlags() const { return desc.ici.flags; }
  eastl::vector<ViewInfo>::iterator addView(const ViewInfo &view) { return views.insert(views.end(), view); }
  VkImageType getType() const { return desc.ici.imageType; }
  uint16_t getArrayLayers() const { return desc.ici.arrayLayers; }
  ValueRange<uint16_t> getArrayLayerRange() const { return {0, desc.ici.arrayLayers}; }
  uint8_t getMipLevels() const { return desc.ici.mipLevels; }
  ValueRange<uint8_t> getMipLevelRange() const { return {0, desc.ici.mipLevels}; }
  VkSampleCountFlagBits getSampleCount() const { return desc.ici.samples; }
  uint32_t getSubresourceMaxSize();

  Image(const Description &in_desc, bool manage = true);

  void wrapImage(VkImage image);

  void cleanupReferences();

  const VkExtent3D &getBaseExtent() const { return desc.ici.extent; }
  VkExtent2D getBaseExtent2D() const { return VkExtent2D{desc.ici.extent.width, desc.ici.extent.height}; }

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

  bool isGPUWritable()
  {
    return getUsage() &
           (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
  }

  bool isSampledSRV() { return !isGPUWritable() && (getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT); }

  static Image *create(const ImageCreateInfo &ii);
  static bool checkImageCreate(const VkImageCreateInfo &ici, FormatStore format);

  bool checkImageViewFormat(FormatStore fmt, VkImageUsageFlags usage);
  VulkanImageViewHandle getImageView(ImageViewState state);
};

void viewFormatListFrom(FormatStore format, VkImageUsageFlags usage, Image::ViewFormatList &viewFormats);

} // namespace drv3d_vulkan