// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "render_pass_resource.h"
#include "globals.h"
#include "resource_manager.h"
#include "sampler_cache.h"
#include "vk_format_utils.h"
#include "execution_context.h"
#include "backend.h"
#include "execution_scratch.h"
#include "execution_sync.h"
#include "bindless.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

Image::Image(const Description &in_desc, bool manage) : ImageImplBase(in_desc, manage), hostCopy(nullptr)
{
  G_ASSERT(desc.ici.samples > 0);
  layout.init(desc.ici.mipLevels, desc.ici.arrayLayers, desc.initialLayout);
}

namespace drv3d_vulkan
{

template <>
void Image::onDelayedCleanupBackend<Image::CLEANUP_DESTROY>()
{
  // cleanup bindless a bit earlier to avoid leaving texture in bindless on followup frame
  // i.e. "finish" cb happens after waiting for frame on what we did deletion,
  // so if we cleanup bindless there, while we waiting we still may reference dead texture in setted up work frame
  for (uint32_t i : bindlessSlots)
    Backend::bindless.cleanupBindlessTexture(i, this);
  bindlessSlots.clear();
}

template <>
void Image::onDelayedCleanupFinish<Image::CLEANUP_DESTROY>()
{
  // delayed destroy images must be marked dead here
  //(because they are "surely" referenced at deletion time)
  markDead();

  // shader var system * delayed state tansit may give us delete-bind command order, avoid crashin on it
  if (Backend::State::pipe.isReferenced(this))
  {
    Backend::State::pendingCleanups.removeReferenced(this);
    return;
  }

  if (desc.memFlags & MEM_IN_PLACED_HEAP)
  {
    destroyVulkanObject();
    freeMemory();
    releaseHeap();
  }

  ResourceManager &rm = Globals::Mem::res;
  if (hostCopy)
    rm.free(hostCopy);
  rm.free(this);
}

template <>
void Image::onDelayedCleanupFinish<Image::CLEANUP_DELAYED_DESTROY>()
{
  // destroy in next frame if we are not using image
  Backend::State::pendingCleanups.removeReferenced(this);
}

template <>
void Image::onDelayedCleanupBackend<Image::CLEANUP_DELAYED_DESTROY>()
{}

} // namespace drv3d_vulkan

void ImageCreateInfo::initByTexCreate(uint32_t cflag, bool cube_tex)
{
  format = FormatStore::fromCreateFlags(cflag);
  samples = VkSampleCountFlagBits(get_sample_count(cflag));
  setUsageBitsFromCflags(cflag);
  setVulkanCreateFlagsFromCflags(cflag, cube_tex);
  residencyFlags = (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) ? Image::MEM_LAZY_ALLOCATION : Image::MEM_NORMAL;
}

