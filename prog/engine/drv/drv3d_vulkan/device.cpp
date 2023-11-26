#include <3d/dag_drv3d.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_string.h>
#include "buffer.h"
#include "texture.h"
#include "pipeline_barrier.h"

using namespace drv3d_vulkan;

#if D3D_HAS_RAY_TRACING && (VK_KHR_raytracing_pipeline || VK_KHR_ray_query)

namespace
{
VkGeometryFlagsKHR toGeometryFlagsKHR(RaytraceGeometryDescription::Flags flags)
{
  VkGeometryFlagsKHR result = 0;
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::IS_OPAQUE))
  {
    result |= VK_GEOMETRY_OPAQUE_BIT_KHR;
  }
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::NO_DUPLICATE_ANY_HIT_INVOCATION))
  {
    result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  }
  return result;
}
VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRAABBs(VulkanDevice &vkDev,
  const RaytraceGeometryDescription::AABBsInfo &info)
{
  auto buf = (GenericBufferInterface *)info.buffer;
  auto devBuf = buf->getDeviceBuffer();
  VkAccelerationStructureGeometryKHR result = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  result.pNext = nullptr;
  result.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
  result.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
  result.geometry.aabbs.pNext = nullptr;
  result.geometry.aabbs.data.deviceAddress = devBuf->getDeviceAddress(vkDev) + info.offset;
  result.geometry.aabbs.stride = info.stride;
  result.flags = toGeometryFlagsKHR(info.flags);
  return result;
}
VkAccelerationStructureGeometryKHR RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRTriangles(VulkanDevice &,
  const RaytraceGeometryDescription::TrianglesInfo &info)
{
  auto devVbuf = ((GenericBufferInterface *)info.vertexBuffer)->getDeviceBuffer();

  VkAccelerationStructureGeometryKHR result = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  result.pNext = nullptr;
  result.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  result.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  result.geometry.triangles.pNext = nullptr;
  result.geometry.triangles.vertexData.deviceAddress = devVbuf->devOffsetLoc(info.vertexOffset * info.vertexStride);
  result.geometry.triangles.maxVertex = devVbuf->getBlockSize() / info.vertexStride - 1; // assume any vertex can be accessed
  result.geometry.triangles.vertexStride = info.vertexStride;
  result.geometry.triangles.vertexFormat = VSDTToVulkanFormat(info.vertexFormat);
  if (info.indexBuffer)
  {
    auto ibuf = (GenericBufferInterface *)info.indexBuffer;
    auto devIbuf = ibuf->getDeviceBuffer();
    result.geometry.triangles.indexData.deviceAddress = devIbuf->devOffsetLoc(0);
    result.geometry.triangles.indexType = ibuf->getIndexType();
  }
  result.flags = toGeometryFlagsKHR(info.flags);
  return result;
}
} // namespace

VkAccelerationStructureGeometryKHR drv3d_vulkan::RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(VulkanDevice &vkDev,
  const RaytraceGeometryDescription &desc)
{
  switch (desc.type)
  {
    case RaytraceGeometryDescription::Type::TRIANGLES:
      return RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRTriangles(vkDev, desc.data.triangles);
    case RaytraceGeometryDescription::Type::AABBS:
      return RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHRAABBs(vkDev, desc.data.aabbs);
  }
  VkAccelerationStructureGeometryKHR def{};
  return def;
}
#endif

uint32_t Device::getMinimalBufferAlignment() const
{
  return max<uint32_t>(1, max(max(max(physicalDeviceInfo.properties.limits.minStorageBufferOffsetAlignment,
                                    physicalDeviceInfo.properties.limits.minTexelBufferOffsetAlignment),
                                physicalDeviceInfo.properties.limits.minUniformBufferOffsetAlignment),
                            physicalDeviceInfo.properties.limits.nonCoherentAtomSize));
}

bool Device::shouldUsePool(const VkImageCreateInfo &ici)
{
  if (features.test(DeviceFeaturesConfig::USE_DEDICATED_MEMORY_FOR_IMAGES))
  {
    if (features.test(DeviceFeaturesConfig::USE_DEDICATED_MEMORY_FOR_RENDER_TARGETS))
    {
      // never put render targets on pool so that it can benifit form dedicated memory
      if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        return false;
    }

    // large (what is large?) textures get thier own memory block to benifit from
    // dedicated memory
    return (ici.extent.width * ici.extent.height * max(ici.extent.depth, 1u)) <= dedicatedMemoryForImageTexelCountThreshold;
  }
  return true;
}

void Device::configure(const Config &ucfg, const PhysicalDeviceSet &pds)
{
  DriverInfo info = {};
  switch (pds.vendorId)
  {
    default: break;
    case D3D_VENDOR_NVIDIA: nvidia::get_driver_info(info); break;
    case D3D_VENDOR_ATI: amd::get_driver_info(info); break;
    case D3D_VENDOR_INTEL: intel::get_driver_info(info); break;
  }

  features = ucfg.features;

  dedicatedMemoryForImageTexelCountThreshold = ucfg.dedicatedMemoryForImageTexelCountThreshold;
  texUploadLimit = ucfg.texUploadLimitMb * 1024 * 1024 / (FRAME_FRAME_BACKLOG_LENGTH + MAX_PENDING_WORK_ITEMS);
  if (ucfg.memoryStatisticsPeriod)
  {
    context.getBackend().memoryStatisticsPeriod = ucfg.memoryStatisticsPeriod * 1000 * 1000;
    debug("vulkan: logging memory usage every %lu seconds", ucfg.memoryStatisticsPeriod);
  }

  debugLevel = ucfg.debugLevel;
}

bool Device::checkImageViewFormat(FormatStore fmt, Image *img)
{
  return checkFormatSupport(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, img->getUsage(), img->getCreateFlags(),
    img->getSampleCount());
}

