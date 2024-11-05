// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dummy_resources.h"
#include "driver.h"
#include "globals.h"
#include "resource_manager.h"
#include "sampler_cache.h"
#include "device_context.h"

using namespace drv3d_vulkan;

void adjustMaxPixelSize(uint32_t &pix_size, Image *img) { pix_size = max(pix_size, img->getFormat().getBytesPerPixelBlock()); }

void DummyResources::createImages()
{
  Image::Description::TrimmedCreateInfo ii;
  ii.imageType = VK_IMAGE_TYPE_2D;
  ii.extent.width = tex_dim_size;
  ii.extent.height = tex_dim_size;
  ii.extent.depth = tex_dim_size;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  ii.mipLevels = 1;
  ii.arrayLayers = tex_array_count;
  ii.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  ii.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

  img.srv2D = createImage(ii, false /*needCompare*/);
  img.srv2Dcompare = createImage(ii, true /*needCompare*/);

  ii.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  img.uav2D = createImage(ii, false /*needCompare*/);

  ii.imageType = VK_IMAGE_TYPE_3D;
  ii.arrayLayers = 1;
  ii.flags = 0;
  ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  img.srv3D = createImage(ii, false /*needCompare*/);
  img.srv3Dcompare = createImage(ii, true /*needCompare*/);

  ii.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  img.uav3D = createImage(ii, false /*needCompare*/);
}