void ImageCreateInfo::setUsageBitsFromCflags(uint32_t cflag)
{
  usage = 0;
  if (Image::lazyAllocationFromCflags(cflag))
  {
    usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
  }
  else
  {
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (!(cflag & TEXCF_SYSMEM))
      usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  const bool isRT = cflag & TEXCF_RTARGET;
  const bool isDepth = format.getAspektFlags() & VK_IMAGE_ASPECT_DEPTH_BIT;
  if (isRT)
  {
    if (Globals::cfg.has.renderTo3D || (type != VK_IMAGE_TYPE_3D))
      usage |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  usage |= (cflag & TEXCF_UNORDERED) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
  G_ASSERT(!(isDepth && (cflag & TEXCF_READABLE)));
}

void ImageCreateInfo::setVulkanCreateFlagsFromCflags(uint32_t cflag, bool cube_tex)
{
  flags = cube_tex ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
  if ((format.isSrgbCapableFormatType() && FormatStore::needMutableFormatForCreateFlags(cflag)) || (cflag & TEXCF_UNORDERED))
    flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

#if VK_KHR_maintenance1
  const bool isRT = cflag & TEXCF_RTARGET;
  if (isRT && (type == VK_IMAGE_TYPE_3D) && Globals::cfg.has.renderTo3D)
    flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
#endif
}

void Image::destroyVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;

  Globals::VK::dev.vkDestroyImage(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

SamplerInfo *Image::getSampler(SamplerState sampler_state)
{
  if (!cachedSampler || !(sampler_state == cachedSamplerState))
  {
    cachedSampler = Globals::samplers.get(sampler_state);
    cachedSamplerState = sampler_state;
  }

  return cachedSampler;
}

bool Image::useFallbackFormat(VkImageCreateInfo &ici)
{
  // if we not use memory row pitch directly, we can fake format support by using
  // closest one that is available
  constexpr VkImageUsageFlags pitchIndependent =
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  if (!(ici.usage & pitchIndependent))
    return false;

  switch (ici.format)
  {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM: ici.format = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: ici.format = VK_FORMAT_R16G16B16A16_SFLOAT; break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16: ici.format = VK_FORMAT_B8G8R8A8_UNORM; break;
    default: return false;
  }

  desc.format = FormatStore::fromVkFormat(ici.format);
  return checkImageCreate(ici, desc.format);
}

void Image::createVulkanObject()
{
  VulkanDevice &dev = Globals::VK::dev;
  VkImageCreateInfo ici = desc.ici.toVk();
  ici.format = desc.format.asVkFormat();

  ViewFormatList viewFormats;
#if VK_KHR_image_format_list
  VkImageFormatListCreateInfoKHR iflci;
#endif
  if (ici.flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
  {
    viewFormatListFrom(desc.format, desc.ici.usage, viewFormats);

    if (!(viewFormats.size() > 1))
    {
      // only one format, no mutable format support needed
      ici.flags &= ~VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }
#if VK_KHR_image_format_list
    else if (dev.hasExtension<ImageFormatListKHR>())
    {
      iflci.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
      iflci.pNext = nullptr;
      iflci.viewFormatCount = viewFormats.size();
      iflci.pViewFormats = viewFormats.data();
      ici.pNext = &iflci;
    }
#endif
  }

  if (!checkImageCreate(ici, desc.format))
  {
    FormatStore linearVariant = desc.format.getLinearVariant();
    // for as_uint UAV bindings on formats without SRGB logic
    if (linearVariant.asVkFormat() == ici.format && (desc.ici.usage & VK_IMAGE_USAGE_STORAGE_BIT))
      ici.format = VK_FORMAT_R32_UINT;
    else
    {
      debug("vulkan: retrying with liniear variant of %s -> %s", desc.format.getNameString(), linearVariant.getNameString());
      ici.format = linearVariant.asVkFormat();
    }
    if (!checkImageCreate(ici, linearVariant))
    {
      if (!useFallbackFormat(ici))
        DAG_FATAL("vulkan: texture format %s not supported", linearVariant.getNameString());
      else
        debug("vulkan: using %s format as fallback for %s", desc.format.getNameString(), linearVariant.getNameString());
    }
    else
      desc.format = linearVariant;
  }

  desc.ici.flags = ici.flags;

  const unsigned disallowedTransientUsageFlags = ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
  G_UNUSED(disallowedTransientUsageFlags);
  G_ASSERT((ici.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0 || (ici.usage & disallowedTransientUsageFlags) == 0);

  Handle ret{};
  VULKAN_EXIT_ON_FAIL(dev.vkCreateImage(dev.get(), &ici, nullptr, ptr(ret)));

  setHandle(generalize(ret));
}

MemoryRequirementInfo Image::getMemoryReq()
{
  G_ASSERT(getBaseHandle());

  VulkanDevice &dev = Globals::VK::dev;

  return get_memory_requirements(dev, getHandle());
}

VkMemoryRequirements Image::getSharedHandleMemoryReq()
{
  DAG_FATAL("vulkan: no image handle suballoc");
  return {};
}

void Image::bindMemory()
{
  G_ASSERT(getBaseHandle());
  G_ASSERT(getMemoryId() != -1);

  VulkanDevice &dev = Globals::VK::dev;

  const ResourceMemory &mem = getMemory();
  VULKAN_EXIT_ON_FAIL(dev.vkBindImageMemory(dev.get(), getHandle(), mem.deviceMemory(), mem.offset));
}

void Image::bindMemoryFromHeap(MemoryHeapResource *in_heap, ResourceMemoryId heap_mem_id)
{
  setMemoryId(heap_mem_id);
  bindMemory();
  setHeap(in_heap);
}

void Image::reuseHandle() { DAG_FATAL("vulkan: no image handle suballoc"); }

void Image::releaseSharedHandle() { DAG_FATAL("vulkan: no image handle suballoc"); }

void Image::shutdown() { cleanupReferences(); }

bool Image::nonResidentCreation()
{
  if (!isEvictable())
    return false;

  createVulkanObject();
  return true;
}

void Image::evict() { cleanupReferences(); }

void Image::restoreFromSysCopy(ExecutionContext &ctx)
{
  // forward patching for layout
  layout.resetTo(hostCopy ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED);
  // was not evicted, created non-residently
  if (!hostCopy)
    return;
  ctx.queueImageResidencyRestore(this);
}

void Image::delayedRestoreFromSysCopy()
{
  G_ASSERT(hostCopy);
  carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
  fillImage2BufferCopyData(copies, hostCopy);


  ExecutionContext::PrimaryPipelineBarrier layoutSwitch(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  layoutSwitch.modifyImageTemplate(this);
  layoutSwitch.modifyImageTemplate({VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT});
  layoutSwitch.modifyImageTemplate(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  layoutSwitch.modifyImageTemplate(0, (uint8_t)desc.ici.mipLevels, 0, (uint8_t)desc.ici.arrayLayers);
  layoutSwitch.addImageByTemplate();
  layoutSwitch.submit();
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyBufferToImage(hostCopy->getHandle(), getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    desc.ici.mipLevels, copies.data()));

  // TODO: make sure usage barrier is correct after this operation (should be generated srcless due to layout change)

  Backend::gpuJob.get().cleanups.enqueueFromBackend<Buffer::CLEANUP_DESTROY>(*hostCopy);
  hostCopy = nullptr;
}

bool Image::isEvictable()
{
  const VkImageUsageFlags transferableMask = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if ((getUsage() & transferableMask) != transferableMask)
    return false;
  if (isUsedInBindless())
    return false;
  return (desc.memFlags & MEM_NOT_EVICTABLE) == 0;
}

void Image::Description::fillAllocationDesc(AllocationDesc &alloc_desc) const
{
  alloc_desc.reqs = {};
  alloc_desc.memClass =
    (memFlags & Image::MEM_LAZY_ALLOCATION) > 0 ? DeviceMemoryClass::TRANSIENT_IMAGE : DeviceMemoryClass::DEVICE_RESIDENT_IMAGE;
  alloc_desc.forceDedicated = (memFlags & Image::MEM_DEDICATE_ALLOC) > 0;
  alloc_desc.canUseSharedHandle = 0;
  alloc_desc.temporary = 0;
  alloc_desc.objectBaked = 0;
}

//////////////

uint32_t Image::getSubresourceMaxSize()
{
  return desc.format.calculateImageSize(desc.ici.extent.width, desc.ici.extent.height, desc.ici.extent.depth, 1);
}

VkBufferImageCopy Image::makeBufferCopyInfo(uint8_t mip, uint16_t arr, VkDeviceSize buf_offset)
{
  return make_copy_info(desc.format, mip, arr, 1, desc.ici.extent, buf_offset);
}

Buffer *Image::getHostCopyBuffer()
{
  if (!hostCopy)
  {
    uint32_t size =
      desc.format.calculateImageSize(desc.ici.extent.width, desc.ici.extent.height, desc.ici.extent.depth, desc.ici.mipLevels) *
      desc.ici.arrayLayers;
    hostCopy = Buffer::create(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::NONE);
  }
  return hostCopy;
}

void Image::fillImage2BufferCopyData(carray<VkBufferImageCopy, MAX_MIPMAPS> &copies, Buffer *targetBuffer)
{
  VkDeviceSize bufferOffset = 0;

  for (uint32_t j = 0; j < desc.ici.mipLevels; ++j)
  {
    VkBufferImageCopy &copy = copies[j];
    copy = make_copy_info(desc.format, j, 0, desc.ici.arrayLayers, desc.ici.extent, targetBuffer->bufOffsetLoc(bufferOffset));

    bufferOffset += desc.format.calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1) *
                    desc.ici.arrayLayers;
  }
}

void Image::makeSysCopy(ExecutionContext &ctx)
{
  Buffer *buf = getHostCopyBuffer();
  carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
  fillImage2BufferCopyData(copies, buf);
  ctx.copyImageToBufferOrdered(buf, this, copies.data(), desc.ici.mipLevels);
}

void Image::cleanupReferences()
{
  VulkanDevice &dev = Globals::VK::dev;

  if (getUsage() & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    auto iterCb = [img = this](RenderPassResource *i) { i->destroyFBsWithImage(img); };

    Globals::Mem::res.iterateAllocated<RenderPassResource>(iterCb);
  }

  for (auto &&view : views)
  {
    if (view.state.isRenderTarget)
      Globals::passes.unloadWith(dev, view.view);
    VULKAN_LOG_CALL(dev.vkDestroyImageView(dev.get(), view.view, NULL));
  }
  views.clear();

  if (isUsedInBindless())
    D3D_ERROR("vulkan: image %p:%s was registred in bindless while being removed", this, getDebugName());
}

void Image::wrapImage(VkImage image)
{
  Handle conv;
  conv.value = image;
  setHandle(generalize(conv));
}

// returns true if a image should be allocated via pool memory or
// use a dedicated allocation
bool shouldUsePool(const VkImageCreateInfo &ici)
{
  if (Globals::cfg.bits.useDedicatedMemForImages)
  {
    if (Globals::cfg.bits.useDedicatedMemForRenderTargets)
    {
      // never put render targets on pool so that it can benifit form dedicated memory
      if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        return false;
    }

    // large (what is large?) textures get thier own memory block to benifit from
    // dedicated memory
    return (ici.extent.width * ici.extent.height * max(ici.extent.depth, 1u)) <=
           Globals::cfg.dedicatedMemoryForImageTexelCountThreshold;
  }
  return true;
}

Image *Image::create(const ImageCreateInfo &ii)
{
  Description::TrimmedCreateInfo ici;
  ici.fillFromImageCreate(ii);

  bool useDedAlloc = !shouldUsePool(ici.toVk());
  uint8_t memFlags = (useDedAlloc ? (MEM_DEDICATE_ALLOC) : 0) | ii.residencyFlags;

  WinAutoLock lk(Globals::Mem::mutex);
  return Globals::Mem::res.alloc<Image>({ici, memFlags, ii.format, VK_IMAGE_LAYOUT_UNDEFINED}, true);
}

static const char *image_type_name[] = {"1D", "2D", "3D"};

static const char *image_tiling_name[] = {"optimal", "linear"};

bool Image::checkImageCreate(const VkImageCreateInfo &ici, FormatStore format)
{
#define FAIL_MESSAGE(extra, ...)                                                                                   \
  debug("vulkan: image create check failed, for image w=%u, h=%u, d=%u, fmt=%s, "                                  \
        "type=%s, tilling=%s, usage=[%s], flags=%u, samples = %u" extra,                                           \
    ici.extent.width, ici.extent.height, ici.extent.depth, format.getNameString(), image_type_name[ici.imageType], \
    image_tiling_name[ici.tiling], formatImageUsageFlags(ici.usage), ici.flags, ici.samples, __VA_ARGS__)

  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(Globals::VK::dev.getInstance().vkGetPhysicalDeviceImageFormatProperties(
    Globals::VK::phy.device, ici.format, ici.imageType, ici.tiling, ici.usage, ici.flags, &properties));
  if (VULKAN_FAIL(result))
  {
    FAIL_MESSAGE(", because vkGetPhysicalDeviceImageFormatProperties failed", "");
    return false;
  }

  if (!(ici.samples & properties.sampleCounts))
  {
    FAIL_MESSAGE(", because sample count %u is not supported (supported bits %u)", ici.samples, properties.sampleCounts);
  }

  if ((ici.imageType == VK_IMAGE_TYPE_1D) || (ici.imageType == VK_IMAGE_TYPE_2D) || (ici.imageType == VK_IMAGE_TYPE_3D))
  {
    if (properties.maxExtent.width < ici.extent.width)
    {
      FAIL_MESSAGE(", because image width %u exceeds limit of %u", ici.extent.width, properties.maxExtent.width);
      return false;
    }
  }
  if ((ici.imageType == VK_IMAGE_TYPE_2D) || (ici.imageType == VK_IMAGE_TYPE_3D))
  {
    if (properties.maxExtent.height < ici.extent.height)
    {
      FAIL_MESSAGE(", because image height %u exceeds limit of %u", ici.extent.height, properties.maxExtent.height);
      return false;
    }
  }
  if ((ici.imageType == VK_IMAGE_TYPE_3D))
  {
    if (properties.maxExtent.depth < ici.extent.depth)
    {
      FAIL_MESSAGE(", because image depth %u exceeds limit of %u", ici.extent.depth, properties.maxExtent.depth);
      return false;
    }
  }
  if (ici.arrayLayers > properties.maxArrayLayers)
  {
    FAIL_MESSAGE(", because image array layers %u exceeds limit of %u", ici.arrayLayers, properties.maxArrayLayers);
    return false;
  }
  if (ici.mipLevels > properties.maxMipLevels)
  {
    FAIL_MESSAGE(", because image mip map levels %u exceeds limit of %u", ici.mipLevels, properties.maxMipLevels);
    return false;
  }
#undef FAIL_MESSAGE
  return true;
}

bool Image::checkImageViewFormat(FormatStore fmt, VkImageUsageFlags usage)
{
  return Globals::VK::fmt.support(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, getCreateFlags(),
    getSampleCount());
}

VulkanImageViewHandle Image::getImageView(ImageViewState state)
{
  auto iter = eastl::find_if(views.begin(), views.end(), [state](const Image::ViewInfo &view) { return view.state == state; });
  if (iter == views.end())
  {
    // this should not happen at every frame, add marker to catch such things
    TIME_PROFILE(vulkan_get_image_view_ctor);
    VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr};
    VkImageViewUsageCreateInfo ivuci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, nullptr};
    FormatStore fmt = state.getFormat();
    VkImageUsageFlags maskedUsage = (state.isUAV || state.isRenderTarget) ? getUsage() : VK_IMAGE_USAGE_SAMPLED_BIT;
    ivuci.usage = maskedUsage;
    bool fmtOk = checkImageViewFormat(fmt, maskedUsage);
    if (!fmtOk && fmt != getFormat())
    {
      debug("vulkan: unsupported view format %s, falling back for image original format %s", fmt.getNameString(),
        getFormat().getNameString());
      fmt = getFormat();
      fmtOk = checkImageViewFormat(fmt, maskedUsage);
    }
    if (!fmtOk)
    {
      D3D_ERROR("vulkan: failed to create image view for %p:%s with format %s usage %s samples %u flags %u", this, getDebugName(),
        fmt.getNameString(), formatImageUsageFlags(getUsage()), getSampleCount(), getCreateFlags());
      return VulkanImageViewHandle{};
    }
    G_ASSERT((state.isRenderTarget == 1 && state.isCubemap == 0) || (state.isRenderTarget == 0));
    G_ASSERT((state.getMipBase() + state.getMipCount()) <= getMipLevels());
    ivci.viewType = state.getViewType(getType());
    ivci.image = getHandle();
    ivci.format = fmt.asVkFormat();
    memset(&ivci.components, 0, sizeof(ivci.components));

    // apply correct swizzle for mapped formats
    // for compatibility in shaders
    switch (fmt.getFormatFlags())
    {
      case TEXFMT_A8:
        // as A8 is mapped to R8, we need to properly swizzle it
        if (fmt.asVkFormat() == VK_FORMAT_R8_UNORM)
        {
          ivci.components.r = VK_COMPONENT_SWIZZLE_ZERO;
          ivci.components.g = VK_COMPONENT_SWIZZLE_ZERO;
          ivci.components.b = VK_COMPONENT_SWIZZLE_ZERO;
          ivci.components.a = VK_COMPONENT_SWIZZLE_R;
        }
        break;
      case TEXFMT_R5G6B5:
        // no need to swizzle if we reading from render target
        if ((fmt.asVkFormat() == VK_FORMAT_R5G6B5_UNORM_PACK16) && !(getUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        {
          ivci.components.r = VK_COMPONENT_SWIZZLE_B;
          ivci.components.g = VK_COMPONENT_SWIZZLE_G;
          ivci.components.b = VK_COMPONENT_SWIZZLE_R;
          ivci.components.a = VK_COMPONENT_SWIZZLE_ONE;
        }
        break;
      default:; // no action needed, keep swizzle as is
    }

    ivci.flags = 0;
    ivci.subresourceRange.aspectMask = fmt.getAspektFlags();
    if (!state.isRenderTarget)
    {
      // either sample stencil or depth
      if (state.sampleStencil)
        ivci.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_DEPTH_BIT;
      else
        ivci.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    ivci.subresourceRange.baseMipLevel = state.getMipBase();
    ivci.subresourceRange.levelCount = state.getMipCount();
    ivci.subresourceRange.baseArrayLayer = state.getArrayBase();
    ivci.subresourceRange.layerCount = state.getArrayCount();

    Image::ViewInfo viewInfo;
    viewInfo.state = state;
    if (Globals::cfg.has.separateImageViewUsage)
      chain_structs(ivci, ivuci);
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateImageView(Globals::VK::dev.get(), &ivci, NULL, ptr(viewInfo.view)));

    if (Globals::cfg.debugLevel)
      Globals::Dbg::naming.setImageViewName(viewInfo.view, getDebugName());

    iter = addView(viewInfo);
  }

  return iter->view;
}

void drv3d_vulkan::viewFormatListFrom(FormatStore format, VkImageUsageFlags usage, Image::ViewFormatList &viewFormats)
{
  static_assert(Image::MAX_VIEW_FORMAT >= 4, "Max view format count must be at least 4");

  viewFormats.clear();
  viewFormats.push_back(format.asVkFormat());

  VkFormat linearFormat = format.getLinearVariant().asVkFormat();
  VkFormat srgbFormat = format.getSRGBVariant().asVkFormat();

  if (viewFormats[0] != linearFormat)
  {
    viewFormats.push_back(linearFormat);
  }

  if (viewFormats[0] != srgbFormat)
  {
    viewFormats.push_back(srgbFormat);
  }

  bool allowRawUint32 = ((VK_IMAGE_USAGE_STORAGE_BIT & usage) && format.getBytesPerPixelBlock() == 4);
  if (allowRawUint32)
  {
    // only add if the base format is different.
    if (linearFormat != VK_FORMAT_R32_UINT)
    {
      viewFormats.push_back(VK_FORMAT_R32_UINT);
    }
  }
}
