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

namespace drv3d_dx12
{
class BaseTex;
class Image;
struct D3DTextures
{
  Image *image = nullptr;
  HostDeviceSharedMemoryRegion stagingMemory;
  uint32_t memSize = 0;
  uint32_t realMipLevels = 0;

  void release(uint64_t progress);
};

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

class BaseTex final : public BaseTexture
{
public:
  bool isRenderTarget() const { return 0 != (cflg & TEXCF_RTARGET); }
  bool isUav() const { return 0 != (cflg & TEXCF_UNORDERED); }
  bool isMultisampled() const { return 0 != (cflg & TEXCF_SAMPLECOUNT_MASK); }

  void setResApiName(const char *name) const override;

  FormatStore getFormat() const;
  Image *getDeviceImage() const { return tex.image; }
  ImageViewState getViewInfoUav(MipMapIndex mip, ArrayLayerIndex layer, bool as_uint) const;
  ImageViewState getViewInfoRenderTarget(MipMapIndex mip, ArrayLayerIndex layer, bool as_const) const;
  ImageViewState getViewInfo() const;

  void updateDeviceSampler();
  D3D12_CPU_DESCRIPTOR_HANDLE getDeviceSampler();

  Extent3D getMipmapExtent(uint32_t level) const;

  void notifySamplerChange();
  void notifySrvChange();
  void notifyTextureReplaceFinish();
  void dirtyBoundSrvsNoLock();
  void dirtyBoundUavsNoLock();
  void dirtyBoundRtvsNoLock() { dirty_rendertarget_no_lock(this, getRtvBinding()); }

  void setUavBinding(uint32_t stage, uint32_t index, bool s);
  void setSrvBinding(uint32_t stage, uint32_t index, bool s);
  void setRtvBinding(uint32_t index, bool s);
  void setDsvBinding(bool s);
  eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> getRtvBinding() const;
  bool getDsvBinding() const { return stateBitSet.test(active_binding_dsv_offset); }
#if DAGOR_DBGLEVEL > 0
  bool wasUsed() const { return stateBitSet.test(acitve_binding_was_used_offset); }
  void setWasUsed() { stateBitSet.set(acitve_binding_was_used_offset); }
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

  bool downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;
  bool upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level, unsigned level_offset) override;

  static uint32_t update_flags_for_linear_layout(uint32_t cflags, FormatStore format);
  static DeviceMemoryClass get_memory_class(uint32_t cflags);

  D3DTextures tex;
  FormatStore fmt;
  uint32_t cflg = 0;
  int resType = 0;

  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t depth = 0;

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

  SamplerState samplerState;

  struct ImageMem
  {
    void *ptr = nullptr;
    uint32_t rowPitch = 0;
    uint32_t slicePitch = 0;
    uint32_t memSize = 0;
  };

private:
  bool isSrgbWriteAllowed() const { return (cflg & TEXCF_SRGBWRITE) != 0; }
  bool isSrgbReadAllowed() const { return (cflg & TEXCF_SRGBREAD) != 0; }

  void resolve(Image *dst);

  void releaseTex();
  void destroyObject();
  void release() { releaseTex(); }

  void setTexName(const char *name);

  bool isTexResEqual(BaseTexture *bt) const { return bt && ((BaseTex *)bt)->tex.image == tex.image; }

  static constexpr float default_lodbias = 0.f;
  static constexpr int default_aniso = 1;

  static constexpr uint32_t active_binding_rtv_offset = 0;
  static constexpr uint32_t active_binding_dsv_offset = Driver3dRenderTarget::MAX_SIMRT + active_binding_rtv_offset;
  static constexpr uint32_t acitve_binding_was_used_offset = 1 + active_binding_dsv_offset;
  static constexpr uint32_t active_binding_is_prealloc_before_load_offset = 1 + acitve_binding_was_used_offset;
  static constexpr uint32_t active_binding_is_sample_stencil = 1 + active_binding_is_prealloc_before_load_offset;
  static constexpr uint32_t active_binding_is_array_cube_offset = 1 + active_binding_is_sample_stencil;
  static constexpr uint32_t unlock_image_is_upload_skipped = active_binding_is_array_cube_offset + 1;
  static constexpr uint32_t texture_state_bits_count = unlock_image_is_upload_skipped + 1;
  eastl::bitset<dxil::MAX_T_REGISTERS> srvBindingStages[STAGE_MAX_EXT];
  eastl::bitset<dxil::MAX_U_REGISTERS> uavBindingStages[STAGE_MAX_EXT];
  eastl::bitset<texture_state_bits_count> stateBitSet;

  uint32_t lockFlags = 0;
  uint64_t waitProgress = 0;

  uint32_t lockedSubRes = 0;

  SamplerState lastSamplerState; //-V730_NOINIT

  ImageMem lockMsr;

  D3D12_CPU_DESCRIPTOR_HANDLE sampler{0};
  uint8_t mipLevels = 0;
  uint8_t minMipLevel = 0;
  uint8_t maxMipLevel = 0;
};

static inline BaseTex *getbasetex(BaseTexture *t) { return static_cast<BaseTex *>(t); }
static inline const BaseTex *getbasetex(const BaseTexture *t) { return static_cast<const BaseTex *>(t); }

inline BaseTex *cast_to_texture_base(BaseTexture *t) { return getbasetex(t); }
inline BaseTex &cast_to_texture_base(BaseTexture &t) { return *getbasetex(&t); }
typedef BaseTex TextureInterfaceBase;

} // namespace drv3d_dx12