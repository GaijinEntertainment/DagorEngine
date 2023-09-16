#include "texture.h"
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_drv3dReset.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_atomic.h>
#include <image/dag_texPixel.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_asyncIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include "basetexture.h"
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <EASTL/memory.h>
#include <EASTL/tuple.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <generic/dag_tab.h>
#include <util/dag_watchdog.h>
#include <validateUpdateSubRegion.h>
#include "util/backtrace.h"

#if _TARGET_PC
#include <3d/dag_drv3d_pc.h>
#endif

#if 0
#define VERBOSE_DEBUG debug
#else
#define VERBOSE_DEBUG(...)
#endif

using namespace drv3d_vulkan;

namespace
{

uint32_t texfmt_to_d3dformat(/*D3DFORMAT*/ uint32_t fmt)
{
  switch (fmt)
  {
    case TEXFMT_DEFAULT: return drv3d_vulkan::D3DFMT_A8R8G8B8;
    case TEXFMT_R8G8B8A8: return drv3d_vulkan::D3DFMT_A8B8G8R8;
    case TEXFMT_A2B10G10R10: return drv3d_vulkan::D3DFMT_A2B10G10R10;
    case TEXFMT_A16B16G16R16: return drv3d_vulkan::D3DFMT_A16B16G16R16;
    case TEXFMT_A16B16G16R16F: return drv3d_vulkan::D3DFMT_A16B16G16R16F;
    case TEXFMT_A32B32G32R32F: return drv3d_vulkan::D3DFMT_A32B32G32R32F;
    case TEXFMT_G16R16: return drv3d_vulkan::D3DFMT_G16R16;
    case TEXFMT_V16U16: return drv3d_vulkan::D3DFMT_V16U16;
    case TEXFMT_L16: return drv3d_vulkan::D3DFMT_L16;
    case TEXFMT_A8: return drv3d_vulkan::D3DFMT_A8;
    case TEXFMT_R8: return drv3d_vulkan::D3DFMT_L8;
    case TEXFMT_A8L8: return drv3d_vulkan::D3DFMT_A8L8;
    case TEXFMT_G16R16F: return drv3d_vulkan::D3DFMT_G16R16F;
    case TEXFMT_G32R32F: return drv3d_vulkan::D3DFMT_G32R32F;
    case TEXFMT_R16F: return drv3d_vulkan::D3DFMT_R16F;
    case TEXFMT_R32F: return drv3d_vulkan::D3DFMT_R32F;
    case TEXFMT_DXT1: return drv3d_vulkan::D3DFMT_DXT1;
    case TEXFMT_DXT3: return drv3d_vulkan::D3DFMT_DXT3;
    case TEXFMT_DXT5: return drv3d_vulkan::D3DFMT_DXT5;
    case TEXFMT_A4R4G4B4: return drv3d_vulkan::D3DFMT_A4R4G4B4; // dxgi 1.2
    case TEXFMT_A1R5G5B5: return drv3d_vulkan::D3DFMT_A1R5G5B5;
    case TEXFMT_R5G6B5: return drv3d_vulkan::D3DFMT_R5G6B5;
    case TEXFMT_ATI1N: return _MAKE4C('ATI1');
    case TEXFMT_ATI2N: return _MAKE4C('ATI2');
  }
  G_ASSERTF(0, "can't convert tex format: %d", fmt);
  return drv3d_vulkan::D3DFMT_A8R8G8B8;
}

uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t mnsz)
{
  uint32_t lev = 1;
  while (w > mnsz && h > mnsz)
  {
    lev++;
    w >>= 1;
    h >>= 1;
  }
  return lev;
}

eastl::tuple<int32_t, int> fixup_tex_params(int w, int h, int32_t flg, int levels)
{
  const bool rt = 0 == (TEXCF_RTARGET & flg);
  auto fmt = TEXFMT_MASK & flg;

  if (rt)
  {
    if (0 != (TEXCF_SRGBWRITE & flg))
    {
      if ((TEXFMT_A8R8G8B8 != fmt))
      {
        // TODO verify this requirement
        if (0 != (TEXCF_SRGBREAD & flg))
        {
          debug("vulkan: Adding TEXCF_SRGBREAD to texture flags, because chosen format needs it");
          flg |= TEXCF_SRGBREAD;
        }
      }
    }
  }

  if (0 == levels)
  {
    levels = auto_mip_levels_count(w, h, rt ? 1 : 4);
    debug("vulkan: Auto compute for texture mip levels yielded %d", levels);
  }

  // update format info if it had changed
  return eastl::make_tuple((flg & ~TEXFMT_MASK) | fmt, levels);
}

