//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_d3dResource.h>
#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_texFlags.h>
#include <math/dag_color.h>
#include <memory/dag_mem.h>
#include <util/dag_stdint.h>
#include <string.h>

// forward declarations for tex pixel formats
struct TexPixel32;
struct TexPixel8a;
class IGenLoad;
namespace ddsx
{
struct Header;
}


// Textures for Driver3d //

struct TextureChannelFormatDesc
{
  int8_t bits;
  int8_t offset; // in bits
  bool isFloatPoint;
  bool isSigned;
  bool isNormalized;
};

enum class ChannelDType
{
  NONE,
  UNORM,  // fixed point real \in [0, 1]
  SNORM,  // fixed point real \in [0, 1]
  UFLOAT, // floating point real \in [0, +\infty]
  SFLOAT, // floating point real \in [-\infty, +\infty]
  UINT,   // int \in [0, +\infty]
  SINT    // int \in [-\infty, +\infty]
};

struct TextureFormatDesc
{
  uint32_t dagorTextureFormat;
  uint16_t bytesPerElement;
  bool isBlockFormat;
  uint8_t elementWidth;  // pixels
  uint8_t elementHeight; // pixels
  // Type for rgba and depth, which is always the same
  ChannelDType mainChannelsType;
  // Separate type for stencil (as it usually is different)
  ChannelDType stencilChannelType;
  TextureChannelFormatDesc r;
  TextureChannelFormatDesc g;
  TextureChannelFormatDesc b;
  TextureChannelFormatDesc a;
  TextureChannelFormatDesc depth;
  TextureChannelFormatDesc stencil;

  bool hasAlpha() const { return a.bits > 0; }

  bool isDepth() const { return depth.bits > 0; }

  int rgbaChannelsCount() const { return (r.bits > 0 ? 1 : 0) + (g.bits > 0 ? 1 : 0) + (b.bits > 0 ? 1 : 0) + (a.bits > 0 ? 1 : 0); }

  int channelsCount() const { return rgbaChannelsCount() + (depth.bits > 0 ? 1 : 0) + (stencil.bits > 0 ? 1 : 0); }
};

bool __forceinline is_alpha_texformat(unsigned flags)
{
  flags &= TEXFMT_MASK;
  if (flags >= TEXFMT_A2R10G10B10 && flags <= TEXFMT_A32B32G32R32F)
    return true;
  if (flags == TEXFMT_A8R8G8B8 || flags == TEXFMT_A8 || flags == TEXFMT_A1R5G5B5 || flags == TEXFMT_A4R4G4B4 || //
      flags == TEXFMT_DXT3 || flags == TEXFMT_DXT5)
    return true;
  return false;
}

bool __forceinline is_bc_texformat(unsigned flags)
{
  flags &= TEXFMT_MASK;
  if (flags >= TEXFMT_DXT1 && flags <= TEXFMT_DXT5)
    return true;
  if (flags >= TEXFMT_BC6H && flags <= TEXFMT_BC7)
    return true;
  if (flags >= TEXFMT_ATI1N && flags <= TEXFMT_ATI2N)
    return true;
  return false;
}

// Lock flags.
enum
{
  TEXLOCK_DISCARD = 0x00002000L,
  TEXLOCK_RAWDATA = 0x00004000L,         // PS4. Do not convert data
  TEXLOCK_NO_DIRTY_UPDATE = 0x00008000L, //?
  TEXLOCK_NOSYSLOCK = 0x00000800L,
  TEXLOCK_READ = 0x01,
  TEXLOCK_WRITE = 0x02,
  TEXLOCK_READWRITE = TEXLOCK_READ | TEXLOCK_WRITE,
  TEXLOCK_RWMASK = TEXLOCK_READWRITE,
  TEXLOCK_NOOVERWRITE = 0x08,               // TEXLOCK_WRITE only
  TEXLOCK_DELSYSMEMCOPY = 0x10,             // delete sysmemcopy on unlock
  TEXLOCK_SYSTEXLOCK = 0x20,                // locks tex copy in sysTex (system memory texture)
  TEXLOCK_UPDATEFROMSYSTEX = 0x40,          // makes copy from system memory to video on unlock
  TEXLOCK_DONOTUPDATEON9EXBYDEFAULT = 0x80, // compatibility! not makes copy from system memory to video on unlock by default, if 9ex
  TEXLOCK_COPY_STAGING = 0x100,
  TEXLOCK_DEFAULT = TEXLOCK_READWRITE,
};


// texture loading flags
enum
{
  TEXLF_CALCMIPMAPS = 0x0001,
};

enum
{
  CUBEFACE_POSX = 0,
  CUBEFACE_NEGX = 1,
  CUBEFACE_POSY = 2,
  CUBEFACE_NEGY = 3,
  CUBEFACE_POSZ = 4,
  CUBEFACE_NEGZ = 5,
};


