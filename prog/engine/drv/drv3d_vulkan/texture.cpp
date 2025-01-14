// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texture.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <drv/3d/dag_texture.h>
#include <image/dag_texPixel.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_memIo.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <validateUpdateSubRegion.h>
#include <validation/texture.h>
#if _TARGET_PC_WIN
#include <drv/3d/dag_platform_pc.h>
#endif

#include "driver_defs.h"
#include "basetexture.h"
#include "globals.h"
#include "resource_manager.h"
#include "vk_format_utils.h"
#include "device_context.h"
#include "global_lock.h"
#include "resource_upload_limit.h"

#if 0
#define VERBOSE_DEBUG debug
#else
#define VERBOSE_DEBUG(...)
#endif

using namespace drv3d_vulkan;

namespace
{

void create_tex_fill(BaseTex::D3DTextures &tex, BaseTex *bt_in, const ImageCreateInfo &ici, BaseTex::ImageMem *initial_data)
{
  uint32_t flg = bt_in->cflg;
  // transient image can't be cleared/filled with data
  // and if any other fillers are not applicable, skip fill
  if (
    (ici.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) || (!initial_data && ((flg & (TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE)) == 0)))
    return;

  if (flg & TEXCF_RTARGET)
  {
    // init render target to a known state
    VkImageSubresourceRange area;
    area.aspectMask = ici.format.getAspektFlags();
    area.baseMipLevel = 0;
    area.levelCount = ici.mips;
    area.baseArrayLayer = 0;
    area.layerCount = ici.arrays;

    if (area.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
    {
      VkClearColorValue cv = {};
      Globals::ctx.clearColorImage(tex.image, area, cv, true);
    }
    else
    {
      VkClearDepthStencilValue cv = {};
      Globals::ctx.clearDepthStencilImage(tex.image, area, cv);
    }
  }
  else
  {
    const bool tempStage = tex.stagingBuffer == nullptr;
    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;

    if (initial_data)
    {
      if (tempStage)
        tex.useStaging(ici.format, ici.size.width, ici.size.height, ici.size.depth, ici.mips, ici.arrays, true);

      uint32_t offset = 0;
      for (uint32_t i = 0; i < ici.arrays; ++i)
      {
        uint32_t flushStart = offset;
        for (uint32_t j = 0; j < ici.mips; ++j)
        {
          copies[j] = make_copy_info(ici.format, j, i, 1, {ici.size.width, ici.size.height, ici.size.depth},
            tex.stagingBuffer->bufOffsetLoc(offset));

          BaseTex::ImageMem &src = initial_data[ici.mips * i + j];
          memcpy(tex.stagingBuffer->ptrOffsetLoc(offset), src.ptr, src.memSize);
          offset += src.memSize;
        }
        tex.stagingBuffer->markNonCoherentRangeLoc(flushStart, offset - flushStart, true);
        Globals::ctx.copyBufferToImage(tex.stagingBuffer, tex.image, ici.mips, copies.data(), true);
      }
    }
    else
    {
      if (tempStage)
        tex.useStaging(ici.format, ici.size.width, ici.size.height, ici.size.depth, 1, 1, true);

      uint32_t size = tex.stagingBuffer->getBlockSize();
      memset(tex.stagingBuffer->ptrOffsetLoc(0), 0, size);
      tex.stagingBuffer->markNonCoherentRangeLoc(0, size, true);

      for (uint32_t i = 0; i < ici.arrays; ++i)
      {
        for (uint32_t j = 0; j < ici.mips; ++j)
          copies[j] =
            make_copy_info(ici.format, j, i, 1, {ici.size.width, ici.size.height, ici.size.depth}, tex.stagingBuffer->bufOffsetLoc(0));
        Globals::ctx.copyBufferToImage(tex.stagingBuffer, tex.image, ici.mips, copies.data(), false);
      }
    }
    if (tempStage)
      tex.destroyStaging();
  }
}

void create_tex_tql(BaseTex::D3DTextures &tex, BaseTex *bt_in)
{
  tex.memSize = bt_in->ressize();
  bt_in->updateTexName();
  TEXQL_ON_ALLOC(bt_in);
}

bool create_tex_image(const ImageCreateInfo &ici, BaseTex::D3DTextures &tex, BaseTex *bt_in)
{
  tex.image = Image::create(ici);
  if (tex.image)
  {
    G_ASSERTF((bt_in->cflg & (TEXCF_RTARGET | TEXCF_SYSMEM)) != (TEXCF_RTARGET | TEXCF_SYSMEM),
      "vulkan: can't create SYSMEM render target");
    if (bt_in->cflg & (TEXCF_READABLE | TEXCF_SYSMEM))
      tex.useStaging(ici.format, ici.size.width, ici.size.height, ici.size.depth, ici.mips, ici.arrays);
  }
  else
    tex.destroyStaging();

  return tex.image != nullptr;
}

bool create_tex2d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t levels, bool cube,
  BaseTex::ImageMem *initial_data, int array_size = 1)
{
  uint32_t &flg = bt_in->cflg;
  G_ASSERT(!((flg & TEXCF_SAMPLECOUNT_MASK) && initial_data != nullptr));
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & (TEXCF_DYNAMIC | TEXCF_RTARGET))));
  tex.realMipLevels = levels;

  ImageCreateInfo ici;
  ici.type = VK_IMAGE_TYPE_2D;
  ici.size.width = w;
  ici.size.height = h;
  ici.size.depth = 1;
  ici.mips = levels;
  ici.arrays = (cube ? 6 : 1) * array_size;
  ici.initByTexCreate(flg, cube);

  if (!create_tex_image(ici, tex, bt_in))
    return false;

  create_tex_fill(tex, bt_in, ici, initial_data);
  create_tex_tql(tex, bt_in);
  return true;
}

