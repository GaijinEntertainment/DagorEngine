// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_DRIVER 1

#include <generic/dag_tab.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_genIo.h>
#include <image/dag_texPixel.h>
#include <3d/dag_gpuConfig.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include "render.h"
#include "textureloader.h"

#include <osApiWrappers/dag_critSec.h>
#include <image/dag_dxtCompress.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_imageFunctions.h>
#include <vecmath/dag_vecMath.h>
#include <validation/texture.h>

#include <basetexture.h>

using namespace drv3d_metal;

namespace drv3d_metal
{
  extern Driver3dDesc g_device_desc;
}

/// create texture; if img==NULL, size (w/h) must be specified
/// if img!=NULL and size==0, dimension will be taken from img
/// levels specify maximum number of mipmap levels
/// if levels==0, whole mipmap set will be created if device supports mipmaps
/// img format is TexPixel32
/// returns NULL on error
::Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char* name)
{
  check_texture_creation_args(w, h, flg, name);
  if (img != NULL)
  {
    w = img->w;
    h = img->h;
  }

  drv3d_metal::Texture* texture = new drv3d_metal::Texture(w, h, levels, 1, RES3D_TEX, flg, flg &TEXFMT_MASK, name, false, true);

  if (texture->metal_format != MTLPixelFormatDepth32Float &&
      texture->metal_format != MTLPixelFormatDepth32Float_Stencil8)
  {
    if (img)
    {
      int nBytes = 0;
      int widBytes = 0;

      void* image = texture->lock(widBytes, nBytes, 0, 0, TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_DELSYSMEMCOPY);
      memcpy(image, img->getPixels(), nBytes);

      texture->unlock();
    }
  }

  return texture;
}

/// create texture from DDSx stream;
/// quality_id specifies quality index (0=high, 1=medium, 2=low, 3=ultralow)
/// levels specifies number of loaded mipmaps (0=all, >0=only first 'levels' mipmaps)
/// returns NULL on error
BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char* name)
{
  ddsx::Header hdr;
  if (!crd.readExact(&hdr, sizeof(hdr)))
    return NULL;

  drv3d_metal::Texture *tex = (drv3d_metal::Texture *)alloc_ddsx_tex(hdr, flg, quality_id, levels, name);
  if (tex != NULL)
  {
    if (load_ddsx_tex_contents(tex, hdr, crd, quality_id) == TexLoadRes::OK)
      return tex;
    else
      tex->destroy();
  }
  return NULL;
}


/// create texture object using DDSx header (not texture contents loaded at this time)
BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels,
                 const char *name, int stub_tex_idx)
{
  uint32_t fmt = implant_d3dformat(flg, hdr.d3dFormat);
#if _TARGET_PC_MACOSX
  if (hdr.d3dFormat == D3DFMT_A4R4G4B4 || hdr.d3dFormat == D3DFMT_X4R4G4B4 || hdr.d3dFormat == D3DFMT_R5G6B5)
    fmt = implant_d3dformat(flg, D3DFMT_A8R8G8B8);
#endif
  if (!hdr.checkLabel() || fmt == -1)
  {
    debug("invalid DDSx format");
    return NULL;
  }
  G_ASSERT((fmt & TEXCF_RTARGET) == 0);
  fmt |= (hdr.flags & hdr.FLG_GAMMA_EQ_1) ? 0 : TEXCF_SRGBREAD;

  int type = RES3D_TEX;
  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    type = RES3D_CUBETEX;
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    type = RES3D_VOLTEX;
  else if (hdr.flags & ddsx::Header::FLG_ARRTEX)
    type = RES3D_ARRTEX;

  drv3d_metal::Texture* tex = new drv3d_metal::Texture(hdr.w, hdr.h, hdr.levels, hdr.depth, type, fmt, fmt & TEXFMT_MASK, name, false, false);

  int skip_levels = hdr.getSkipLevels(hdr.getSkipLevelsFromQ(quality_id), levels);
  int w = max(hdr.w>>skip_levels, 1), h = max(hdr.h>>skip_levels, 1), d = max(hdr.depth>>skip_levels, 1);
  if (!(hdr.flags & hdr.FLG_VOLTEX))
    d = (hdr.flags & hdr.FLG_ARRTEX) ? hdr.depth : 1;

  tex->stubTexIdx = stub_tex_idx;
  if (stub_tex_idx >= 0)
  {
    tex->apiTex = tex->getStubTex()->apiTex;
    return tex;
  }

  tex->allocate();
  if (tex->apiTex != NULL)
    return tex;

  tex->destroy();
  tex = nullptr;

  return tex;
}

CubeTexture *d3d::create_cubetex(int size, int flg, int levels, const char *name)
{
  return new drv3d_metal::Texture(size, size, levels, 1, RES3D_CUBETEX, flg, flg &TEXFMT_MASK, name, false, true);
}

VolTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char* name)
{
  return new drv3d_metal::Texture(w, h, levels, d, RES3D_VOLTEX, flg, flg &TEXFMT_MASK, name, false, true);
}

ArrayTexture *d3d::create_cube_array_tex(int size, int d, int flg, int levels, const char *name)
{
  return new drv3d_metal::Texture(size, size, levels, d, RES3D_CUBEARRTEX, flg, flg &TEXFMT_MASK, name, false, true);
}

ArrayTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char* name)
{
  return new drv3d_metal::Texture(w, h, levels, d, RES3D_ARRTEX, flg, flg &TEXFMT_MASK, name, false, true);
}

static bool set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler, uint32_t face, uint32_t mip_level, bool is_rw, bool as_uint)
{
  G_ASSERT(face == 0 && "setting face is not supported by metal yet");

  int slot_offset = 0;

  if (is_rw)
  {
    slot_offset = MAX_SHADER_TEXTURES;
  }

  if (!tex)
  {
    render.setTexture(shader_stage, slot + slot_offset, 0, use_sampler, mip_level, false);
    return true;
  }

  drv3d_metal::Texture* tx = (drv3d_metal::Texture*)tex;
  render.setTexture(shader_stage, slot + slot_offset, tx, use_sampler, mip_level, as_uint);

  return true;
}

bool d3d::set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler)
{
  set_tex(shader_stage, slot, tex, use_sampler, 0, 0, false, false);

  return true;
}

bool d3d::set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint)
{
  set_tex(shader_stage, slot, tex, false, face, mip_level, true, as_uint);

  return true;
}

bool d3d::clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level)
{
  render.clearTex((drv3d_metal::Texture*)tex, val, mip_level, face);

  return true;
}

bool d3d::clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level)
{
  render.clearTex((drv3d_metal::Texture*)tex, val, mip_level, face);

  return true;
}

static bool is_depth_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg >= TEXFMT_FIRST_DEPTH && cflg <= TEXFMT_LAST_DEPTH;
}

bool d3d::clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val)
{
  auto texture = (drv3d_metal::Texture*)rt.tex;
  if (is_depth_format_flg(texture->cflg))
  {
    SCOPE_RENDER_TARGET;
    set_depth(texture, DepthAccess::RW);
    clearview(CLEAR_ZBUFFER|CLEAR_STENCIL, 0x00000000, clear_val.asDepth, clear_val.asStencil);
  }
  else
  {
    switch (get_tex_format_desc(texture->cflg & TEXFMT_MASK).mainChannelsType)
    {
      case ChannelDType::UNORM:
      case ChannelDType::SNORM:
      case ChannelDType::UFLOAT:
      case ChannelDType::SFLOAT:
      {
        render.clearTex(texture, clear_val.asFloat, rt.mip_level, rt.layer);
        break;
      }
      case ChannelDType::UINT:
      {
        render.clearTex(texture, clear_val.asUint, rt.mip_level, rt.layer);
        break;
      }
      case ChannelDType::SINT:
      {
        render.clearTex(texture, clear_val.asInt, rt.mip_level, rt.layer);
        break;
      }
      default:
      {
        G_ASSERT_LOG(false, "Unknown texture format");
        render.clearTex(texture, clear_val.asFloat, rt.mip_level, rt.layer);
      }
    }
  }
  return true;
}

bool d3d::issame_texformat(int cflg1, int cflg2)
{
  return drv3d_metal::Texture::isSameFormat(cflg1, cflg2);
}

static bool supportsCompressedTexture(int restype)
{
#if _TARGET_IOS | _TARGET_TVOS
  return false;
#else
  return true;
#endif
}