struct TextureInfo
{
  //! width, height, depth (for VOLTEX), array slices (slice count for ARRTEX or 6 for CUBETEX)
  unsigned short w = 1, h = 1, d = 1, a = 1;
  //! all mips and res type
  unsigned short mipLevels = 0, resType = 0;
  //! texture creation flags
  unsigned cflg = 0;
};

// result of texture load methods
enum class TexLoadRes
{
  ERR = 0,
  OK = 1,
  ERR_RUB = 2
};

class BaseTexture : public D3dResource
{
public:
  struct IReloadData
  {
    virtual ~IReloadData() {}
    virtual void reloadD3dRes(BaseTexture *t) = 0;
    virtual void destroySelf() = 0;
  };
  virtual bool setReloadCallback(IReloadData *) { return false; }

  virtual int generateMips() = 0;
  // Requires TEXCF_UPDATE_DESTINATION, TEXCF_RTARGET or TEXCF_UNORDERED usage for this texture.
  virtual int update(BaseTexture *src) = 0; // update texture from

  static inline int calcSubResIdx(int level, int slice, int mip_levels) { return level + slice * mip_levels; }
  inline int calcSubResIdx(int level, int slice = 0) const { return calcSubResIdx(level, slice, level_count()); }
  // Requires TEXCF_UPDATE_DESTINATION, TEXCF_RTARGET or TEXCF_UNORDERED usage for this texture.
  virtual int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) = 0;
  virtual int updateSubRegionNoOrder(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h,
    int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z)
  {
    return updateSubRegion(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, dest_subres_idx, dest_x, dest_y, dest_z);
  }
  virtual int level_count() const = 0; // number of mipmap levels
  int texaddr(int addrmode)
  {
    G_ASSERT(samplerEnabled);
    return texaddrImpl(addrmode);
  } // set texaddru,texaddrv,...
  int texaddru(int addrmode)
  {
    G_ASSERT(samplerEnabled);
    return texaddruImpl(addrmode);
  } // default is TEXADDR_WRAP
  int texaddrv(int addrmode)
  {
    G_ASSERT(samplerEnabled);
    return texaddrvImpl(addrmode);
  } // default is TEXADDR_WRAP
  int texaddrw(int addrmode)
  {
    G_ASSERT(samplerEnabled);
    return texaddrwImpl(addrmode);
  } // default is TEXADDR_WRAP
  int texbordercolor(E3DCOLOR color)
  {
    G_ASSERT(samplerEnabled);
    return texbordercolorImpl(color);
  }
  int texfilter(int filtermode)
  {
    G_ASSERT(samplerEnabled);
    return texfilterImpl(filtermode);
  } // default is TEXFILTER_DEFAULT
  int texmipmap(int mipmapmode)
  {
    G_ASSERT(samplerEnabled);
    return texmipmapImpl(mipmapmode);
  } // default is TEXMIPMAP_DEFAULT
  int texlod(float mipmaplod)
  {
    G_ASSERT(samplerEnabled);
    return texlodImpl(mipmaplod);
  } // default is zero. Sets texture lod bias
  int setAnisotropy(int level)
  {
    G_ASSERT(samplerEnabled);
    return setAnisotropyImpl(level);
  } // default is 1.
  void disableSampler() { samplerEnabled = false; }
  bool isSamplerEnabled() { return samplerEnabled; }
  virtual int texmiplevel(int minlevel, int maxlevel) = 0; // default is 0, 0
  virtual bool isCubeArray() const { return false; }

  virtual void setReadStencil(bool) {} // for depth stencil textures, if on will read stencil, not depth. Currently either of two
  virtual void setTID(TEXTUREID /*tid*/) {}
  virtual TEXTUREID getTID() const { return BAD_TEXTUREID; }
  const char *getTexName() const { return getResName(); }
  void setResApiName(const char * /*name*/) const override {}

  // lock image data - no conversion is performed
  // render target image is read-only by default
  // non-render-target image is read-write by default
  // returns 0 on error
  virtual int lockimg(void **, int &stride_bytes, int level, unsigned flags) = 0;
  virtual int lockimg(void **, int &stride_bytes, int layer, int level, unsigned flags) = 0;
  virtual int unlockimg() { return 0; };
  int unlock() { return unlockimg(); };

  virtual int lockbox(void **, int &, int &, int, unsigned) { return 0; };
  virtual int unlockbox() { return 0; };

  virtual int getinfo(TextureInfo &info, int level = 0) const = 0;

  template <typename T>
  inline int lockimgEx(T **p, int &stride_bytes, int level = 0, unsigned flags = TEXLOCK_DEFAULT)
  {
    void *vp;
    if (!lockimg(&vp, stride_bytes, level, flags))
    {
      *p = NULL;
      return 0;
    }
    *p = (T *)vp;
    return 1;
  }

  //! created temporary BaseTexture with texture res object with given dimensions (texture format and texture subtype is used from
  //! original)
  virtual BaseTexture *makeTmpTexResCopy(int /*w*/, int /*h*/, int /*d*/, int /*l*/, bool /*staging_tex*/ = false) { return nullptr; }
  //! replaces texture res object with new one of new_tex and then destroys new_tex (destruction may be delayed by driver)
  virtual void replaceTexResObject(BaseTexture *&new_tex) { del_d3dres(new_tex); }

  //! forces allocation of texture resource; return true if tex allocated and ready for use
  virtual bool allocateTex() { return true; }
  //! discards texture and returns it to STUB state
  virtual void discardTex() {}