bool create_tex2d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t levels, bool cube,
  BaseTex::ImageMem *initial_data, int array_size = 1, bool temp_alloc = false)
{
  uint32_t &flg = bt_in->cflg;
  G_ASSERT(!((flg & TEXCF_MULTISAMPLED) && initial_data != nullptr));
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & (TEXCF_DYNAMIC | TEXCF_RTARGET))));

  ImageCreateInfo desc;
  desc.type = VK_IMAGE_TYPE_2D;
  desc.size.width = w;
  desc.size.height = h;
  desc.size.depth = 1;
  desc.mips = levels;
  tex.realMipLevels = levels;
  desc.arrays = (cube ? 6 : 1) * array_size;
  desc.residencyFlags = Image::MEM_NOT_EVICTABLE;

  bool multisample = flg & (TEXCF_MULTISAMPLED | TEXCF_MSAATARGET);
  bool transient = flg & TEXCF_TRANSIENT;
  if ((flg & TEXFMT_MASK) == TEXFMT_MSAA_MAX_SAMPLES)
  {
    const PhysicalDeviceSet::MsaaMaxSamplesDesc &maxSamplesFormat = get_device().getDeviceProperties().maxSamplesFormat;
    if (maxSamplesFormat.samples <= 1)
      return false;
    flg = (flg & ~TEXFMT_MASK) | FormatStore::fromVkFormat(maxSamplesFormat.format).asTexFlags() |
          (TEXCF_MULTISAMPLED | TEXCF_MSAATARGET | TEXCF_RTARGET);
    desc.samples = maxSamplesFormat.samples;
    multisample = true;
  }
  else
    desc.samples = multisample ? get_device().calcMSAAQuality() : VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

  desc.format = FormatStore::fromCreateFlags(flg);
  desc.usage = 0;

  if (multisample || transient)
  {
    desc.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
  }
  else
  {
    desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (!(flg & TEXCF_SYSMEM))
      desc.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  const bool isDepth = desc.format.getAspektFlags() & VK_IMAGE_ASPECT_DEPTH_BIT;
  const bool isRT = flg & TEXCF_RTARGET;
  if (isRT)
  {
    desc.usage |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    desc.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  desc.usage |= (flg & TEXCF_UNORDERED) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

  G_ASSERT(!(isDepth && (flg & TEXCF_READABLE)));

  Device &device = get_device();
  if (flg & TEXCF_SYSMEM)
  {
    tex.useStaging(desc.format, desc.size.width, desc.size.height, desc.size.depth, desc.mips, desc.arrays);
    G_ASSERT(!isRT && !isDepth);
  }

  desc.flags = cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
  if ((desc.format.isSrgbCapableFormatType() && FormatStore::needMutableFormatForCreateFlags(flg)) || (flg & TEXCF_UNORDERED))
    desc.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

  if (!temp_alloc)
    TEXQL_PRE_CLEAN(bt_in->ressize());

  // allocate image without forced device residency if it is streamable & not GPU writable
  if (!(isRT | isDepth | (flg & TEXCF_UNORDERED)) && (bt_in->stubTexIdx >= 0))
    desc.residencyFlags = 0;

  tex.image = device.createImage(desc);
  if (!tex.image)
  {
    tex.destroyStaging();
    return false;
  }

  if ((flg & (TEXCF_READABLE | TEXCF_SYSMEM)) == TEXCF_READABLE)
    tex.useStaging(desc.format, desc.size.width, desc.size.height, desc.size.depth, desc.mips, desc.arrays);

  if (initial_data && tex.image->isResident())
  {
    bool tempStage = tex.stagingBuffer == nullptr;
    if (tempStage)
      tex.useStaging(desc.format, desc.size.width, desc.size.height, desc.size.depth, desc.mips, desc.arrays, true);

    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;

    uint32_t offset = 0;
    for (uint32_t i = 0; i < desc.arrays; ++i)
    {
      uint32_t flushStart = offset;
      for (uint32_t j = 0; j < desc.mips; ++j)
      {
        BaseTex::ImageMem &src = initial_data[desc.mips * i + j];
        VkBufferImageCopy &copy = copies[j];
        copy = make_copy_info(desc.format, j, i, 1, {desc.size.width, desc.size.height, 1}, tex.stagingBuffer->dataOffset(offset));

        memcpy(tex.stagingBuffer->dataPointer(offset), src.ptr, src.memSize);
        offset += src.memSize;
      }
      tex.stagingBuffer->markNonCoherentRange(flushStart, offset - flushStart, true);
      device.getContext().copyBufferToImage(tex.stagingBuffer, tex.image, desc.mips, copies.data(), true);
    }

    if (tempStage)
      tex.destroyStaging();
  }
  else if (isRT)
  {
    // init render target to a known state
    VkImageSubresourceRange area;
    area.aspectMask = desc.format.getAspektFlags();
    area.baseMipLevel = 0;
    area.levelCount = levels;
    area.baseArrayLayer = 0;
    area.layerCount = desc.arrays;
    // a transient image only can be "cleared" in a renderpass
    if ((desc.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0)
    {
      if (area.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
      {
        VkClearColorValue cv = {};
        device.getContext().clearColorImage(tex.image, area, cv);
      }
      else
      {
        VkClearDepthStencilValue cv = {};
        device.getContext().clearDepthStencilImage(tex.image, area, cv);
      }
    }
  }
  else if ((flg & TEXCF_CLEAR_ON_CREATE) && tex.image->isResident())
  {
    bool tempStage = tex.stagingBuffer == nullptr;
    if (tempStage)
      tex.useStaging(desc.format, desc.size.width, desc.size.height, desc.size.depth, 1, 1, true);

    uint32_t size = tex.stagingBuffer->dataSize();
    memset(tex.stagingBuffer->dataPointer(0), 0, size);
    tex.stagingBuffer->markNonCoherentRange(0, size, true);

    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;

    for (uint32_t i = 0; i < desc.arrays; ++i)
    {
      for (uint32_t j = 0; j < desc.mips; ++j)
        copies[j] = make_copy_info(desc.format, j, i, 1, {desc.size.width, desc.size.height, 1}, tex.stagingBuffer->dataOffset(0));
      device.getContext().copyBufferToImage(tex.stagingBuffer, tex.image, desc.mips, copies.data(), false);
    }

    if (tempStage)
      tex.destroyStaging();
  }

  tex.memSize = bt_in->ressize();
  bt_in->updateTexName();
  TEXQL_ON_ALLOC(bt_in);
  return true;
}
bool create_tex3d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t d, uint32_t flg, uint32_t levels,
  BaseTex::ImageMem *initial_data)
{
  G_ASSERT((flg & TEXCF_MULTISAMPLED) == 0);
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & TEXCF_DYNAMIC)));

  ImageCreateInfo desc;
  desc.type = VK_IMAGE_TYPE_3D;
  desc.size.width = w;
  desc.size.height = h;
  desc.size.depth = d;
  desc.mips = levels;
  tex.realMipLevels = levels;
  desc.arrays = 1;
  desc.format = FormatStore::fromCreateFlags(flg);
  desc.residencyFlags = Image::MEM_NOT_EVICTABLE;
  desc.samples = VK_SAMPLE_COUNT_1_BIT;

  desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if (!(flg & TEXCF_SYSMEM))
    desc.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

  const bool isRT = flg & TEXCF_RTARGET;
  if (flg & TEXCF_UNORDERED)
    desc.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

  desc.flags = 0;
  if ((desc.format.isSrgbCapableFormatType() && FormatStore::needMutableFormatForCreateFlags(flg)) || (flg & TEXCF_UNORDERED))
    desc.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

  TEXQL_PRE_CLEAN(bt_in->ressize());

  Device &device = get_device();
  const bool isDepth = (desc.format.getAspektFlags() & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
  if (isRT)
  {
    if (device.canRenderTo3D())
    {
      desc.usage |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
#if VK_KHR_maintenance1
      desc.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
#endif
    }
  }
  tex.image = device.createImage(desc);
  if (!tex.image)
    return false;
  // emulate render to 3d slices by creating a 2d array image
  // with a depth of the 3d image
  if (isRT && !device.canRenderTo3D())
  {
    desc.type = VK_IMAGE_TYPE_2D;
    desc.size.depth = 1;
    desc.arrays = d;
    desc.usage |= isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    desc.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    tex.renderTarget3DEmul = device.createImage(desc);
    if (!tex.renderTarget3DEmul)
    {
      debug("vulkan: can not create array texture to emulate render to vol texture, releasing vol "
            "texture");
      device.getContext().destroyImage(tex.image);
      tex.image = nullptr;
      return false;
    }
  }
  if (initial_data)
  {
    uint32_t size = desc.format.calculateImageSize(desc.size.width, desc.size.height, desc.size.depth, desc.mips);
    auto stage = device.createBuffer(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::TEMP);
    stage->setStagingDebugName(tex.image);

    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
    VkDeviceSize bufferOffset = 0;

    for (uint32_t j = 0; j < levels; ++j)
    {
      VkBufferImageCopy &copy = copies[j];
      copy =
        make_copy_info(desc.format, j, 0, 1, {desc.size.width, desc.size.height, desc.size.depth}, stage->dataOffset(bufferOffset));

      uint32_t imgSize = desc.format.calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
      memcpy(stage->dataPointer(bufferOffset), initial_data[j].ptr, imgSize);
      bufferOffset += imgSize;
    }
    stage->markNonCoherentRange(0, size, true);
    device.getContext().copyBufferToImage(stage, tex.image, levels, copies.data(), true);
    device.getContext().destroyBuffer(stage);
  }
  else if (isRT)
  {
    // init render target to a known state
    VkImageSubresourceRange area;
    area.aspectMask = desc.format.getAspektFlags();
    area.baseMipLevel = 0;
    area.levelCount = levels;
    area.baseArrayLayer = 0;
    area.layerCount = 1;
    if (area.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
    {
      VkClearColorValue cv = {};
      device.getContext().clearColorImage(tex.renderTarget3DEmul ? tex.renderTarget3DEmul : tex.image, area, cv);
    }
    else
    {
      VkClearDepthStencilValue cv = {};
      device.getContext().clearDepthStencilImage(tex.renderTarget3DEmul ? tex.renderTarget3DEmul : tex.image, area, cv);
    }
  }
  else if (flg & TEXCF_CLEAR_ON_CREATE)
  {
    uint32_t size = desc.format.calculateImageSize(desc.size.width, desc.size.height, desc.size.depth, 1);
    auto stage = device.createBuffer(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::TEMP);
    stage->setStagingDebugName(tex.image);
    memset(stage->dataPointer(0), 0, size);
    stage->markNonCoherentRange(0, size, true);

    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
    for (uint32_t j = 0; j < levels; ++j)
      copies[j] = make_copy_info(desc.format, j, 0, 1, {desc.size.width, desc.size.height, desc.size.depth}, stage->dataOffset(0));

    device.getContext().copyBufferToImage(stage, tex.image, levels, copies.data(), false);
    device.getContext().destroyBuffer(stage);
  }

  tex.memSize = bt_in->ressize();
  bt_in->updateTexName();
  TEXQL_ON_ALLOC(bt_in);
  return true;
}

} // namespace