// hot function
VulkanImageViewHandle Device::getImageView(Image *img, ImageViewState state)
{
  const eastl::vector<Image::ViewInfo> &views = img->getViews();
  auto iter = eastl::find_if(views.begin(), views.end(), [state](const Image::ViewInfo &view) { return view.state == state; });
  if (iter == views.end())
  {
    // this should not happen at every frame, add marker to catch such things
    TIME_PROFILE(vulkan_get_image_view_ctor);
    VkImageViewCreateInfo ivci;
    FormatStore fmt = state.getFormat();
    bool fmtOk = checkImageViewFormat(fmt, img);
    if (!fmtOk && fmt != img->getFormat())
    {
      debug("vulkan: unsupported view format %s, falling back for image original format %s", fmt.getNameString(),
        img->getFormat().getNameString());
      fmt = img->getFormat();
      fmtOk = checkImageViewFormat(fmt, img);
    }
    if (!fmtOk)
    {
      logerr("vulkan: failed to create image view for %p:%s with format %s usage %s samples %u flags %u", img, img->getDebugName(),
        fmt.getNameString(), formatImageUsageFlags(img->getUsage()), img->getSampleCount(), img->getCreateFlags());
      return VulkanImageViewHandle{};
    }
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.pNext = NULL;
    G_ASSERT((state.isRenderTarget == 1 && state.isCubemap == 0) || (state.isRenderTarget == 0));
    G_ASSERT((state.getMipBase() + state.getMipCount()) <= img->getMipLevels());
    ivci.viewType = state.getViewType(img->getType());
    ivci.image = img->getHandle();
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
        if ((fmt.asVkFormat() == VK_FORMAT_R5G6B5_UNORM_PACK16) && !(img->getUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
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
    VULKAN_EXIT_ON_FAIL(device.vkCreateImageView(device.get(), &ivci, NULL, ptr(viewInfo.view)));

    if (debugLevel)
      setVkObjectDebugName(generalize(viewInfo.view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, VK_OBJECT_TYPE_IMAGE_VIEW,
        img->getDebugName());

    iter = img->addView(viewInfo);
  }

  return iter->view;
}

static const char *image_type_name[] = {"1D", "2D", "3D"};

static const char *image_tiling_name[] = {"optimal", "linear"};

bool Device::checkImageCreate(const VkImageCreateInfo &ici, FormatStore format)
{
#define FAIL_MESSAGE(extra, ...)                                                                                   \
  debug("vulkan: image create check failed, for image w=%u, h=%u, d=%u, fmt=%s, "                                  \
        "type=%s, tilling=%s, usage=[%s], flags=%u, samples = %u" extra,                                           \
    ici.extent.width, ici.extent.height, ici.extent.depth, format.getNameString(), image_type_name[ici.imageType], \
    image_tiling_name[ici.tiling], formatImageUsageFlags(ici.usage), ici.flags, ici.samples, __VA_ARGS__)

  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(device.getInstance().vkGetPhysicalDeviceImageFormatProperties(physicalDeviceInfo.device,
    ici.format, ici.imageType, ici.tiling, ici.usage, ici.flags, &properties));
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

void Device::tryCreateDummyImage(const Image::Description::TrimmedCreateInfo &ii, const char *tex_name, bool needCompare,
  Image *&target)
{
  // not all hardware supports uav to default,
  // need to try all reasonable format types
  const FormatStore formatTableWithSimpleSampler[] = //
    {FormatStore::fromCreateFlags(TEXFMT_DEFAULT), FormatStore::fromCreateFlags(TEXFMT_R8G8B8A8),
      FormatStore::fromCreateFlags(TEXFMT_A16B16G16R16)};

  const FormatStore formatTableWithComparsionSampler[] = //
    {FormatStore::fromCreateFlags(TEXFMT_DEPTH16), FormatStore::fromCreateFlags(TEXFMT_DEPTH24),
      FormatStore::fromCreateFlags(TEXFMT_DEPTH32)};

  VkImageCreateInfo ici = ii.toVk();
  for (auto &&fmt : (needCompare ? formatTableWithComparsionSampler : formatTableWithSimpleSampler))
  {
    ici.format = fmt.asVkFormat();
    if (checkImageCreate(ici, fmt))
    {
      target =
        resources.alloc<Image>({ii, Image::MEM_NOT_EVICTABLE | Image::MEM_DEDICATE_ALLOC, fmt, VK_IMAGE_LAYOUT_UNDEFINED}, true);
      break;
    }
  }
  if (!target)
  {
    // fallback to non compare format if compare one is not available
    if (needCompare)
    {
      tryCreateDummyImage(ii, tex_name, false, target);
      return;
    }
    else
      fatal("vulkan: can't create dummy %s texture", tex_name);
  }
  setTexName(target, tex_name);
}

void Device::setupDummyResources()
{
  const uint32_t dummy_array_count = 6 * 2;
  SamplerState st;
  st.setMip(VK_SAMPLER_MIPMAP_MODE_LINEAR);
  st.setFilter(VK_FILTER_LINEAR);
  st.setU(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  st.setV(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  st.setW(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  st.setBias(0.f);
  st.setAniso(1.f);
  st.setBorder(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
  defaultSampler = getSampler(st);

  Image::Description::TrimmedCreateInfo ii;
  ii.imageType = VK_IMAGE_TYPE_2D;
  ii.extent.width = 1;
  ii.extent.height = 1;
  ii.extent.depth = 1;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  ii.mipLevels = 1;
  ii.arrayLayers = dummy_array_count;
  ii.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  ii.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

  tryCreateDummyImage(ii, "dummy2D-1x1-with-12-array-slices-and-cube-flag", false /*needCompare*/, dummyImage2D);
  tryCreateDummyImage(ii, "dummy2D-1x1-with-12-array-slices-and-cube-flag-pcf", true /*needCompare*/, dummyImage2DwithCompare);
  auto maxFormatPixelSize = dummyImage2D->getFormat().getBytesPerPixelBlock();

  ii.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  tryCreateDummyImage(ii, "dummy2DStorage-1x1-with-12-array-slices-and-cube-flag", false /*needCompare*/, dummyStorage2D);
  maxFormatPixelSize = max(maxFormatPixelSize, dummyStorage2D->getFormat().getBytesPerPixelBlock());

  ii.imageType = VK_IMAGE_TYPE_3D;
  ii.arrayLayers = 1;
  ii.flags = 0;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  tryCreateDummyImage(ii, "dummy3D-1x1x1", false /*needCompare*/, dummyImage3D);
  tryCreateDummyImage(ii, "dummy3D-1x1x1-pcf", true /*needCompare*/, dummyImage3DwithCompare);
  maxFormatPixelSize = max(maxFormatPixelSize, dummyImage3D->getFormat().getBytesPerPixelBlock());

  ii.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  tryCreateDummyImage(ii, "dummy3DStorage-1x1x1", false /*needCompare*/, dummyStorage3D);
  maxFormatPixelSize = max(maxFormatPixelSize, dummyStorage3D->getFormat().getBytesPerPixelBlock());

  auto bufViewFmt = FormatStore::fromCreateFlags(TEXFMT_A32B32G32R32F);
  VkDeviceSize dummySize = max<VkDeviceSize>(1024 * 8 * bufViewFmt.getBytesPerPixelBlock(),
    maxFormatPixelSize * ii.extent.width * ii.extent.height * ii.extent.depth * dummy_array_count);
  dummyBuffer = createBuffer(dummySize, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
  setBufName(dummyBuffer, "dummyBuffer-1Mibyte");
  addBufferView(dummyBuffer, bufViewFmt);
  // zero out the buffer
  auto stage = createBuffer(dummySize, DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER, 1, BufferMemoryFlags::NONE);
  memset(stage->ptrOffsetLoc(0), 0, dummySize);

  // everything is 0 execept target image aspect, layer count and extent
  VkBufferImageCopy region = {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = dummy_array_count;
  region.imageExtent = ii.extent;

  context.copyBufferToImage(stage, dummyImage2D, 1, &region, true);
  region.imageSubresource.layerCount = 1;
  context.copyBufferToImage(stage, dummyImage3D, 1, &region, true);

  region.imageSubresource.aspectMask = dummyImage2DwithCompare->getFormat().getAspektFlags();
  region.imageSubresource.layerCount = dummy_array_count;
  context.copyBufferToImage(stage, dummyImage2DwithCompare, 1, &region, true);

  region.imageSubresource.aspectMask = dummyImage3DwithCompare->getFormat().getAspektFlags();
  region.imageSubresource.layerCount = 1;
  context.copyBufferToImage(stage, dummyImage3DwithCompare, 1, &region, true);

  context.uploadBuffer(stage, dummyBuffer, 0, 0, dummySize);

  context.destroyBuffer(stage);

  ImageViewState ivs;
  ivs.setMipBase(0);
  ivs.setMipCount(1);
  ivs.setArrayBase(0);

  ivs.setArrayCount(1);
  ivs.isArray = 0;
  ivs.isCubemap = 0;
  ivs.setFormat(dummyImage2D->getFormat());
  auto imageView = getImageView(dummyImage2D, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].resource = (void *)dummyImage2D;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.sampler = defaultSampler->colorSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyImage2DwithCompare->getFormat());
  imageView = getImageView(dummyImage2DwithCompare, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX].resource = (void *)dummyImage2DwithCompare;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX].descriptor.image.sampler = defaultSampler->compareSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyStorage2D->getFormat());
  imageView = getImageView(dummyStorage2D, ivs);
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_INDEX].resource = (void *)dummyStorage2D;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  ivs.setArrayCount(dummy_array_count);
  ivs.isArray = 1;
  ivs.setFormat(dummyImage2D->getFormat());
  imageView = getImageView(dummyImage2D, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX].resource = (void *)dummyImage2D;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX].descriptor.image.sampler = defaultSampler->colorSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyImage2DwithCompare->getFormat());
  imageView = getImageView(dummyImage2DwithCompare, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX].resource = (void *)dummyImage2DwithCompare;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX].descriptor.image.sampler =
    defaultSampler->compareSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyStorage2D->getFormat());
  imageView = getImageView(dummyStorage2D, ivs);
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX].resource = (void *)dummyStorage2D;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  ivs.setArrayCount(6);
  ivs.isArray = 0;
  ivs.isCubemap = 1;
  ivs.setFormat(dummyImage2D->getFormat());
  imageView = getImageView(dummyImage2D, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX].resource = (void *)dummyImage2D;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX].descriptor.image.sampler = defaultSampler->colorSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyImage2DwithCompare->getFormat());
  imageView = getImageView(dummyImage2DwithCompare, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX].resource = (void *)dummyImage2DwithCompare;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX].descriptor.image.sampler = defaultSampler->compareSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyStorage2D->getFormat());
  imageView = getImageView(dummyStorage2D, ivs);
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX].resource = (void *)dummyStorage2D;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  ivs.setArrayCount(dummy_array_count);
  ivs.isArray = 1;
  ivs.isCubemap = 1;
  ivs.setFormat(dummyImage2D->getFormat());
  imageView = getImageView(dummyImage2D, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX].resource = (void *)dummyImage2D;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX].descriptor.image.sampler = defaultSampler->colorSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyImage2DwithCompare->getFormat());
  imageView = getImageView(dummyImage2DwithCompare, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX].resource = (void *)dummyImage2DwithCompare;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX].descriptor.image.sampler =
    defaultSampler->compareSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyStorage2D->getFormat());
  imageView = getImageView(dummyStorage2D, ivs);
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX].resource = (void *)dummyStorage2D;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  ivs.setArrayCount(1);
  ivs.isArray = 0;
  ivs.isCubemap = 0;
  ivs.setFormat(dummyImage3D->getFormat());
  imageView = getImageView(dummyImage3D, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_3D_INDEX].resource = (void *)dummyImage3D;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_3D_INDEX].descriptor.image.sampler = defaultSampler->colorSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_3D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_3D_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyImage3DwithCompare->getFormat());
  imageView = getImageView(dummyImage3DwithCompare, ivs);
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX].resource = (void *)dummyImage3DwithCompare;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX].descriptor.image.sampler = defaultSampler->compareSampler();
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX].descriptor.image.imageLayout =
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  ivs.setFormat(dummyStorage3D->getFormat());
  imageView = getImageView(dummyStorage3D, ivs);
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_3D_INDEX].resource = (void *)dummyStorage3D;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_3D_INDEX].descriptor.image.imageView = imageView;
  dummyResourceTable[spirv::MISSING_STORAGE_IMAGE_3D_INDEX].descriptor.image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  dummyResourceTable[spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX].resource = (void *)dummyBuffer;
  dummyResourceTable[spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX].descriptor.texelBuffer = dummyBuffer->getView();

  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX].resource = (void *)dummyBuffer;
  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX].descriptor.texelBuffer = dummyBuffer->getView();

  VkDeviceSize dummyUniformSize =
    min<uint32_t>(dummyBuffer->getBlockSize(), physicalDeviceInfo.properties.limits.maxUniformBufferRange);

  dummyResourceTable[spirv::MISSING_BUFFER_INDEX].resource = (void *)dummyBuffer;
  dummyResourceTable[spirv::MISSING_BUFFER_INDEX].descriptor.buffer.buffer = dummyBuffer->getHandle();
  dummyResourceTable[spirv::MISSING_BUFFER_INDEX].descriptor.buffer.offset = dummyBuffer->bufOffsetLoc(0);
  dummyResourceTable[spirv::MISSING_BUFFER_INDEX].descriptor.buffer.range = dummyUniformSize;

  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_INDEX].resource = (void *)dummyBuffer;
  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_INDEX].descriptor.buffer.buffer = dummyBuffer->getHandle();
  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_INDEX].descriptor.buffer.offset = dummyBuffer->bufOffsetLoc(0);
  dummyResourceTable[spirv::MISSING_STORAGE_BUFFER_INDEX].descriptor.buffer.range = dummyBuffer->getBlockSize();

  dummyResourceTable[spirv::MISSING_CONST_BUFFER_INDEX].resource = (void *)dummyBuffer;
  dummyResourceTable[spirv::MISSING_CONST_BUFFER_INDEX].descriptor.buffer.buffer = dummyBuffer->getHandle();
  dummyResourceTable[spirv::MISSING_CONST_BUFFER_INDEX].descriptor.buffer.offset = dummyBuffer->bufOffsetLoc(0);
  dummyResourceTable[spirv::MISSING_CONST_BUFFER_INDEX].descriptor.buffer.range = dummyUniformSize;

  // dummy is special, should be treated as null to avoid clearing up after fallback replacement
  for (auto &i : dummyResourceTable)
    i.descriptor.type = VkAnyDescriptorInfo::TYPE_NULL;

  // no need to set them, constructor will set them to null
  // memset(&dummyResourceTable[spirv::compute::MISSING_IS_FATAL_INDEX], 0,
  // sizeof(dummyResourceTable[0]));
  // memset(&dummyResourceTable[spirv::compute::FALLBACK_TO_C_GLOBAL_BUFFER], 0,
  // sizeof(dummyResourceTable[0]));
}

