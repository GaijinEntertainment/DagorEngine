#pragma once
#include "device.h"
#include <util/dag_globDef.h>
#include <generic/dag_smallTab.h>
#include <3d/ddsxTex.h>
#if _TARGET_PC_WIN
#include <DXGIFormat.h>
#endif
#include "generic/dag_tabExt.h"
#include <3d/tql.h>

namespace ddsx
{
struct Header;
}

namespace drv3d_vulkan
{
struct BaseTex;

class ExecutionContext;

struct BaseTex final : public BaseTexture
{
  static constexpr float default_lodbias = 0.f;
  static constexpr int default_aniso = 1;

  int resType = 0;

  struct D3DTextures
  {
    Image *image = nullptr;
    Image *renderTarget3DEmul = nullptr;
    Buffer *stagingBuffer = nullptr;
    uint32_t memSize = 0;
    uint32_t realMipLevels = 0;

    void release(AsyncCompletionState &event);
    void releaseDelayed(AsyncCompletionState &event);
    void useStaging(FormatStore fmt, int32_t w, int32_t h, int32_t d, int32_t levels, uint16_t arrays, bool temporary = false);
    void destroyStaging();
  } tex;

  uint32_t mappedSubresource = 0;
  uint32_t lockFlags = 0;
  AsyncCompletionState waitEvent;

  uint32_t cflg = 0;
  FormatStore fmt;

  inline bool allowSrgbWrite() const { return (cflg & TEXCF_SRGBWRITE) != 0; }
  inline bool allowSrgbRead() const { return (cflg & TEXCF_SRGBREAD) != 0; }

  inline FormatStore getFormat() const { return tex.image ? tex.image->getFormat() : fmt; }
  inline Image *getDeviceImage(bool render_target) const
  {
    if (resType == RES3D_VOLTEX && tex.renderTarget3DEmul)
    {
      return render_target ? tex.renderTarget3DEmul : tex.image;
    }
    else
    {
      return tex.image;
    }
  }
  inline ImageViewState getViewInfoUAV(uint32_t mip, uint32_t layer, bool as_uint) const
  {
    ImageViewState result;
    if (resType == RES3D_TEX)
    {
      result.isCubemap = 0;
      result.isArray = 0;
      result.setArrayBase(0);
      result.setArrayCount(1);
      G_ASSERTF(layer == 0, "UAV view for layer/face %u requested, but texture was 2d and has no layers/faces", layer);
    }
    else if (resType == RES3D_CUBETEX)
    {
      result.isCubemap = 1;
      result.isArray = 0;
      result.setArrayBase(layer);
      result.setArrayCount(6 - layer);
      G_ASSERTF(layer < 6,
        "UAV view for layer/face %u requested, but texture was cubemap and has only 6 "
        "faces",
        layer);
    }
    else if (resType == RES3D_ARRTEX)
    {
      result.isArray = 1;
      result.isCubemap = isArrayCube;
      result.setArrayBase(layer);
      result.setArrayCount(getArrayCount() - layer);
      G_ASSERTF(layer < getArrayCount(), "UAV view for layer/face %u requested, but texture has only %u layers", layer,
        getArrayCount());
    }
    else if (resType == RES3D_VOLTEX)
    {
      result.isArray = 0;
      result.isCubemap = 0;
      result.setArrayBase(0);
      result.setArrayCount(1);
      G_ASSERTF(layer == 0, "UAV view for layer/face %u requested, but texture was 3d and has no layers/faces", layer);
    }
    if (as_uint)
    {
      G_ASSERT(getFormat().getBytesPerPixelBlock() == 4);
      result.setFormat(FormatStore::fromCreateFlags(TEXFMT_R32UI));
    }
    else
    {
      result.setFormat(getFormat().getLinearVariant());
    }
    result.setMipBase(mip);
    result.setMipCount(1);
    result.isRenderTarget = 0;
    return result;
  }
  inline ImageViewState getViewInfoRenderTarget(uint32_t mip, uint32_t layer) const
  {
    ImageViewState result;
    result.isArray = 0;
    result.isCubemap = 0;
    result.setFormat(allowSrgbWrite() ? getFormat() : getFormat().getLinearVariant());
    bool renderToWholeArray = layer >= d3d::RENDER_TO_WHOLE_ARRAY;
    result.setMipBase(mip);
    result.setMipCount(1);
    result.setArrayBase(renderToWholeArray ? 0 : layer);
    if (renderToWholeArray)
    {
      result.isArray = 1;
      switch (resType)
      {
        case RES3D_CUBETEX: result.setArrayCount(6); break;
        case RES3D_ARRTEX: result.setArrayCount(depth); break;
        case RES3D_VOLTEX: result.setArrayCount(max<uint32_t>(1u, depth >> mip)); break;
        default: result.setArrayCount(1); break;
      }
    }
    else
      result.setArrayCount(1);
    result.isRenderTarget = 1;
    return result;
  }
  inline ImageViewState getViewInfo() const
  {
    ImageViewState ret = imageViewCache;

    // need it here as streaming resource swap have race that can lead to problems if we update cache at swap time
    // (where it should be)
    int32_t baseMip = clamp<int32_t>(maxMipLevel, 0, max(0, (int32_t)tex.realMipLevels - 1));
    int32_t mipCount = (minMipLevel - maxMipLevel) + 1;
    if (mipCount <= 0 || baseMip + mipCount > tex.realMipLevels)
      mipCount = tex.realMipLevels - baseMip;

    ret.setMipBase(baseMip);
    ret.setMipCount(max(mipCount, 1));

    return ret;
  }