bool create_tex3d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t d, uint32_t flg, uint32_t levels,
  BaseTex::ImageMem *initial_data)
{
  G_ASSERT((flg & TEXCF_SAMPLECOUNT_MASK) == 0);
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & TEXCF_DYNAMIC)));
  tex.realMipLevels = levels;

  ImageCreateInfo ici;
  ici.type = VK_IMAGE_TYPE_3D;
  ici.size.width = w;
  ici.size.height = h;
  ici.size.depth = d;
  ici.mips = levels;
  ici.arrays = 1;
  ici.initByTexCreate(flg, false /*cube_tex*/);

  if (!create_tex_image(ici, tex, bt_in))
    return false;

  // check 3d render support
  const bool isRT = flg & TEXCF_RTARGET;
  if (isRT && !Globals::cfg.has.renderTo3D)
  {
    debug("vulkan: render to voltex is not supported");
    Globals::ctx.destroyImage(tex.image);
    tex.image = nullptr;
    return false;
  }

  create_tex_fill(tex, bt_in, ici, initial_data);
  create_tex_tql(tex, bt_in);
  return true;
}

} // namespace

namespace drv3d_vulkan
{

void BaseTex::cleanupWaitEvent()
{
  if (waitEvent.isRequested())
  {
    TIME_PROFILE(vulkan_tex_cleanup_wait_event);
    Globals::ctx.waitForIfPending(waitEvent);
    waitEvent.reset();
  }
}

void BaseTex::blockingReadbackWait()
{
  TIME_PROFILE(vulkan_texture_blocking_readback);
  Globals::ctx.wait();
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

  if (!(Globals::VK::fmt.features(fmt.asVkFormat()) & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
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

  Globals::ctx.copyImage(tex.image, dst, regions.data(), mipLevels, 0, 0, mipLevels);
}

void BaseTex::resolve(Image *dst) { Globals::ctx.resolveMultiSampleImage(tex.image, dst); }

int BaseTex::updateSubRegionInternal(BaseTexture *srcBaseTex, int src_subres_idx, int src_x, int src_y, int src_z, int src_w,
  int src_h, int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z, bool unordered)
{
  if (isStub())
  {
    D3D_ERROR("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (srcBaseTex)
    if (BaseTex *stex = getbasetex(srcBaseTex))
      if (stex->isStub())
      {
        D3D_ERROR("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", srcBaseTex->getTexName(),
          stex->stubTexIdx);
        return 0;
      }

  if ((RES3D_TEX != resType) && (RES3D_CUBETEX != resType) && (RES3D_VOLTEX != resType) && (RES3D_ARRTEX != resType))
    return 0;

  BaseTex *src = getbasetex(srcBaseTex);
  if (!src)
    return 0;

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

  Globals::ctx.copyImage(src->tex.image, tex.image, &region, 1, src_subres_idx % src->mipLevels, dest_subres_idx % mipLevels, 1,
    unordered);
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
    D3D_ERROR("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (src)
    if (BaseTex *stex = getbasetex(src))
      if (stex->isStub())
      {
        D3D_ERROR("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", src->getTexName(), stex->stubTexIdx);
        return 0;
      }

  if ((RES3D_TEX == resType) || (RES3D_CUBETEX == resType) || (RES3D_VOLTEX == resType))
  {
    BaseTex *btex = getbasetex(src);
    if (btex)
    {
      if (btex->cflg & TEXCF_SAMPLECOUNT_MASK)
        btex->resolve(tex.image);
      else if (btex->tex.image && tex.image)
      {
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
  return BaseTex::updateSubRegionInternal(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, dest_subres_idx, dest_x,
    dest_y, dest_z, false);
}

int BaseTex::updateSubRegionNoOrder(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h,
  int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z)
{
  return BaseTex::updateSubRegionInternal(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, dest_subres_idx, dest_x,
    dest_y, dest_z, true);
}

void BaseTex::destroy()
{
  release();
  destroyObject();
}

//-----------------------------------------------------------------------------

int BaseTex::generateMips()
{
  if (mipLevels > 1)
    Globals::ctx.generateMipmaps(tex.image);
  return 1;
}

void BaseTex::D3DTextures::useStaging(FormatStore fmt, int32_t w, int32_t h, int32_t d, int32_t levels, uint16_t arrays,
  bool temporary /*=false*/)
{
  if (!stagingBuffer)
  {
    uint32_t size = fmt.calculateImageSize(w, h, d, levels) * arrays;
    stagingBuffer = Buffer::create(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1,
      temporary ? BufferMemoryFlags::TEMP : BufferMemoryFlags::NONE);

    stagingBuffer->setStagingDebugName(image);
  }
}

void BaseTex::D3DTextures::destroyStaging()
{
  if (stagingBuffer)
  {
    Globals::ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }
}

void BaseTex::D3DTextures::releaseDelayed(AsyncCompletionState &event)
{
  // races to staging buffer is a caller problem, so just delete it asap
  if (stagingBuffer)
  {
    Globals::ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }

  if (event.isRequested())
  {
    TIME_PROFILE(vulkan_tex_release_delayed_wait_event);
    Globals::ctx.waitForIfPending(event);
    event.reset();
  }

  if (image)
    Globals::ctx.destroyImageDelayed(image);
}

void BaseTex::D3DTextures::release(AsyncCompletionState &event)
{
  if (stagingBuffer)
  {
    Globals::ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }

  if (event.isRequested())
  {
    TIME_PROFILE(vulkan_tex_release_wait_event);
    Globals::ctx.waitForIfPending(event);
    event.reset();
  }

  if (image)
  {
    Globals::ctx.destroyImage(image);
    image = nullptr;
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

  // important: swap before release, sync on followup front lock atomic RW
  tex = new_tex;
  texc.releaseDelayed(waitEvent);

  // old texture will be deleted, so we are swapping to nullptr
  new_tex.image = nullptr;
  new_tex.stagingBuffer = nullptr;
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
  if (this == other_tex)
    return;

  {
    BaseTex *other = getbasetex(other_tex);
    G_ASSERT_RETURN(other, );

    // safely swap texture objects
    swapInnerTex(other->tex);
    updateTexName();
    // swap dimensions
    eastl::swap(width, other->width);
    eastl::swap(height, other->height);
    eastl::swap(depth, other->depth);
    eastl::swap(mipLevels, other->mipLevels);
    eastl::swap(minMipLevel, other->minMipLevel);
    eastl::swap(maxMipLevel, other->maxMipLevel);
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
    case RES3D_CUBETEX: return create_tex2d(tex, this, width, height, mipLevels, resType == RES3D_CUBETEX, nullptr, 1);
    case RES3D_CUBEARRTEX:
    case RES3D_ARRTEX: return create_tex2d(tex, this, width, height, mipLevels, resType == RES3D_CUBEARRTEX, nullptr, depth);
  }
  return false;
}

int BaseTex::lockimg(void **p, int &stride, int lev, unsigned flags)
{
  if (RES3D_TEX != resType)
    return 0;

  bool stagingIsTemporary = (flags & TEXLOCK_DELSYSMEMCOPY) && !(cflg & TEXCF_DYNAMIC);

  if (!tex.image && (flags & TEXLOCK_WRITE) && !(flags & TEXLOCK_READ))
    if (!create_tex2d(tex, this, width, height, mipLevels, false, nullptr))
    {
      D3D_ERROR("failed to auto-create tex.tex2D on lockImg");
      return 0;
    }
  stride = 0;

  uint32_t prevFlags = lockFlags;
  if (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED))
  {
    bool resourceCopied = (lockFlags == TEX_COPIED);
    lockFlags = 0;
    if ((getFormat().getAspektFlags() & VK_IMAGE_ASPECT_DEPTH_BIT) && !(flags & TEXLOCK_COPY_STAGING))
    {
      D3D_ERROR("can't lock depth format");
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
        copy = make_copy_info(getFormat(), j, 0, layers, {width, height, 1}, tex.stagingBuffer->bufOffsetLoc(bufferOffset));

        bufferOffset +=
          getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1) * layers;
      }
      if (flags & TEXLOCK_COPY_STAGING)
      {
        Globals::ctx.copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), nullptr);
        blockingReadbackWait();
        lockFlags = TEXLOCK_COPY_STAGING;
        return 1;
      }

      // another case of start readback logic
      if (!resourceCopied && p && (flags & TEXLOCK_NOSYSLOCK))
      {
        cleanupWaitEvent();
        Globals::ctx.copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), &waitEvent);
        lockFlags = TEX_COPIED;
        stride = 0;
        return 0;
      }

      if (!p)
      {
        // on some cases the requested copy is never used
        cleanupWaitEvent();
        Globals::ctx.copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), &waitEvent);
      }
      else if (!resourceCopied)
      {
        Globals::ctx.copyImageToBuffer(tex.image, tex.stagingBuffer, mipLevels, copies.data(), nullptr);
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
        D3D_ERROR("nullptr in lockimg");
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

    lockMsr.ptr = tex.stagingBuffer->ptrOffsetLoc(offset);
    lockMsr.rowPitch = getFormat().calculateRowPitch(max(width >> lev, 1));
    lockMsr.slicePitch = getFormat().calculateSlicePich(max(width >> lev, 1), max(height >> lev, 1));

    if (waitEvent.isRequested())
    {
      if (flags & TEXLOCK_NOSYSLOCK)
      {
        if (!waitEvent.isCompleted())
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
        TIME_PROFILE(vulkan_tex_lock_early_readback_finish);
        Globals::ctx.waitForIfPending(waitEvent);
      }
      waitEvent.reset();
    }

    tex.stagingBuffer->markNonCoherentRangeLoc(offset, lockMsr.slicePitch * getArrayCount(), false);
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
  auto &context = Globals::ctx;
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
        copy = make_copy_info(getFormat(), j, 0, 1, {width, height, depth}, tex.stagingBuffer->bufOffsetLoc(bufferOffset));

        bufferOffset += getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
      }
      tex.stagingBuffer->markNonCoherentRangeLoc(0, bufferOffset, true);
      Globals::ctx.copyBufferToImage(tex.stagingBuffer, tex.image, mipLevels, copies.data(), false);
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
          make_copy_info(getFormat(), lockedLevel, 0, 1, {width, height, depth}, tex.stagingBuffer->bufOffsetLoc(0));

        uint32_t dirtySize =
          getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
        tex.stagingBuffer->markNonCoherentRangeLoc(0, dirtySize, true);
        Globals::ctx.copyBufferToImageOrdered(tex.stagingBuffer, tex.image, 1, &copy);
        tex.destroyStaging();
      }
      else if ((lockFlags & TEXLOCK_DONOTUPDATEON9EXBYDEFAULT) != 0)
        unlockImageUploadSkipped = true;
      else if ((lockFlags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) != 0)
      {
        carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
        VkDeviceSize bufferOffset = 0;

        for (uint32_t j = 0; j < mipLevels; ++j)
        {
          VkBufferImageCopy &copy = copies[j];
          copy = make_copy_info(getFormat(), j, 0, 1, {width, height, depth}, tex.stagingBuffer->bufOffsetLoc(bufferOffset));

          bufferOffset += getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
        }
        tex.stagingBuffer->markNonCoherentRangeLoc(0, bufferOffset, true);
        // Sometimes we use TEXLOCK_DONOTUPDATEON9EXBYDEFAULT flag and don't copy locked subresource on unlock.
        // Copy all mips is required after that action.
        const eastl::span<VkBufferImageCopy> fullResource{copies.data(), mipLevels};
        const eastl::span<VkBufferImageCopy> oneSubresource{&copies[lockedLevel], 1};
        const auto uploadRegions = unlockImageUploadSkipped ? fullResource : oneSubresource;
        Globals::ctx.copyBufferToImage(tex.stagingBuffer, tex.image, uploadRegions.size(), uploadRegions.data(), false);
        unlockImageUploadSkipped = false;
      }
    }

    if ((lockFlags & TEXLOCK_DELSYSMEMCOPY) && !(cflg & TEXCF_DYNAMIC))
    {
      G_ASSERT(!unlockImageUploadSkipped);
      tex.destroyStaging();
    }

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
          make_copy_info(getFormat(), lockedLevel, lockedLayer, 1, {width, height, 1}, tex.stagingBuffer->bufOffsetLoc(0));

        tex.stagingBuffer->markNonCoherentRangeLoc(0, tex.stagingBuffer->getBlockSize(), true);
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
    auto &context = Globals::ctx;

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
        VkBufferImageCopy copy = make_copy_info(format, lev, face, 1, {width, height, 1}, tex.stagingBuffer->bufOffsetLoc(0));

        if (p)
        {
          context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, nullptr);
          blockingReadbackWait();
        }
        else
        {
          // sometimes async read back are issued without waiting for it and then re-issuing again
          cleanupWaitEvent();
          context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, &waitEvent);
        }
      }

      if (p)
      {
        cleanupWaitEvent();
        tex.stagingBuffer->markNonCoherentRangeLoc(0, tex.stagingBuffer->getBlockSize(), false);
        *p = tex.stagingBuffer->ptrOffsetLoc(0);
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
    auto &context = Globals::ctx;

    auto format = getFormat();
    uint32_t levelWidth = max(width >> level, 1);
    uint32_t levelHeight = max(height >> level, 1);
    uint32_t levelDepth = max(depth >> level, 1);
    lockFlags = flags;
    lockedLevel = level;

    // only get a buffer large enough to hold the locked level
    tex.useStaging(format, levelWidth, levelHeight, levelDepth, 1, 1, true);
    *data = tex.stagingBuffer->ptrOffsetLoc(0);

    if (flags & TEXLOCK_READ)
    {
      VkBufferImageCopy copy = make_copy_info(format, level, 0, 1, {width, height, depth}, tex.stagingBuffer->bufOffsetLoc(0));

      context.copyImageToBuffer(tex.image, tex.stagingBuffer, 1, &copy, nullptr);
      blockingReadbackWait();

      tex.stagingBuffer->markNonCoherentRangeLoc(0, tex.stagingBuffer->getBlockSize(), false);
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

  auto &context = Globals::ctx;

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
        make_copy_info(getFormat(), lockedLevel, 0, 1, {width, height, depth}, tex.stagingBuffer->bufOffsetLoc(0));

      tex.stagingBuffer->markNonCoherentRangeLoc(0, tex.stagingBuffer->getBlockSize(), true);
      context.copyBufferToImage(tex.stagingBuffer, tex.image, 1, &copy, false);
    }

    tex.destroyStaging();
  }

  lockFlags = 0;
  return 1;
}