void DummyResources::createBuffers()
{
  auto bufViewFmt = FormatStore::fromCreateFlags(buf_view_fmt_flag);
  buf.srv = Buffer::create(dummySize, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
  buf.srv->addBufferView(bufViewFmt);

  buf.uav = Buffer::create(dummySize, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
  buf.uav->addBufferView(bufViewFmt);
}

void DummyResources::calcDummySize()
{
  uint32_t maxFormatPixelSize = 0;
  adjustMaxPixelSize(maxFormatPixelSize, img.srv2D);
  adjustMaxPixelSize(maxFormatPixelSize, img.srv2Dcompare);
  adjustMaxPixelSize(maxFormatPixelSize, img.srv3D);
  adjustMaxPixelSize(maxFormatPixelSize, img.srv3Dcompare);
  adjustMaxPixelSize(maxFormatPixelSize, img.uav2D);
  adjustMaxPixelSize(maxFormatPixelSize, img.uav3D);

  auto bufViewFmt = FormatStore::fromCreateFlags(buf_view_fmt_flag);
  dummySize = max<VkDeviceSize>(buf_dummy_elements * bufViewFmt.getBytesPerPixelBlock(),
    maxFormatPixelSize * tex_dim_size * tex_dim_size * tex_dim_size * tex_array_count);
}

void DummyResources::setObjectNames()
{
  Globals::Dbg::naming.setTexName(img.srv2D, "dummy2D-cube");
  Globals::Dbg::naming.setTexName(img.srv2Dcompare, "dummy2D-cube-pcf");
  Globals::Dbg::naming.setTexName(img.uav2D, "dummy2DStorage-cube");
  Globals::Dbg::naming.setTexName(img.srv3D, "dummy3D");
  Globals::Dbg::naming.setTexName(img.srv3Dcompare, "dummy3D-pcf");
  Globals::Dbg::naming.setTexName(img.uav3D, "dummy3DStorage");
  Globals::Dbg::naming.setBufName(buf.srv, "dummySRV");
  Globals::Dbg::naming.setBufName(buf.uav, "dummyUAV");
}

void DummyResources::uploadZeroContent()
{
  auto stage = Buffer::create(dummySize, DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER, 1, BufferMemoryFlags::NONE);
  memset(stage->ptrOffsetLoc(0), 0, dummySize);

  // everything is 0 execept target image aspect, layer count and extent
  VkBufferImageCopy region = {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = tex_array_count;
  region.imageExtent = {tex_dim_size, tex_dim_size, tex_dim_size};

  DeviceContext &ctx = Globals::ctx;

  ctx.copyBufferToImage(stage, img.srv2D, 1, &region, true);
  region.imageSubresource.layerCount = 1;
  ctx.copyBufferToImage(stage, img.srv3D, 1, &region, true);

  region.imageSubresource.aspectMask = img.srv2Dcompare->getFormat().getAspektFlags();
  region.imageSubresource.layerCount = tex_array_count;
  ctx.copyBufferToImage(stage, img.srv2Dcompare, 1, &region, true);

  region.imageSubresource.aspectMask = img.srv3Dcompare->getFormat().getAspektFlags();
  region.imageSubresource.layerCount = 1;
  ctx.copyBufferToImage(stage, img.srv3Dcompare, 1, &region, true);

  ctx.uploadBuffer(BufferRef{stage}, BufferRef{buf.srv}, 0, 0, dummySize);
  ctx.uploadBuffer(BufferRef{stage}, BufferRef{buf.uav}, 0, 0, dummySize);

  ctx.destroyBuffer(stage);
}

void DummyResources::fillTableImg(uint32_t idx, Image *image, ImageViewState ivs, VkImageLayout layout,
  VulkanSamplerHandle sampler_handle)
{
  table[idx].resource = (void *)image;
  table[idx].descriptor.image = {sampler_handle, image->getImageView(ivs), layout};
}

void DummyResources::fillTableBuf(uint32_t idx, Buffer *buffer, VkDeviceSize sz)
{
  table[idx].resource = (void *)buffer;
  table[idx].descriptor.buffer = {buffer->getHandle(), buffer->bufOffsetLoc(0), sz};
}

void DummyResources::fillTable()
{
  // dummy is special, should be treated as null to avoid clearing up after fallback replacement
  for (auto &i : table)
    i.descriptor.type = VkAnyDescriptorInfo::TYPE_NULL;

  // images
  {
    ImageViewState ivs;
    ivs.setMipBase(0);
    ivs.setMipCount(1);
    ivs.setArrayBase(0);

    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = 0;

    // 2d
    ivs.setFormat(img.srv2D->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_2D_INDEX, img.srv2D, ivs, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      sampler->colorSampler());

    ivs.setFormat(img.srv2Dcompare->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_INDEX, img.srv2Dcompare, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->compareSampler());

    ivs.isUAV = 1;
    ivs.setFormat(img.uav2D->getFormat());
    fillTableImg(spirv::MISSING_STORAGE_IMAGE_2D_INDEX, img.uav2D, ivs, //
      VK_IMAGE_LAYOUT_GENERAL, VulkanSamplerHandle());

    // 2d array
    ivs.setArrayCount(tex_array_count);
    ivs.isArray = 1;
    ivs.isUAV = 0;
    ivs.setFormat(img.srv2D->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_2D_ARRAY_INDEX, img.srv2D, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->colorSampler());

    ivs.setFormat(img.srv2Dcompare->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_2D_ARRAY_INDEX, img.srv2Dcompare, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->compareSampler());

    ivs.isUAV = 1;
    ivs.setFormat(img.uav2D->getFormat());
    fillTableImg(spirv::MISSING_STORAGE_IMAGE_2D_ARRAY_INDEX, img.uav2D, ivs, //
      VK_IMAGE_LAYOUT_GENERAL, VulkanSamplerHandle());

    // cube
    ivs.setArrayCount(6);
    ivs.isArray = 0;
    ivs.isCubemap = 1;
    ivs.isUAV = 0;
    ivs.setFormat(img.srv2D->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_CUBE_INDEX, img.srv2D, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->colorSampler());

    ivs.setFormat(img.srv2Dcompare->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_INDEX, img.srv2Dcompare, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->compareSampler());

    ivs.isUAV = 1;
    ivs.setFormat(img.uav2D->getFormat());
    fillTableImg(spirv::MISSING_STORAGE_IMAGE_CUBE_INDEX, img.uav2D, ivs, //
      VK_IMAGE_LAYOUT_GENERAL, VulkanSamplerHandle());

    // cube array
    ivs.setArrayCount(tex_array_count);
    ivs.isArray = 1;
    ivs.isCubemap = 1;
    ivs.isUAV = 0;
    ivs.setFormat(img.srv2D->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_CUBE_ARRAY_INDEX, img.srv2D, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->colorSampler());

    ivs.setFormat(img.srv2Dcompare->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_CUBE_ARRAY_INDEX, img.srv2Dcompare, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->compareSampler());

    ivs.isUAV = 1;
    ivs.setFormat(img.uav2D->getFormat());
    fillTableImg(spirv::MISSING_STORAGE_IMAGE_CUBE_ARRAY_INDEX, img.uav2D, ivs, //
      VK_IMAGE_LAYOUT_GENERAL, VulkanSamplerHandle());

    // 3d
    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = 0;
    ivs.setFormat(img.srv3D->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_3D_INDEX, img.srv3D, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->colorSampler());

    ivs.setFormat(img.srv3Dcompare->getFormat());
    fillTableImg(spirv::MISSING_SAMPLED_IMAGE_WITH_COMPARE_3D_INDEX, img.srv3Dcompare, ivs, //
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler->compareSampler());

    ivs.isUAV = 1;
    ivs.setFormat(img.uav3D->getFormat());
    fillTableImg(spirv::MISSING_STORAGE_IMAGE_3D_INDEX, img.uav3D, ivs, //
      VK_IMAGE_LAYOUT_GENERAL, VulkanSamplerHandle());
  }

  // texel buffers
  {
    table[spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX].resource = (void *)buf.srv;
    table[spirv::MISSING_BUFFER_SAMPLED_IMAGE_INDEX].descriptor.texelBuffer = BufferRef(buf.srv).getView();

    table[spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX].resource = (void *)buf.srv;
    table[spirv::MISSING_STORAGE_BUFFER_SAMPLED_IMAGE_INDEX].descriptor.texelBuffer = BufferRef(buf.srv).getView();
  }

  // buffers
  {
    VkDeviceSize dummyUniformSize = min<uint32_t>(buf.srv->getBlockSize(), Globals::VK::phy.properties.limits.maxUniformBufferRange);

    fillTableBuf(spirv::MISSING_BUFFER_INDEX, buf.srv, dummyUniformSize);
    fillTableBuf(spirv::MISSING_STORAGE_BUFFER_INDEX, buf.uav, buf.uav->getBlockSize());
    fillTableBuf(spirv::MISSING_CONST_BUFFER_INDEX, buf.srv, dummyUniformSize);
  }
}

void DummyResources::init()
{
  sampler = Globals::samplers.get(SamplerState::make_default());
  createImages();
  calcDummySize();
  createBuffers();
  setObjectNames();
  uploadZeroContent();
  fillTable();
}

Image *DummyResources::createImage(const Image::Description::TrimmedCreateInfo &ii, bool needCompare)
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
    if (Image::checkImageCreate(ici, fmt))
      return Globals::Mem::res.alloc<Image>({ii, Image::MEM_NOT_EVICTABLE | Image::MEM_DEDICATE_ALLOC, fmt, VK_IMAGE_LAYOUT_UNDEFINED},
        true);
  }
  // fallback to non compare format if compare one is not available
  if (needCompare)
    return createImage(ii, false);
  DAG_FATAL("vulkan: can't create dummy texture");
  return nullptr;
}

void DummyResources::shutdown(DeviceContext &ctx)
{
  ctx.destroyBuffer(buf.srv);
  ctx.destroyBuffer(buf.uav);
  ctx.destroyImage(img.srv2D);
  ctx.destroyImage(img.srv2Dcompare);
  ctx.destroyImage(img.srv3D);
  ctx.destroyImage(img.srv3Dcompare);
  ctx.destroyImage(img.uav2D);
  ctx.destroyImage(img.uav3D);
}