namespace drv3d_vulkan
{

void BaseTex::blockingReadbackWait()
{
  TIME_PROFILE(vulkan_texture_blocking_readback);
  get_device().getContext().wait();
}

void BaseTex::setParams(int w, int h, int d, int levels, const char *stat_name)
{
  G_ASSERT(levels > 0);
  fmt = FormatStore::fromCreateFlags(cflg);
  mipLevels = levels;
  width = w;
  height = h;
  depth = d;
  maxMipLevel = 0;
  minMipLevel = levels - 1;
  setTexName(stat_name);

  if (!(get_device().getFormatFeatures(fmt.asVkFormat()) & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    debug("vulkan: texture %s default filter changed to nearest due to format %s not supporting linear", stat_name,
      fmt.getNameString());
    samplerState.setFilter(VK_FILTER_NEAREST);
  }

  setInitialImageViewState();
}

void BaseTex::copy(Image *dst)
{
  const uint32_t layerCount = getArrayCount();
  const uint32_t depthSlices = getDepthSlices();

  carray<VkImageCopy, MAX_MIPMAPS> regions;
  for (uint32_t i = 0; i < mipLevels; ++i)
  {
    VkImageCopy &region = regions[i];
    region.srcSubresource.aspectMask = getFormat().getAspektFlags();
    region.srcSubresource.mipLevel = i;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = layerCount;
    region.srcOffset.x = 0;
    region.srcOffset.y = 0;
    region.srcOffset.z = 0;
    region.dstSubresource.aspectMask = getFormat().getAspektFlags();
    region.dstSubresource.mipLevel = i;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = layerCount;
    region.dstOffset.x = 0;
    region.dstOffset.y = 0;
    region.dstOffset.z = 0;
    region.extent.width = max<uint32_t>(1, width >> i);
    region.extent.height = max<uint32_t>(1, height >> i);
    region.extent.depth = max<uint32_t>(1u, depthSlices >> i);
  }

  get_device().getContext().copyImage(tex.image, dst, regions.data(), mipLevels, 0, 0, mipLevels);

#if DAGOR_DBGLEVEL > 0
  wasUsed = 1;
#endif
}

void BaseTex::resolve(Image *dst) { get_device().getContext().resolveMultiSampleImage(tex.image, dst); }

int BaseTex::updateSubRegionInternal(BaseTex *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h,
  int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z)
{
  if (!validate_update_sub_region_params(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, this, dest_subres_idx, dest_x,
        dest_y, dest_z))
    return 0;

  VkImageCopy region;
  region.srcSubresource.aspectMask = getFormat().getAspektFlags();
  region.srcSubresource.mipLevel = src_subres_idx % src->mipLevels;
  region.srcSubresource.baseArrayLayer = src_subres_idx / src->mipLevels;
  region.srcSubresource.layerCount = 1;
  region.srcOffset.x = src_x;
  region.srcOffset.y = src_y;
  region.srcOffset.z = src_z;
  region.dstSubresource.aspectMask = getFormat().getAspektFlags();
  region.dstSubresource.mipLevel = dest_subres_idx % mipLevels;
  region.dstSubresource.baseArrayLayer = dest_subres_idx / mipLevels;
  region.dstSubresource.layerCount = 1;
  region.dstOffset.x = dest_x;
  region.dstOffset.y = dest_y;
  region.dstOffset.z = dest_z;
  region.extent.width = src_w;
  region.extent.height = src_h;
  region.extent.depth = src_d;

  // check that target compressed texture have enough space when copying from uncompressed
  // other APIs silently handle this and looks like GPU not even sample from that levels
  // where condition is always true (mip with size < block size)
  // so we can silently skip copy too
  if (!src->getFormat().isBlockFormat() && getFormat().isBlockFormat())
  {
    uint32_t blockX, blockY;
    getFormat().getBytesPerPixelBlock(&blockX, &blockY);

    if ((blockX * src_w) > (width >> (region.dstSubresource.mipLevel)) ||
        (blockY * src_h) > (height >> (region.dstSubresource.mipLevel)))
    {
      return 0;
    }
  }

  get_device().getContext().copyImage(src->tex.image, tex.image, &region, 1, src_subres_idx % src->mipLevels,
    dest_subres_idx % mipLevels, 1);

#if DAGOR_DBGLEVEL > 0
  src->wasUsed = 1;
#endif
  return 1;
}

bool BaseTex::recreate()
{
  if (RES3D_TEX == resType)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), width, height, "rt|dyn");
      return create_tex2d(tex, this, width, height, mipLevels, false, NULL, 1);
    }

    if (!preallocBeforeLoad)
    {
      VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), width, height, "empty");
      return create_tex2d(tex, this, width, height, mipLevels, false, NULL, 1);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      auto stubTex = getStubTex();
      if (stubTex)
        tex.image = stubTex->tex.image;
      return 1;
    }
    return create_tex2d(tex, this, 4, 4, 1, false, NULL, 1);
  }

  if (RES3D_CUBETEX == resType)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), width, height, "rt|dyn");
      return create_tex2d(tex, this, width, height, mipLevels, true, NULL, 1);
    }

    if (!preallocBeforeLoad)
    {
      VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), width, height, "empty");
      return create_tex2d(tex, this, width, height, mipLevels, true, NULL, 1);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      auto stubTex = getStubTex();
      if (stubTex)
        tex.image = stubTex->tex.image;
      return 1;
    }
    return create_tex2d(tex, this, 4, 4, 1, true, NULL, 1);
  }

  if (RES3D_VOLTEX == resType)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("<%s> recreate %dx%dx%d (%s)", getResName(), width, height, depth, "rt|dyn");
      return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, NULL);
    }

    if (!preallocBeforeLoad)
    {
      VERBOSE_DEBUG("<%s> recreate %dx%dx%d (%s)", getResName(), width, height, depth, "empty");
      return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, NULL);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      auto stubTex = getStubTex();
      if (stubTex)
        tex.image = stubTex->tex.image;
      return 1;
    }
    return create_tex3d(tex, this, 4, 4, 1, cflg, 1, NULL);
  }

  if (RES3D_ARRTEX == resType)
  {
    VERBOSE_DEBUG("<%s> recreate %dx%dx%d (%s)", getResName(), width, height, depth, "rt|dyn");
    return create_tex2d(tex, this, width, height, mipLevels, isArrayCube, NULL, depth);
  }

  return false;
}