  inline static VkBorderColor remap_border_color(E3DCOLOR color, bool as_float)
  {
    bool isBlack = (color.r == 0) && (color.g == 0) && (color.b == 0);
    bool isTransparent = color.a == 0;
    static const VkBorderColor colorTable[] = //
      {VK_BORDER_COLOR_INT_OPAQUE_WHITE, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK};
    return colorTable[int(as_float) | (int(isBlack) << 1) | (int(isTransparent) << 2)];
  }

  void rebindTRegs(SamplerState new_sampler) { samplerState = new_sampler; }

  inline void setUsedAsRenderTarget() { dirtyRt = 1; }

  inline Image *getResolveTarget() const
  {
    if (resType == RES3D_VOLTEX)
      return tex.image;
    return nullptr;
  }

  inline VkExtent3D getMipmapExtent(uint32_t level) const
  {
    VkExtent3D result;
    result.width = max(width >> level, 1);
    result.height = max(height >> level, 1);
    result.depth = resType == RES3D_VOLTEX ? max(depth >> level, 1) : 1;
    return result;
  }

  inline uint32_t getMemorySize() const { return tex.memSize; }

  ImageViewState imageViewCache;
  SamplerState samplerState;
  uint8_t mipLevels = 0;
  uint8_t minMipLevel = 0;
  uint8_t maxMipLevel = 0;
  bool delayedCreate = false;
  bool preallocBeforeLoad = false;
  bool isArrayCube = false;
  bool unlockImageUploadSkipped = false;

  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t depth = 0;
  // uint16_t layers = 0;

  uint8_t lockedLevel = 0;
  uint8_t lockedLayer = 0;

  SmallTab<uint8_t, MidmemAlloc> texCopy; //< ddsx::Header + ddsx data; sysCopyQualityId stored in hdr.hqPartLevel

  struct ImageMem
  {
    void *ptr = nullptr;
    uint32_t rowPitch = 0;
    uint32_t slicePitch = 0;
    uint32_t memSize = 0;
  };

  char *debugLockData = nullptr;
  ImageMem lockMsr;

  uint32_t dirtyRt : 6;
  DECLARE_TQL_TID_AND_STUB()

  void setParams(int w, int h, int d, int levels, const char *stat_name);

  inline uint32_t getArrayCount() const
  {
    if (resType == RES3D_CUBETEX)
      return 6;
    else if (resType == RES3D_ARRTEX)
      return (isArrayCube ? 6 : 1) * depth;
    return 1;
  }

