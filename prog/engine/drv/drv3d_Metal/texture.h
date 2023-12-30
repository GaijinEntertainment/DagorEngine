
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#include <EASTL/vector.h>
#include <generic/dag_smallTab.h>
#include <3d/tql.h>

namespace drv3d_metal
{
class Texture;

static inline Texture *getbasetex(BaseTexture *t) { return (Texture *)t; }

class Texture : public BaseTexture
{
public:
  struct SubMip
  {
    int minLevel;
    int maxLevel;
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
    eastl::vector<SubMip> sub_mip_textures;
    int tid = -1;
    int texture_size = 0;

    ApiTexture(id<MTLTexture> tex, const char *name);
    ApiTexture(Texture *base);
    void release(bool immediate);
    id<MTLTexture> allocateOrCreateSubmip(int set_minlevel, int set_maxlevel, bool is_uav);
    void destroyObject() {}
  };

  uint32_t cflg;
  int base_format;
  uint8_t type;

  E3DCOLOR borderColor = 0;
  float lodBias = 0.0f;

  uint8_t anisotropyLevel = 1;
  uint8_t addrU = TEXADDR_WRAP;
  uint8_t addrV = TEXADDR_WRAP;
  uint8_t addrW = TEXADDR_WRAP;
  uint8_t texFilter = TEXFILTER_DEFAULT;
  uint8_t mipFilter = TEXMIPMAP_DEFAULT;
  uint8_t mipLevels = 1;
  uint8_t lockedLevel = 0;
  uint8_t minMipLevel = 20;
  uint8_t maxMipLevel = 0;

  uint16_t width = 1, height = 1, depth = 1, samples = 1;

  struct SamplerState
  {
    int addrU;
    int addrV;
    int addrW;
    int texFilter;
    int mipmap;
    int max_level;
    int lod;
    int anisotropy;
    E3DCOLOR border_color;

    id<MTLSamplerState> sampler;

    SamplerState()
    {
      addrU = TEXADDR_WRAP;
      addrV = TEXADDR_WRAP;
      addrW = TEXADDR_WRAP;
      texFilter = TEXFILTER_SMOOTH;
      mipmap = TEXFILTER_SMOOTH;
      lod = 0;
      anisotropy = 1;
      max_level = 1024;
      border_color = E3DCOLOR(0, 0, 0, 0);
    }

    friend inline bool operator==(const SamplerState &left, const SamplerState &right)
    {
      if (left.addrU == right.addrU && left.addrV == right.addrV && left.addrW == right.addrW && left.texFilter == right.texFilter &&
          left.mipmap == right.mipmap && left.max_level == right.max_level && left.lod == right.lod &&
          left.anisotropy == right.anisotropy && left.border_color == right.border_color)
      {
        return true;
      }

      return false;
    }
  };

  static MTLSamplerDescriptor *samplerDescriptor;
  static Tab<SamplerState> states;
  id<MTLSamplerState> sampler;

  int start_level;
  int use_dxt;
  bool read_stencil = false;
  bool memoryless = false;
  bool use_upload_buffer = false;

  ApiTexture *apiTex = nullptr;

  MTLPixelFormat metal_format;
  MTLPixelFormat metal_rt_format;
  MTLTextureType metal_type;

  uint64_t last_render_frame = ~0ull;

  const int TEXLOCK_BUSY_WAITING_IN_QUEUE = 0x80000000;
  uint32_t lockFlags = 0;

  struct SysImage
  {
    id<MTLBuffer> buffer = nil;
    uint64_t frame = 0;
    int lock_level;
    int lock_face;
    int guard_offest;
    int nBytes;
    int widBytes;
    char *image;
  };

  Tab<SysImage> sys_images;

  SamplerState sampler_state;
  bool sampler_dirty;

  bool waiting2delete;

  Texture(int w, int h, int levels, int d, int tp, int flags, int fmt, const char *name, bool memoryless, bool alloc);
  Texture(id<MTLTexture> tex, int fmt, const char *name);
  ~Texture();
  void allocate();

  static MTLPixelFormat format2Metal(uint32_t fmt);
  static MTLPixelFormat format2MetalsRGB(uint32_t fmt);
  static int getMaxLevels(int width, int height);
  static void getStride(uint32_t fmt, int width, int height, int level, int &widBytes, int &nBytes);
  static MTLSamplerAddressMode getAddressMode(int mode);
  static bool isSameFormat(int cflg1, int cflg2);

  int restype() const override { return type; };
  int ressize() const override;

  int getWidth();
  int getHeight();

  int level_count() const override;
  virtual int texaddr(int a);
  virtual int texaddru(int addrmode);
  virtual int texaddrv(int addrmode);
  virtual int texaddrw(int addrmode);
  virtual int texbordercolor(E3DCOLOR color);
  virtual int texfilter(int filtermode);
  virtual int texmipmap(int mipmapmode);
  virtual int texlod(float mipmaplod);
  virtual int texmiplevel(int minlevel, int maxlevel);
  void setMiplevelImpl(int minlevel, int maxlevel);
  virtual int setAnisotropy(int level);
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

  virtual void setResApiName(const char * /*name*/) const {} // TODO implement it when setting resource name on the API is implemented

  bool isCubeArray() const override { return type == RES3D_CUBEARRTEX; }
  int getinfo(TextureInfo &ti, int level) const override;

  int calcBaseLevel(int w, int h);
  void setBaseLevel(int base_level);
  void setMinMipLevel(int min_mip_level);

  void *lock(int &row_pitch, int &slice_pitch, int level, int face, unsigned flags, bool readback = false);
  void unlock();

  void doSetAddressing(int u, int v, int w);
  void doSetBorder(E3DCOLOR color);
  void doSetFilter(int filter, int mipfilter);
  void doSetAnisotropy(int level);

  void apply(id<MTLTexture> &out_tex, bool is_read_stencil, int mip_level, bool is_uav);
  void applySampler(id<MTLSamplerState> &out_smp);

  virtual void destroy();
  void release();

  DECLARE_TQL_TID_AND_STUB()

  bool allocateTex() override
  {
    ensureSwapTexDone();
    if (apiTex && !isStub())
      return true;
    apiTex = nullptr;
    allocate();
    return apiTex != nullptr;
  }
  void discardTex() override;
  void releaseTex(bool recreate_after);
  inline bool isTexResEqual(BaseTexture *bt) const { return bt && getbasetex(bt)->apiTex == apiTex; }

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;
  void replaceTexResObjectImpl(BaseTexture *&other_tex);

  void ensureSwapTexDone() const
  {
    if (lockFlags & TEXLOCK_BUSY_WAITING_IN_QUEUE)
      waitSwapTexDone();
  }
  void waitSwapTexDone() const;

  static void cleanup();
};

void clear_textures_pool_garbage();
} // namespace drv3d_metal