void BaseTex::destroyObject()
{
  // we are no removing texture from states here, as only problem it can provoke
  // is getting current render target that was deleted in external thread while global driver lock is acquired
  // and this is code error, not a driver problem to handle
  // other cases with still in use RT that is deleted are handled by handleObjectRemoval sequence
  // remove_tex_from_states(this);
  releaseTex();
  free_texture(this);
}

int BaseTex::update(BaseTexture *src)
{
  if (isStub())
  {
    logerr("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (src)
    if (BaseTex *stex = getbasetex(src))
      if (stex->isStub())
      {
        logerr("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", src->getTexName(), stex->stubTexIdx);
        return 0;
      }

  if ((RES3D_TEX == resType) || (RES3D_CUBETEX == resType) || (RES3D_VOLTEX == resType))
  {
    BaseTex *btex = getbasetex(src);
    if (btex)
    {
      if (btex->cflg & TEXCF_MULTISAMPLED)
        btex->resolve(tex.image);
      else if (btex->tex.image && tex.image)
      {
#if DAGOR_DBGLEVEL > 0
        btex->wasUsed = 1;
#endif
        btex->copy(tex.image);
        return 1;
      }
      else
        return 0;
    }
  }
  return 0;
}

int BaseTex::updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
  int dest_subres_idx, int dest_x, int dest_y, int dest_z)
{
  if (isStub())
  {
    logerr("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (src)
    if (BaseTex *stex = getbasetex(src))
      if (stex->isStub())
      {
        logerr("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", src->getTexName(), stex->stubTexIdx);
        return 0;
      }

  if ((RES3D_TEX == resType) || (RES3D_CUBETEX == resType) || (RES3D_VOLTEX == resType) || (RES3D_ARRTEX == resType))
  {
    BaseTex *btex = getbasetex(src);
    if (btex)
      return BaseTex::updateSubRegionInternal(btex, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, dest_subres_idx, dest_x,
        dest_y, dest_z);
  }
  return 0;
}

void BaseTex::destroy()
{
  release();
#if DAGOR_DBGLEVEL > 1
  if (!wasUsed)
    logwarn("texture %p, of size %dx%dx%d total=%dbytes, name=%s was destroyed but was never used "
            "in rendering",
      this, width, height, depth, tex.memSize, getResName());
#elif DAGOR_DBGLEVEL > 0
  if (!wasUsed)
    debug("texture %p, of size %dx%dx%d total=%dbytes, name=%s was destroyed but was never used in "
          "rendering",
      this, width, height, depth, tex.memSize, getResName());
#endif
  destroyObject();
}

//-----------------------------------------------------------------------------

int BaseTex::generateMips()
{
  if (mipLevels > 1)
    get_device().getContext().generateMipmaps(tex.image);
  return 1;
}

static constexpr int TEX_COPIED = 1 << 30;

void BaseTex::D3DTextures::useStaging(FormatStore fmt, int32_t w, int32_t h, int32_t d, int32_t levels, uint16_t arrays,
  bool temporary /*=false*/)
{
  if (!stagingBuffer)
  {
    uint32_t size = fmt.calculateImageSize(w, h, d, levels) * arrays;
    stagingBuffer = get_device().createBuffer(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1,
      temporary ? BufferMemoryFlags::TEMP : BufferMemoryFlags::NONE);

    stagingBuffer->setStagingDebugName(image);
  }
}

void BaseTex::D3DTextures::destroyStaging()
{
  if (stagingBuffer)
  {
    get_device().getContext().destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }
}

void BaseTex::D3DTextures::releaseDelayed(AsyncCompletionState &event)
{
  Device &device = get_device();
  // races to staging buffer is a caller problem, so just delete it asap
  if (stagingBuffer)
  {
    device.getContext().destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }

  if (event.isPending())
  {
    device.getContext().wait();
    event.reset();
  }

  if (image)
    device.getContext().destroyImageDelayed(image);

  if (renderTarget3DEmul)
    device.getContext().destroyImageDelayed(renderTarget3DEmul);
}

void BaseTex::D3DTextures::release(AsyncCompletionState &event)
{
  Device &device = get_device();
  if (stagingBuffer)
  {
    device.getContext().destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }

  if (event.isPending())
  {
    device.getContext().wait();
    event.reset();
  }

  if (image)
  {
    device.getContext().destroyImage(image);
    image = nullptr;
  }

  if (renderTarget3DEmul)
  {
    device.getContext().destroyImage(renderTarget3DEmul);
    renderTarget3DEmul = nullptr;
  }
}

void BaseTex::releaseTex()
{
  if (isStub())
    tex.image = nullptr;
  else if (tex.image)
    TEXQL_ON_RELEASE(this);
  tex.release(waitEvent);
}

void BaseTex::swapInnerTex(D3DTextures &new_tex)
{
  D3DTextures texc = tex;
  if (isStub())
    texc.image = nullptr;
  else if (texc.image)
    // this will create some small overbudget, but we can't move BaseTex to backend
    TEXQL_ON_RELEASE(this);

  // sometimes engine can swap to image with uninitialized subresources
  // we need to avoid this in order to not add barrier when such texture is used right away
  if (tex.image)
    get_device().getContext().fillEmptySubresources(tex.image);
  // important: swap before release, sync on followup front lock atomic RW
  tex = new_tex;
  std::atomic_thread_fence(std::memory_order_release);
  texc.releaseDelayed(waitEvent);

  // old texture will be deleted, so we are swapping to nullptr
  new_tex.image = nullptr;
  new_tex.stagingBuffer = nullptr;
  new_tex.renderTarget3DEmul = nullptr;
}

BaseTexture *BaseTex::makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex)
{
  if (resType != RES3D_ARRTEX && resType != RES3D_VOLTEX && resType != RES3D_CUBEARRTEX)
    d = 1;
  BaseTex *clonedTex = allocate_texture(resType, cflg | (staging_tex ? TEXCF_SYSMEM : 0));
  if (!staging_tex)
    clonedTex->tidXored = tidXored, clonedTex->stubTexIdx = stubTexIdx;
  clonedTex->setParams(w, h, d, l, String::mk_str_cat(staging_tex ? "stg:" : "tmp:", getTexName()));
  clonedTex->preallocBeforeLoad = clonedTex->delayedCreate = true;
  if (!clonedTex->allocateTex())
    del_d3dres(clonedTex);
  return clonedTex;
}
void BaseTex::replaceTexResObject(BaseTexture *&other_tex)
{
  {
    WinAutoLock lock(get_device().getContext().getStubSwapGuard());
    BaseTex *other = getbasetex(other_tex);
    G_ASSERT_RETURN(other, );

    // safely swap texture objects
    swapInnerTex(other->tex);
    // swap dimensions
    eastl::swap(width, other->width);
    eastl::swap(height, other->height);
    eastl::swap(depth, other->depth);
    eastl::swap(mipLevels, other->mipLevels);
    eastl::swap(minMipLevel, other->minMipLevel);
    eastl::swap(maxMipLevel, other->maxMipLevel);

    other->wasUsed = true;
  }
  del_d3dres(other_tex);
}

bool BaseTex::allocateTex()
{
  if (tex.image)
    return true;
  switch (resType)
  {
    case RES3D_VOLTEX: return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, nullptr);
    case RES3D_TEX:
    case RES3D_CUBETEX: return create_tex2d(tex, this, width, height, mipLevels, resType == RES3D_CUBETEX, nullptr, 1, false);
    case RES3D_CUBEARRTEX:
    case RES3D_ARRTEX: return create_tex2d(tex, this, width, height, mipLevels, resType == RES3D_CUBEARRTEX, nullptr, depth, false);
  }
  return false;
}

int BaseTex::lockimg(void **p, int &stride, int lev, unsigned flags)
{
  if (RES3D_TEX != resType)
    return 0;

  bool stagingIsTemporary = (flags & TEXLOCK_DELSYSMEMCOPY) && !(cflg & TEXCF_DYNAMIC);

  Device &device = get_device();
  if (!tex.image && (flags & TEXLOCK_WRITE) && !(flags & TEXLOCK_READ))
    if (!create_tex2d(tex, this, width, height, mipLevels, false, nullptr))
    {
      logerr("failed to auto-create tex.tex2D on lockImg");
      return 0;
    }
  stride = 0;

  uint32_t prevFlags = lockFlags;
  if (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED))
  {
#if DAGOR_DBGLEVEL > 0
    wasUsed = 1;
#endif
    bool resourceCopied = (lockFlags == TEX_COPIED);
    lockFlags = 0;
    if ((getFormat().getAspektFlags() & VK_IMAGE_ASPECT_DEPTH_BIT) && !(flags & TEXLOCK_COPY_STAGING))
    {
      logerr("can't lock depth format");
      return 0;
    }
    tex.useStaging(getFormat(), width, height, getDepthSlices(), mipLevels, getArrayCount(), stagingIsTemporary);
    // make pvs happy ... tex.stagingBuffer can not be null, createBuffer crashes everything
    // if it should fail
    if (tex.stagingBuffer)
    {
      carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
      VkDeviceSize bufferOffset = 0;

      uint32_t layers = resType == RES3D_CUBETEX ? 6 : 1;
      for (uint32_t j = 0; j < mipLevels; ++j)
      {
        VkBufferImageCopy &copy = copies[j];
        copy = make_copy_info(getFormat(), j, 0, layers, {width, height, 1}, tex.stagingBuffer->dataOffset(bufferOffset));

        bufferOffset +=
          getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1) * layers;
      }
      if (flags & TEXLOCK_COPY_STAGING)
      {
        device.getContext().copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), nullptr);
        blockingReadbackWait();
        lockFlags = TEXLOCK_COPY_STAGING;
        return 1;
      }
      if (!p)
      {
        // on some cases the requested copy is never used
        if (waitEvent.isPending())
          device.getContext().wait();
        device.getContext().copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), &waitEvent);
      }
      else if (!resourceCopied)
      {
        device.getContext().copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), nullptr);
        blockingReadbackWait();
      }
    }
    if (!p)
    {
      lockFlags = TEX_COPIED;
      stride = 0;
      return 1;
    }
  }
  else
  {
    lockFlags = flags;
    if (p == nullptr)
    {
      if (flags & TEXLOCK_RWMASK)
      {
        logerr("nullptr in lockimg");
        return 0;
      }
    }
    else
      *p = 0;
  }

  if (flags & TEXLOCK_RWMASK)
  {
    uint32_t offset = 0;
    if (flags & TEXLOCK_DISCARD)
    {
      G_ASSERT(!tex.stagingBuffer);

      uint32_t levelWidth = max(width >> lev, 1);
      uint32_t levelHeight = max(height >> lev, 1);
      uint32_t levelDepth = max<uint32_t>(getDepthSlices() >> lev, 1);

      tex.useStaging(getFormat(), levelWidth, levelHeight, levelDepth, 1, getArrayCount(), true);
    }
    else
    {
      offset = getFormat().calculateImageSize(width, height, getDepthSlices(), lev);
      tex.useStaging(getFormat(), width, height, getDepthSlices(), mipLevels, getArrayCount(), stagingIsTemporary);
    }

    lockMsr.ptr = tex.stagingBuffer->dataPointer(offset);
    lockMsr.rowPitch = getFormat().calculateRowPitch(max(width >> lev, 1));
    lockMsr.slicePitch = getFormat().calculateSlicePich(max(width >> lev, 1), max(height >> lev, 1));

    // fast check is ok here
    if (waitEvent.isPendingOrCompletedFast())
    {
      if (flags & TEXLOCK_NOSYSLOCK)
      {
        if (waitEvent.isPending())
        {
          // restore TEX_COPIED flag to prevent
          // breaking previous copy if lock is non fenced
          //(i.e. readback reading lock called multiple times without external sync check)
          // basically same as DXGI_ERROR_WAS_STILL_DRAWING path
          if (prevFlags == TEX_COPIED)
            lockFlags = TEX_COPIED;
          return 0;
        }
      }
      else
      {
        if (waitEvent.isPending())
          device.getContext().wait();
      }

      waitEvent.reset();
    }

    tex.stagingBuffer->markNonCoherentRange(offset, lockMsr.slicePitch * getArrayCount(), false);
    *p = lockMsr.ptr;

    stride = getFormat().calculateRowPitch(width >> lev);
    lockFlags = flags;
    lockedLevel = lev;
    return 1;
  }
  lockFlags = flags;
  return 1;
}