#if DAGOR_DBGLEVEL > 0
  // Not implemented for all platforms, only for shared implementation BaseTextureImpl which is not used everywhere yet
  // And a need is questionable by now
  virtual int getTexfilter() const { return 0; }
#endif
  // Replaces the texture with a smaller one defined by info, overlapping mip levels are automatically
  // migrated to the new texture. This does not need TEXCF_UPDATE_DESTINATION to work, even if the default
  // implementation would do so. Drivers that can not executed updateSubRegion correctly without the
  // TEXCF_UPDATE_DESTINATION flag have to implement this in a way so it will work correctly.
  //
  // Returns true on success, may return false on fail. Only fail case can be the failure to allocate the
  // replacement texture.
  [[nodiscard]] virtual BaseTexture *downSize(int width, int height, int depth, int mips, unsigned start_src_level,
    unsigned level_offset)
  {
    auto rep = makeTmpTexResCopy(width, height, depth, mips, false);
    if (!rep)
      return nullptr;

    TextureInfo selfInfo;
    getinfo(selfInfo);

    unsigned sourceLevel = max<unsigned>(level_offset, start_src_level);
    unsigned sourceLevelEnd = min<unsigned>(selfInfo.mipLevels, mips + level_offset);
    rep->texmiplevel(sourceLevel - level_offset, sourceLevelEnd - level_offset - 1);
    for (; sourceLevel < sourceLevelEnd; sourceLevel++)
    {
      for (int s = 0; s < selfInfo.a; s++)
      {
        rep->updateSubRegion(this, calcSubResIdx(sourceLevel, s, selfInfo.mipLevels), 0, 0, 0, max<int>(selfInfo.w >> sourceLevel, 1),
          max<int>(selfInfo.h >> sourceLevel, 1), max<int>(selfInfo.d >> sourceLevel, selfInfo.a),
          calcSubResIdx(sourceLevel - level_offset, s, mips), 0, 0, 0);
      }
    }
    return rep;
  }
  // Replaces the texture with a larger one defined by info, overlapping mip levels are automatically
  // migrated to the new texture. This does not need TEXCF_UPDATE_DESTINATION to work, even if the default
  // implementation would do so. Drivers that can not executed updateSubRegion correctly without the
  // TEXCF_UPDATE_DESTINATION flag have to implement this in a way so it will work correctly.
  //
  // Returns true on success, may return false on fail. Only fail case can be the failure to allocate the
  // replacement texture.
  [[nodiscard]] virtual BaseTexture *upSize(int width, int height, int depth, int mips, unsigned start_src_level,
    unsigned level_offset)
  {
    auto rep = makeTmpTexResCopy(width, height, depth, mips, false);
    if (!rep)
      return nullptr;

    TextureInfo selfInfo;
    getinfo(selfInfo);

    unsigned destinationLevel = level_offset + start_src_level;
    unsigned destinationLevelEnd = min<unsigned>(selfInfo.mipLevels + level_offset, mips);
    rep->texmiplevel(destinationLevel, destinationLevelEnd - 1);
    for (; destinationLevel < destinationLevelEnd; destinationLevel++)
    {
      for (int s = 0; s < selfInfo.a; s++)
      {
        rep->updateSubRegion(this, calcSubResIdx(destinationLevel - level_offset, s, selfInfo.mipLevels), 0, 0, 0,
          max<int>(width >> destinationLevel, 1), max<int>(height >> destinationLevel, 1),
          max<int>(depth >> destinationLevel, selfInfo.a), calcSubResIdx(destinationLevel, s, mips), 0, 0, 0);
      }
    }

    return rep;
  }

  BaseTexture() = default;
  BaseTexture(BaseTexture &&) = default;
  BaseTexture &operator=(BaseTexture &&) = default;