Device::~Device() { shutdown(); }

DeviceMemoryConfiguration Device::getDeviceMemoryConfiguration() const { return memory.unlocked().getMemoryConfiguration(); }

bool Device::checkFormatSupport(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
  VkImageCreateFlags flags, VkSampleCountFlags samples)
{
  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(device.getInstance().vkGetPhysicalDeviceImageFormatProperties(physicalDeviceInfo.device,
    format, type, tiling, usage, flags, &properties));
  if (VULKAN_FAIL(result))
    return false;
  // needed ?
  if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
    return properties.maxArrayLayers >= 6;
  if (!(samples & properties.sampleCounts))
    return false;
  return true;
}

VkFormatFeatureFlags Device::getFormatFeatures(VkFormat format)
{
  VkFormatProperties props;
  VULKAN_LOG_CALL(device.getInstance().vkGetPhysicalDeviceFormatProperties(physicalDeviceInfo.device, format, &props));
  return props.optimalTilingFeatures;
}

VkSampleCountFlags Device::getFormatSamples(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage)
{
  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(device.getInstance().vkGetPhysicalDeviceImageFormatProperties(physicalDeviceInfo.device,
    format, type, tiling, usage, 0, &properties));

  return VULKAN_OK(result) ? properties.sampleCounts : 0;
}

