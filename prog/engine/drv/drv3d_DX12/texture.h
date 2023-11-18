#pragma once

#include <EASTL/bitset.h>
#include <3d/tql.h>
#include <3d/dag_drv3d.h>
#include <generic/dag_smallTab.h>
#include <dxil/compiled_shader_header.h>

#include "device_memory_class.h"
#include "host_device_shared_memory_region.h"
#include "format_store.h"
#include "extents.h"
#include "image_view_state.h"
#include "sampler_state.h"


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

inline constexpr uint32_t MAX_MIPMAPS = 16;
inline constexpr uint32_t TEXTURE_TILE_SIZE = 64 * 1024;

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
  bool isMultisampled() const { return 0 != (cflg & TEXCF_SAMPLECOUNT_MASK); }

  virtual void setResApiName(const char *name) const override;

  void resolve(Image *dst);

  FormatStore getFormat() const;
  Image *getDeviceImage() const { return tex.image; }
  ImageViewState getViewInfoUAV(MipMapIndex mip, ArrayLayerIndex layer, bool as_uint) const;
  ImageViewState getViewInfoRenderTarget(MipMapIndex mip, ArrayLayerIndex layer, bool as_const) const;
  ImageViewState getViewInfo() const;

  void updateDeviceSampler();
  D3D12_CPU_DESCRIPTOR_HANDLE getDeviceSampler();

  Extent3D getMipmapExtent(uint32_t level) const;
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

  void notifySamplerChange();
  void notifySRViewChange();
  void notifyTextureReplaceFinish();
  void dirtyBoundSRVsNoLock();
  void dirtyBoundUAVsNoLock();
  void dirtyBoundRTVsNoLock() { dirty_rendertarget_no_lock(this, getRTVBinding()); }
  void setUAVBinding(uint32_t stage, uint32_t index, bool s);
  void setSRVBinding(uint32_t stage, uint32_t index, bool s);
  void setRTVBinding(uint32_t index, bool s);
  void setDSVBinding(bool s);
  eastl::bitset<dxil::MAX_U_REGISTERS> getUAVBinding(uint32_t stage) const { return uavBindingStages[stage]; }
  eastl::bitset<dxil::MAX_T_REGISTERS> getSRVBinding(uint32_t stage) const { return srvBindingStages[stage]; }
  eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> getRTVBinding() const;
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
  void setUsedWithBindless();
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

  ArrayLayerCount getArrayCount() const;

  uint32_t getDepthSlices() const
  {
    if (resType == RES3D_VOLTEX)
      return depth;
    return 1;
  }

  BaseTex(int res_type, uint32_t cflg_);
  ~BaseTex() override;

  /// ->>
  void setReadStencil(bool on) override { setSampleStencil(on && getFormat().isStencil()); }
  int restype() const override { return resType; }
  int ressize() const override;
  int getinfo(TextureInfo &ti, int level = 0) const override;
  bool addDirtyRect(const RectInt &) override { return true; }
  int level_count() const override { return mipLevels; }

  int texaddr(int a) override;
  int texaddru(int a) override;
  int texaddrv(int a) override;
  int texaddrw(int a) override;
  int texbordercolor(E3DCOLOR c) override;
  int texfilter(int m) override;
  int texmipmap(int m) override;
  int texlod(float mipmaplod) override;
  int texmiplevel(int minlevel, int maxlevel) override;
  int setAnisotropy(int level) override;

  int generateMips() override;

  bool setReloadCallback(IReloadData *_rld) override;

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
  void discardTex() override;

  bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->tex.image == tex.image; }
  bool isCubeArray() const { return isArrayCube(); }

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  bool downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;
  bool upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;

  static uint32_t update_flags_for_linear_layout(uint32_t cflags, FormatStore format);
  static DeviceMemoryClass get_memory_class(uint32_t cflags);
};

static inline BaseTex *getbasetex(BaseTexture *t) { return t ? (BaseTex *)t : nullptr; }

static inline const BaseTex *getbasetex(const BaseTexture *t) { return t ? (const BaseTex *)t : nullptr; }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }

inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;

} // namespace drv3d_dx12