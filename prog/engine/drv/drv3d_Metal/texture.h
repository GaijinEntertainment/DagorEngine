// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#include <EASTL/vector.h>
#include <generic/dag_smallTab.h>
#include <3d/tql.h>
#include <resourceName.h>

#include <mutex>

struct ResourceHeap;

namespace drv3d_metal
{
class Texture;

static inline Texture *getbasetex(BaseTexture *t) { return (Texture *)t; }

struct HazardEncoder
{
  uint64_t submit : 48 = 0;
  uint64_t encoder : 16 = 0;

  bool operator!=(const HazardEncoder &rhs) const { return encoder != rhs.encoder || submit != rhs.submit; }
  bool operator==(const HazardEncoder &rhs) const { return encoder == rhs.encoder && submit != rhs.submit; }
};

struct HazardTracker
{
  eastl::vector<HazardEncoder> encoders_read_in;
  HazardEncoder encoder_written_in;
  ResourceHeap *heap = nullptr;
  uint32_t heap_offset = 0;
  uint32_t heap_size = 0;
};

class Texture final : public D3dResourceNameImpl<BaseTexture>, public HazardTracker
{
public:
  struct SubMip
  {
    int minLevel;
    int maxLevel;
    int slice = 0;
    bool is_uav = false;
    id<MTLTexture> tex;
  };

  struct ApiTexture
  {
    Texture *base = nullptr;
    id<MTLTexture> texture = nil;
    id<MTLTexture> texture_no_srgb = nil;
    id<MTLTexture> sub_texture = nil;
    id<MTLTexture> sub_texture_no_srgb = nil;
    id<MTLTexture> rt_texture = nil;
    id<MTLTexture> stencil_read_texture = nil;
    id<MTLTexture> texture_as_uint = nil;
    eastl::vector<SubMip> sub_mip_textures;
    int tid = -1;
    uint32_t texture_size = 0;

    ApiTexture(Texture *base, id<MTLTexture> tex, const char *name);
    ApiTexture(Texture *base);
    void release(bool immediate);
    id<MTLTexture> allocateOrCreateSubmip(int set_minlevel, int set_maxlevel, bool is_uav, int start_slice);
    void destroyObject() {}

    void applyName(const char *name);
  };

  uint32_t cflg;
  int base_format;

  D3DResourceType type = D3DResourceType::TEX;
  uint8_t mipLevels = 1;

  uint16_t width = 1, height = 1, depth = 1, samples = 1;

  int start_level;
  int use_dxt;
  bool read_stencil = false;
  bool memoryless = false;
  bool use_upload_buffer = false;

  ApiTexture *apiTex = nullptr;

  MTLPixelFormat metal_format;
  MTLPixelFormat metal_rt_format;
  MTLTextureType metal_type;

  uint64_t last_readback_submit = ~0ull;
  uint32_t lockFlags = 0;

  struct SysImage
  {
    id<MTLBuffer> buffer = nil;
    int lock_level;
    int lock_face;
    int nBytes;
    int widBytes;
    char *image;
  };

  Tab<SysImage> sys_images;

  bool waiting2delete;

  Texture(int w, int h, int levels, int d, D3DResourceType type, int flags, int fmt, const char *name, bool memoryless, bool alloc);
  Texture(id<MTLTexture> tex, int fmt, const char *name);
  Texture(id<MTLTexture> tex, MTLTextureDescriptor *desc, uint32_t flg, const char *name);
  ~Texture();
  void allocate();

  static MTLPixelFormat format2Metal(uint32_t fmt);
  static MTLPixelFormat format2MetalsRGB(uint32_t fmt);
  static int getMaxLevels(int width, int height);
  static void getStride(uint32_t fmt, int width, int height, int level, int &widBytes, int &nBytes);
  static bool isSameFormat(int cflg1, int cflg2);

  D3DResourceType getType() const override { return type; };
  uint32_t getSize() const override;

  int getWidth();
  int getHeight();

  int level_count() const override;
  virtual int texmiplevel(int minlevel, int maxlevel);
  virtual void setReadStencil(bool on) override { read_stencil = on; }
  bool isReadStencil() const { return read_stencil; }

  virtual int lockimg(void **, int &stride_bytes, int level, unsigned flags);
  virtual int lockimg(void **, int &stride_bytes, int layer, int level, unsigned flags);
  virtual int unlockimg();

  virtual int lockbox(void **, int &, int &, int, unsigned);
  virtual int unlockbox();

  virtual int generateMips();

  virtual int update(BaseTexture *src);
  virtual int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z);

  virtual void setApiName(const char * /*name*/) const;

  bool isCubeArray() const override { return type == D3DResourceType::CUBEARRTEX; }
  int getinfo(TextureInfo &ti, int level) const override;

  int calcBaseLevel(int w, int h);
  void setBaseLevel(int base_level);
  void setMinMipLevel(int min_mip_level);

  void *lock(int &row_pitch, int &slice_pitch, int level, int face, unsigned flags, bool readback = false);
  void unlock();

  void apply(id<MTLTexture> &out_tex, bool is_read_stencil, int mip_level, int start_slice, bool is_uav, bool as_uint);

  virtual void destroy();
  void release();

  DECLARE_TQL_TID_AND_STUB()

  bool allocateTex() override
  {
    if (apiTex && !isStub())
      return true;
    apiTex = nullptr;
    allocate();
    return apiTex != nullptr;
  }
  void discardTex() override;
  void releaseTex(bool recreate_after);
  inline bool isTexResEqual(BaseTexture *bt) const { return bt && getbasetex(bt)->apiTex == apiTex; }

  bool updateTexResFormat(unsigned d3d_format) override;
  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  static void cleanup();
};

void clear_textures_pool_garbage();
} // namespace drv3d_metal