int BaseTex::unlockimg()
{
  auto &device = get_device();
  auto &context = device.getContext();
  if (RES3D_TEX == resType)
  {
    if (lockFlags == TEXLOCK_COPY_STAGING)
    {
      carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
      VkDeviceSize bufferOffset = 0;

      if (!tex.stagingBuffer)
        return 0;
      for (uint32_t j = 0; j < mipLevels; ++j)
      {
        VkBufferImageCopy &copy = copies[j];
        copy = make_copy_info(getFormat(), j, 0, 1, {width, height, depth}, tex.stagingBuffer->dataOffset(bufferOffset));

        bufferOffset += getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
      }
      tex.stagingBuffer->markNonCoherentRange(0, bufferOffset, true);
      get_device().getContext().copyBufferToImage(tex.stagingBuffer, tex.image, mipLevels, copies.data(), false);
      return 1;
    }

    if ((cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)) && (lockFlags == TEX_COPIED))
      return 1;

    ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
    if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy) && lockMsr.ptr && !hdr.compressionType())
    {
      int rpitch = hdr.getSurfacePitch(lockedLevel);
      int h = hdr.getSurfaceScanlines(lockedLevel);
      uint8_t *src = (uint8_t *)lockMsr.ptr;
      uint8_t *dest = texCopy.data() + sizeof(ddsx::Header);

      for (int i = 0; i < lockedLevel; i++)
        dest += hdr.getSurfacePitch(i) * hdr.getSurfaceScanlines(i);
      G_ASSERT(dest < texCopy.data() + sizeof(ddsx::Header) + hdr.memSz);

      G_ASSERTF(rpitch <= lockMsr.rowPitch, "%dx%d: tex.pitch=%d copy.pitch=%d, lev=%d", width, height, lockMsr.rowPitch, rpitch,
        lockedLevel);
      for (int y = 0; y < h; y++, dest += rpitch)
        memcpy(dest, src + y * lockMsr.rowPitch, rpitch);
      VERBOSE_DEBUG("%s %dx%d updated DDSx for TEXCF_SYSTEXCOPY", getResName(), hdr.w, hdr.h, data_size(texCopy));
    }

    if (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED))
      ; // no-op
    else if (tex.stagingBuffer && tex.image)
    {
      if (lockFlags & TEXLOCK_DISCARD)
      {
        VkBufferImageCopy copy =
          make_copy_info(getFormat(), lockedLevel, 0, 1, {width, height, depth}, tex.stagingBuffer->dataOffset(0));

        uint32_t dirtySize =
          getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
        tex.stagingBuffer->markNonCoherentRange(0, dirtySize, true);
        get_device().getContext().copyBufferToImageOrdered(tex.stagingBuffer, tex.image, 1, &copy);
        tex.destroyStaging();
      }
      else if ((lockFlags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) != 0 && !(lockFlags & TEXLOCK_DONOTUPDATEON9EXBYDEFAULT))
      {
        carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
        VkDeviceSize bufferOffset = 0;

        for (uint32_t j = 0; j < mipLevels; ++j)
        {
          VkBufferImageCopy &copy = copies[j];
          copy = make_copy_info(getFormat(), j, 0, 1, {width, height, depth}, tex.stagingBuffer->dataOffset(bufferOffset));

          bufferOffset += getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
        }
        tex.stagingBuffer->markNonCoherentRange(0, bufferOffset, true);
        get_device().getContext().copyBufferToImage(tex.stagingBuffer, tex.image, mipLevels, copies.data(), false);
      }
    }

    if ((lockFlags & TEXLOCK_DELSYSMEMCOPY) && !(cflg & TEXCF_DYNAMIC))
      tex.destroyStaging();

    lockFlags = 0;
    return 1;
  }
  else if (RES3D_CUBETEX == resType)
  {

    if (tex.stagingBuffer)
    {
      if (lockFlags & TEXLOCK_WRITE)
      {
        VkBufferImageCopy copy =
          make_copy_info(getFormat(), lockedLevel, lockedLayer, 1, {width, height, 1}, tex.stagingBuffer->dataOffset(0));

        tex.stagingBuffer->markNonCoherentRange(0, tex.stagingBuffer->dataSize(), true);
        context.copyBufferToImage(tex.stagingBuffer, tex.image, 1, &copy, false);
      }
      tex.destroyStaging();
    }

    lockFlags = 0;
    return 1;
  }
  return 0;
}

