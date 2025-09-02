// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texture.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <drv/3d/dag_texture.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <validateUpdateSubRegion.h>
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

namespace drv3d_vulkan
{

bool BaseTexParams::verify() const
{
  D3D_CONTRACT_ASSERTF((flg & (TEXCF_RTARGET | TEXCF_SYSMEM)) != (TEXCF_RTARGET | TEXCF_SYSMEM),
    "vulkan: can't create SYSMEM render target");
  // check 3d render support
  if (isVolume() && (flg & TEXCF_RTARGET) && !Globals::cfg.has.renderTo3D)
  {
    debug("vulkan: render to voltex is not supported");
    return false;
  }
  if (flg & TEXCF_TILED_RESOURCE)
  {
    if (!Globals::VK::phy.features.sparseBinding)
      return false;
    if (isVolume())
      return Globals::VK::phy.features.sparseResidencyImage3D;
    else
      return Globals::VK::phy.features.sparseResidencyImage2D;
  }
  return true;
}

void BaseTex::onDeviceReset()
{
  if (rld)
    ;
  else if (stubTexIdx >= 0 && (!isStub() && image) && !pars.isGPUWritable())
  {
    release();
  }
  readback.reset();
  bindlessStubSwapQueue.clear();
}

void BaseTex::afterDeviceReset()
{
  if (rld)
    rld->reloadD3dRes(this);
#if VULKAN_TEXCOPY_SUPPORT > 0
  else if ((pars.flg & TEXCF_SYSTEXCOPY) && data_size(texCopy))
  {
    ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
    int8_t sysCopyQualityId = hdr.hqPartLevels;
    InPlaceMemLoadCB mcrd(texCopy.data() + sizeof(hdr), data_size(texCopy) - (int)sizeof(hdr));
    d3d::load_ddsx_tex_contents(this, hdr, mcrd, sysCopyQualityId);
  }
#endif
  else if (stubTexIdx >= 0 && !image && !pars.isGPUWritable())
  {
    recreate();
  }
  else if (image)
    initialImageFill(pars.getImageCreateInfo(), nullptr);
  if (image)
    updateTexName();
}

void BaseTex::finishReadback()
{
  if (readback.isRequested())
  {
    TIME_PROFILE(vulkan_tex_cleanup_wait_event);
    Globals::ctx.waitForIfPending(readback);
    readback.reset();
  }
}

void BaseTex::copy(Image *dst)
{
  const uint32_t layerCount = pars.getArrayCount();
  const uint32_t depthSlices = pars.getDepthSlices();

  carray<VkImageCopy, MAX_MIPMAPS> regions;
  for (uint32_t i = 0; i < pars.levels; ++i)
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
    region.extent.width = max<uint32_t>(1, pars.w >> i);
    region.extent.height = max<uint32_t>(1, pars.h >> i);
    region.extent.depth = max<uint32_t>(1u, depthSlices >> i);
  }

  Globals::ctx.copyImage(image, dst, regions.data(), pars.levels, 0, 0, pars.levels);
}

void BaseTex::resolve(Image *dst) { Globals::ctx.resolveMultiSampleImage(image, dst); }