bool Device::init(VulkanInstance &inst, const Config &ucfg, const PhysicalDeviceSet &set, const VkDeviceCreateInfo &config,
  const SwapchainMode &swc_info, const DeviceQueueGroup::Info &queue_info)
{
  physicalDeviceInfo = set;
  if (!device.init(&inst, set.device, config))
    return false;

  bool hasAllRequiredExt =
    device_has_all_required_extensions(device, [](const char *name) { debug("vulkan: Missing device extension %s", name); });
  if (!hasAllRequiredExt)
  {
    debug("vulkan: Rejecting device, some required extensions where not loaded (broken driver)");
    device.shutdown();
    return false;
  }

  debug("vulkan: Enabled extensions...");
  device.iterateEnabledExtensionNames([](const char *name) { debug("vulkan: ...%s...", name); });

  queues = DeviceQueueGroup(device, queue_info);

  memory.unlocked().init(set.memoryProperties);
#if VK_EXT_memory_budget
  if (set.memoryBudgetInfoAvailable)
    memory.unlocked().applyBudget(set.memoryBudgetInfo);
#endif
  {
    resources.init(&memory.getGuard(), set);
  }

  if (ucfg.features.test(DeviceFeaturesConfig::COMMAND_MARKER))
    execMarkers.init();


  configure(ucfg, set);
  if (!swapchain.init(swc_info))
  {
    device.shutdown();
    return false;
  }

  ExecutionMode execMode = ExecutionMode::DEFERRED;
  if (ucfg.features.test(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION))
  {
    execMode = ExecutionMode::THREADED;
    RenderWork::recordCommandCallers = ucfg.features.test(DeviceFeaturesConfig::RECORD_COMMAND_CALLER);
  }

  timelineMan.init();

  context.getFrontend().replayRecord.start();
  context.getBackend().contextState.frame.start();
  shaders::RenderState emptyRS{};
  context.getBackend().contextState.renderStateSystem.setRenderStateData((shaders::DriverRenderStateId)0, emptyRS, *this);
  context.getBackend().pipelineCompiler.init();
  context.initTimingRecord();
  context.initTempBuffersConfiguration();
  context.initMode(execMode);
  ExecutionSyncTracker::LogicAddress::setAttachmentNoStoreSupport(hasAttachmentNoStoreOp());

  setupDummyResources();

  arrayImageTo3DBufferSize = 0;
  arrayImageTo3DBuffer = NULL;

  if (!canRenderTo3D())
  {
    const uint32_t initialCopyBufferSize = 1024 * 1024 * 4;
    arrayImageTo3DBuffer = createBuffer(initialCopyBufferSize, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
    setBufName(arrayImageTo3DBuffer, "3d rendering emul transfer buffer");
    arrayImageTo3DBufferSize = initialCopyBufferSize;
  }

  return true;
}

bool Device::isInitialized() const { return !is_null(device.get()); }

void Device::shutdownDummyResources()
{
  context.destroyBuffer(dummyBuffer);
  context.destroyImage(dummyImage2D);
  context.destroyImage(dummyImage2DwithCompare);
  context.destroyImage(dummyImage3D);
  context.destroyImage(dummyImage3DwithCompare);
  context.destroyImage(dummyStorage2D);
  context.destroyImage(dummyStorage3D);
}

void Device::shutdown()
{
  if (!device.isValid())
    return;

  // shutdown surface if we exited prior window destruction
  SwapchainMode newMode(swapchain.getMode());
  newMode.surface = VulkanNullHandle();
  swapchain.setMode(newMode);

  if (features.test(DeviceFeaturesConfig::COMMAND_MARKER))
    execMarkers.shutdown();

  if (arrayImageTo3DBuffer)
    context.destroyBuffer(arrayImageTo3DBuffer);

  shutdownDummyResources();

#if D3D_HAS_RAY_TRACING
  if (raytraceData.scratchBuffer)
    context.destroyBuffer(raytraceData.scratchBuffer);
#endif

  surveys.shutdownDataBuffers();

  context.shutdown();

  VkResult res = device.vkDeviceWaitIdle(device.get());
  if (VULKAN_FAIL(res))
    logerr("vulkan: Device::shutdown vkDeviceWaitIdle failed with %08lX", res);

  for (int i = 0; i < samplers.size(); ++i)
  {
    VULKAN_LOG_CALL(device.vkDestroySampler(device.get(), samplers[i]->sampler[0], NULL));
    VULKAN_LOG_CALL(device.vkDestroySampler(device.get(), samplers[i]->sampler[1], NULL));
  }
  clear_all_ptr_items_and_shrink(samplers);

  timelineMan.shutdown();

  pipeMan.unloadAll(device);

  VULKAN_LOG_CALL(device.vkDestroyPipelineCache(device.get(), pipelineCache, NULL));
  pipelineCache = VulkanNullHandle();

  passMan.unloadAll(device);

  fenceManager.resetAll(device);

  resources.shutdown();

  eventPool.resetAll(device);

  surveys.shutdownPools();
  timestamps.shutdownPools();

  BuiltinPipelineBarrierCache::clear();

  device.shutdown();
}

void Device::initBindless()
{
  if (!physicalDeviceInfo.hasBindless)
    return;

  const DataBlock *propsBlk = ::dgs_get_settings()->getBlockByNameEx("vulkan");

  // disable bindless by default, enable bindless by per-project configs
  const bool enableBindless = propsBlk->getBool("enableBindless", false);
  const uint32_t requiredBindlessTextureCount = propsBlk->getInt("bindlessTextureCount", 256 * 1024);
  const uint32_t requiredBindlessSamplerCount = propsBlk->getInt("bindlessSamplerCount", 2048);

  const bool hasEnoughDescriptorSets =
    physicalDeviceInfo.properties.limits.maxBoundDescriptorSets >= spirv::graphics::MAX_SETS + spirv::bindless::MAX_SETS;
  const bool hasEnoughBindlessTexture = physicalDeviceInfo.maxBindlessTextures >= requiredBindlessTextureCount;
  const bool hasEnoughBindlessSampler = physicalDeviceInfo.maxBindlessSamplers >= requiredBindlessSamplerCount;

  // some drivers have buggy bindless support, allow to disable it there if needed
  const bool driverDisabledBindless = getPerDriverPropertyBlock("disableBindless")->getBool("affected", !enableBindless);

  physicalDeviceInfo.hasBindless =
    hasEnoughDescriptorSets && hasEnoughBindlessTexture && hasEnoughBindlessSampler && !driverDisabledBindless;

  if (physicalDeviceInfo.hasBindless)
  {
    debug("vulkan: bindless enabled, max sampled images: %u (device max: %u), max samplers: %u (device max: %u)",
      requiredBindlessTextureCount, physicalDeviceInfo.maxBindlessTextures, requiredBindlessSamplerCount,
      physicalDeviceInfo.maxBindlessSamplers);

    auto &bindlessManagerBackend = context.getBackend().contextState.bindlessManagerBackend;
    bindlessManagerBackend.init(device, requiredBindlessTextureCount, requiredBindlessSamplerCount);

    PipelineBindlessConfig::bindlessSetCount = bindlessManagerBackend.getActiveBindlessSetCount();
    PipelineBindlessConfig::bindlessTextureSetLayout = bindlessManagerBackend.getTextureDescriptorLayout();
    PipelineBindlessConfig::bindlessSamplerSetLayout = bindlessManagerBackend.getSamplerDescriptorLayout();
  }
  else
    debug("vulkan: bindless disabled [%s %s %s %s %s]", enableBindless ? "-" : "global", driverDisabledBindless ? "hw_driver" : "-",
      hasEnoughDescriptorSets ? "-" : "descriptor_sets", hasEnoughBindlessTexture ? "-" : "tex_limit",
      hasEnoughBindlessSampler ? "-" : "sampler_limit");
}

const DataBlock *Device::getPerDriverPropertyBlock(const char *prop_name)
{
  const DataBlock *propsBlk = ::dgs_get_settings()
                                ->getBlockByNameEx("vulkan")
                                ->getBlockByNameEx("vendor")
                                ->getBlockByNameEx(physicalDeviceInfo.vendorName)
                                ->getBlockByNameEx("driverProps")
                                ->getBlockByNameEx(prop_name);

  uint32_t drvMajor = physicalDeviceInfo.driverVersionDecoded[0];
  uint32_t drvMinor = physicalDeviceInfo.driverVersionDecoded[1];
  uint32_t drvVerMax = 0x7FFFFFFF;

  const DataBlock *ret = &DataBlock::emptyBlock;
  dblk::iterate_child_blocks_by_name(*propsBlk, "entry", [&](const DataBlock &entry) {
    bool fitsMin = drvMajor >= entry.getInt("driverMinVer0", 0) && drvMinor >= entry.getInt("driverMinVer1", 0);
    bool fitsMax = drvMajor <= entry.getInt("driverMaxVer0", drvVerMax) && drvMinor <= entry.getInt("driverMaxVer1", drvVerMax);

    bool gpuMatch = true;
    if (entry.paramExists("gpu"))
    {
      gpuMatch = false;
      dblk::iterate_params_by_name(entry, "gpu",
        [&](int idx, int, int) { gpuMatch |= strstr(physicalDeviceInfo.properties.deviceName, entry.getStr(idx)) != NULL; });
    }

    if (fitsMin && fitsMax && gpuMatch)
      ret = entry.getBlockByNameEx("data");
  });

  if (ret == &DataBlock::emptyBlock)
    ret = ::dgs_get_settings()
            ->getBlockByNameEx("vulkan")
            ->getBlockByNameEx("vendor")
            ->getBlockByNameEx("default")
            ->getBlockByNameEx("driverProps")
            ->getBlockByNameEx(prop_name);

  return ret;
}

#if _TARGET_ANDROID
void Device::fillMobileCaps(Driver3dDesc &caps)
{
  switch (physicalDeviceInfo.vendorId)
  {
    // Qualcomm Adreno GPUs
    case D3D_VENDOR_QUALCOMM:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      // CS writes to 3d texture are not working for some reason
      caps.issues.hasComputeCanNotWrite3DTex = true;
      // driver bug/crash in pipeline construction when attachment ref is VK_ATTACHMENT_UNUSED
      caps.issues.hasStrictRenderPassesOnly = true;
      // have buggy/time limited compute stage
      caps.issues.hasComputeTimeLimited = true;

      // have buggy maxTexelBufferElements defenition, above 65k, but in real is 65k!
      // will device lost on some chips/drivers when sampling above 65k elements
      caps.issues.hasSmallSampledBuffers = true;

      caps.issues.hasRenderPassClearDataRace = getPerDriverPropertyBlock("adrenoClearStoreRace")->getBool("affected", false);

      break;
    // Arm Mali GPUs
    case D3D_VENDOR_ARM:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      // have buggy/time limited compute stage
      caps.issues.hasComputeTimeLimited = true;
      // have buggy maxTexelBufferElements defenition, above 65k, but in real is 65k!
      // will device lost on all chips/drivers when sampling above 65k elements
      caps.issues.hasSmallSampledBuffers = true;
#if VK_KHR_driver_properties
      if (physicalDeviceInfo.hasExtension<DriverPropertiesKHR>())
      {
        // driver info contains string in format "v1.r<major>p<minor>-01<tag>0.<sha1>"
        // where tag can be quite anything, but for bugged devices we have on hands it is
        // bet2-mbs2v39_ so trigger DD4 flag by it for now
        if (strstr(physicalDeviceInfo.driverProps.driverInfo, "-01bet"))
        {
          caps.issues.hasBrokenBaseInstanceID = true;
          debug("vulkan: arm: detected driver with broken base instance id support");
        }
      }
#endif
      break;
    // Imagetec PowerVR GPUs
    case D3D_VENDOR_IMGTEC:
      // TBDR GPU
      caps.caps.hasTileBasedArchitecture = true;
      break;
    default:
      // we can run android on non-mobile hardware too
      break;
  }

  // Some GPUs will drop to immediate mode when multi draw indirect is present
  // this is performance-wise-bad, so allow to explicitly disable it via config
  if (getPerDriverPropertyBlock("disableMultiDrawIndirect")->getBool("affected", false))
    caps.caps.hasWellSupportedIndirect = false;

  if (getPerDriverPropertyBlock("msaaAndInstancingHang")->getBool("affected", false))
  {
    caps.issues.hasMultisampledAndInstancingHang = true;
    debug("vulkan: DeviceDriverCapabilities::bugMultisampledAndInstancingHang - GPU hangs with combination of MSAA, non-RGB8 RT and "
          "instancing in RIEx");
  }

  if (getPerDriverPropertyBlock("ignoreDeviceLost")->getBool("affected", false))
  {
    caps.issues.hasIgnoreDeviceLost = true;
    debug("vulkan: DeviceDriverCapabilities::bugIgnoreDeviceLost - spams device_lost, but continues to work as normal");
  }

  if (getPerDriverPropertyBlock("pollFences")->getBool("affected", false))
  {
    caps.issues.hasPollDeviceFences = true;
    debug("vulkan: DeviceDriverCapabilities::bugPollDeviceFences - vkWaitForFences randomly returns device_lost, use vkGetFenceStatus "
          "instead");
  }

  if (getPerDriverPropertyBlock("msaaAndBlendingHang")->getBool("affected", false))
  {
    caps.issues.hasMultisampledAndBlendingHang = true;
    debug("vulkan: DeviceDriverCapabilities::bugMultisampledAndBlendingHang - GPU hangs with combination of MSAA and some "
          "alphablending shaders");
  }

  if (getPerDriverPropertyBlock("brokenShadersAfterAppSwitch")->getBool("affected", false))
  {
    caps.issues.hasBrokenShadersAfterAppSwitch = true;
    debug("vulkan: DeviceDriverCapabilities::hasBrokenShadersAfterAppSwitch - may not render some long shaders after switching from "
          "the application and back");
  }

  if (caps.caps.hasTileBasedArchitecture)
  {
    // only check on TBDRs, all other stuff is considered bugged driver or some software renderers
    // and we don't want to support them at cost of exploding shader variants
    // 0x4000000 is next majorly supported limit from vulkan.gpuinfo.org
    caps.issues.hasSmallSampledBuffers = physicalDeviceInfo.properties.limits.maxTexelBufferElements < 0x4000000;
    // FIXME:
    // issuing draws with
    //   -disabled depth test, enabled  depth bound
    //   -enabled  depth test, disabled depth bound
    // in one render pass corrupt frame on tilers
    // it looks like they can't decide who should write to RT
    // buggin it out by not writing data at all or flickering between said draws
    // we can't make separate passes for this situation right now
    // so disable it for time being
    caps.caps.hasDepthBoundsTest = false;
  }

  // some older drivers fail to compile/execute properly DXC generated shaders
  // note app about it to either use workaround interval or use hlslcc dump
  if (getPerDriverPropertyBlock("limitedDXCSupport")->getBool("affected", false))
  {
    caps.issues.hasLimitedDXCShaders = true;
    physicalDeviceInfo.hasBindless = false;
    debug("vulkan: running on device-driver with bad DXC support, related features are OFF!");
  }

  if (caps.issues.hasRenderPassClearDataRace)
    debug("vulkan: running on device-driver combo with clear-store race");

  if (getPerDriverPropertyBlock("disableShaderFloat16")->getBool("affected", false))
  {
    caps.caps.hasShaderFloat16Support = false;
    debug("vulkan: running on device-driver with bad shader float16 support, related features are OFF!");
  }

  caps.caps.hasLazyMemory = physicalDeviceInfo.hasLazyMemory;
}
#endif

void Device::adjustCaps(Driver3dDesc &caps)
{
  VkPhysicalDeviceProperties &properties = physicalDeviceInfo.properties;

#if _TARGET_ANDROID
  // ~99% of device / drivers support it, the remaining 1% is probably not compatible anyways
  caps.caps.hasAnisotropicFilter = hasAnisotropicSampling();
#endif

#if _TARGET_PC_WIN
  caps.caps.hasDepthReadOnly = true;
  caps.caps.hasStructuredBuffers = true;
  caps.caps.hasNoOverwriteOnShaderResourceBuffers = true;
  // Until implemented, for non pc this is constant false
  // DX11/DX12 approach with single sample RT attachment is not available on vulkan
  // only with combination of multisample DS and VK_AMD_mixed_attachment_samples/VK_NV_framebuffer_mixed_samples exts
  // VUID-VkGraphicsPipelineCreateInfo-subpass-00757
  // VUID-VkGraphicsPipelineCreateInfo-subpass-01411
  // VUID-VkGraphicsPipelineCreateInfo-subpass-01505
  caps.caps.hasForcedSamplerCount = false;
  caps.caps.hasVolMipMap = true;
  caps.caps.hasAsyncCompute = false;
  caps.caps.hasOcclusionQuery = true;
  caps.caps.hasConstBufferOffset = false;
  caps.caps.hasResourceCopyConversion = true;
  caps.caps.hasReadMultisampledDepth = true;
  caps.caps.hasQuadTessellation = hasTesselationShader();
  caps.caps.hasGather4 = true;
  caps.caps.hasRaytracing = false;
  caps.caps.hasRaytracingT11 = false;
  caps.caps.hasNVApi = false;
  caps.caps.hasATIApi = false;
  // needs VK_NV_shading_rate_image
  caps.caps.hasVariableRateShading = false;
  caps.caps.hasVariableRateShadingTexture = false;
  // no extension supports these yet
  caps.caps.hasVariableRateShadingShaderOutput = false;
  caps.caps.hasVariableRateShadingCombiners = false;
  caps.caps.hasVariableRateShadingBy4 = false;
  caps.caps.hasAliasedTextures = false;
  caps.caps.hasResourceHeaps = false;
  caps.caps.hasBufferOverlapCopy = false;
  caps.caps.hasBufferOverlapRegionsCopy = false;
  caps.caps.hasShader64BitIntegerResources = false;
  caps.caps.hasTiled2DResources = false;
  caps.caps.hasTiled3DResources = false;
  caps.caps.hasTiledSafeResourcesAccess = false;
  caps.caps.hasTiledMemoryAliasing = false;
  caps.caps.hasDLSS = false;
  caps.caps.hasXESS = false;
  caps.caps.hasMeshShader = false;
  caps.caps.hasBasicViewInstancing = false;
  caps.caps.hasOptimizedViewInstancing = false;
  caps.caps.hasAcceleratedViewInstancing = false;
#endif

#if _TARGET_PC_WIN
  caps.caps.hasDrawID = true;
  caps.caps.hasNativeRenderPassSubPasses = true;
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID
  caps.caps.hasDepthBoundsTest = hasDepthBoundsTest();
  // only partially instance id support, no idea how to expos this right.
  caps.caps.hasInstanceID = hasDrawIndirectFirstInstance();
  caps.caps.hasConservativeRassterization = device.hasExtension<ConservativeRasterizationEXT>();
  caps.caps.hasWellSupportedIndirect = hasMultiDrawIndirect();
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_C3 || _TARGET_ANDROID
  caps.caps.hasConditionalRender = hasConditionalRender();
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID
  caps.caps.hasUAVOnlyForcedSampleCount = hasUAVOnlyForcedSampleCount();
#endif

  caps.caps.hasBindless = false;
  caps.caps.hasRenderPassDepthResolve = physicalDeviceInfo.hasDepthStencilResolve;

  caps.shaderModel = 6.0_sm;
  if (!hasWaveOperations(VK_SHADER_STAGE_FRAGMENT_BIT) || !hasWaveOperations(VK_SHADER_STAGE_COMPUTE_BIT))
  {
    caps.shaderModel = 5.0_sm;
  }
  if (!hasFragmentShaderUAV())
  {
    // not entirely correct
    caps.shaderModel = 4.1_sm;
  }

#if (_TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID) && D3D_HAS_RAY_TRACING
  caps.caps.hasRaytracing = physicalDeviceInfo.hasRayTracingPipeline && physicalDeviceInfo.hasAccelerationStructure;
  caps.caps.hasRaytracingT11 = physicalDeviceInfo.hasRayQuery && physicalDeviceInfo.hasAccelerationStructure;

  caps.raytraceShaderGroupSize = physicalDeviceInfo.raytraceShaderHeaderSize;
  caps.raytraceMaxRecursion = physicalDeviceInfo.raytraceMaxRecursionDepth;
  caps.raytraceTopAccelerationInstanceElementSize = physicalDeviceInfo.raytraceTopAccelerationInstanceElementSize;

  // Explicitly disable pipeline raytracing until it's ready just like DX12 driver does
  caps.caps.hasRaytracing = false;
#endif

  caps.caps.hasShaderFloat16Support = physicalDeviceInfo.hasShaderFloat16;

#if _TARGET_ANDROID
  fillMobileCaps(caps);
#endif

  // bindless initialization can fail and depends on mobile caps
  // do it right here to properly reflect dependencies
  initBindless();
  caps.caps.hasBindless = physicalDeviceInfo.hasBindless;


  caps.maxtexw = min(caps.maxtexw, (int)properties.limits.maxImageDimension2D);
  caps.maxtexh = min(caps.maxtexh, (int)properties.limits.maxImageDimension2D);
  caps.maxcubesize = min(caps.maxcubesize, (int)properties.limits.maxImageDimensionCube);
  caps.maxvolsize = min(caps.maxvolsize, (int)properties.limits.maxImageDimension3D);
  caps.maxtexaspect = max(caps.maxtexaspect, 0);
  caps.maxtexcoord = min(caps.maxtexcoord, (int)properties.limits.maxVertexInputAttributes);
  caps.maxsimtex = min(caps.maxsimtex, (int)properties.limits.maxDescriptorSetSampledImages);
  caps.maxvertexsamplers = min(caps.maxvertexsamplers, (int)properties.limits.maxPerStageDescriptorSampledImages);
  caps.maxclipplanes = min(caps.maxclipplanes, (int)properties.limits.maxClipDistances);
  caps.maxstreams = min(caps.maxstreams, (int)properties.limits.maxVertexInputBindings);
  caps.maxstreamstr = min(caps.maxstreamstr, (int)properties.limits.maxVertexInputBindingStride);
  caps.maxvpconsts = min(caps.maxvpconsts, (int)(properties.limits.maxUniformBufferRange / (sizeof(float) * 4)));
  caps.maxprims = min(caps.maxprims, (int)properties.limits.maxDrawIndexedIndexValue);
  caps.maxvertind = min(caps.maxvertind, (int)properties.limits.maxDrawIndexedIndexValue);
  caps.maxSimRT = min(caps.maxSimRT, (int)properties.limits.maxColorAttachments);
  caps.minWarpSize = physicalDeviceInfo.warpSize;
}

void Device::loadPipelineCache(PipelineCacheFile &src)
{
  VkPipelineCacheCreateInfo pcci;
  pcci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  pcci.pNext = NULL;
  pcci.flags = 0;
  Tab<uint8_t> data;
  if (src.getCacheFor(physicalDeviceInfo.properties.pipelineCacheUUID, data))
  {
    pcci.initialDataSize = data.size();
    pcci.pInitialData = data.data();
  }
  else
  {
    pcci.initialDataSize = 0;
    pcci.pInitialData = NULL;
  }

  VulkanPipelineCacheHandle loadedCache;

  if (VULKAN_CHECK_OK(device.vkCreatePipelineCache(device.get(), &pcci, NULL, ptr(loadedCache))))
  {
    debug("vulkan: loaded pipeline cache with size %uKb", pcci.initialDataSize >> 10);

    if (is_null(pipelineCache))
      pipelineCache = loadedCache;
    else
      context.addPipelineCache(loadedCache);
  }
}

void Device::storePipelineCache(PipelineCacheFile &dst)
{
  size_t count;
  if (VULKAN_CHECK_FAIL(device.vkGetPipelineCacheData(device.get(), pipelineCache, &count, NULL)))
    count = 0;

  debug("vulkan: extracting pipeline cache data of size %u Kb from driver", count >> 10);

  if (count)
  {
    Tab<uint8_t> data;
    data.resize(count);
    if (VULKAN_CHECK_OK(device.vkGetPipelineCacheData(device.get(), pipelineCache, &count, data.data())))
      dst.storeCacheFor(physicalDeviceInfo.properties.pipelineCacheUUID, data);
  }
}

// check extension presense
#if VK_KHR_maintenance1
bool Device::canRenderTo3D() const
{
  return !features.test(DeviceFeaturesConfig::DISABLE_RENDER_TO_3D_IMAGE) && device.hasExtension<Maintenance1KHR>();
}
#else
bool Device::canRenderTo3D() const { return false; }
#endif

bool Device::adrenoGraphicsViewportConflictWithCS() { return physicalDeviceInfo.vendorId == D3D_VENDOR_QUALCOMM; }

#if D3D_HAS_RAY_TRACING

bool Device::hasRaytraceSupport() const
{
  return
#if VK_KHR_ray_tracing_pipeline
    device.hasExtension<RaytracingPipelineKHR>() ||
#endif
#if VK_KHR_ray_query
    device.hasExtension<RayQueryKHR>() ||
#endif
    false;
}

#endif

bool Device::hasGpuTimestamps()
{
  return !(physicalDeviceInfo.properties.limits.timestampComputeAndGraphics == VK_FALSE ||
           queues[DeviceQueueType::GRAPHICS].getTimestampBits() < 1);
}

uint64_t Device::getGpuTimestampFrequency() const
{
  return uint64_t(1000000000.0 / physicalDeviceInfo.properties.limits.timestampPeriod);
}

Image *Device::createImage(const ImageCreateInfo &ii)
{
  Image::Description::TrimmedCreateInfo ici;
  ici.flags = ii.flags;
  ici.imageType = ii.type;
  ici.extent = ii.size;
  ici.mipLevels = ii.mips;
  ici.arrayLayers = ii.arrays;
  ici.usage = ii.usage;
  ici.samples = ii.samples;

  bool useDedAlloc = !shouldUsePool(ici.toVk());

  uint8_t memFlags = (useDedAlloc ? (Image::MEM_DEDICATE_ALLOC | Image::MEM_NOT_EVICTABLE) : 0) | ii.residencyFlags;

  if (ici.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    memFlags = Image::MEM_LAZY_ALLOCATION;

  SharedGuardedObjectAutoLock lock(resources);
  return resources.alloc<Image>({ici, memFlags, ii.format, VK_IMAGE_LAYOUT_UNDEFINED}, true);
}

Buffer *Device::createBuffer(uint32_t size, DeviceMemoryClass memory_class, uint32_t discard_count, BufferMemoryFlags mem_flags)
{
  G_ASSERTF(discard_count > 0, "discard count has to be at least one");
  const uint32_t sizeAlignment = getMinimalBufferAlignment();
  uint32_t blockSize = size;
  blockSize = (blockSize + (sizeAlignment - 1)) & ~(sizeAlignment - 1);

  SharedGuardedObjectAutoLock bufferDataLock(resources);
  return resources.alloc<Buffer>({blockSize, memory_class, discard_count, mem_flags}, true);
}

void Device::addBufferView(Buffer *buffer, FormatStore format)
{
  G_ASSERTF((buffer->getBlockSize() % format.getBytesPerPixelBlock()) == 0,
    "invalid view of buffer, buffer has a size of %u, format (%s) element size is %u"
    " which leaves %u dangling bytes",
    buffer->getBlockSize(), format.getNameString(), format.getBytesPerPixelBlock(),
    buffer->getBlockSize() % format.getBytesPerPixelBlock());

  uint32_t viewCount = buffer->getMaxDiscardLimit();
  eastl::unique_ptr<VulkanBufferViewHandle[]> views(new VulkanBufferViewHandle[viewCount]);

  VkBufferViewCreateInfo bvci;
  bvci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
  bvci.pNext = NULL;
  bvci.flags = 0;
  bvci.buffer = buffer->getHandle();
  bvci.format = format.asVkFormat();
  bvci.range = buffer->getBlockSize();

  {
    uint64_t sizeLimit = physicalDeviceInfo.properties.limits.maxStorageBufferRange;
    sizeLimit = min(sizeLimit, (uint64_t)physicalDeviceInfo.properties.limits.maxTexelBufferElements * format.getBytesPerPixelBlock());

    // should be assert, but some GPUs have broken limit values, working fine with bigger buffers
    if (sizeLimit < bvci.range)
    {
      debug("maxStorageBufferRange=%d, maxTexelBufferElements=%d, getBytesPerPixelBlock=%d",
        physicalDeviceInfo.properties.limits.maxStorageBufferRange, physicalDeviceInfo.properties.limits.maxTexelBufferElements,
        format.getBytesPerPixelBlock());
      logerr("vulkan: too big buffer view (%llu bytes max while asking for %u) for %llX:%s", sizeLimit, bvci.range,
        generalize(buffer->getHandle()), buffer->getDebugName());
      bvci.range = VK_WHOLE_SIZE;
    }
  }

  for (uint32_t d = 0; d < viewCount; ++d)
  {
    bvci.offset = buffer->bufOffsetAbs(d * buffer->getBlockSize());

    VULKAN_EXIT_ON_FAIL(device.vkCreateBufferView(device.get(), &bvci, NULL, ptr(views[d])));
  }
  buffer->setViews(std::move(views));
}

SamplerInfo *Device::getSampler(SamplerState state)
{
  for (int i = 0; i < samplers.size(); ++i)
    if (samplers[i]->state == state)
      return samplers[i];

  VkSamplerCreateInfo sci;
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.pNext = NULL;
  sci.flags = 0;
  sci.magFilter = sci.minFilter = state.getFilter();
  sci.addressModeU = state.getU();
  sci.addressModeV = state.getV();
  sci.addressModeW = state.getW();
  if (hasAnisotropicSampling())
  {
    sci.maxAnisotropy = clamp(state.getAniso(), 1.f, physicalDeviceInfo.properties.limits.maxSamplerAnisotropy);
  }
  else
  {
    sci.maxAnisotropy = 1.f;
  }
  sci.anisotropyEnable = sci.maxAnisotropy > 1.f;
  sci.mipLodBias = state.getBias();
  sci.compareOp = state.getCompareOp();
  sci.minLod = -1000.f;
  sci.maxLod = 1000.f;
  sci.borderColor = state.getBorder();
  sci.unnormalizedCoordinates = VK_FALSE;

  SamplerInfo *info = new SamplerInfo;
  info->state = state;

  sci.mipmapMode = state.getMip();
  sci.compareEnable = VK_FALSE;
  VULKAN_EXIT_ON_FAIL(device.vkCreateSampler(device.get(), &sci, NULL, ptr(info->sampler[0])));

  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sci.compareEnable = VK_TRUE;
  VULKAN_EXIT_ON_FAIL(device.vkCreateSampler(device.get(), &sci, NULL, ptr(info->sampler[1])));

  samplers.push_back(info);
  return info;
}

#if 0
// some debug info for images, keep it for now if ever is a need again
static const char *image_state_to_string(uint8_t index)
{
  switch (index)
  {
    case SN_INITIAL:
      return "initial";
    case SN_SAMPLED_FROM:
      return "sampled from";
    case SN_RENDER_TARGET:
      return "render target";
    case SN_CONST_RENDER_TARGET:
      return "constant render target";
    case SN_COPY_FROM:
      return "copy from";
    case SN_COPY_TO:
      return "copy to";
    case SN_SHADER_WRITE:
      return "shader write";
    default:
      return "error state!";
  }
}

static void report_image_porperties_with_info(Image *image)
{
  debug("Image object report for %p", image);
  debug("vulkan handle = %p", image->image.value);
  debug("memory object = (heap = %p, offset = %u, size = %u)", image->memory.heap.memory.value,
        image->memory.offset, image->memory.size);
  debug("format = %s", image->format.getNameString());
  for (auto &view : image->views)
  {
    debug("image view = (handle = %p, format = %s, mipBase = %u, mipRange = %u, arrayBase = %u, "
          "arrayRange = %u, isRenderTarget = %u, isCubemap = %u, isArray = %u",
          view.view.value, view.state.getFormat().getNameString(), view.state.getMipBase(),
          view.state.getMipCount(), view.state.getArrayBase(), view.state.getArrayCount(),
          (int)view.state.isRenderTarget, (int)view.state.isCubemap, (int)view.state.isArray);
  }
  switch (image->imageType)
  {
    case VK_IMAGE_TYPE_1D:
      debug("imageType = 1D");
      break;
    case VK_IMAGE_TYPE_2D:
      debug("imageType = 2D");
      break;
    case VK_IMAGE_TYPE_3D:
      debug("imageType = 3D");
      break;
  }
  debug("layerCount = %u", image->layerCount);
  debug("mipLevels = %u", image->mipLevels);
  debug("extent = (width = %u, height = %u, depth = %u)", image->extent.width, image->extent.height,
        image->extent.depth);
  debug("isDirty = %u", image->isDirty);
  for (uint32_t m = 0; m < image->mipLevels; ++m)
  {
    for (uint32_t a = 0; a < image->layerCount; ++a)
    {
      uint8_t frameStartState = image->getFrameStartState(m, a);
      uint8_t currentState = image->getState(m, a);
      debug("state[%u][%u] = (frameStart = %s, current = %s)", a, m,
            image_state_to_string(frameStartState), image_state_to_string(currentState));
    }
  }
#if VULKAN_RESOURCE_DEBUG_NAMES
  debug("debugName = %s", image->getDebugName());
#endif
}
#endif

void Device::setVkObjectDebugName(VulkanHandle handle, VkDebugReportObjectTypeEXT type1, VkObjectType type2, const char *name)
{
  G_UNUSED(handle);
  G_UNUSED(type1);
  G_UNUSED(type2);
  G_UNUSED(name);

#if VK_EXT_debug_utils
  auto &instance = device.getInstance();
  if (instance.hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.pNext = nullptr;
    info.objectType = type2;
    info.objectHandle = handle;
    info.pObjectName = name;
    VULKAN_LOG_CALL(instance.vkSetDebugUtilsObjectNameEXT(device.get(), &info));
    // if we have VK_EXT_debug_utils & VK_EXT_debug_marker are mutually exclusive via dependency
    return;
  }
#endif
#if VK_EXT_debug_marker
  if (device.hasExtension<DebugMarkerEXT>())
  {
    VkDebugMarkerObjectNameInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    info.pNext = NULL;
    info.objectType = type1;
    info.object = handle;
    info.pObjectName = name;
    VULKAN_LOG_CALL(device.vkDebugMarkerSetObjectNameEXT(device.get(), &info));
  }
#endif
}

void Device::setPipelineLayoutName(VulkanPipelineLayoutHandle layout, const char *name)
{
  setVkObjectDebugName(generalize(layout), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void Device::setDescriptorName(VulkanDescriptorSetHandle set_handle, const char *name)
{
  setVkObjectDebugName(generalize(set_handle), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void Device::setShaderModuleName(VulkanShaderModuleHandle shader, const char *name)
{
  setVkObjectDebugName(generalize(shader), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, VK_OBJECT_TYPE_SHADER_MODULE, name);
}

void Device::setPipelineName(VulkanPipelineHandle pipe, const char *name)
{
  setVkObjectDebugName(generalize(pipe), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, VK_OBJECT_TYPE_PIPELINE, name);
}

void Device::setTexName(Image *img, const char *name)
{
  img->setDebugName(name);
  setVkObjectDebugName(generalize(img->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, VK_OBJECT_TYPE_IMAGE, name);
}

void Device::setBufName(Buffer *buf, const char *name)
{
  buf->setDebugName(name);
  setVkObjectDebugName(generalize(buf->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, VK_OBJECT_TYPE_BUFFER, name);
}

void Device::setRenderPassName(RenderPassResource *rp, const char *name)
{
  rp->setDebugName(name);
  setVkObjectDebugName(generalize(rp->getHandle()), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, VK_OBJECT_TYPE_RENDER_PASS, name);
}


VulkanShaderModuleHandle Device::makeVkModule(const ShaderModuleBlob *module)
{
  VkShaderModuleCreateInfo smci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0};
  smci.codeSize = module->getBlobSize();
  smci.pCode = module->blob.data();

  VulkanShaderModuleHandle shader = VulkanShaderModuleHandle();
  VULKAN_EXIT_ON_FAIL(device.vkCreateShaderModule(device.get(), &smci, NULL, ptr(shader)));

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  setShaderModuleName(shader, module->name.c_str());
#endif
  return shader;
}

int Device::getDeviceVendor() const { return physicalDeviceInfo.vendorId; }

bool Device::hasGeometryShader() const { return VK_FALSE != physicalDeviceInfo.features.geometryShader; }

bool Device::hasTesselationShader() const
{
  const bool hasEnoughDescriptorSets =
    physicalDeviceInfo.properties.limits.maxBoundDescriptorSets > spirv::graphics::evaluation::REGISTERS_SET_INDEX;

  return physicalDeviceInfo.features.tessellationShader && hasEnoughDescriptorSets;
}

bool Device::hasFragmentShaderUAV() const { return VK_FALSE != physicalDeviceInfo.features.fragmentStoresAndAtomics; }

bool Device::hasDepthBoundsTest() const { return VK_FALSE != physicalDeviceInfo.features.depthBounds; }

bool Device::hasAnisotropicSampling() const { return VK_FALSE != physicalDeviceInfo.features.samplerAnisotropy; }

bool Device::hasDepthClamp() const { return VK_FALSE != physicalDeviceInfo.features.depthClamp; }

bool Device::hasImageCubeArray() const { return VK_FALSE != physicalDeviceInfo.features.imageCubeArray; }

bool Device::hasMultiDrawIndirect() const { return VK_FALSE != physicalDeviceInfo.features.multiDrawIndirect; }

bool Device::hasDrawIndirectFirstInstance() const { return VK_FALSE != physicalDeviceInfo.features.drawIndirectFirstInstance; }

bool Device::hasConditionalRender() const
{
#if VK_EXT_conditional_rendering
  return physicalDeviceInfo.hasConditionalRender && device.hasExtension<ConditionalRenderingEXT>();
#else
  return false;
#endif
}

bool Device::hasUAVOnlyForcedSampleCount() const
{
  // d3d:: interface does not query what samples are supported (precisely)
  // treat this feature is available if at least all variants up to 8 samples are supported
  constexpr VkSampleCountFlags minimalMask =
    VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT;
  return (physicalDeviceInfo.properties.limits.framebufferNoAttachmentsSampleCounts & minimalMask) == minimalMask;
}

bool Device::hasAttachmentNoStoreOp() const
{
  bool ret = false;
#if VK_EXT_load_store_op_none
  ret |= device.hasExtension<LoadStoreOpNoneEXT>();
#endif
#if VK_QCOM_render_pass_store_ops
  ret |= device.hasExtension<RenderPassStoreOpsQCOM>();
#endif
  return ret;
}

bool Device::hasDepthStencilResolve() const { return physicalDeviceInfo.hasDepthStencilResolve; }

bool Device::hasCreateRenderPass2() const
{
#if VK_KHR_create_renderpass2
  return device.hasExtension<CreateRenderPass2KHR>();
#endif
  return false;
}

bool Device::hasImagelessFramebuffer() const
{
#if VK_KHR_imageless_framebuffer && VK_KHR_maintenance2
  return physicalDeviceInfo.hasImagelessFramebuffer && device.hasExtension<ImagelessFramebufferKHR>() &&
         device.hasExtension<Maintenance2KHR>();
#else
  return false;
#endif
}

bool Device::hasWaveOperations(VkShaderStageFlags stages) const
{
  if ((physicalDeviceInfo.subgroupProperties.supportedStages & stages) != stages)
    return false;

  // chosen to match SM6.0 requirements
  static const VkSubgroupFeatureFlags neededSubgroupFeatures = VK_SUBGROUP_FEATURE_BASIC_BIT | // Wave Query
                                                               VK_SUBGROUP_FEATURE_VOTE_BIT | // Wave Vote + (partially) Wave Reduction
                                                               VK_SUBGROUP_FEATURE_BALLOT_BIT |     // Wave Broadcast
                                                               VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | // Wave Scan and Prefix +
                                                                                                    // (partially) Wave Reduction
                                                               VK_SUBGROUP_FEATURE_QUAD_BIT         // Quad-wide Shuffle operations
    ;

  return (physicalDeviceInfo.subgroupProperties.supportedOperations & neededSubgroupFeatures) == neededSubgroupFeatures;
}

uint32_t Device::getCurrentAvailableMemoryKb()
{
  // TODO: use cached values if reading takes too much time
  return physicalDeviceInfo.getCurrentAvailableMemoryKb(device.getInstance());
}

Buffer *Device::getArrayImageTo3DBuffer(size_t required_size)
{
  if (arrayImageTo3DBufferSize < required_size)
  {
    if (arrayImageTo3DBuffer)
      context.destroyBuffer(arrayImageTo3DBuffer);
    arrayImageTo3DBuffer = createBuffer(required_size, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
    setBufName(arrayImageTo3DBuffer, "3d rendering emul transfer buffer");
    arrayImageTo3DBufferSize = required_size;
  }
  return arrayImageTo3DBuffer;
}

Texture *Device::wrapVKImage(VkImage tex_res, ResourceBarrier current_state, int width, int height, int layers, int mips,
  const char *name, int flg)
{
  VkImageLayout currentVkLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (current_state == ResourceBarrier::RB_RW_RENDER_TARGET)
    currentVkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  if ((current_state & ResourceBarrier::RB_RO_SRV) != 0)
    currentVkLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  bool isDepthStencil = ((flg & TEXFMT_MASK) >= TEXFMT_FIRST_DEPTH) && ((flg & TEXFMT_MASK) <= TEXFMT_LAST_DEPTH);

  FormatStore dagorFormat = FormatStore::fromCreateFlags(flg);

  VkImageUsageFlags vkUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if ((flg & TEXCF_RTARGET) != 0)
  {
    if (isDepthStencil)
      vkUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else
      vkUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  if ((flg & TEXCF_UNORDERED) != 0)
    vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
  if ((dagorFormat.isSrgbCapableFormatType() && FormatStore::needMutableFormatForCreateFlags(flg)) || (flg & TEXCF_UNORDERED) != 0)
    vkUsage |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

  TextureInterfaceBase *tex = allocate_texture(layers > 1 ? RES3D_ARRTEX : RES3D_TEX, flg);
  tex->setParams(width, height, layers, mips, name);

  Image::Description::TrimmedCreateInfo ici;
  ici.flags = 0;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.extent = VkExtent3D{uint32_t(width), uint32_t(height), 1};
  ici.mipLevels = mips;
  ici.arrayLayers = layers;
  ici.usage = vkUsage;
  ici.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

  SharedGuardedObjectAutoLock lock(resources);
  tex->tex.image = resources.alloc<Image>({ici, Image::MEM_NOT_EVICTABLE, dagorFormat, currentVkLayout}, false);
  tex->tex.image->setDerivedHandle(VulkanHandle(tex_res));
  tex->tex.memSize = tex->ressize();

  return tex;
}

#if D3D_HAS_RAY_TRACING
RaytraceAccelerationStructure *Device::createRaytraceAccelerationStructure(RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  SharedGuardedObjectAutoLock lock(resources);
  return resources.alloc<RaytraceAccelerationStructure>({desc, count, flags});
#else
  G_UNUSED(desc);
  G_UNUSED(count);
  G_UNUSED(flags);
  return nullptr;
#endif
}

RaytraceAccelerationStructure *Device::createRaytraceAccelerationStructure(uint32_t elements, RaytraceBuildFlags flags)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  SharedGuardedObjectAutoLock lock(resources);
  return resources.alloc<RaytraceAccelerationStructure>({nullptr, elements, flags});
#else
  G_UNUSED(elements);
  G_UNUSED(flags);
  return nullptr;
#endif
}

void Device::ensureRoomForRaytraceBuildScratchBuffer(VkDeviceSize size)
{
  // TODO: put magic align number into named constant
  // Align to the next 1k
  size = (size + 1023) & ~1023;
  VkDeviceSize currentSize = 0;
  WinAutoLock lock(raytraceData.scratchReallocMutex);
  if (raytraceData.scratchBuffer)
  {
    currentSize = raytraceData.scratchBuffer->getBlockSize();
  }

  if (currentSize < size)
  {
    if (raytraceData.scratchBuffer)
      context.destroyBuffer(raytraceData.scratchBuffer);

    raytraceData.scratchBuffer = createBuffer(size, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
    setBufName(raytraceData.scratchBuffer, "RT scratch buffer");
  }
}
#endif

Device::Config drv3d_vulkan::get_device_config(const DataBlock *cfg)
{
  Device::Config result;

  result.texUploadLimitMb = cfg->getInt("texUploadLimitMb", 128);
  // don't use it for now, its about 10% slower than with heaps...
  result.features.set(DeviceFeaturesConfig::USE_DEDICATED_MEMORY_FOR_IMAGES, cfg->getBool("allowDedicatedImageMemory", false));
  result.features.set(DeviceFeaturesConfig::USE_DEDICATED_MEMORY_FOR_RENDER_TARGETS,
    cfg->getBool("alwaysUseDedicatedMemoryForRenderTargets", true));
  result.dedicatedMemoryForImageTexelCountThreshold = cfg->getInt("dedicatedImageMemoryTexelCountThreshold", 256 * 256);
  result.memoryStatisticsPeriod = cfg->getInt("memoryStatisticsPeriodSeconds", MEMORY_STATISTICS_PERIOD);
  result.features.set(DeviceFeaturesConfig::RESUSE_DESCRIPTOR_SETS, cfg->getBool("reuseDescriptorSets", false));
  result.features.set(DeviceFeaturesConfig::UPDATE_DESCRIPTOR_SETS, cfg->getBool("updateDescriptorSets", true));
  result.features.set(DeviceFeaturesConfig::RESET_DESCRIPTOR_SETS_ON_FRAME_START,
    cfg->getBool("resetDescriptorSetsOnFrameStart", false));
  result.features.set(DeviceFeaturesConfig::RESET_COMMAND_POOLS, cfg->getBool("resetCommandPools", true));
  // default is false, true has the potential to kill performance on some devices (intel)
  result.features.set(DeviceFeaturesConfig::RESET_COMMANDS_RELEASE_TO_SYSTEM, cfg->getBool("resetCommandsReleasesToSystem", false));
  result.features.set(DeviceFeaturesConfig::USE_ASYNC_BUFFER_COPY, cfg->getBool("useAsyncBufferCopy", true));
  result.features.set(DeviceFeaturesConfig::USE_ASNYC_IMAGE_COPY, cfg->getBool("useAsyncImageCopy", true));
  result.features.set(DeviceFeaturesConfig::FORCE_GPU_HIGH_POWER_STATE_ON_WAIT, cfg->getBool("forceGPUHighPowerStateOnWait", false));
#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
  result.features.set(DeviceFeaturesConfig::FLUSH_AFTER_EACH_DRAW_AND_DISPATCH,
    cfg->getBool("flushAndWaitAfterEachDrawAndDispatch", false));
#endif
  result.features.set(DeviceFeaturesConfig::OPTIMIZE_BUFFER_UPLOADS, cfg->getBool("optimizeBufferUploads", true));
  // mostly for debugging, as the debug layer complains about wrong image states if slices of 3d images are used as 2d or array image
  // this can be removed if the debug layers supports vk_khr_maintenance1 properly
  result.features.set(DeviceFeaturesConfig::DISABLE_RENDER_TO_3D_IMAGE, cfg->getBool("disableRenderTo3DImage", false));

  result.features.set(DeviceFeaturesConfig::COMMAND_MARKER, cfg->getBool("commandMarkers", false));

  // on some platforms (ex. Android) vulkan drivers has issue with corrupted coherent memory state
  result.features.set(DeviceFeaturesConfig::HAS_COHERENT_MEMORY, cfg->getBool("allowCoherentMemory", true));

  result.features.set(DeviceFeaturesConfig::HEADLESS, cfg->getBool("headless", false));
  debug("vulkan: running in headless mode is %s", result.features.test(DeviceFeaturesConfig::HEADLESS) ? "yes" : "no");

  result.features.set(DeviceFeaturesConfig::PRE_ROTATION, cfg->getBool("preRotation", false));
  debug("vulkan: pre-rotation in swapchain: %s", result.features.test(DeviceFeaturesConfig::PRE_ROTATION) ? "yes" : "no");

  result.features.set(DeviceFeaturesConfig::KEEP_LAST_RENDERED_IMAGE, cfg->getBool("keepLastRenderedImage", false));
  debug("vulkan: keep last rendered image in swapchain: %s",
    result.features.test(DeviceFeaturesConfig::KEEP_LAST_RENDERED_IMAGE) ? "yes" : "no");

  static const char deferred_exection_name[] = "deferred";
  static const char threaded_exection_name[] = "threaded";
  const char *config_exection = cfg->getStr("executionMode", threaded_exection_name);
  if (0 == strcmp(threaded_exection_name, config_exection))
  {
    result.features.set(DeviceFeaturesConfig::USE_DEFERRED_COMMAND_EXECUTION, false);
    result.features.set(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION, true);
  }
  else if (0 == strcmp(deferred_exection_name, config_exection))
  {
    result.features.set(DeviceFeaturesConfig::USE_DEFERRED_COMMAND_EXECUTION, true);
    result.features.set(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION, false);
  }
  else
  {
    String execModeList = String(0, "%s, %s", threaded_exection_name, deferred_exection_name);
    fatal("vulkan: invalid execution mode %s, use one of [%s]", config_exection, execModeList);
  }

  if (result.features.test(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION))
  {
    debug("vulkan: threaded execution mode selected");
  }
  else if (result.features.test(DeviceFeaturesConfig::USE_DEFERRED_COMMAND_EXECUTION))
  {
    debug("vulkan: deferred execution mode selected");
  }

  result.debugLevel = cfg->getInt("debugLevel", 0);
  debug("vulkan: debug level is %u", result.debugLevel);

  result.features.set(DeviceFeaturesConfig::ALLOW_DEBUG_MARKERS,
    cfg->getBool("allowDebugMarkers", result.debugLevel > 0 ? true : false));

  result.features.set(DeviceFeaturesConfig::RECORD_COMMAND_CALLER, cfg->getBool("recordCommandCaller", false));

  result.features.set(DeviceFeaturesConfig::ALLOW_BUFFER_DMA_LOCK_PATH, cfg->getBool("allowBufferDMALockPath", true));
  debug("vulkan: %s DMA lock path", result.features.test(DeviceFeaturesConfig::ALLOW_BUFFER_DMA_LOCK_PATH) ? "allow" : "disallow");

  return result;
}