int BaseTex::lockimg(void **p, int &stride, int face, int lev, unsigned flags)
{
  if (RES3D_CUBETEX != resType)
    return 0;

  G_ASSERTF(!(cflg & TEXCF_SYSTEXCOPY), "cube texture with system copy not implemented yet");
  G_ASSERTF(!(flags & TEXLOCK_DISCARD), "Discard for cube texture is not implemented yet");
  G_ASSERTF(!((flags & TEXCF_RTARGET) && (flags & TEXLOCK_WRITE)), "you can not write to a render "
                                                                   "target");

  if ((flags & TEXLOCK_RWMASK))
  {
    auto &device = get_device();
    auto &context = device.getContext();

    lockFlags = flags;
    lockedLevel = lev;
    lockedLayer = face;

    auto format = getFormat();
    uint32_t levelWidth = max(width >> lev, 1);
    uint32_t levelHeight = max(height >> lev, 1);

    if (!tex.stagingBuffer)
    {
      // only get a buffer large enough to hold the locked level
      tex.useStaging(format, levelWidth, levelHeight, 1, 1, 1, true);
      if (flags & TEXLOCK_READ)
      {
        VkBufferImageCopy copy = make_copy_info(format, lev, face, 1, {width, height, 1}, tex.stagingBuffer->dataOffset(0));

        if (p)
        {
          context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, nullptr);
          blockingReadbackWait();
        }
        else
        {
          // sometimes async read back are issued without waiting for it and then re-issuing again
          if (waitEvent.isPending())
            context.wait();
          context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, &waitEvent);
        }
      }

      if (p)
      {
        if (waitEvent.isPending())
          context.wait();

        waitEvent.reset();
        tex.stagingBuffer->markNonCoherentRange(0, tex.stagingBuffer->dataSize(), false);
        *p = tex.stagingBuffer->dataPointer(0);
        stride = format.calculateRowPitch(levelWidth);
      }
    }
    return 1;
  }
  return 0;
}

int BaseTex::lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags)
{
  if (RES3D_VOLTEX != resType)
  {
    logwarn("vulkan: called lockbox on a non volume texture");
    return 0;
  }

  G_ASSERTF(!(flags & TEXLOCK_DISCARD), "Discard for volume texture is not implemented yet");
  G_ASSERTF(data != nullptr, "for lockbox you need to provide a output pointer");
  G_ASSERTF(!((flags & TEXCF_RTARGET) && (flags & TEXLOCK_WRITE)), "you can not write to a render "
                                                                   "target");

  if ((flags & TEXLOCK_RWMASK) && data)
  {
    auto &device = get_device();
    auto &context = device.getContext();

    auto format = getFormat();
    uint32_t levelWidth = max(width >> level, 1);
    uint32_t levelHeight = max(height >> level, 1);
    uint32_t levelDepth = max(depth >> level, 1);
    lockFlags = flags;
    lockedLevel = level;

    // only get a buffer large enough to hold the locked level
    tex.useStaging(format, levelWidth, levelHeight, levelDepth, 1, 1, true);
    *data = tex.stagingBuffer->dataPointer(0);

    if (flags & TEXLOCK_READ)
    {
      VkBufferImageCopy copy = make_copy_info(format, level, 0, 1, {width, height, depth}, tex.stagingBuffer->dataOffset(0));

      context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, nullptr);
      blockingReadbackWait();

      tex.stagingBuffer->markNonCoherentRange(0, tex.stagingBuffer->dataSize(), false);
    }
    row_pitch = format.calculateRowPitch(levelWidth);
    slice_pitch = format.calculateSlicePich(levelWidth, levelHeight);

    if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy))
    {
      lockMsr.ptr = *data;
      lockMsr.rowPitch = row_pitch;
      lockMsr.slicePitch = slice_pitch;
      lockedLevel = level;
    }
    return 1;
  }
  else
  {
    logwarn("vulkan: lockbox called with no effective action (either no read/write flag or null "
            "pointer passed)");
    return 0;
  }
}

