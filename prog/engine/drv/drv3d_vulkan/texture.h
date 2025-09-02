// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_smallTab.h>
#if _TARGET_PC_WIN
#include <DXGIFormat.h>
#endif
#include <3d/tql.h>
#include <ioSys/dag_memIo.h>
#include <resourceName.h>

#include "image_resource.h"
#include "async_completion_state.h"
#include "globals.h"
#include "translate_d3d_to_vk.h"
#include "basetexture.h"
#include "vk_format_utils.h"

namespace drv3d_vulkan
{

// close to TextureInfo, but not tied to it, as we may store more or instead, less
struct BaseTexParams
{
  uint16_t w;
  uint16_t h;
  uint16_t d;
  uint8_t levels;
  D3DResourceType type;
  int flg;

  void updateLevelsIfNeeded() { levels = count_mips_if_needed(w, h, flg, levels); }
  bool allowSrgbWrite() const { return (flg & TEXCF_SRGBWRITE) != 0; }
  bool allowSrgbRead() const { return (flg & TEXCF_SRGBREAD) != 0; }
  uint32_t getDepthSlices() const { return type == D3DResourceType::VOLTEX ? d : 1; }
  bool isArray() const { return (type == D3DResourceType::ARRTEX) || (type == D3DResourceType::CUBEARRTEX); }
  bool isCubeMap() const { return (type == D3DResourceType::CUBETEX) || (type == D3DResourceType::CUBEARRTEX); }
  bool isVolume() const { return type == D3DResourceType::VOLTEX; }
  bool is2DTex() const { return type == D3DResourceType::TEX; }
  bool isGPUWritable() const { return flg & (TEXCF_RTARGET | TEXCF_UNORDERED); }
  bool isStagingPermanent() const { return flg & (TEXCF_READABLE | TEXCF_SYSMEM | TEXCF_DYNAMIC); }
  bool isStagingDiscardable() const { return flg & TEXCF_DYNAMIC; }

  ImageCreateInfo getImageCreateInfo() const
  {
    ImageCreateInfo ret;
    ret.type = isVolume() ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    ret.size.width = w;
    ret.size.height = h;
    ret.size.depth = getDepthSlices();
    ret.mips = levels;
    ret.arrays = getArrayCount();
    ret.initByTexCreate(flg, isCubeMap());
    return ret;
  }

  uint32_t getArrayCount() const
  {
    switch (type)
    {
      case D3DResourceType::CUBETEX: return 6;
      case D3DResourceType::ARRTEX: return d;
      case D3DResourceType::CUBEARRTEX: return 6 * d;
      default: return 1;
    }
    return 1;
  }

  uint32_t getMipBiasedDepthSlices(uint32_t level) const { return type == D3DResourceType::VOLTEX ? max(d >> level, 1) : 1; }

  VkExtent3D getMipExtent(uint32_t level) const
  {
    VkExtent3D result;
    result.width = max(w >> level, 1);
    result.height = max(h >> level, 1);
    result.depth = getMipBiasedDepthSlices(level);
    return result;
  }

  bool verify() const;
};

struct BaseTex final : public D3dResourceNameImpl<BaseTexture>
{
  /// views
  ImageViewState imageViewCache;

  inline ImageViewState getViewInfoUAV(uint32_t mip, uint32_t layer, bool as_uint) const
  {
    ImageViewState result;
    result.isArray = pars.isArray();
    result.isCubemap = pars.isCubeMap();
    switch (pars.type)
    {
      case D3DResourceType::CUBETEX:
      case D3DResourceType::CUBEARRTEX:
      case D3DResourceType::ARRTEX:
        result.setArrayBase(layer);
        result.setArrayCount(pars.getArrayCount() - layer);
        D3D_CONTRACT_ASSERTF(layer < pars.getArrayCount(),
          "vulkan: UAV view for layer/face %u requested, but texture has only %u layers", layer, pars.getArrayCount());
        break;
      case D3DResourceType::TEX:
      case D3DResourceType::VOLTEX:
        result.setArrayBase(0);
        result.setArrayCount(1);
        D3D_CONTRACT_ASSERTF(layer == 0, "vulkan: UAV view for layer/face %u requested, but texture was 2d/3d and has no layers/faces",
          layer);
        break;
      default: D3D_CONTRACT_ASSERT_FAIL("vulkan: unknown res type for UAV view %u", eastl ::to_underlying(pars.type)); break;
    }
    if (as_uint)
    {
      D3D_CONTRACT_ASSERT(getFormat().getBytesPerPixelBlock() == 4);
      result.setFormat(FormatStore::fromCreateFlags(TEXFMT_R32UI));
    }
    else
      result.setFormat(getFormat().getLinearVariant());

    result.setMipBase(mip);
    result.setMipCount(1);
    result.isRenderTarget = 0;
    result.isUAV = 1;
    return result;
  }