unsigned d3d::get_texformat_usage(int cflg, int restype)
{
  int fmt = cflg & TEXFMT_MASK;

  unsigned ret = 0;
  if (render.caps.readWriteTextureTier2 ||
      (render.caps.readWriteTextureTier1 && (fmt == TEXFMT_R32F || fmt == TEXFMT_R32UI || fmt == TEXFMT_R32SI)))
  {
    ret |= USAGE_PIXREADWRITE | USAGE_UNORDERED | USAGE_UNORDERED_LOAD;
  }

  switch (fmt)
  {
    case TEXFMT_R8:
    case TEXFMT_R8G8:
    case TEXFMT_A8R8G8B8:
        return USAGE_RTARGET | USAGE_TEXTURE | USAGE_VERTEXTEXTURE | USAGE_FILTER | USAGE_BLEND | USAGE_SRGBREAD | USAGE_SRGBWRITE | ret;
    case TEXFMT_A1R5G5B5:
    case TEXFMT_R5G6B5:
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_A32B32G32R32F:
    case TEXFMT_A2R10G10B10:
    case TEXFMT_R32F:      return USAGE_RTARGET | USAGE_TEXTURE | USAGE_VERTEXTEXTURE | USAGE_FILTER | USAGE_BLEND | ret;

    case TEXFMT_A8:        return USAGE_TEXTURE | USAGE_VERTEXTEXTURE | USAGE_FILTER | USAGE_BLEND | ret;

    case TEXFMT_R11G11B10F: return USAGE_RTARGET | USAGE_TEXTURE | USAGE_FILTER | USAGE_SRGBREAD | USAGE_SRGBWRITE | USAGE_BLEND | USAGE_VERTEXTEXTURE | ret;

    case TEXFMT_A2B10G10R10:
    case TEXFMT_A16B16G16R16:
    case TEXFMT_G32R32F:
    case TEXFMT_R16F:
    case TEXFMT_A4R4G4B4:
    case TEXFMT_L16:
    case TEXFMT_G16R16:
    case TEXFMT_G16R16F:     return USAGE_RTARGET | USAGE_TEXTURE | USAGE_FILTER | USAGE_VERTEXTEXTURE | USAGE_BLEND | ret;

    case TEXFMT_R9G9B9E5:   return USAGE_TEXTURE | USAGE_FILTER | ([render.device supportsFamily:MTLGPUFamilyMac2] ? 0 : USAGE_UNORDERED | USAGE_RTARGET);

    case TEXFMT_DEPTH16:
    case TEXFMT_DEPTH24:
    case TEXFMT_DEPTH32:
    case TEXFMT_DEPTH32_S8:  return USAGE_DEPTH | USAGE_TEXTURE | ret;

    case TEXFMT_ETC2_RGBA:
#if _TARGET_IOS | _TARGET_TVOS
      if (@available(iOS 13, macOS 11.0, *))
      {
        return USAGE_TEXTURE | USAGE_FILTER | USAGE_SRGBREAD | ret;
      }
      else
#endif
      {
        return 0;
      }
    case TEXFMT_ETC2_RG:
#if _TARGET_IOS | _TARGET_TVOS
      if (@available(iOS 13, macOS 11.0, *))
      {
        return USAGE_TEXTURE | USAGE_FILTER | ret;
      }
      else
#endif
      {
        return 0;
      }
    case TEXFMT_ATI1N:
    case TEXFMT_ATI2N:
    case TEXFMT_DXT1:
    case TEXFMT_DXT3:
    case TEXFMT_DXT5:      return (supportsCompressedTexture(restype) ? (USAGE_TEXTURE | USAGE_FILTER | USAGE_SRGBREAD) : 0) | ret;
    default:
        return USAGE_TEXTURE | USAGE_FILTER | USAGE_SRGBREAD | USAGE_SRGBWRITE | USAGE_BLEND | USAGE_VERTEXTEXTURE | ret;
  }
}

bool check_texformat(int cflg, int resType)
{
  unsigned flags = d3d::get_texformat_usage(cflg, resType);
  unsigned mask = d3d::USAGE_TEXTURE;
  if (is_depth_format_flg(cflg))
    mask |= d3d::USAGE_DEPTH;
  else
  {
    if (cflg & TEXCF_RTARGET)
      mask |= d3d::USAGE_RTARGET;
    if (cflg & TEXCF_UNORDERED)
      mask |= d3d::USAGE_UNORDERED;
  }
  return (flags & mask) == mask ? true : false;
}

// Texture management
bool d3d::check_texformat(int cflg)
{
  return check_texformat(cflg, RES3D_TEX);
}

int d3d::get_max_sample_count(int cflg)
{
  for (int samples = get_sample_count(TEXCF_SAMPLECOUNT_MAX); samples; samples >>= 1)
  {
    if ([render.device supportsTextureSampleCount:samples])
      return samples;
  }

  return 1;
}

/// check whether this cube texture format is available
/// returns false if cube texture of the specified format can't be created
bool d3d::check_cubetexformat(int cflg)
{
  return check_texformat(cflg, RES3D_CUBETEX);
}

/// check whether this volume texture format is available
/// returns false if cube texture of the specified format can't be created
bool d3d::check_voltexformat(int cflg)
{
  return check_texformat(cflg, RES3D_VOLTEX);
}

BaseTexture *d3d::alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  return nullptr;
}

CubeTexture *d3d::alias_cubetex(CubeTexture *baseTexture, int size, int flg, int levels, const char* stat_name)
{
  return nullptr;
}

VolTexture *d3d::alias_voltex(VolTexture *baseTexture, int w, int h, int d, int flg, int levels, const char* stat_name)
{
  return nullptr;
}

ArrayTexture *d3d::alias_array_tex(ArrayTexture *baseTexture, int w, int h, int d, int flg, int levels, const char* stat_name)
{
  return nullptr;
}

ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *baseTexture, int side, int d, int flg, int levels, const char* stat_name)
{
  return nullptr;
}
