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
#include <util/dag_bitwise_cast.h>
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
  bool isFloatPoint : 1;
  bool isSigned : 1;
  bool isNormalized : 1;
};

enum class ChannelDType : uint8_t
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
  if (flags == TEXFMT_BC5S)
    return true;
  return false;
}

// Lock flags.
enum
{
  TEXLOCK_DISCARD = 0x00002000L,
  TEXLOCK_NOSYSLOCK = 0x00000800L, // if lock considered blocking one (GPU wait), do early exit instead of blocking
  TEXLOCK_READ = 0x01,
  TEXLOCK_WRITE = 0x02,
  TEXLOCK_READWRITE = TEXLOCK_READ | TEXLOCK_WRITE,
  TEXLOCK_RWMASK = TEXLOCK_READWRITE,
  TEXLOCK_DELSYSMEMCOPY = 0x10, // delete sysmemcopy on unlock
  TEXLOCK_DONOTUPDATE = 0x80,   // do not update video memory at unlock, use to batch updates for multiple subresources
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
  //! width, height, depth (for VOLTEX)
  uint16_t w = 1, h = 1, d = 1;
  //! array slices (slice count for ARRTEX, 6 for CUBETEX, or 6 * <array size> for CUBEARRTEX)
  uint16_t a = 1;
  //! all mips
  uint16_t mipLevels = 0;
  //! resource type (D3DResourceType::TEX, ...)
  D3DResourceType type = {};
  // committed resource flag, we can't alias it (can be used for other flags also)
  uint8_t isCommitted = 0;
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
  virtual int level_count() const = 0;                     // number of mipmap levels
  virtual int texmiplevel(int minlevel, int maxlevel) = 0; // default is 0, 0
  virtual bool isCubeArray() const { return false; }

  virtual void setReadStencil(bool) {} // for depth stencil textures, if on will read stencil, not depth. Currently either of two
  virtual void setTID(TEXTUREID /*tid*/) {}
  virtual TEXTUREID getTID() const { return BAD_TEXTUREID; }
  const char *getTexName() const { return getName(); }

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

  //! changes texture format (if possible and supported by driver) to specified 'd3d_format' (ddsx::Header::d3dFormat)
  virtual bool updateTexResFormat(unsigned /*d3d_format*/) { return false; }
  //! created temporary BaseTexture with texture res object with given dimensions (texture subtype and format are used from original)
  virtual BaseTexture *makeTmpTexResCopy(int /*w*/, int /*h*/, int /*d*/, int /*l*/) { return nullptr; }
  //! replaces texture res object with new one of new_tex and then destroys new_tex (destruction may be delayed by driver)
  virtual void replaceTexResObject(BaseTexture *&new_tex) { del_d3dres(new_tex); }

  //! forces allocation of texture resource; return true if tex allocated and ready for use
  virtual bool allocateTex() { return true; }
  //! discards texture and returns it to STUB state
  virtual void discardTex() {}

  BaseTexture() = default;
  BaseTexture(BaseTexture &&) = default;
  BaseTexture &operator=(BaseTexture &&) = default;

protected:
  ~BaseTexture() = default;

  // Flag is used to mark the read back status for textures to prepare for cpu access,
  // It's either copy to staging memory or direct access in case of consoles.
  static constexpr int TEX_COPIED = 1 << 30;

#if _TARGET_C1



#endif
};

typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;
typedef BaseTexture CubeArrayTexture;

uint32_t auto_mip_levels_count(uint32_t w, uint32_t min_size);
uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t min_size);
uint32_t auto_mip_levels_count(uint32_t w, uint32_t h, uint32_t d, uint32_t min_size);

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
 * the bits of a 32-bit float that matches the logical value of the channel
 * as closely as possible
 * \note Conversion is done as per section 3.9 of Vulkan API specification
 * \note The float crosses the call as bits: the engine builds with fast math,
 * which does not permit inf or NaN in a float parameter or return value, and
 * float formats hold both
 * \warning This is extremely slow!
 * \param bits The bits of the channel
 * \param type The type of the value stored in the channel
 * \param channel The description of the channel's format
 * \return The bits of a float that matches the logical value of the channel
 */
uint32_t channel_bits_to_float_bits(uint32_t bits, ChannelDType type, const TextureChannelFormatDesc &channel);

/// \brief channel_bits_to_float_bits with the result as a float
__forceinline float channel_bits_to_float(uint32_t bits, ChannelDType type, const TextureChannelFormatDesc &channel)
{
  return dag::bit_cast<float>(channel_bits_to_float_bits(bits, type, channel));
}


/** \brief Converts the bits of a 32-bit float into raw bits of a channel in a certain
 * format that match the provided float as closely as possible
 * \note Conversion is done as per section 3.9 of Vulkan API specification
 * \note Bits unused by the channel are set to 0, even for signed formats
 * \note See channel_bits_to_float_bits for why the float is passed as bits
 * \warning This is extremely slow!
 * \param value_bits The bits of the floating point value to convert
 * \param type The type of the value stored in the channel
 * \param channel The description of the channel's format
 * \return A uint32_t that contains the bits of the channel
 */
uint32_t float_bits_to_channel_bits(uint32_t value_bits, ChannelDType type, const TextureChannelFormatDesc &channel);

/// \brief float_bits_to_channel_bits with the value as a float
__forceinline uint32_t float_to_channel_bits(float value, ChannelDType type, const TextureChannelFormatDesc &channel)
{
  return float_bits_to_channel_bits(dag::bit_cast<uint32_t>(value), type, channel);
}

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
  void *__restrict output, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb, bool invert);

//--- include defines specific to target 3d -------
#include <drv/3d/dag_consts_base.h>