Texture *BaseTex::wrapVKImage(VkImage tex_res, ResourceBarrier current_state, int width, int height, int layers, int mips,
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

  WinAutoLock lk(Globals::Mem::mutex);
  tex->tex.image = Globals::Mem::res.alloc<Image>({ici, Image::MEM_NOT_EVICTABLE, dagorFormat, currentVkLayout}, false);
  tex->tex.image->setDerivedHandle(VulkanHandle(tex_res));
  tex->tex.memSize = tex->ressize();

  return tex;
}

} // namespace drv3d_vulkan

Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  check_texture_creation_args(w, h, flg, stat_name);
  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    D3D_ERROR("create_tex: can not create dynamic render target");
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

  levels = count_mips_if_needed(w, h, flg, levels);

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
      D3D_ERROR("create_tex: image size differs from texture size (%dx%d != %dx%d)", img->w, img->h, w, h);
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
    D3D_ERROR("create_cubtex: can not create dynamic render target");
    return nullptr;
  }

  const Driver3dDesc &dd = d3d::get_driver_desc();
  size = get_bigger_pow2(clamp<int>(size, dd.mincubesize, dd.maxcubesize));

  levels = count_mips_if_needed(size, size, flg, levels);

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
    D3D_ERROR("create_voltex: can not create dynamic render target");
    return nullptr;
  }

  levels = count_mips_if_needed(w, h, flg, levels);

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
  levels = count_mips_if_needed(w, h, flg, levels);

  auto tex = allocate_texture(RES3D_ARRTEX, flg);
  tex->setParams(w, h, d, levels, stat_name);

  if (!create_tex2d(tex->tex, tex, w, h, levels, false, nullptr, d))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