  inline ImageViewState getViewInfoRenderTarget(uint32_t mip, uint32_t layer) const
  {
    ImageViewState result;
    result.isCubemap = 0;
    result.setFormat(pars.allowSrgbWrite() ? getFormat().getSRGBVariant() : getFormat().getLinearVariant());
    if (layer < d3d::RENDER_TO_WHOLE_ARRAY)
    {
      result.setArrayBase(layer);
      result.setArrayCount(1);
      result.isArray = 0;
    }
    else
    {
      result.setArrayBase(0);
      result.setArrayCount(pars.type == D3DResourceType::VOLTEX ? pars.getMipBiasedDepthSlices(mip) : pars.getArrayCount());
      result.isArray = 1;
    }

    result.setMipBase(mip);
    result.setMipCount(1);
    result.isRenderTarget = 1;
    result.isUAV = 0;
    return result;
  }

  void setInitialImageViewState()
  {
    // this fields not gonna change over texture lifetime
    imageViewCache.setFormat(pars.allowSrgbRead() ? getFormat() : getFormat().getLinearVariant());
    imageViewCache.isArray = pars.isArray();
    imageViewCache.isCubemap = pars.isCubeMap();
    imageViewCache.setArrayBase(0);
    imageViewCache.setArrayCount(pars.getArrayCount());
    imageViewCache.isRenderTarget = 0;
    imageViewCache.isUAV = 0;

    // rare changing field, keep it as cache and change on user api request
    imageViewCache.sampleStencil = false;
  }

  inline ImageViewState getViewInfo() const
  {
    ImageViewState ret = imageViewCache;

    // need it here as streaming resource swap have race that can lead to problems if we update cache at swap time
    // (where it should be)
    int32_t baseMip = clamp<int32_t>(maxMipLevel, 0, max(0, (int32_t)pars.levels - 1));
    int32_t mipCount = (minMipLevel - maxMipLevel) + 1;
    if (mipCount <= 0 || baseMip + mipCount > pars.levels)
      mipCount = pars.levels - baseMip;

    ret.setMipBase(baseMip);
    ret.setMipCount(max(mipCount, 1));

    return ret;
  }

  /// sys copy
  struct ImageMem
  {
    void *ptr = nullptr;
    uint32_t rowPitch = 0;
    uint32_t slicePitch = 0;
  };

#if VULKAN_TEXCOPY_SUPPORT > 0
  SmallTab<uint8_t, MidmemAlloc> texCopy; //< ddsx::Header + ddsx data; sysCopyQualityId stored in hdr.hqPartLevel
  void storeSysCopy(void *data);
  void updateSystexCopyFromCurrentLock();
  uint8_t *allocSysCopyForDDSX(ddsx::Header &hdr, int quality_id);
#else
  void storeSysCopy(void *) {}
  void updateSystexCopyFromCurrentLock() {}
  uint8_t *allocSysCopyForDDSX(ddsx::Header &, int) { return nullptr; }
#endif

  /// bindless streaming tracking

  // stub textures don't have unique image, so we can't swap them in bindless slots without additional data
  dag::Vector<uint32_t> bindlessStubSwapQueue;

  void enqueueBindlessStubSwap(uint32_t slot)
  {
    for (uint32_t i : bindlessStubSwapQueue)
    {
      if (i == slot)
        return;
    }
    bindlessStubSwapQueue.push_back(slot);
  }

  void updateBindlessViewsForStreamedTextures();
  void onDeviceReset();
  void afterDeviceReset();

  /// naming
  void updateTexName()
  {
    // don't propagate down to stub images
    if (isStub())
      return;
    if (image)
      Globals::Dbg::naming.setTexName(image, getName());
  }

  void setTexName(const char *name)
  {
    setName(name);
    updateTexName();
  }

  /// underlying resource managment
  void release();
  bool recreate();
  void destroyObject();
  bool allocateImage(BaseTex::ImageMem *initial_data = nullptr);

  /// action-like external callable methods
  void copy(Image *dst);
  void resolve(Image *dst);
  int updateSubRegionInternal(BaseTexture *src, int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_level, int dest_x, int dest_y, int dest_z, bool unordered);

  /// user facing interface
  DECLARE_TQL_TID_AND_STUB()

  void setReadStencil(bool on) override
  {
    imageViewCache.sampleStencil = on && (getFormat().getAspektFlags() & VK_IMAGE_ASPECT_STENCIL_BIT);
  }

  D3DResourceType getType() const override { return pars.type; }
  bool isCubeArray() const override { return getType() == D3DResourceType::CUBEARRTEX; }
  int level_count() const override { return pars.levels; }

  uint32_t getSize() const override
  {
    if (isStub())
      return 0;
    return getFormat().calculateImageSize(pars.w, pars.h, pars.getDepthSlices(), pars.levels) * pars.getArrayCount();
  }

  int getinfo(TextureInfo &info, int level = 0) const override
  {
    level = clamp<int>(level, 0, pars.levels - 1);

    info.w = max<uint32_t>(1u, pars.w >> level);
    info.h = max<uint32_t>(1u, pars.h >> level);
    info.a = pars.getArrayCount();
    info.d = pars.getMipBiasedDepthSlices(level);
    info.mipLevels = pars.levels;
    info.type = pars.type;
    info.cflg = pars.flg;
    return 1;
  }

