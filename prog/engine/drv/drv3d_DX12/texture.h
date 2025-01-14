// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "device_memory_class.h"
#include "extents.h"
#include "format_store.h"
#include "host_device_shared_memory_region.h"
#include "image_view_state.h"
#include "sampler_state.h"

#include <3d/tql.h>
#include <drv/3d/dag_driver.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <generic/dag_bitset.h>
#include <generic/dag_smallTab.h>

namespace drv3d_dx12
{
class BaseTex;
class Image;

void notify_delete(BaseTex *texture, const Bitset<dxil::MAX_T_REGISTERS> *srvs, const Bitset<dxil::MAX_U_REGISTERS> *uavs,
  Bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, bool dsv);
void mark_texture_stages_dirty_no_lock(BaseTex *texture, const Bitset<dxil::MAX_T_REGISTERS> *srvs,
  const Bitset<dxil::MAX_U_REGISTERS> *uavs, Bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, bool dsv);
void dirty_srv_no_lock(BaseTex *texture, uint32_t stage, Bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_srv(BaseTex *texture, uint32_t stage, Bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_sampler(BaseTex *texture, uint32_t stage, Bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_srv_and_sampler_no_lock(BaseTex *texture, uint32_t stage, Bitset<dxil::MAX_T_REGISTERS> slots);
void dirty_uav_no_lock(BaseTex *texture, uint32_t stage, Bitset<dxil::MAX_U_REGISTERS> slots);
void dirty_rendertarget_no_lock(BaseTex *texture, Bitset<Driver3dRenderTarget::MAX_SIMRT> slots);

class BaseTex final : public BaseTexture
{
public:
  bool isRenderTarget() const { return 0 != (cflg & TEXCF_RTARGET); }
  bool isUav() const { return 0 != (cflg & TEXCF_UNORDERED); }
  bool isMultisampled() const { return 0 != (cflg & TEXCF_SAMPLECOUNT_MASK); }

  bool isLocked() const { return lockFlags != 0; }
  bool isLockedNoCopy() const { return (lockFlags & ~(TEX_COPIED | TEXLOCK_COPY_STAGING)) != 0; }

  void setResApiName(const char *name) const override;

  FormatStore getFormat() const;
  Image *getDeviceImage() const { return image; }
  ImageViewState getViewInfoUav(MipMapIndex mip, ArrayLayerIndex layer, bool as_uint) const;
  ImageViewState getViewInfoRenderTarget(MipMapIndex mip, ArrayLayerIndex layer, bool as_const) const;
  ImageViewState getViewInfo() const;

  void updateDeviceSampler();
  D3D12_CPU_DESCRIPTOR_HANDLE getDeviceSampler();

  Extent3D getMipmapExtent(uint32_t level) const;

  void notifySamplerChange();
  void notifySrvChange();
  void dirtyBoundSrvsNoLock();
  void dirtyBoundUavsNoLock();
  void dirtyBoundRtvsNoLock() { dirty_rendertarget_no_lock(this, getRtvBinding()); }

  void setUavBinding(uint32_t stage, uint32_t index, bool s);
  void setSrvBinding(uint32_t stage, uint32_t index, bool s);
  void setRtvBinding(uint32_t index, bool s);
  void setDsvBinding(bool s);
  Bitset<Driver3dRenderTarget::MAX_SIMRT> getRtvBinding() const;
  bool getDsvBinding() const { return stateBitSet.test(active_binding_dsv_offset); }
#if DAGOR_DBGLEVEL > 0
  bool wasUsed() const { return stateBitSet.test(active_binding_was_used_offset); }
  void setWasUsed() { stateBitSet.set(active_binding_was_used_offset); }
#endif
  bool isPreallocBeforeLoad() const { return stateBitSet.test(active_binding_is_prealloc_before_load_offset); }
  void setIsPreallocBeforeLoad(bool s) { stateBitSet.set(active_binding_is_prealloc_before_load_offset, s); }
  bool isSampleStencil() const { return stateBitSet.test(active_binding_is_sample_stencil); }
  void setIsSampleStencil(bool s) { stateBitSet.set(active_binding_is_sample_stencil, s); }
  bool isArrayCube() const { return stateBitSet.test(active_binding_is_array_cube_offset); }
  void setIsArrayCube(bool s) { stateBitSet.set(active_binding_is_array_cube_offset, s); }

  DECLARE_TQL_TID_AND_STUB()

  void setParams(int w, int h, int d, int levels, const char *stat_name);

  ArrayLayerCount getArrayCount() const;

  BaseTex(int res_type, uint32_t cflg_);

  /// ->>
  void setReadStencil(bool on) override { setIsSampleStencil(on && getFormat().isStencil()); }
  int restype() const override { return resType; }
  int ressize() const override;
  int getinfo(TextureInfo &ti, int level = 0) const override;
  int level_count() const override { return mipLevels; }

  int texmiplevel(int minlevel, int maxlevel) override;

  int generateMips() override;

  bool setReloadCallback(IReloadData *_rld) override;

  void reset();

  void updateTexName();

  int lockimg(void **, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int lockimg(void **, int &stride_bytes, int face, int level = 0, unsigned flags = TEXLOCK_DEFAULT) override;
  int update(BaseTexture *src) override;
  int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;
  int unlockimg() override;
  int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags = TEXLOCK_DEFAULT) override;
  int unlockbox() override;
  void destroy() override;

  bool recreate();

  bool allocateTex() override;
  void discardTex() override;

  bool isCubeArray() const override { return isArrayCube(); }

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  BaseTexture *downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
    unsigned level_offset) override;
  BaseTexture *upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
    unsigned level_offset) override;

  static uint32_t update_flags_for_linear_layout(uint32_t cflags, FormatStore format);
  static DeviceMemoryClass get_memory_class(uint32_t cflags);

  // similar to cmp exchange:
  // If expected matches the current Image object it gets replace with replacement and true is returned
  // Otherwise false is returned
  bool swapTextureNoLock(Image *expected, Image *replacement);

  void preRecovery();

  SmallTab<uint8_t, MidmemAlloc> texCopy; //< ddsx::Header + ddsx data; sysCopyQualityId stored in hdr.hqPartLevel

  struct ReloadDataHandler
  {
    void operator()(IReloadData *rd)
    {
      if (rd)
        rd->destroySelf();
    }
  };

  eastl::unique_ptr<IReloadData, ReloadDataHandler> rld;

  Image *image = nullptr;
  HostDeviceSharedMemoryRegion stagingMemory;
  uint32_t memSize = 0;
  uint32_t realMipLevels = 0;

  uint32_t cflg = 0;
  const int resType = 0;

  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t depth = 0;

  FormatStore fmt;

  SamplerState samplerState;

  struct ImageMem
  {
    void *ptr = nullptr;
    uint32_t rowPitch = 0;
    uint32_t slicePitch = 0;
    uint32_t memSize = 0;
  };

  static HostDeviceSharedMemoryRegion allocate_read_write_staging_memory(const Image *image,
    const SubresourceRange &subresource_range);

  uint32_t getCreationFenceProgress() const { return creationFenceProgress; }

private:
  bool isSrgbWriteAllowed() const { return (cflg & TEXCF_SRGBWRITE) != 0; }
  bool isSrgbReadAllowed() const { return (cflg & TEXCF_SRGBREAD) != 0; }

  void resolve(Image *dst);

  void release();
  void releaseMemory(uint64_t progress);
  void destroyObject();

  void setTexName(const char *name);

  bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->image == image; }

#if _TARGET_XBOX
  void lockimgXboxLinearLayout(void **pointer, int &stride, int level, int face, uint32_t flags);
#endif
  bool copyAllSubresourcesToStaging(void **pointer, uint32_t flags, uint32_t prev_flags);
  void unlockimgRes3d();

  bool waitAndResetProgress();
  void fillLockedLevelInfo(int level, uint64_t offset);
  uint64_t prepareReadWriteStagingMemoryAndGetOffset(uint32_t flags);

  SamplerState lastSamplerState; //-V730_NOINIT

  D3D12_CPU_DESCRIPTOR_HANDLE sampler{0};

  uint8_t mipLevels = 0;
  uint8_t minMipLevel = 0;
  uint8_t maxMipLevel = 0;

  uint32_t lockedSubRes = 0;

  uint32_t lockFlags = 0;
  uint64_t waitProgress = 0;

  uint64_t creationFenceProgress = 0;

  ImageMem lockMsr;

  static constexpr float default_lodbias = 0.f;
  static constexpr int default_aniso = 1;

  static constexpr uint32_t active_binding_rtv_offset = 0;
  static constexpr uint32_t active_binding_dsv_offset = Driver3dRenderTarget::MAX_SIMRT + active_binding_rtv_offset;
  static constexpr uint32_t active_binding_was_used_offset = 1 + active_binding_dsv_offset;
  static constexpr uint32_t active_binding_is_prealloc_before_load_offset = 1 + active_binding_was_used_offset;
  static constexpr uint32_t active_binding_is_sample_stencil = 1 + active_binding_is_prealloc_before_load_offset;
  static constexpr uint32_t active_binding_is_array_cube_offset = 1 + active_binding_is_sample_stencil;
  static constexpr uint32_t unlock_image_is_upload_skipped = active_binding_is_array_cube_offset + 1;
  static constexpr uint32_t texture_state_bits_count = unlock_image_is_upload_skipped + 1;
  Bitset<texture_state_bits_count> stateBitSet;
  Bitset<dxil::MAX_T_REGISTERS> srvBindingStages[STAGE_MAX_EXT];
  Bitset<dxil::MAX_U_REGISTERS> uavBindingStages[STAGE_MAX_EXT];

protected:
  int texaddrImpl(int a) override;
  int texaddruImpl(int a) override;
  int texaddrvImpl(int a) override;
  int texaddrwImpl(int a) override;
  int texbordercolorImpl(E3DCOLOR c) override;
  int texfilterImpl(int m) override;
  int texmipmapImpl(int m) override;
  int texlodImpl(float mipmaplod) override;
  int setAnisotropyImpl(int level) override;
};

static inline BaseTex *getbasetex(BaseTexture *t) { return static_cast<BaseTex *>(t); }
static inline const BaseTex *getbasetex(const BaseTexture *t) { return static_cast<const BaseTex *>(t); }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }
inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;

} // namespace drv3d_dx12

#define DX12_UPLOAD_TO_IMAGE_AND_CHECK_DEST(CTX, DEBUG_INFO, IMAGE, ...)                                 \
  do                                                                                                     \
  {                                                                                                      \
    if ((IMAGE).getDeviceImage()->getHandle() != nullptr)                                                \
      (CTX).uploadToImage((IMAGE), __VA_ARGS__);                                                         \
    else                                                                                                 \
      D3D_ERROR("DX12: ctx.uploadToImage error: tex.image->getHandle() is equal to 0 (" DEBUG_INFO ")"); \
  } while (false)
