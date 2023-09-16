#include "device.h"
#include "pipeline.h"
#include "render_pass_resource.h"

using namespace drv3d_vulkan;

Image::Image(const Description &in_desc, bool manage) : ImageImplBase(in_desc, manage), hostCopy(nullptr)
{
  G_ASSERT(desc.ici.samples > 0);

  state.resize(desc.ici.mipLevels * desc.ici.arrayLayers);
  for (auto &&s : state)
    s = desc.initialState;
}

namespace drv3d_vulkan
{

template <>
void Image::onDelayedCleanupBackend<Image::CLEANUP_DESTROY>(ContextBackend &back)
{
  // cleanup bindless a bit earlier to avoid leaving texture in bindless on followup frame
  // i.e. "finish" cb happens after waiting for frame on what we did deletion,
  // so if we cleanup bindless there, while we waiting we still may reference dead texture in setted up work frame
  for (uint32_t i : bindlessSlots)
    back.contextState.bindlessManagerBackend.cleanupBindlessTexture(i, this);
  bindlessSlots.clear();
}

template <>
void Image::onDelayedCleanupFinish<Image::CLEANUP_DESTROY>()
{
  // delayed destroy images must be marked dead here
  //(because they are "surely" referenced at deletion time)
  markDead();

  ResourceManager &rm = get_device().resources;
  if (!isResident())
    rm.free(hostCopy);
  rm.free(this);
}

template <>
void Image::onDelayedCleanupFinish<Image::CLEANUP_EVICT>()
{
  ResourceAlgorithm<Image> alg(*this);
  alg.evict();
}

template <>
void Image::onDelayedCleanupBackend<Image::CLEANUP_EVICT>(ContextBackend &)
{}

template <>
void Image::onDelayedCleanupFinish<Image::CLEANUP_DELAYED_DESTROY>()
{
  ContextBackend &back = get_device().getContext().getBackend();
  // destroy in next frame if we are not using image
  back.pipelineStatePendingCleanups.removeReferenced(this);
}

template <>
void Image::onDelayedCleanupBackend<Image::CLEANUP_DELAYED_DESTROY>(ContextBackend &)
{}

} // namespace drv3d_vulkan

void Image::destroyVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  get_device().getVkDevice().vkDestroyImage(dev.get(), getHandle(), nullptr);
  setHandle(generalize(Handle()));
}

SamplerInfo *Image::getSampler(SamplerState sampler_state)
{
  if (!cachedSampler || (sampler_state != cachedSamplerState))
  {
    cachedSampler = get_device().getSampler(sampler_state);
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
    case VK_FORMAT_R5G6B5_UNORM_PACK16: ici.format = VK_FORMAT_B8G8R8A8_UNORM; break;
    default: return false;
  }

  desc.format = FormatStore::fromVkFormat(ici.format);
  return get_device().checkImageCreate(ici, desc.format);
}

void Image::createVulkanObject()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();
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

  if (!drvDev.checkImageCreate(ici, desc.format))
  {
    FormatStore linearVariant = desc.format.getLinearVariant();
    debug("vulkan: retrying with liniear variant of %s -> %s", desc.format.getNameString(), linearVariant.getNameString());
    ici.format = linearVariant.asVkFormat();
    if (!drvDev.checkImageCreate(ici, linearVariant))
    {
      if (!useFallbackFormat(ici))
        fatal("vulkan: texture format %s not supported", linearVariant.getNameString());
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

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  return get_memory_requirements(dev, getHandle());
}

void Image::bindMemory()
{
  G_ASSERT(getBaseHandle());
  G_ASSERT(getMemoryId() != -1);

  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  const ResourceMemory &mem = getMemory();
  VULKAN_EXIT_ON_FAIL(dev.vkBindImageMemory(dev.get(), getHandle(), mem.deviceMemory(), mem.offset));
}

void Image::reuseHandle() { fatal("vulkan: no image handle suballoc"); }

void Image::shutdown() { cleanupReferences(); }

bool Image::nonResidentCreation()
{
  return false;
  // TODO: residency is not ready yet
  // debug("image %s created in non resident state", getDebugName());
  // getHostCopyBuffer();
}

void Image::evict() { cleanupReferences(); }

void Image::restoreFromSysCopy() { doResidencyRestore(get_device().getContext()); }

bool Image::isEvictable()
{
  // TODO: residency is not ready yet
  return false;
  // return (desc.memFlags & (MEM_DEDICATE_ALLOC | MEM_NOT_EVICTABLE)) == 0;
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


void Image::doResidencyRestore(DeviceContext &context)
{
  G_ASSERT(hostCopy);
  state.resize(desc.ici.mipLevels * desc.ici.arrayLayers);
  for (auto &&s : state)
    s = SN_INITIAL;

  carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
  fillImage2BufferCopyData(copies, hostCopy);
  context.copyHostCopyToImage(hostCopy, this, desc.ici.mipLevels, copies.data());
  hostCopy = nullptr;
}

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
    hostCopy = get_device().createBuffer(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::NONE);
  }
  return hostCopy;
}

void Image::fillImage2BufferCopyData(carray<VkBufferImageCopy, MAX_MIPMAPS> &copies, Buffer *targetBuffer)
{
  VkDeviceSize bufferOffset = 0;

  for (uint32_t j = 0; j < desc.ici.mipLevels; ++j)
  {
    VkBufferImageCopy &copy = copies[j];
    copy = make_copy_info(desc.format, j, 0, desc.ici.arrayLayers, desc.ici.extent, targetBuffer->dataOffset(bufferOffset));

    bufferOffset += desc.format.calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1) *
                    desc.ici.arrayLayers;
  }
}

void Image::makeSysCopy()
{
  Buffer *buf = getHostCopyBuffer();
  carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
  fillImage2BufferCopyData(copies, buf);
  get_device().getContext().copyImageToHostCopyBuffer(this, buf, desc.ici.mipLevels, copies.data());
}

void Image::cleanupReferences()
{
  Device &drvDev = get_device();
  VulkanDevice &dev = drvDev.getVkDevice();

  if (getUsage() & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    auto iterCb = [img = this](RenderPassResource *i) { i->destroyFBsWithImage(img); };

    drvDev.resources.iterateAllocated<RenderPassResource>(iterCb);
  }

  for (auto &&view : views)
  {
    if (view.state.isRenderTarget)
      drvDev.passMan.unloadWith(dev, view.view);
    VULKAN_LOG_CALL(dev.vkDestroyImageView(dev.get(), view.view, NULL));
  }
  views.clear();
  clear_and_shrink(state);
  flags.isSealed = 0;

  if (bindlessSlots.size())
    logerr("vulkan: image %p:%s was registred in bindless while being removed", this, getDebugName());
}

void Image::wrapImage(VkImage image)
{
  Handle conv;
  conv.value = image;
  setHandle(generalize(conv));
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