  int texmiplevel(int minlevel, int maxlevel) override
  {
    uint8_t oldMaxMip = maxMipLevel;
    uint8_t oldMinMip = minMipLevel;
    maxMipLevel = (minlevel >= 0) ? minlevel : 0;
    minMipLevel = (maxlevel >= 0) ? maxlevel : (pars.levels - 1);

    if (usedInBindless && (oldMaxMip != maxMipLevel || oldMinMip != minMipLevel))
      updateBindlessViewsForStreamedTextures();
    return 1;
  }

  bool allocateTex() override { return allocateImage(); }
  void destroy() override;
  bool updateTexResFormat(unsigned d3d_format) override;
  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;
  void discardTex() override
  {
    if (stubTexIdx >= 0)
    {
      release();
      recreate();
    }
  }

  bool setReloadCallback(IReloadData *_rld) override;
  void setApiName(const char *name) const override { Globals::Dbg::naming.setTexName(image, name); }

  int generateMips() override;
  int update(BaseTexture *src) override;
  int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;
  int updateSubRegionNoOrder(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;

  int lockimg(void **p, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override
  {
    return lock(p, stride_bytes, nullptr, level, 0, flags);
  }
  int lockimg(void **p, int &stride_bytes, int face, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override
  {
    return lock(p, stride_bytes, nullptr, level, face, flags);
  }
  int unlockimg() override { return unlock(); }

  int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags = TEXLOCK_DEFAULT) override
  {
    return lock(data, row_pitch, &slice_pitch, level, 0, flags);
  }
  int unlockbox() override { return unlock(); }

  // other
  inline FormatStore getFormat() const { return image ? image->getFormat() : fmt; }
  inline bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->image == image; }
  IReloadData *rld = nullptr;

  Image *image = nullptr;
  BaseTexParams pars;
  FormatStore fmt;
  uint8_t minMipLevel = 0;
  uint8_t maxMipLevel = 0;
  bool usedInBindless = false;
  bool delayedCreate = false;
  bool preallocBeforeLoad = false;
  bool unlockImageUploadSkipped = false;

  BaseTex(BaseTexParams _pars, const char *stat_name) :
    pars(_pars), lockedLevel(0), minMipLevel(20), maxMipLevel(0), lockFlags(0), delayedCreate(false), preallocBeforeLoad(false)
  {
    pars.updateLevelsIfNeeded();
    D3D_CONTRACT_ASSERT(pars.levels > 0);

    fmt = FormatStore::fromCreateFlags(pars.flg);
    maxMipLevel = 0;
    minMipLevel = pars.levels - 1;
    setTexName(stat_name);

    setInitialImageViewState();
  }

  ~BaseTex() { finishReadback(); }

  static Texture *wrapVKImage(VkImage tex_res, ResourceBarrier current_state, int width, int height, int layers, int mips,
    const char *name, int flg);

private:
  bool allocateImageInternal(BaseTex::ImageMem *initial_data);
  void initialImageFill(const ImageCreateInfo &ici, BaseTex::ImageMem *initial_data);

  /// lock related logic
  uint32_t lockFlags = 0;
  uint8_t lockedLevel = 0;
  uint8_t lockedLayer = 0;
  bool lockedSmallStaging = false;
  ImageMem lockMsr;

  int lock(void **p, int &row_stride, int *slice_stride, int mip, int slice, unsigned flags);
  int unlock();
  int clearLockState();
  bool isStagingTemporaryForCurrentLock();
  bool allocateOnLockIfNeeded(unsigned flags);
  uint32_t subresourceOffsetInFullStaging(int mip, int slice);

  /// readback helpers
  AsyncCompletionState readback;
  void finishReadback();
  void startReadback(int mip, int slice, int staging_offset);

  /// staging buffer
  Buffer *stagingBuffer = nullptr;
  AsyncCompletionState gpuUpload;
  void useStagingForWholeTexture(bool temporary = false)
  {
    useStaging(pars.w, pars.h, pars.getDepthSlices(), pars.levels, pars.getArrayCount(), temporary);
  }
  void useStagingForSubresource(int mip, bool temporary = false)
  {
    VkExtent3D mipExtent = pars.getMipExtent(mip);
    useStaging(mipExtent.width, mipExtent.height, mipExtent.depth, 1, 1, temporary);
  }
  void useStaging(int32_t w, int32_t h, int32_t d, int32_t levels, uint16_t arrays, bool temporary = false);
  void destroyStaging();
};

static inline BaseTex *getbasetex(/*const*/ BaseTexture *t) { return t ? (BaseTex *)t : nullptr; }

static inline const BaseTex *getbasetex(const BaseTexture *t) { return t ? (const BaseTex *)t : nullptr; }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }

inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;
TextureInterfaceBase *allocate_texture(uint16_t w, uint16_t h, uint16_t d, uint8_t levels, D3DResourceType type, int flg,
  const char *stat_name);
void free_texture(TextureInterfaceBase *texture);

} // namespace drv3d_vulkan