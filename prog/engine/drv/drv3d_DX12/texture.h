#pragma once

#include <3d/tql.h>
#include <generic/dag_smallTab.h>

namespace ddsx
{
struct Header;
}

namespace drv3d_dx12
{
struct BaseTex;
class Image;
struct D3DTextures
{
  Image *image = nullptr;
  HostDeviceSharedMemoryRegion stagingMemory;
  D3D12_RESOURCE_FLAGS usage = D3D12_RESOURCE_FLAG_NONE;
  uint32_t memSize = 0;
  uint32_t realMipLevels = 0;

  void release(uint64_t progress);
};

struct TextureReplacer
{
  BaseTex *target;
  D3DTextures newTex;

  eastl::pair<Image *, HostDeviceSharedMemoryRegion> update() const;
};
void execute_texture_replace(const TextureReplacer &replacer);
void notify_delete(BaseTex *texture, const eastl::bitset<dxil::MAX_T_REGISTERS> *srvs,
  const eastl::bitset<dxil::MAX_U_REGISTERS> *uavs, eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, bool dsv);
void dirty_srv_no_lock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_srv(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_sampler(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_srv_and_sampler_no_lock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_uav_no_lock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_U_REGISTERS> slots);
void dirty_rendertarget_no_lock(BaseTex *texture, eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> slots);

static constexpr uint32_t MAX_MIPMAPS = 16;
static constexpr uint32_t TEXTURE_TILE_SIZE = 64 * 1024;

struct BaseTex final : public BaseTexture
{
  static constexpr float default_lodbias = 0.f;
  static constexpr int default_aniso = 1;

  int resType = 0;
  D3DTextures tex;

  uint32_t lockFlags = 0;
  uint64_t waitProgress = 0;

  uint32_t cflg = 0;
  FormatStore fmt;

  bool allowSrgbWrite() const { return (cflg & TEXCF_SRGBWRITE) != 0; }
  bool allowSrgbRead() const { return (cflg & TEXCF_SRGBREAD) != 0; }
  bool isRenderTarget() const { return 0 != (cflg & TEXCF_RTARGET); }
  bool isUAV() const { return 0 != (cflg & TEXCF_UNORDERED); }

  virtual void setResApiName(const char *name) const override;

  FormatStore getFormat() const;
  Image *getDeviceImage() const { return tex.image; }
  __forceinline ImageViewState getViewInfoUAV(MipMapIndex mip, ArrayLayerIndex layer, bool as_uint) const
  {
    ImageViewState result;
    if (resType == RES3D_TEX)
    {
      result.isCubemap = 0;
      result.isArray = 0;
      result.setSingleArrayRange(ArrayLayerIndex::make(0));
      G_ASSERTF(layer.index() == 0, "UAV view for layer/face %u requested, but texture was 2d and has no layers/faces", layer);
    }
    else if (resType == RES3D_CUBETEX)
    {
      result.isCubemap = 1;
      result.isArray = 0;
      result.setArrayRange(getArrayCount().front(layer));
      G_ASSERTF(layer < getArrayCount(),
        "UAV view for layer/face %u requested, but texture was cubemap and has only 6 "
        "faces",
        layer);
    }
    else if (resType == RES3D_ARRTEX)
    {
      result.isArray = 1;
      result.isCubemap = isArrayCube();
      result.setArrayRange(getArrayCount().front(layer));
      G_ASSERTF(layer < getArrayCount(), "UAV view for layer/face %u requested, but texture has only %u layers", layer,
        getArrayCount());
    }
    else if (resType == RES3D_VOLTEX)
    {
      result.isArray = 0;
      result.isCubemap = 0;
      result.setDepthLayerRange(0, max<uint16_t>(1, depth >> mip.index()));
      G_ASSERTF(layer.index() == 0, "UAV view for layer/face %u requested, but texture was 3d and has no layers/faces", layer);
    }
    if (as_uint)
    {
      G_ASSERT(getFormat().getBytesPerPixelBlock() == 4 ||
               (getFormat().getBytesPerPixelBlock() == 8 && d3d::get_driver_desc().caps.hasShader64BitIntegerResources));
      if (getFormat().getBytesPerPixelBlock() == 4)
        result.setFormat(FormatStore::fromCreateFlags(TEXFMT_R32UI));
      else if (getFormat().getBytesPerPixelBlock() == 8)
        result.setFormat(FormatStore::fromCreateFlags(TEXFMT_R32G32UI));
    }
    else
    {
      result.setFormat(getFormat().getLinearVariant());
    }
    result.setSingleMipMapRange(mip);
    result.setUAV();
    return result;
  }
  __forceinline ImageViewState getViewInfoRenderTarget(MipMapIndex mip, ArrayLayerIndex layer, bool as_const) const
  {
    FormatStore format = allowSrgbWrite() ? getFormat() : getFormat().getLinearVariant();
    ImageViewState result;
    result.isArray = resType == RES3D_ARRTEX;
    result.isCubemap = resType == RES3D_CUBETEX;
    result.setFormat(format);
    result.setSingleMipMapRange(mip);

    if (layer.index() < d3d::RENDER_TO_WHOLE_ARRAY)
    {
      result.setSingleArrayRange(layer);
    }
    else
    {
      if (RES3D_VOLTEX == resType)
      {
        result.setDepthLayerRange(0, max<uint16_t>(1, depth >> mip.index()));
      }
      else
      {
        result.setArrayRange(getArrayCount());
      }
    }

    if (format.isColor())
    {
      result.setRTV();
    }
    else
    {
      result.setDSV(as_const);
    }

    return result;
  }
  __forceinline ImageViewState getViewInfo() const
  {
    ImageViewState result;
    result.setFormat(allowSrgbRead() ? getFormat() : getFormat().getLinearVariant());
    result.isArray = resType == RES3D_ARRTEX ? 1 : 0;
    result.isCubemap = resType == RES3D_CUBETEX ? 1 : (resType == RES3D_ARRTEX ? int(isArrayCube()) : 0);
    int32_t baseMip = clamp<int32_t>(maxMipLevel, 0, max(0, (int32_t)tex.realMipLevels - 1));
    int32_t mipCount = (minMipLevel - maxMipLevel) + 1;
    if (mipCount <= 0 || baseMip + mipCount > tex.realMipLevels)
    {
      mipCount = tex.realMipLevels - baseMip;
    }
    if (isStub())
    {
      baseMip = 0;
      mipCount = 1;
    }
    result.setMipBase(MipMapIndex::make(baseMip));
    result.setMipCount(max(mipCount, 1));
    result.setArrayRange(getArrayCount());
    result.setSRV();
    result.sampleStencil = sampleStencil();
    return result;
  }

  void updateDeviceSampler();
  D3D12_CPU_DESCRIPTOR_HANDLE getDeviceSampler();

  Extent3D getMipmapExtent(uint32_t level) const
  {
    Extent3D result;
    result.width = max(width >> level, 1);
    result.height = max(height >> level, 1);
    result.depth = resType == RES3D_VOLTEX ? max(depth >> level, 1) : 1;
    return result;
  }

  uint32_t getMemorySize() const { return tex.memSize; }

  struct ReloadDataHandler
  {
    void operator()(IReloadData *rd)
    {
      if (rd)
        rd->destroySelf();
    }
  };

  typedef eastl::unique_ptr<IReloadData, ReloadDataHandler> ReloadDataPointer;

  SamplerState samplerState;
  SamplerState lastSamplerState; //-V730_NOINIT
  D3D12_CPU_DESCRIPTOR_HANDLE sampler{0};
  uint8_t mipLevels = 0;
  uint8_t minMipLevel = 0;
  uint8_t maxMipLevel = 0;
  static constexpr uint32_t active_binding_rtv_offset = 0;
  static constexpr uint32_t active_binding_dsv_offset = Driver3dRenderTarget::MAX_SIMRT + active_binding_rtv_offset;
  static constexpr uint32_t acitve_binding_used_offset = 1 + active_binding_dsv_offset;
  static constexpr uint32_t active_binding_delayed_create_offset = 1 + acitve_binding_used_offset;
  static constexpr uint32_t active_binding_prealloc_before_load_offset = 1 + active_binding_delayed_create_offset;
  static constexpr uint32_t active_binding_sample_stencil = 1 + active_binding_prealloc_before_load_offset;
  static constexpr uint32_t active_binding_is_array_cube_offset = 1 + active_binding_sample_stencil;
  static constexpr uint32_t active_binding_was_copied_to_stage_offset = 1 + active_binding_is_array_cube_offset;
  static constexpr uint32_t active_binding_dirty_rt = 1 + active_binding_was_copied_to_stage_offset;
  static constexpr uint32_t active_binding_bindless_used_offset = 1 + active_binding_dirty_rt;
  static constexpr uint32_t active_binding_state_count = 1 + active_binding_bindless_used_offset;
  eastl::bitset<dxil::MAX_T_REGISTERS> srvBindingStages[STAGE_MAX_EXT];
  eastl::bitset<dxil::MAX_U_REGISTERS> uavBindingStages[STAGE_MAX_EXT];
  eastl::bitset<active_binding_state_count> stateBitSet;

  void notifySamplerChange()
  {
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (srvBindingStages[s].any())
      {
        dirty_sampler(this, s, srvBindingStages[s]);
      }
    }
  }

  void notifySRViewChange()
  {
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (srvBindingStages[s].any())
      {
        dirty_srv(this, s, srvBindingStages[s]);
      }
    }
  }