  inline uint32_t getDepthSlices() const
  {
    if (resType == RES3D_VOLTEX)
      return depth;
    return 1;
  }

  inline void setInitialImageViewState()
  {
    // this fields not gonna change over texture lifetime
    imageViewCache.setFormat(allowSrgbRead() ? getFormat() : getFormat().getLinearVariant());
    imageViewCache.isArray = resType == RES3D_ARRTEX ? 1 : 0;
    imageViewCache.isCubemap = resType == RES3D_CUBETEX ? 1 : (resType == RES3D_ARRTEX ? int(isArrayCube) : 0);
    imageViewCache.setArrayBase(0);
    imageViewCache.setArrayCount(getArrayCount());
    imageViewCache.isRenderTarget = 0;

    // rare changing field, keep it as cache and change on user api request
    imageViewCache.sampleStencil = false;
  }

  BaseTex(int res_type, uint32_t cflg_) :
    resType(res_type),
    cflg(cflg_),
    mipLevels(0),
    lockedLevel(0),
    minMipLevel(20),
    maxMipLevel(0),
    mappedSubresource(0),
    lockFlags(0),
    delayedCreate(false),
    debugLockData(nullptr),
    dirtyRt(0),
    depth(1),
    width(0),
    height(0)
  {
    samplerState.setBias(default_lodbias);
    samplerState.setAniso(default_aniso);
    samplerState.setW(translate_texture_address_mode_to_vulkan(TEXADDR_WRAP));
    samplerState.setV(translate_texture_address_mode_to_vulkan(TEXADDR_WRAP));
    samplerState.setU(translate_texture_address_mode_to_vulkan(TEXADDR_WRAP));
    samplerState.setMip(VK_SAMPLER_MIPMAP_MODE_LINEAR);
    samplerState.setFilter(VK_FILTER_LINEAR);

    preallocBeforeLoad = false;

    if (RES3D_CUBETEX == resType)
    {
      texaddru(TEXADDR_CLAMP);
      texaddrv(TEXADDR_CLAMP);
    }

    setInitialImageViewState();
  }

  ~BaseTex() override
  {
    Device &device = get_device();
    if (waitEvent.isPending())
      device.getContext().wait();
  }

  /// ->>
  void setReadStencil(bool on) override
  {
    imageViewCache.sampleStencil = on && (getFormat().getAspektFlags() & VK_IMAGE_ASPECT_STENCIL_BIT);
  }
  int restype() const override { return resType; }
  int ressize() const override
  {
    if (isStub())
      return 0;
    const uint32_t w = width;
    const uint32_t h = height;
    const uint32_t d = getDepthSlices();
    const uint32_t a = getArrayCount();
    return getFormat().calculateImageSize(w, h, d, mipLevels) * a;
  }

  int getinfo(TextureInfo &info, int level = 0) const override
  {
    level = clamp<int>(level, 0, mipLevels - 1);

    info.w = max<uint32_t>(1u, width >> level);
    info.h = max<uint32_t>(1u, height >> level);
    switch (resType)
    {
      case RES3D_CUBETEX:
        info.d = 1;
        info.a = 6;
        break;
      case RES3D_CUBEARRTEX:
      case RES3D_ARRTEX:
        info.d = 1;
        info.a = getArrayCount();
        break;
      case RES3D_VOLTEX:
        info.d = max<uint32_t>(1u, getDepthSlices() >> level);
        info.a = 1;
        break;
      default:
        info.d = 1;
        info.a = 1;
        break;
    }

    info.mipLevels = mipLevels;
    info.resType = resType;
    info.cflg = cflg;
    return 1;
  }

  bool addDirtyRect(const RectInt &) override { return true; }

  int level_count() const override { return mipLevels; }

  int texaddr(int a) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setW(translate_texture_address_mode_to_vulkan(a));
    newSampler.setV(translate_texture_address_mode_to_vulkan(a));
    newSampler.setU(translate_texture_address_mode_to_vulkan(a));
    rebindTRegs(newSampler);
    return 1;
  }

