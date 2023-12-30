#include <3d/dag_drv3d.h>
#include <EASTL/tuple.h>
#include <generic/dag_smallTab.h>


class BaseTextureImpl : public BaseTexture
{
public:
  E3DCOLOR borderColor = 0;
  float lodBias = 0.0f;
  union
  {
    struct
    {
      uint32_t anisotropyLevel : 5;
      uint32_t addrU : 3;
      uint32_t addrV : 3;
      uint32_t addrW : 3;

      uint32_t texFilter : 3;
      uint32_t mipFilter : 3;
      uint32_t minMipLevel : 4;
      uint32_t maxMipLevel : 4;
      uint32_t mipLevels : 4;
    };
    uint32_t key;
  };
  static constexpr int SAMPLER_KEY_MASK = (1 << 20) - 1; /// miplevels are not part of key

  int base_format;
  uint32_t cflg;
  uint32_t lockFlags = 0;
  uint16_t width = 1, height = 1, depth = 1;
  uint8_t type;
  uint8_t lockedLevel = 0;


  static bool isSameFormat(int cflg1, int cflg2);

  BaseTextureImpl(int set_cflg, int set_type);

  int restype() const override { return type; }

  virtual int update(BaseTexture *src) = 0;
  virtual int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) = 0;

  int level_count() const override;
  virtual int texaddr(int a) = 0;
  virtual int texaddru(int a) = 0;
  virtual int texaddrv(int a) = 0;
  virtual int texaddrw(int a) = 0;
  virtual int texbordercolor(E3DCOLOR c) = 0;
  virtual int texfilter(int m) = 0;
  virtual int texmipmap(int m) = 0;

  virtual int texlod(float mipmaplod) = 0;
  virtual int texmiplevel(int minlev, int maxlev) = 0;
  virtual int setAnisotropy(int level) = 0;

  void setTexName(const char *name);

  virtual int lockimg(void **, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT) = 0;
  virtual int unlockimg() = 0;

  virtual int generateMips() = 0;

  int getinfo(TextureInfo &ti, int level) const override;

#if DAGOR_DBGLEVEL > 0
  virtual int getTexfilter() const { return texFilter; }
#endif
};

int count_mips_if_needed(int w, int h, int32_t flg, int levels);

// common helpers to convert DDSx format to TEXFMT_ one
uint32_t d3dformat_to_texfmt(/*D3DFORMAT*/ uint32_t fmt);
uint32_t texfmt_to_d3dformat(/*D3DFORMAT*/ uint32_t fmt);
static inline uint32_t implant_d3dformat(uint32_t cflg, uint32_t fmt) { return (cflg & ~TEXFMT_MASK) | d3dformat_to_texfmt(fmt); }