ArrayTexture *d3d::create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  levels = count_mips_if_needed(side, side, flg, levels);

  auto tex = allocate_texture(RES3D_ARRTEX, flg);
  tex->setParams(side, side, d, levels, stat_name);
  tex->isArrayCube = true;
  tex->setInitialImageViewState();

  if (!create_tex2d(tex->tex, tex, side, side, levels, true, nullptr, d))
    return nullptr;

  tex->tex.memSize = tex->ressize();

  return tex;
}

static BaseTexture *retry_load_ddsx_tex_contents(BaseTexture *tex, const ddsx::Header &hdr, int quality_id, const void *ptr, int sz,
  const char *stat_name)
{
  // this retry should always complete with OK status, as there is no higher level retry logic on use places
  // so just use RUB overallocation
  Frontend::resUploadLimit.setNoFailOnThread(true);
  InPlaceMemLoadCB mcrd(ptr, sz);
  TexLoadRes ldRet = d3d::load_ddsx_tex_contents(tex, hdr, mcrd, quality_id);
  Frontend::resUploadLimit.setNoFailOnThread(false);
  if (ldRet != TexLoadRes::OK)
    D3D_ERROR("vulkan: retry failure at texture %s ddsx load code %u", stat_name, (int)ldRet);
  // caller don't expect nullptr, so complain, but not crash!
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
        LOGERR_CTX("inconsistent input tex data, data_sz=%d tex=%s", data_sz, stat_name);
        del_d3dres(tex);
        return NULL;
      }
      VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(bt->texCopy));
      InPlaceMemLoadCB mcrd(bt->texCopy.data() + sizeof(hdr), data_sz);
      TexLoadRes ldRet = load_ddsx_tex_contents(tex, hdr, mcrd, quality_id);
      if (ldRet == TexLoadRes::OK)
        return tex;

      return retry_load_ddsx_tex_contents(tex, hdr, quality_id, bt->texCopy.data() + sizeof(hdr), data_sz, stat_name);
    }
    else
    {
      // Use temporary buffer to read compressed texture data
      // crd could be a FastSeqReadCB which has a limit to seekto back in the stream
      // and mught end up with fatal error.
      const int dataSz = hdr.packedSz ? hdr.packedSz : hdr.memSz;
      eastl::unique_ptr<char[]> tmpBuffer(new char[dataSz]);

      if (!crd.readExact(tmpBuffer.get(), dataSz))
      {
        LOGERR_CTX("Cannot read %d of texture %s", dataSz, stat_name);
        return nullptr;
      }

      InPlaceMemLoadCB tmpCrd(tmpBuffer.get(), dataSz);
      TexLoadRes ldRet = load_ddsx_tex_contents(tex, hdr, tmpCrd, quality_id);
      if (ldRet == TexLoadRes::OK)
        return tex;

      return retry_load_ddsx_tex_contents(tex, hdr, quality_id, tmpBuffer.get(), dataSz, stat_name);
    }
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

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  if (num_textures)
    *num_textures = 0;
  if (total_mem)
    *total_mem = 0;

  // TODO: make a proper one like in dx11/dx12/etc
  // but now at least make log on app.tex being called
  WinAutoLock lk(drv3d_vulkan::Globals::Mem::mutex);
  drv3d_vulkan::Globals::Mem::res.printStats(out_dump != nullptr, true);
}

Texture *d3d::alias_tex(Texture *, TexImage32 *, int, int, int, int, const char *) { return nullptr; }

CubeTexture *d3d::alias_cubetex(CubeTexture *, int, int, int, const char *) { return nullptr; }

VolTexture *d3d::alias_voltex(VolTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_array_tex(ArrayTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *, int, int, int, int, const char *) { return nullptr; }