  int texaddru(int a) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setU(translate_texture_address_mode_to_vulkan(a));
    rebindTRegs(newSampler);
    return 1;
  }

  int texaddrv(int a) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setV(translate_texture_address_mode_to_vulkan(a));
    rebindTRegs(newSampler);
    return 1;
  }

  int texaddrw(int a) override
  {
    if (RES3D_VOLTEX == resType)
    {
      SamplerState newSampler = samplerState;
      newSampler.setW(translate_texture_address_mode_to_vulkan(a));
      rebindTRegs(newSampler);
      return 1;
    }
    return 0;
  }

  int texbordercolor(E3DCOLOR c) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setBorder(remap_border_color(c, getFormat().isSampledAsFloat()));
    rebindTRegs(newSampler);
    return 1;
  }

  int texfilter(int m) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setFilter(translate_filter_type_to_vulkan(m));
    newSampler.setIsCompare(m == TEXFILTER_COMPARE);
    rebindTRegs(newSampler);
    return 1;
  }

  int texmipmap(int m) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setMip(translate_mip_filter_type_to_vulkan(m));
    rebindTRegs(newSampler);
    return 1;
  }

  int texlod(float mipmaplod) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setBias(mipmaplod);
    rebindTRegs(newSampler);
    return 1;
  }

  int texmiplevel(int minlevel, int maxlevel) override
  {
    maxMipLevel = (minlevel >= 0) ? minlevel : 0;
    minMipLevel = (maxlevel >= 0) ? maxlevel : (mipLevels - 1);

    // workaround fact that sampler state does not contains min/map mip
    rebindTRegs(samplerState);
    return 1;
  }

  int setAnisotropy(int level) override
  {
    SamplerState newSampler = samplerState;
    newSampler.setAniso(clamp<int>(level, 1, 16));
    rebindTRegs(newSampler);
    return 1;
  }

  int generateMips() override;

  bool setReloadCallback(IReloadData *) override { return false; }

  void releaseTex();
  void swapInnerTex(D3DTextures &new_tex);
  void preload() {}

  int lockimg(void **, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int lockimg(void **, int &stride_bytes, int face, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int update(BaseTexture *src) override;
  int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;
  int unlockimg() override;
  int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags = TEXLOCK_DEFAULT) override;
  int unlockbox() override;
  void destroy() override;

  /// <<-

  void release() { releaseTex(); }
  bool recreate();

  void copy(Image *dst);
  void resolve(Image *dst);
  int updateSubRegionInternal(BaseTex *src, int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_level, int dest_x, int dest_y, int dest_z);

  void destroyObject();

  void updateTexName()
  {
    // don't propagate down to stub images
    if (isStub())
      return;
    if (tex.image)
    {
      get_device().setTexName(tex.image, getResName());
    }
    if (tex.renderTarget3DEmul)
    {
      String emulName(getResName());
      emulName += "_3d_emulation_helper";
      get_device().setTexName(tex.renderTarget3DEmul, emulName);
    }
  }

  void setTexName(const char *name)
  {
    setResName(name);
    updateTexName();
  }

  virtual void setResApiName(const char *name) const override { get_device().setTexName(tex.image, name); }

  bool allocateTex() override;
  void discardTex() override
  {
    if (stubTexIdx >= 0)
    {
      releaseTex();
      recreate();
    }
  }
  inline bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->tex.image == tex.image; }
  bool isCubeArray() const { return isArrayCube; }

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  void blockingReadbackWait();
};

static inline BaseTex *getbasetex(/*const*/ BaseTexture *t) { return t ? (BaseTex *)t : nullptr; }

static inline const BaseTex *getbasetex(const BaseTexture *t) { return t ? (const BaseTex *)t : nullptr; }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }

inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;
TextureInterfaceBase *allocate_texture(int res_type, uint32_t cflg);
void free_texture(TextureInterfaceBase *texture);

} // namespace drv3d_vulkan