  void notifyTextureReplaceFinish()
  {
    // if we are ending up here, we are still holding the lock of the state tracker
    // to avoid a deadlock by reentering we have to do the dirtying without a lock
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (srvBindingStages[s].any())
      {
        dirty_srv_and_sampler_no_lock(this, s, srvBindingStages[s]);
      }
    }
  }

  void dirtyBoundSRVsNoLock()
  {
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (srvBindingStages[s].any())
      {
        dirty_srv_no_lock(this, s, srvBindingStages[s]);
      }
    }
  }

  void dirtyBoundUAVsNoLock()
  {
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (uavBindingStages[s].any())
      {
        dirty_uav_no_lock(this, s, uavBindingStages[s]);
      }
    }
  }

  void dirtyBoundRTVsNoLock() { dirty_rendertarget_no_lock(this, getRTVBinding()); }

  void setUAVBinding(uint32_t stage, uint32_t index, bool s)
  {
    uavBindingStages[stage].set(index, s);
    stateBitSet.set(acitve_binding_used_offset);
    if (s)
    {
      stateBitSet.reset(active_binding_was_copied_to_stage_offset);
    }
  }

  void setSRVBinding(uint32_t stage, uint32_t index, bool s)
  {
    srvBindingStages[stage].set(index, s);
    stateBitSet.set(acitve_binding_used_offset);
  }

  void setRTVBinding(uint32_t index, bool s)
  {
    G_ASSERT(index < Driver3dRenderTarget::MAX_SIMRT);
    stateBitSet.set(active_binding_rtv_offset + index, s);
    stateBitSet.set(acitve_binding_used_offset);
    if (s)
    {
      stateBitSet.reset(active_binding_was_copied_to_stage_offset);
      stateBitSet.set(active_binding_dirty_rt);
    }
  }

  void setDSVBinding(bool s)
  {
    stateBitSet.set(active_binding_dsv_offset, s);
    stateBitSet.set(acitve_binding_used_offset);
    if (s)
    {
      stateBitSet.reset(active_binding_was_copied_to_stage_offset);
      stateBitSet.set(active_binding_dirty_rt);
    }
  }
  eastl::bitset<dxil::MAX_U_REGISTERS> getUAVBinding(uint32_t stage) const { return uavBindingStages[stage]; }
  eastl::bitset<dxil::MAX_T_REGISTERS> getSRVBinding(uint32_t stage) const { return srvBindingStages[stage]; }
  eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> getRTVBinding() const
  {
    eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> ret;
    ret.from_uint64((stateBitSet >> active_binding_rtv_offset).to_uint64());
    return ret;
  }
  bool getDSVBinding() const { return stateBitSet.test(active_binding_dsv_offset); }
  bool wasUsed() const { return stateBitSet.test(acitve_binding_used_offset); }
  void setWasUsed() { stateBitSet.set(acitve_binding_used_offset); }
  bool delayedCreate() const { return stateBitSet.test(active_binding_delayed_create_offset); }
  void setDelayedCreate(bool s) { stateBitSet.set(active_binding_delayed_create_offset, s); }
  bool preallocBeforeLoad() const { return stateBitSet.test(active_binding_prealloc_before_load_offset); }
  void setPreallocBeforeLoad(bool s) { stateBitSet.set(active_binding_prealloc_before_load_offset, s); }

  bool sampleStencil() const { return stateBitSet.test(active_binding_sample_stencil); }

  void setSampleStencil(bool s) { stateBitSet.set(active_binding_sample_stencil, s); }

  bool isArrayCube() const { return stateBitSet.test(active_binding_is_array_cube_offset); }

  void setIsArrayCube(bool s) { stateBitSet.set(active_binding_is_array_cube_offset, s); }

  bool wasCopiedToStage() const { return stateBitSet.test(active_binding_was_copied_to_stage_offset); }
  void setWasCopiedToStage(bool s) { stateBitSet.set(active_binding_was_copied_to_stage_offset, s); }

  bool dirtyRt() const { return stateBitSet.test(active_binding_dirty_rt); }
  void setDirtyRty(bool s) { stateBitSet.set(active_binding_dirty_rt, s); }

  void setUsedWithBindless()
  {
    stateBitSet.set(active_binding_bindless_used_offset);
    stateBitSet.set(acitve_binding_used_offset);
  }

  bool wasUsedWithBindless() const { return stateBitSet.test(active_binding_bindless_used_offset); }

  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t depth = 0;
  // uint16_t layers = 0;

  uint32_t lockedSubRes = 0;

  SmallTab<uint8_t, MidmemAlloc> texCopy; //< ddsx::Header + ddsx data; sysCopyQualityId stored in hdr.hqPartLevel
  ReloadDataPointer rld;

  struct ImageMem
  {
    void *ptr = nullptr;
    uint32_t rowPitch = 0;
    uint32_t slicePitch = 0;
    uint32_t memSize = 0;
  };

  ImageMem lockMsr;

  DECLARE_TQL_TID_AND_STUB()

  void setParams(int w, int h, int d, int levels, const char *stat_name);

  ArrayLayerCount getArrayCount() const
  {
    if (resType == RES3D_CUBETEX)
    {
      return ArrayLayerCount::make(6);
    }
    else if (resType == RES3D_ARRTEX)
    {
      return ArrayLayerCount::make((isArrayCube() ? 6 : 1) * depth);
    }
    return ArrayLayerCount::make(1);
  }

  uint32_t getDepthSlices() const
  {
    if (resType == RES3D_VOLTEX)
      return depth;
    return 1;
  }

  BaseTex(int res_type, uint32_t cflg_) :
    resType(res_type), cflg(cflg_), minMipLevel(20), maxMipLevel(0), lockFlags(0), depth(1), width(0), height(0)
  {
    samplerState.setBias(default_lodbias);
    samplerState.setAniso(default_aniso);
    samplerState.setW(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    samplerState.setV(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    samplerState.setU(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    samplerState.setMip(D3D12_FILTER_TYPE_LINEAR);
    samplerState.setFilter(D3D12_FILTER_TYPE_LINEAR);

    if (RES3D_CUBETEX == resType)
    {
      samplerState.setV(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
      samplerState.setU(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    }
  }

  ~BaseTex() override;

  /// ->>
  void setReadStencil(bool on) override { setSampleStencil(on && getFormat().isStencil()); }
  int restype() const override { return resType; }
  int ressize() const override;

  int getinfo(TextureInfo &ti, int level = 0) const override
  {
    level = clamp<int>(level, 0, mipLevels - 1);

    ti.w = max<uint32_t>(1u, width >> level);
    ti.h = max<uint32_t>(1u, height >> level);
    switch (resType)
    {
      case RES3D_CUBETEX:
        ti.d = 1;
        ti.a = 6;
        break;
      case RES3D_CUBEARRTEX:
      case RES3D_ARRTEX:
        ti.d = 1;
        ti.a = getArrayCount().count();
        break;
      case RES3D_VOLTEX:
        ti.d = max<uint32_t>(1u, getDepthSlices() >> level);
        ti.a = 1;
        break;
      default:
        ti.d = 1;
        ti.a = 1;
        break;
    }

    ti.mipLevels = mipLevels;
    ti.resType = resType;
    ti.cflg = cflg;
    return 1;
  }

  bool addDirtyRect(const RectInt &) override { return true; }

  int level_count() const override { return mipLevels; }

  int texaddr(int a) override
  {
    samplerState.setW(translate_texture_address_mode_to_dx12(a));
    samplerState.setV(translate_texture_address_mode_to_dx12(a));
    samplerState.setU(translate_texture_address_mode_to_dx12(a));
    notifySamplerChange();
    return 1;
  }

  int texaddru(int a) override
  {
    samplerState.setU(translate_texture_address_mode_to_dx12(a));
    notifySamplerChange();
    return 1;
  }

  int texaddrv(int a) override
  {
    samplerState.setV(translate_texture_address_mode_to_dx12(a));
    notifySamplerChange();
    return 1;
  }

  int texaddrw(int a) override
  {
    if (RES3D_VOLTEX == resType)
    {
      samplerState.setW(translate_texture_address_mode_to_dx12(a));
      notifySamplerChange();
      return 1;
    }
    return 0;
  }

  int texbordercolor(E3DCOLOR c) override
  {
    samplerState.setBorder(c);
    notifySamplerChange();
    return 1;
  }

  int texfilter(int m) override
  {
    samplerState.isCompare = m == TEXFILTER_COMPARE;
    samplerState.setFilter(translate_filter_type_to_dx12(m));
    notifySamplerChange();
    return 1;
  }

  int texmipmap(int m) override
  {
    samplerState.setMip(translate_mip_filter_type_to_dx12(m));
    notifySamplerChange();
    return 1;
  }

  int texlod(float mipmaplod) override
  {
    samplerState.setBias(mipmaplod);
    notifySamplerChange();
    return 1;
  }

  int texmiplevel(int minlevel, int maxlevel) override
  {
    maxMipLevel = (minlevel >= 0) ? minlevel : 0;
    minMipLevel = (maxlevel >= 0) ? maxlevel : (mipLevels - 1);
    notifySRViewChange();
    return 1;
  }

  int setAnisotropy(int level) override
  {
    samplerState.setAniso(clamp<int>(level, 1, 16));
    notifySamplerChange();
    return 1;
  }

  int generateMips() override;

  bool setReloadCallback(IReloadData *_rld) override
  {
    setRld(_rld);
    return true;
  }

  void resetTex();
  void releaseTex();

  int lockimg(void **, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int lockimg(void **, int &stride_bytes, int face, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int update(BaseTexture *src) override;
  int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;
  int unlockimg() override;
  int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags = TEXLOCK_DEFAULT) override;
  int unlockbox() override;
  void destroy() override;

  void setRld(BaseTexture::IReloadData *_rld) { rld.reset(_rld); }

  void release() { releaseTex(); }
  bool recreate();

  void destroyObject();

  void updateTexName();

  void setTexName(const char *name)
  {
    setResName(name);
    updateTexName();
  }

  bool allocateTex() override;
  void discardTex() override
  {
    if (stubTexIdx >= 0)
    {
      releaseTex();
      recreate();
    }
  }
  bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->tex.image == tex.image; }
  bool isCubeArray() const { return isArrayCube(); }

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  bool downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;
  bool upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;

  static uint32_t update_flags_for_linear_layout(uint32_t cflags, FormatStore format);
  static DeviceMemoryClass get_memory_class(uint32_t cflags);
};

static inline BaseTex *getbasetex(/*const*/ BaseTexture *t) { return t ? (BaseTex *)t : nullptr; }

static inline const BaseTex *getbasetex(const BaseTexture *t) { return t ? (const BaseTex *)t : nullptr; }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }

inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;

} // namespace drv3d_dx12