int BaseTex::unlockbox()
{
  if (RES3D_VOLTEX != resType)
    return 0;

  G_ASSERTF(lockFlags != 0, "Unlock without any lock before?");

  auto &device = get_device();
  auto &context = device.getContext();

  if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy) && lockMsr.ptr)
  {
    ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
    G_ASSERT(!hdr.compressionType());

    int rpitch = hdr.getSurfacePitch(lockedLevel);
    int h = hdr.getSurfaceScanlines(lockedLevel);
    int d = max(depth >> lockedLevel, 1);
    uint8_t *src = (uint8_t *)lockMsr.ptr;
    uint8_t *dest = texCopy.data() + sizeof(ddsx::Header);

    for (int i = 0; i < lockedLevel; i++)
      dest += hdr.getSurfacePitch(i) * hdr.getSurfaceScanlines(i) * max(depth >> i, 1);
    G_ASSERT(dest < texCopy.data() + sizeof(ddsx::Header) + hdr.memSz);

    G_ASSERTF(rpitch <= lockMsr.rowPitch && rpitch * h <= lockMsr.slicePitch, "%dx%dx%d: tex.pitch=%d,%d copy.pitch=%d,%d, lev=%d",
      width, height, depth, lockMsr.rowPitch, lockMsr.slicePitch, rpitch, rpitch * h, lockedLevel);
    for (int di = 0; di < d; di++, src += lockMsr.slicePitch)
      for (int y = 0; y < h; y++, dest += rpitch)
        memcpy(dest, src + y * lockMsr.rowPitch, rpitch);
    VERBOSE_DEBUG("%s %dx%dx%d updated DDSx for TEXCF_SYSTEXCOPY", getResName(), hdr.w, hdr.h, hdr.depth, data_size(texCopy));
  }
  memset(&lockMsr, 0, sizeof(lockMsr));

  if (tex.stagingBuffer)
  {
    if (lockFlags & TEXLOCK_WRITE)
    {
      VkBufferImageCopy copy =
        make_copy_info(getFormat(), lockedLevel, 0, 1, {width, height, depth}, tex.stagingBuffer->dataOffset(0));

      tex.stagingBuffer->markNonCoherentRange(0, tex.stagingBuffer->dataSize(), true);
      context.copyBufferToImage(tex.stagingBuffer, tex.image, 1, &copy, false);
    }

    tex.destroyStaging();
  }

  lockFlags = 0;
  return 1;
}

} // namespace drv3d_vulkan

Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    logerr("create_tex: can not create dynamic render target");
    return nullptr;
  }
  if (img)
  {
    w = img->w;
    h = img->h;
  }

  const Driver3dDesc &dd = d3d::get_driver_desc();
  w = clamp<int>(w, dd.mintexw, dd.maxtexw);
  h = clamp<int>(h, dd.mintexh, dd.maxtexh);

  eastl::tie(flg, levels) = fixup_tex_params(w, h, flg, levels);

  if (img)
  {
    levels = 1;

    if (0 == w && 0 == h)
    {
      w = img->w;
      h = img->h;
    }

    if ((w != img->w) || (h != img->h))
    {
      logerr("create_tex: image size differs from texture size (%dx%d != %dx%d)", img->w, img->h, w, h);
      img = nullptr; // abort copying
    }

    if (FormatStore::fromCreateFlags(flg).getBytesPerPixelBlock() != 4)
      img = nullptr;
  }

  // TODO: check for preallocated RT (with requested, not adjusted tex dimensions)

  auto tex = allocate_texture(RES3D_TEX, flg);

  tex->setParams(w, h, 1, levels, stat_name);

  if (tex->cflg & TEXCF_SYSTEXCOPY)
  {
    if (img)
    {
      int memSz = w * h * 4;
      clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + memSz);

      ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
      memset(&hdr, 0, sizeof(hdr));
      hdr.label = _MAKE4C('DDSx');
      hdr.d3dFormat = drv3d_vulkan::D3DFMT_A8R8G8B8;
      hdr.flags |= ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0) ? hdr.FLG_GAMMA_EQ_1 : 0;
      hdr.w = w;
      hdr.h = h;
      hdr.levels = 1;
      hdr.bitsPerPixel = 32;
      hdr.memSz = memSz;
      /*sysCopyQualityId*/ hdr.hqPartLevels = 0;

      memcpy(tex->texCopy.data() + sizeof(hdr), img + 1, memSz);
      VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
    }
    else if (tex->cflg & TEXCF_LOADONCE)
    {
      int memSz = tex->ressize();
      clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + memSz);
      mem_set_0(tex->texCopy);

      ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
      hdr.label = _MAKE4C('DDSx');
      hdr.d3dFormat = texfmt_to_d3dformat(tex->cflg & TEXFMT_MASK);
      hdr.flags |= ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0) ? hdr.FLG_GAMMA_EQ_1 : 0;
      hdr.w = w;
      hdr.h = h;
      hdr.levels = levels;
      hdr.bitsPerPixel = tex->getFormat().getBytesPerPixelBlock();
      if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT1)
        hdr.dxtShift = 3;
      else if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT3 || hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT5)
        hdr.dxtShift = 4;
      if (hdr.dxtShift)
        hdr.bitsPerPixel = 0;
      hdr.memSz = memSz;
      /*sysCopyQualityId*/ hdr.hqPartLevels = 0;

      VERBOSE_DEBUG("%s %dx%d reserved DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
    }
    else
      tex->cflg &= ~TEXCF_SYSTEXCOPY;
  }

  // only 1 mip
  BaseTex::ImageMem idata;
  if (img)
  {
    idata.ptr = img + 1;
    idata.rowPitch = w * 4;
    // suddenly PSV complains about it
    // idata.slicePitch = 0;
    idata.memSize = w * h * 4;
  }

  if (!create_tex2d(tex->tex, tex, w, h, levels, false, img ? &idata : nullptr))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

CubeTexture *d3d::create_cubetex(int size, int flg, int levels, const char *stat_name)
{
  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    logerr("create_cubtex: can not create dynamic render target");
    return nullptr;
  }

  const Driver3dDesc &dd = d3d::get_driver_desc();
  size = get_bigger_pow2(clamp<int>(size, dd.mincubesize, dd.maxcubesize));

  eastl::tie(flg, levels) = fixup_tex_params(size, size, flg, levels);

  auto tex = allocate_texture(RES3D_CUBETEX, flg);
  tex->setParams(size, size, 1, levels, stat_name);

  if (!create_tex2d(tex->tex, tex, size, size, levels, true, nullptr))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

VolTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    logerr("create_voltex: can not create dynamic render target");
    return nullptr;
  }

  eastl::tie(flg, levels) = fixup_tex_params(w, h, flg, levels);

  auto tex = allocate_texture(RES3D_VOLTEX, flg);
  tex->setParams(w, h, d, levels, stat_name);

  if (!create_tex3d(tex->tex, tex, w, h, d, flg, levels, nullptr))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  if (tex->cflg & TEXCF_SYSTEXCOPY)
  {
    clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + tex->tex.memSize);
    mem_set_0(tex->texCopy);

    ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
    hdr.label = _MAKE4C('DDSx');
    hdr.d3dFormat = texfmt_to_d3dformat(tex->cflg & TEXFMT_MASK);
    hdr.w = w;
    hdr.h = h;
    hdr.depth = d;
    hdr.levels = levels;
    hdr.bitsPerPixel = tex->getFormat().getBytesPerPixelBlock();
    if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT1)
      hdr.dxtShift = 3;
    else if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT3 || hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT5)
      hdr.dxtShift = 4;
    if (hdr.dxtShift)
      hdr.bitsPerPixel = 0;
    hdr.flags = hdr.FLG_VOLTEX;
    if ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0)
      hdr.flags |= hdr.FLG_GAMMA_EQ_1;
    hdr.memSz = tex->tex.memSize;
    /*sysCopyQualityId*/ hdr.hqPartLevels = 0;
    VERBOSE_DEBUG("%s %dx%d reserved DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
  }

  return tex;
}

ArrayTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  eastl::tie(flg, levels) = fixup_tex_params(w, h, flg, levels);

  auto tex = allocate_texture(RES3D_ARRTEX, flg);
  tex->setParams(w, h, d, levels, stat_name);

  if (!create_tex2d(tex->tex, tex, w, h, levels, false, nullptr, d))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

ArrayTexture *d3d::create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  eastl::tie(flg, levels) = fixup_tex_params(side, side, flg, levels);

  auto tex = allocate_texture(RES3D_ARRTEX, flg);
  tex->setParams(side, side, d, levels, stat_name);
  tex->isArrayCube = true;

  if (!create_tex2d(tex->tex, tex, side, side, levels, true, nullptr, d))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

// load compressed texture
BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *stat_name)
{
  ddsx::Header hdr;
  if (!crd.readExact(&hdr, sizeof(hdr)) || !hdr.checkLabel())
  {
    debug("invalid DDSx format");
    return nullptr;
  }

  BaseTexture *tex = alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name);
  if (tex)
  {
    BaseTex *bt = (BaseTex *)tex;
    int st_pos = crd.tell();
    G_ASSERTF_AND_DO(hdr.hqPartLevels == 0, bt->cflg &= ~TEXCF_SYSTEXCOPY,
      "cannot use TEXCF_SYSTEXCOPY with base part of split texture!");
    if (bt->cflg & TEXCF_SYSTEXCOPY)
    {
      int data_sz = hdr.packedSz ? hdr.packedSz : hdr.memSz;
      clear_and_resize(bt->texCopy, sizeof(hdr) + data_sz);
      memcpy(bt->texCopy.data(), &hdr, sizeof(hdr));
      /*sysCopyQualityId*/ ((ddsx::Header *)bt->texCopy.data())->hqPartLevels = quality_id;
      if (!crd.readExact(bt->texCopy.data() + sizeof(hdr), data_sz))
      {
        logerr_ctx("inconsistent input tex data, data_sz=%d tex=%s", data_sz, stat_name);
        del_d3dres(tex);
        return NULL;
      }
      VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(bt->texCopy));
      InPlaceMemLoadCB mcrd(bt->texCopy.data() + sizeof(hdr), data_sz);
      if (load_ddsx_tex_contents(tex, hdr, mcrd, quality_id))
        return tex;
    }
    else if (load_ddsx_tex_contents(tex, hdr, crd, quality_id))
      return tex;

    if (!is_main_thread())
    {
      for (unsigned attempt = 0, tries = 5, f = dagor_frame_no(); attempt < tries;)
        if (dagor_frame_no() < f + 1)
          sleep_msec(1);
        else
        {
          crd.seekto(st_pos);
          if (load_ddsx_tex_contents(tex, hdr, crd, quality_id))
          {
            debug("finally loaded %s (attempt=%d)", stat_name, attempt + 1);
            return tex;
          }
          f = dagor_frame_no();
          attempt++;
        }
      return tex;
    }
    del_d3dres(tex);
  }
  return nullptr;
}

BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int q_id, int levels, const char *stat_name, int stub_tex_idx)
{
  flg = implant_d3dformat(flg, hdr.d3dFormat);
  if (hdr.d3dFormat == ::D3DFMT_A4R4G4B4 || hdr.d3dFormat == ::D3DFMT_X4R4G4B4 || hdr.d3dFormat == ::D3DFMT_R5G6B5)
    flg = implant_d3dformat(flg, ::D3DFMT_A8R8G8B8);
  G_ASSERT((flg & TEXCF_RTARGET) == 0);
  flg |= (hdr.flags & hdr.FLG_GAMMA_EQ_1) ? 0 : TEXCF_SRGBREAD;

  if (levels <= 0)
    levels = hdr.levels;

  int resType;
  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    resType = RES3D_CUBETEX;
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    resType = RES3D_VOLTEX;
  else if (hdr.flags & ddsx::Header::FLG_ARRTEX)
    resType = RES3D_ARRTEX;
  else
    resType = RES3D_TEX;

  auto bt = allocate_texture(resType, flg);

  int skip_levels = hdr.getSkipLevels(hdr.getSkipLevelsFromQ(q_id), levels);
  int w = max(hdr.w >> skip_levels, 1), h = max(hdr.h >> skip_levels, 1), d = max(hdr.depth >> skip_levels, 1);
  if (!(hdr.flags & hdr.FLG_VOLTEX))
    d = (hdr.flags & hdr.FLG_ARRTEX) ? hdr.depth : 1;

  bt->setParams(w, h, d, levels, stat_name);
  bt->stubTexIdx = stub_tex_idx;
  bt->preallocBeforeLoad = bt->delayedCreate = true;

  if (stub_tex_idx >= 0)
  {
    // static analysis says this could be null
    auto subtex = bt->getStubTex();
    if (subtex)
      bt->tex.image = subtex->tex.image;
  }

  return bt;
}

#if _TARGET_PC_WIN
unsigned d3d::pcwin32::get_texture_format(BaseTexture *tex)
{
  auto bt = getbasetex(tex);
  if (!bt)
    return 0;
  return bt->getFormat();
}
const char *d3d::pcwin32::get_texture_format_str(BaseTexture *tex)
{
  auto bt = getbasetex(tex);
  if (!bt)
    return nullptr;
  return bt->getFormat().getNameString();
}
#endif

bool d3d::set_tex_usage_hint(int, int, int, const char *, unsigned int)
{
  debug_ctx("n/a");
  return true;
}

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  if (num_textures)
    *num_textures = 0;
  if (total_mem)
    *total_mem = 0;

  // TODO: make a proper one like in dx11/dx12/etc
  // but now at least make log on app.tex being called
  SharedGuardedObjectAutoLock lock(drv3d_vulkan::get_device().resources);
  drv3d_vulkan::get_device().resources.printStats(out_dump != nullptr, true);
}

Texture *d3d::alias_tex(Texture *, TexImage32 *, int, int, int, int, const char *) { return nullptr; }

CubeTexture *d3d::alias_cubetex(CubeTexture *, int, int, int, const char *) { return nullptr; }

VolTexture *d3d::alias_voltex(VolTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_array_tex(ArrayTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *, int, int, int, int, const char *) { return nullptr; }