protected:
  ~BaseTexture() override {}
  bool samplerEnabled = true;
  virtual int texaddrImpl(int addrmode) = 0;
  virtual int texaddruImpl(int addrmode) = 0;
  virtual int texaddrvImpl(int addrmode) = 0;
  virtual int texaddrwImpl(int) { return 0; };
  virtual int texbordercolorImpl(E3DCOLOR) = 0;
  virtual int texfilterImpl(int filtermode) = 0;
  virtual int texmipmapImpl(int mipmapmode) = 0;
  virtual int texlodImpl(float mipmaplod) = 0;
  virtual int setAnisotropyImpl(int level) = 0;

  // Flag is used to mark the read back status for textures to prepare for cpu access,
  // It's either copy to staging memory or direct access in case of consoles.
  static constexpr int TEX_COPIED = 1 << 30;
};

typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;

uint32_t auto_mip_levels_count(uint32_t w, uint32_t min_size);
uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t min_size);
uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t d, uint32_t min_size);

void apply_gen_tex_props(BaseTexture *t, const struct TextureMetaData &tmd, bool force_addr_from_tmd = true);

uint32_t parse_tex_format(const char *name, uint32_t default_fmt);
const TextureFormatDesc &get_tex_format_desc(uint32_t fmt);
const char *get_tex_format_name(uint32_t fmt);

/** \brief Extracts a the bits from a pixel at memory location \p pixel
 * that correspond to a particular channel in the specified format.
 * \note Bits unused by the channel are set to 0, even for signed formats
 * \warning This is extremely slow!
 * \param pixel Memory location of the pixel
 * \param channel Description of the channel's format
 * \return A uint32_t whose lowest bits contain the bits of the channel
 */
uint32_t get_tex_channel_value(const void *pixel, const TextureChannelFormatDesc &channel);

/** \brief Sets the bits for a pixel at memory location \p pixel that
 * correspond to a particular channel in the specified format.
 * \warning This is extremely slow!
 * \param pixel Memory location of the pixel
 * \param channel Description of the channel's format
 * \param bits The bits to assign to the channel (only the lowest bits are used)
 */
void set_tex_channel_value(void *pixel, const TextureChannelFormatDesc &channel, uint32_t bits);

/** \brief Converts raw bits of a channel in a certain format into
 * a floating point value that matches the logical value of the channel
 * as closely as possible
 * \note Conversion is done as per section 3.9 of Vulkan API specification
 * \warning This is extremely slow!
 * \param bits The bits of the channel
 * \param type The type of the value stored in the channel
 * \param channel The description of the channel's format
 * \return A float that matches the logical value of the channel
 */
float channel_bits_to_float(uint32_t bits, ChannelDType type, const TextureChannelFormatDesc &channel);


/** \brief Converts a float into raw bits of a channel in a certain
 * format that match the provided float as closely as possible
 * \note Conversion is done as per section 3.9 of Vulkan API specification
 * \note Bits unused by the channel are set to 0, even for signed formats
 * \warning This is extremely slow!
 * \param value The floating point value to convert
 * \param type The type of the value stored in the channel
 * \param channel The description of the channel's format
 * \return A uint32_t that contains the bits of the channel
 */
uint32_t float_to_channel_bits(float value, ChannelDType type, const TextureChannelFormatDesc &channel);

/// load texture content from DDSx stream using DDSx header for previously allocated texture
typedef void (*on_tex_slice_loaded_cb_t)();
extern TexLoadRes (*d3d_load_ddsx_tex_contents_impl)(BaseTexture *tex, TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr,
  IGenLoad &crd, int q_id, int start_lev, unsigned tex_ld_lev, on_tex_slice_loaded_cb_t, bool);
inline TexLoadRes d3d_load_ddsx_tex_contents(BaseTexture *tex, TEXTUREID tid, TEXTUREID paired_tid, const ddsx::Header &hdr,
  IGenLoad &crd, int q_id, int start_lev = 0, unsigned tex_ld_lev = 0, on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb = nullptr,
  bool tex_props_inited = false)
{
  return d3d_load_ddsx_tex_contents_impl(tex, tid, paired_tid, hdr, crd, q_id, start_lev, tex_ld_lev, on_tex_slice_loaded_cb,
    tex_props_inited);
}
/// load texture content from DDSx stream using DDSx header to specified slice of previously allocated array texture
extern TexLoadRes (*d3d_load_ddsx_to_slice)(BaseTexture *tex, int slice, const ddsx::Header &hdr, IGenLoad &crd, int q_id,
  int start_lev, unsigned tex_ld_lev);

bool convert_image_line(const void *__restrict input, int width, int in_channels, int in_bits_per_channel, bool in_float,
  void *__restrict output, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb);

//--- include defines specific to target 3d -------
#include <drv/3d/dag_consts_base.h>