int BaseTex::updateSubRegionInternal(BaseTexture *srcBaseTex, int src_subres_idx, int src_x, int src_y, int src_z, int src_w,
  int src_h, int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z, bool unordered)
{
  if (isStub())
  {
    D3D_CONTRACT_ERROR("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (srcBaseTex)
    if (BaseTex *stex = getbasetex(srcBaseTex))
      if (stex->isStub())
      {
        D3D_CONTRACT_ERROR("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", srcBaseTex->getTexName(),
          stex->stubTexIdx);
        return 0;
      }

  BaseTex *src = getbasetex(srcBaseTex);
  if (!src)
    return 0;

  if (!validate_update_sub_region_params(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, this, dest_subres_idx, dest_x,
        dest_y, dest_z))
    return 0;

  VkImageCopy region;
  region.srcSubresource.aspectMask = getFormat().getAspektFlags();
  region.srcSubresource.mipLevel = src_subres_idx % src->pars.levels;
  region.srcSubresource.baseArrayLayer = src_subres_idx / src->pars.levels;
  region.srcSubresource.layerCount = 1;
  region.srcOffset.x = src_x;
  region.srcOffset.y = src_y;
  region.srcOffset.z = src_z;
  region.dstSubresource.aspectMask = getFormat().getAspektFlags();
  region.dstSubresource.mipLevel = dest_subres_idx % pars.levels;
  region.dstSubresource.baseArrayLayer = dest_subres_idx / pars.levels;
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

    if ((blockX * src_w) > (pars.w >> (region.dstSubresource.mipLevel)) ||
        (blockY * src_h) > (pars.h >> (region.dstSubresource.mipLevel)))
    {
      return 0;
    }
  }

  Globals::ctx.copyImage(src->image, image, &region, 1, src_subres_idx % src->pars.levels, dest_subres_idx % pars.levels, 1,
    unordered);
  return 1;
}

bool BaseTex::recreate()
{
  if (!pars.isArray())
  {
    if (pars.flg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("<%s> recreate %dx%dx%d-%u (%s)", getName(), pars.w, pars.h, pars.d, pars.type, "rt|dyn");
      return allocateImageInternal(nullptr);
    }

    if (!preallocBeforeLoad)
    {
      VERBOSE_DEBUG("<%s> recreate %dx%dx%d-%u (%s)", getName(), pars.w, pars.h, pars.d, pars.type, "empty");
      return allocateImageInternal(nullptr);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("<%s> recreate %dx%d (%s)", getName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      auto stubTex = getStubTex();
      if (stubTex)
        image = stubTex->image;
      return 1;
    }
    BaseTexParams savedParams = pars;
    pars.w = 4;
    pars.h = 1;
    pars.d = 1;
    pars.levels = 1;
    bool ret = allocateImageInternal(nullptr);
    pars = savedParams;
    return ret;
  }
  else
  {
    // not used with dynamic or rtarget
    // no stubs are provided usually
    // so path there is exclusive to other tex types
    VERBOSE_DEBUG("<%s> recreate %dx%dx%d (%s)", getName(), pars.w, pars.h, pars.d, "arr tex");
    return allocateImageInternal(nullptr);
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
  release();
  setReloadCallback(nullptr);
  free_texture(this);
}

int BaseTex::update(BaseTexture *src)
{
  if (isStub())
  {
    D3D_CONTRACT_ERROR("updateSubRegion() called for tex=<%s> in stub state: stubTexIdx=%d", getTexName(), stubTexIdx);
    return 0;
  }
  if (src)
    if (BaseTex *stex = getbasetex(src))
      if (stex->isStub())
      {
        D3D_CONTRACT_ERROR("updateSubRegion() called with src tex=<%s> in stub state: stubTexIdx=%d", src->getTexName(),
          stex->stubTexIdx);
        return 0;
      }

  if (!pars.isArray())
  {
    BaseTex *btex = getbasetex(src);
    if (btex)
    {
      if (btex->pars.flg & TEXCF_SAMPLECOUNT_MASK)
        btex->resolve(image);
      else if (btex->image && image)
      {
        btex->copy(image);
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

bool BaseTex::setReloadCallback(IReloadData *_rld)
{
  if (rld && rld != _rld)
    rld->destroySelf();
  rld = _rld;
  return true;
}

int BaseTex::generateMips()
{
  if (pars.levels > 1)
    Globals::ctx.generateMipmaps(image);
  return 1;
}

void BaseTex::useStaging(int32_t w, int32_t h, int32_t d, int32_t levels, uint16_t arrays, bool temporary /*=false*/)
{
  if (!stagingBuffer)
  {
    uint32_t size = fmt.calculateImageSize(w, h, d, levels) * arrays;
    stagingBuffer = Buffer::create(size, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1,
      temporary ? BufferMemoryFlags::TEMP : BufferMemoryFlags::NONE);

    stagingBuffer->setStagingDebugName(image);
  }
}

void BaseTex::destroyStaging()
{
  if (stagingBuffer)
  {
    Globals::ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }
}

void BaseTex::release()
{
  if (isStub())
    image = nullptr;
  else if (image && image->isManaged())
    TEXQL_ON_RELEASE(this);

  finishReadback();
  destroyStaging();

  if (image)
  {
    Globals::ctx.destroyImage(image);
    image = nullptr;
  }
}

bool BaseTex::updateTexResFormat(unsigned d3d_format)
{
  if (!isStub())
    return pars.flg == implant_d3dformat(pars.flg, d3d_format);
  auto *img = image;
  image = nullptr; // reset pointer to stub for getFormat() inside setInitialImageViewState()
  pars.flg = implant_d3dformat(pars.flg, d3d_format);
  fmt = FormatStore::fromCreateFlags(pars.flg);
  setInitialImageViewState();
  image = img; // reset pointer to stub
  return true;
}
BaseTexture *BaseTex::makeTmpTexResCopy(int w, int h, int d, int l)
{
  if (!pars.isArray() && !pars.isVolume())
    d = 1;
  BaseTex *clonedTex = allocate_texture(w, h, d, l, pars.type, pars.flg, String::mk_str_cat("tmp:", getTexName()));
  clonedTex->tidXored = tidXored, clonedTex->stubTexIdx = stubTexIdx;
  clonedTex->preallocBeforeLoad = clonedTex->delayedCreate = true;
  if (!clonedTex->allocateTex())
    del_d3dres(clonedTex);
  return clonedTex;
}

void BaseTex::updateBindlessViewsForStreamedTextures()
{
  D3D_CONTRACT_ASSERTF(Globals::desc.caps.hasBindless,
    "vulkan: updateBindlessViewsForStreamedTextures somehow reached without bindless available");
  if (!image || isStub())
    return;

  // only "streamable"/"loadable" textures should be upadted here
  if (image->isGPUWritable())
    return;

  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
  Frontend::replay->bindlessTexSwaps.push_back({image, image, getViewInfo()});
}

void BaseTex::replaceTexResObject(BaseTexture *&other_tex)
{
  if (this == other_tex)
    return;

  BaseTex *other = getbasetex(other_tex);
  G_ASSERT_RETURN(other, );

  if (isStub())
  {
    image = nullptr;
    if (!other->isStub() && usedInBindless)
    {
      // swapping from stub to non stub, consume slot writes that was done while texture was using stub
      for (uint32_t i : bindlessStubSwapQueue)
      {
        Globals::ctx.updateBindlessResource(i, other_tex, true /*stub_swap*/);
      }
      bindlessStubSwapQueue.clear();
    }
  }
  else if (image)
  {
    // this will create some small overbudget, but we can't move BaseTex to backend
    // propragate updates to bindless slots to not bother caller with this
    if (usedInBindless)
    {
      D3D_CONTRACT_ASSERTF(Globals::desc.caps.hasBindless, "vulkan: bindless swap somehow reached without bindless available");
      OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
      Frontend::replay->bindlessTexSwaps.push_back({image, other->image, other->getViewInfo()});
    }
  }

  eastl::swap(image, other->image);
  eastl::swap(stagingBuffer, other->stagingBuffer);

  updateTexName();
  // swap dimensions
  eastl::swap(pars, other->pars);
  eastl::swap(minMipLevel, other->minMipLevel);
  eastl::swap(maxMipLevel, other->maxMipLevel);
  del_d3dres(other_tex);
}

void BaseTex::initialImageFill(const ImageCreateInfo &ici, BaseTex::ImageMem *initial_data)
{
  uint32_t flg = pars.flg;
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
      Globals::ctx.clearColorImage(image, area, cv, true);
    }
    else
    {
      VkClearDepthStencilValue cv = {};
      Globals::ctx.clearDepthStencilImage(image, area, cv, true);
    }
  }
  else
  {
    const bool tempStage = stagingBuffer == nullptr;
    carray<VkBufferImageCopy, MAX_MIPMAPS> copies;

    if (initial_data)
    {
      if (tempStage)
        useStagingForWholeTexture(true);

      uint32_t offset = 0;
      for (uint32_t i = 0; i < ici.arrays; ++i)
      {
        uint32_t flushStart = offset;
        for (uint32_t j = 0; j < ici.mips; ++j)
        {
          copies[j] = make_copy_info(ici.format, j, i, 1, {ici.size.width, ici.size.height, ici.size.depth},
            stagingBuffer->bufOffsetLoc(offset));

          BaseTex::ImageMem &src = initial_data[ici.mips * i + j];
          uint32_t memsz =
            ici.format.calculateImageSize(copies[j].imageExtent.width, copies[j].imageExtent.height, copies[j].imageExtent.depth, 1);
          memcpy(stagingBuffer->ptrOffsetLoc(offset), src.ptr, memsz);
          offset += memsz;
        }
        stagingBuffer->markNonCoherentRangeLoc(flushStart, offset - flushStart, true);
        Globals::ctx.copyBufferToImage(stagingBuffer, image, ici.mips, copies.data());
      }
    }
    else
    {
      if (tempStage)
        useStagingForSubresource(0, 0);

      uint32_t size = stagingBuffer->getBlockSize();
      if (flg & TEXFMT_ASTC4)
      {
        // For astc "all zeroes" isn't considered a valid compressed block
        // so this uint4{ 98371, 0, 0, 0 } (tied to quant method used in shader)
        // value should be placed instead to be treated like a black color

        const uint32_t blackBlock[] = {98371, 0, 0, 0};
        const int blockedSize = size / (sizeof(blackBlock));
        uint32_t *dst = (uint32_t *)stagingBuffer->ptrOffsetLoc(0);
        for (int i = 0; i < blockedSize; i++, dst += 4)
          memcpy(dst, blackBlock, sizeof(blackBlock));
      }
      else
        memset(stagingBuffer->ptrOffsetLoc(0), 0, size);
      stagingBuffer->markNonCoherentRangeLoc(0, size, true);

      for (uint32_t i = 0; i < ici.arrays; ++i)
      {
        for (uint32_t j = 0; j < ici.mips; ++j)
          copies[j] =
            make_copy_info(ici.format, j, i, 1, {ici.size.width, ici.size.height, ici.size.depth}, stagingBuffer->bufOffsetLoc(0));
        Globals::ctx.copyBufferToImage(stagingBuffer, image, ici.mips, copies.data());
      }
    }
    if (tempStage)
      destroyStaging();
  }
}

bool BaseTex::allocateImageInternal(BaseTex::ImageMem *initial_data)
{
  if (!pars.verify())
    return false;
  D3D_CONTRACT_ASSERT(!((pars.flg & TEXCF_SAMPLECOUNT_MASK) && initial_data != nullptr));

  ImageCreateInfo ici = pars.getImageCreateInfo();
  image = Image::create(ici);

  if (image)
  {
    if (pars.isStagingPermanent())
      useStagingForWholeTexture();
  }
  else
    destroyStaging();

  if (!image)
    return false;

  initialImageFill(ici, initial_data);
  updateTexName();
  TEXQL_ON_ALLOC(this);
  return true;
}

bool BaseTex::allocateImage(BaseTex::ImageMem *initial_data)
{
  if (image)
    return true;
  return allocateImageInternal(initial_data);
}

bool BaseTex::allocateOnLockIfNeeded(unsigned flags)
{
  if (!image)
  {
    if ((flags & TEXLOCK_WRITE) && !(flags & TEXLOCK_READ))
    {
      if (!allocateTex())
      {
        D3D_ERROR("vulkan: failed to auto-create tex2D on lockImg");
        return false;
      }
    }
    else
      return false;
  }
  return true;
}

void BaseTex::startReadback(int mip, int slice, int staging_offset)
{
  D3D_CONTRACT_ASSERTF(stagingBuffer, "vulkan: staging buffer must be preallocated for readback");
  finishReadback();

  VkBufferImageCopy copy =
    make_copy_info(getFormat(), mip, slice, 1, {pars.w, pars.h, pars.getDepthSlices()}, stagingBuffer->bufOffsetLoc(staging_offset));
  Globals::ctx.copyImageToBuffer(image, stagingBuffer, 1, &copy, &readback);
}

bool BaseTex::isStagingTemporaryForCurrentLock()
{
  // for cubemaps and voltex we always use temporary staging if not overriden by pars
  // this comes from original design logic
  // discards are forced to be single subresource staging locks
  return ((lockFlags & (TEXLOCK_DELSYSMEMCOPY | TEXLOCK_DISCARD)) || pars.isCubeMap() || pars.isVolume()) &&
         !pars.isStagingPermanent();
}

int BaseTex::clearLockState()
{
  lockFlags = 0;
  lockedLevel = -1;
  lockedLayer = -1;
  lockMsr = {};
  return 1;
}

int BaseTex::unlock()
{
  // no action specified
  if (!(lockFlags & TEXLOCK_RWMASK))
    return clearLockState();

  G_ASSERTF(image, "vulkan: no image at unlock for res %p:%s", this, getName());
  G_ASSERTF(stagingBuffer, "vulkan: no staging at unlock for res %p:%s", this, getName());

  updateSystexCopyFromCurrentLock();

  if ((lockFlags & TEXLOCK_DONOTUPDATE) != 0)
    unlockImageUploadSkipped = true;
  else if ((lockFlags & TEXLOCK_WRITE) != 0)
  {
    if (lockedSmallStaging)
    {
      VkBufferImageCopy copy = make_copy_info(getFormat(), lockedLevel, lockedLayer, 1, {pars.w, pars.h, pars.getDepthSlices()},
        stagingBuffer->bufOffsetLoc(0));

      stagingBuffer->markNonCoherentRangeLoc(0, stagingBuffer->getBlockSize(), true);
      // discard need to be in frame order, otherwise usual upload
      Globals::ctx.copyBufferToImage(stagingBuffer, image, 1, &copy, lockFlags & TEXLOCK_DISCARD);
    }
    else
    {
      if (unlockImageUploadSkipped)
      {
        carray<VkBufferImageCopy, MAX_MIPMAPS> copies;
        // full resource copy
        VkDeviceSize bufferOffset = 0;
        stagingBuffer->markNonCoherentRangeLoc(0, stagingBuffer->getBlockSize(), true);
        for (uint32_t i = 0; i < pars.getArrayCount(); ++i)
        {
          for (uint32_t j = 0; j < pars.levels; ++j)
          {
            VkBufferImageCopy &copy = copies[j];
            copy =
              make_copy_info(getFormat(), j, i, 1, {pars.w, pars.h, pars.getDepthSlices()}, stagingBuffer->bufOffsetLoc(bufferOffset));

            bufferOffset += getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1);
          }
          Globals::ctx.copyBufferToImage(stagingBuffer, image, pars.levels, copies.data(), lockFlags & TEXLOCK_DISCARD);
        }
        unlockImageUploadSkipped = false;
      }
      else
      {
        VkDeviceSize bufferOffset = subresourceOffsetInFullStaging(lockedLevel, lockedLayer);
        VkBufferImageCopy copy = make_copy_info(getFormat(), lockedLevel, lockedLayer, 1, {pars.w, pars.h, pars.getDepthSlices()},
          stagingBuffer->bufOffsetLoc(bufferOffset));

        stagingBuffer->markNonCoherentRangeLoc(bufferOffset,
          getFormat().calculateImageSize(copy.imageExtent.width, copy.imageExtent.height, copy.imageExtent.depth, 1), true);
        Globals::ctx.copyBufferToImage(stagingBuffer, image, 1, &copy, lockFlags & TEXLOCK_DISCARD);
      }
    }
  }

  if (isStagingTemporaryForCurrentLock())
  {
    G_ASSERT(!unlockImageUploadSkipped);
    if (!readback.isRequested())
      destroyStaging();
  }
  else
  {
    if (lockFlags & TEXLOCK_WRITE)
      gpuUpload.request(Globals::ctx.getCurrentWorkItemId());
  }

  return clearLockState();
}

uint32_t BaseTex::subresourceOffsetInFullStaging(int mip, int slice)
{
  uint32_t sliceSize = getFormat().calculateImageSize(pars.w, pars.h, pars.getDepthSlices(), pars.levels);
  return getFormat().calculateImageSize(pars.w, pars.h, pars.getDepthSlices(), mip) + sliceSize * slice;
}

int BaseTex::lock(void **p, int &row_stride, int *slice_stride, int mip, int slice, unsigned flags)
{
  if (p)
    *p = nullptr;
  row_stride = 0;
  if (slice_stride)
    *slice_stride = 0;

  if (pars.isGPUWritable() && (flags & TEXLOCK_WRITE))
  {
    D3D_CONTRACT_ERROR("vulkan: can't lock GPU writable texture for write");
    return 0;
  }

  lockFlags = flags;
  lockedLevel = mip;
  lockedLayer = slice;

  // no action specified, do early exit
  if (!(flags & TEXLOCK_RWMASK))
    return 1;

  allocateOnLockIfNeeded(flags);

  // setup staging
  uint32_t stagingOffset = 0;
  if (isStagingTemporaryForCurrentLock() && !stagingBuffer)
  {
    G_ASSERTF((lockFlags & TEXLOCK_DONOTUPDATE) == 0, "vulkan: trying to lock with do not update and temporary staging buffer");
    useStagingForSubresource(mip, true);
    lockedSmallStaging = true;
  }
  else
  {
    stagingOffset = subresourceOffsetInFullStaging(mip, slice);
    useStagingForWholeTexture();
    lockedSmallStaging = false;
  }

  // ensure we are not overwriting memory that is in use by GPU
  // and also implement "hidden" discard logic that users rely on using dx11
  if (flags & TEXLOCK_WRITE && !isStagingTemporaryForCurrentLock())
  {
    if (!gpuUpload.isCompletedOrNotRequested())
    {
      if (pars.isStagingDiscardable())
      {
        // discard only if work id changes, to optimize memory usage for different subres locking
        if (!gpuUpload.isRequestedThisFrame(Globals::ctx.getCurrentWorkItemId()) || lockFlags & TEXLOCK_DISCARD)
        {
          BufferRef stgRef(stagingBuffer);
          stagingBuffer =
            Globals::ctx.discardBuffer(stgRef, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, FormatStore(0), 0, 0).buffer;
        }
      }
      else
      {
        TIME_PROFILE(vulkan_tex_lock_wait_last_upload);
        Globals::ctx.waitForIfPending(gpuUpload);
      }
    }
    gpuUpload.reset();
  }

  if (!stagingBuffer)
    return 0;

  // perform readback and sync if needed
  if (flags & TEXLOCK_READ)
  {
    if (pars.isGPUWritable())
    {
      if (p)
      {
        if (!readback.isRequested())
          startReadback(mip, slice, stagingOffset);
        if (!readback.isCompleted())
        {
          if (flags & TEXLOCK_NOSYSLOCK)
            return 0;
          TIME_PROFILE(vulkan_tex_lock_blocking_readback);
          Globals::ctx.waitForIfPending(readback);
        }
        readback.reset();
      }
      else
      {
        startReadback(mip, slice, stagingOffset);
        return 1;
      }
    }
    // if we don't have permanent staging, there is nothing to read from
    else if (!pars.isStagingPermanent())
      return 0;
  }

  // write state & user pointers
  if (!p)
  {
    D3D_CONTRACT_ERROR("vulkan: nullptr in lockimg");
    return 0;
  }
  lockMsr.ptr = stagingBuffer->ptrOffsetLoc(stagingOffset);
  lockMsr.rowPitch = getFormat().calculateRowPitch(max(pars.w >> mip, 1));
  lockMsr.slicePitch = getFormat().calculateSlicePich(max(pars.w >> mip, 1), max(pars.h >> mip, 1));

  if (flags & TEXLOCK_READ)
  {
    VkExtent3D mipExtent = pars.getMipExtent(mip);
    stagingBuffer->markNonCoherentRangeLoc(stagingOffset,
      getFormat().calculateImageSize(mipExtent.width, mipExtent.height, mipExtent.depth, 1), false);
  }
  *p = lockMsr.ptr;
  row_stride = lockMsr.rowPitch;
  if (slice_stride)
    *slice_stride = lockMsr.slicePitch;

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

  TextureInterfaceBase *tex =
    allocate_texture(width, height, layers, mips, layers > 1 ? D3DResourceType::ARRTEX : D3DResourceType::TEX, flg, name);

  Image::Description::TrimmedCreateInfo ici;
  ici.flags = 0;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.extent = VkExtent3D{uint32_t(width), uint32_t(height), 1};
  ici.mipLevels = mips;
  ici.arrayLayers = layers;
  ici.usage = vkUsage;
  ici.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

  WinAutoLock lk(Globals::Mem::mutex);
  tex->image = Globals::Mem::res.alloc<Image>({ici, Image::MEM_NOT_EVICTABLE, dagorFormat, currentVkLayout}, false);
  tex->image->setDerivedHandle(VulkanHandle(tex_res));

  return tex;
}

#if VULKAN_TEXCOPY_SUPPORT > 0

void BaseTex::storeSysCopy(void *data)
{
  int memSz = getSize();
  clear_and_resize(texCopy, sizeof(ddsx::Header) + memSz);

  ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
  if (data)
  {
    memset(&hdr, 0, sizeof(hdr));
    memcpy(texCopy.data() + sizeof(hdr), data, memSz);
  }
  else
    mem_set_0(texCopy);

  hdr.label = _MAKE4C('DDSx');
  hdr.d3dFormat = texfmt_to_d3dformat(pars.flg & TEXFMT_MASK);
  hdr.flags |= ((pars.flg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0) ? hdr.FLG_GAMMA_EQ_1 : 0;
  if (pars.isVolume())
  {
    hdr.flags |= hdr.FLG_VOLTEX;
    hdr.depth = pars.d;
  }
  hdr.w = pars.w;
  hdr.h = pars.h;
  hdr.levels = pars.levels;
  hdr.bitsPerPixel = getFormat().getBytesPerPixelBlock() * 8;
  hdr.memSz = memSz;
  /*sysCopyQualityId*/ hdr.hqPartLevels = 0;

  if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT1)
    hdr.dxtShift = 3;
  else if (hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT3 || hdr.d3dFormat == drv3d_vulkan::D3DFMT_DXT5)
    hdr.dxtShift = 4;
  if (hdr.dxtShift)
    hdr.bitsPerPixel = 0;

  VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", statName, hdr.w, hdr.h, data_size(texCopy));
}

void BaseTex::updateSystexCopyFromCurrentLock()
{
  if (!(pars.flg & TEXCF_SYSTEXCOPY) || !data_size(texCopy) || !lockMsr.ptr)
    return;

  ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
  if (hdr.compressionType())
    return;

  int rpitch = hdr.getSurfacePitch(lockedLevel);
  int h = hdr.getSurfaceScanlines(lockedLevel);
  int d = max(pars.d >> lockedLevel, 1);
  uint8_t *src = (uint8_t *)lockMsr.ptr;
  uint8_t *dest = texCopy.data() + sizeof(ddsx::Header);

  for (int i = 0; i < lockedLevel; i++)
    dest += hdr.getSurfacePitch(i) * hdr.getSurfaceScanlines(i) * max(pars.d >> i, 1);

  D3D_CONTRACT_ASSERT(dest < texCopy.data() + sizeof(ddsx::Header) + hdr.memSz);
  D3D_CONTRACT_ASSERTF(rpitch <= lockMsr.rowPitch && rpitch * h <= lockMsr.slicePitch,
    "%dx%dx%d: tex.pitch=%d,%d copy.pitch=%d,%d, lev=%d", pars.w, pars.h, pars.d, lockMsr.rowPitch, lockMsr.slicePitch, rpitch,
    rpitch * h, lockedLevel);

  for (int di = 0; di < d; di++, src += lockMsr.slicePitch)
    for (int y = 0; y < h; y++, dest += rpitch)
      memcpy(dest, src + y * lockMsr.rowPitch, rpitch);
  VERBOSE_DEBUG("%s %dx%dx%d updated DDSx for TEXCF_SYSTEXCOPY", getName(), hdr.w, hdr.h, hdr.depth, data_size(texCopy));
}

uint8_t *BaseTex::allocSysCopyForDDSX(ddsx::Header &hdr, int quality_id)
{
  if (!(pars.flg & TEXCF_SYSTEXCOPY))
    return nullptr;
  const int dataSz = hdr.packedSz ? hdr.packedSz : hdr.memSz;
  clear_and_resize(texCopy, sizeof(hdr) + dataSz);
  memcpy(texCopy.data(), &hdr, sizeof(hdr));
  /*sysCopyQualityId*/ ((ddsx::Header *)texCopy.data())->hqPartLevels = quality_id;
  return texCopy.data() + sizeof(hdr);
}

#endif

} // namespace drv3d_vulkan
