#include <image/dag_dds.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <3d/ddsFormat.h>
#include <3d/dag_drv3d.h>
#include <osApiWrappers/dag_files.h>

struct BitMaskFormat
{
  uint32_t bitCount;
  uint32_t alphaMask;
  uint32_t redMask;
  uint32_t greenMask;
  uint32_t blueMask;
  D3DFORMAT format;
};

BitMaskFormat bitMaskFormat[] = {
  {24, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_R8G8B8},
  {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8},
  {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8},
  {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5},
  {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5},
  {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5},
  {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4},
  {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2},
  {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8},
  {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8},
  {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2},
  {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4},
  {32, 0xff000000, 0xff, 0xff00, 0xff0000, D3DFMT_A8B8G8R8},
  {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8B8G8R8},
  {16, 0x00000000, 0x0000ff, 0xff00, 0x00, D3DFMT_V8U8},
  {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8},
  {32, 0x00000000, 0x0000FFFF, 0xFFFF0000, 0x00, D3DFMT_V16U16},
  {16, 0x00000000, 0x0000FFFF, 0x00000000, 0x00, D3DFMT_L16},
};

#define UNKNOWN_DDS_FORMAT (0xFFFFFFFFU)

uint32_t get_texformat(D3DFORMAT f)
{
  switch ((int)f)
  {
    case D3DFMT_A8R8G8B8: return TEXFMT_A8R8G8B8;
    case D3DFMT_A16B16G16R16: return TEXFMT_A16B16G16R16;
    case D3DFMT_A16B16G16R16F: return TEXFMT_A16B16G16R16F;
    case D3DFMT_A32B32G32R32F: return TEXFMT_A32B32G32R32F;
    case D3DFMT_DXT1: return TEXFMT_DXT1;
    case D3DFMT_DXT3: return TEXFMT_DXT3;
    case D3DFMT_DXT5: return TEXFMT_DXT5;
    case D3DFMT_G16R16: return TEXFMT_G16R16;
    case D3DFMT_G16R16F: return TEXFMT_G16R16F;
    case D3DFMT_A8B8G8R8: return TEXFMT_R8G8B8A8;
    case D3DFMT_R16F: return TEXFMT_R16F;
    case FOURCC_ATI1N: return TEXFMT_ATI1N;
    case FOURCC_ATI2N: return TEXFMT_ATI2N;
    default: return UNKNOWN_DDS_FORMAT;
  }

  return UNKNOWN_DDS_FORMAT;
}

uint32_t get_texformat_bits(D3DFORMAT f)
{
  switch ((int)f)
  {
    case D3DFMT_A8R8G8B8: return 32;
    case D3DFMT_A16B16G16R16: return 64;
    case D3DFMT_A16B16G16R16F: return 64;
    case D3DFMT_A32B32G32R32F: return 128;
    case D3DFMT_DXT1: return 4;
    case D3DFMT_DXT3: return 8;
    case D3DFMT_DXT5: return 8;
    case D3DFMT_G16R16: return 32;
    case D3DFMT_G16R16F: return 32;
    case D3DFMT_A8B8G8R8: return 32;
    case D3DFMT_R16F: return 16;
    case FOURCC_ATI1N: return 4;
    case FOURCC_ATI2N: return 8;
    default: return 0;
  }

  return 0;
}

bool load_dds(void *ptr, int len, int levels, int topmipmap, ImageInfoDDS &image_info)
{
  if (!ptr)
  {
    logerr("NULL in load_dds");
    return false;
  }

  if (len < sizeof(DDSURFACEDESC2) + 4)
  {
    logerr("invalid DDS format");
    return false;
  }

  if (*(uint32_t *)ptr != MAKEFOURCC('D', 'D', 'S', ' '))
  {
    logerr("invalid DDS format");
    return false;
  }

  DDSURFACEDESC2 &dsc = *(DDSURFACEDESC2 *)((uint32_t *)ptr + 1);

  if (dsc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
  {
    image_info.cube_map = true;
    logerr("cubemap in DDS not supported");
    return false;
  }
  else
    image_info.cube_map = false;

  if (dsc.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME)
  {
    image_info.volume_tex = true;
    logerr("voltex in DDS not supported");
    return false;
  }
  else
    image_info.volume_tex = false;

  int w = dsc.dwWidth;
  int h = dsc.dwHeight;
  int imglev = 1;
  bool dxt = (dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
             ((dsc.ddpfPixelFormat.dwFourCC & 0xFFFFFF) == (FOURCC_DXT1 & 0xFFFFFF) || (dsc.ddpfPixelFormat.dwFourCC & FOURCC_ATI1N) ||
               (dsc.ddpfPixelFormat.dwFourCC & FOURCC_ATI2N));

  if ((dsc.dwFlags & DDSD_MIPMAPCOUNT) || (!(dsc.dwFlags & (DDSD_REFRESHRATE | DDSD_SRCVBHANDLE)) && dsc.dwMipMapCount > 0))
    imglev = dsc.dwMipMapCount;

  if (imglev <= 0)
  {
    logerr("invalid number of mipmaps in DDS");
    return false;
  }

  if (topmipmap >= imglev)
    topmipmap = imglev - 1;

  for (int q = topmipmap; q > 0; --q)
  {
    if (dxt)
    {
      if (w == 4 || h == 4)
      {
        topmipmap -= q;
        break;
      }
    }

    w >>= 1;
    if (w < 1)
      w = 1;
    h >>= 1;
    if (h < 1)
      h = 1;
  }

  if (levels == 0)
    levels = imglev;
  if (levels > imglev - topmipmap)
    levels = imglev - topmipmap;

  D3DFORMAT fmt;
  unsigned shift = 0;
  uint32_t bitCount = 0;

  if (dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
  {
    fmt = (D3DFORMAT)dsc.ddpfPixelFormat.dwFourCC;
    bitCount = get_texformat_bits(fmt);
    if (!bitCount)
      bitCount = dsc.ddpfPixelFormat.dwRGBBitCount;

    if (!bitCount)
    {
      logerr("Unknown DDPF_FOURCC format");
      return false;
    }

    switch ((int)fmt)
    {
      case FOURCC_DXT1:
      case FOURCC_ATI1N: shift = 3; break;
      case FOURCC_DXT2:
      case FOURCC_DXT3:
      case FOURCC_DXT4:
      case FOURCC_DXT5:
      case FOURCC_ATI2N: shift = 4; break;
      default: shift = bitCount / 8; break;
    }
  }
  else
  {
    int i;

    for (i = 0; i < sizeof(bitMaskFormat) / sizeof(BitMaskFormat); i++)
    {
      if (dsc.ddpfPixelFormat.dwRGBBitCount == bitMaskFormat[i].bitCount &&
          (dsc.ddpfPixelFormat.dwRGBAlphaBitMask == bitMaskFormat[i].alphaMask) &&
          (dsc.ddpfPixelFormat.dwRBitMask == bitMaskFormat[i].redMask) &&
          (dsc.ddpfPixelFormat.dwGBitMask == bitMaskFormat[i].greenMask) &&
          (dsc.ddpfPixelFormat.dwBBitMask == bitMaskFormat[i].blueMask))
        break;
    }

    if (i == sizeof(bitMaskFormat) / sizeof(BitMaskFormat))
    {
      logerr("Unknown DDPF_RGB format");
      return false;
    }

    fmt = bitMaskFormat[i].format;
    bitCount = bitMaskFormat[i].bitCount;
    shift = bitCount / 8;

    if (fmt == D3DFMT_V16U16 && dsc.ddpfPixelFormat.dwFlags & DDPF_RGB)
      fmt = D3DFMT_G16R16;
  }

  image_info.nlevels = levels;
  image_info.format = get_texformat(fmt);
  image_info.width = w;
  image_info.height = h;
  image_info.dxt = dxt;
  image_info.shift = shift;

  if (image_info.format == UNKNOWN_DDS_FORMAT)
  {
    logerr("Unknown dds texture format");
    return false;
  }

  w = dsc.dwWidth;
  h = dsc.dwHeight;

  uint8_t *sp = (uint8_t *)ptr + sizeof(DDSURFACEDESC2) + 4;
  len -= sizeof(DDSURFACEDESC2) + 4;
  int lev;

  for (lev = 0; lev < topmipmap; ++lev)
  {
    uint32_t ws = w >> 2;
    if (ws < 1)
      ws = 1;
    uint32_t hs = h >> 2;
    if (hs < 1)
      hs = 1;
    sp += ((ws * hs) << shift) * (dxt ? 1 : shift);
    w >>= 1;
    if (w < 1)
      w = 1;
    h >>= 1;
    if (h < 1)
      h = 1;
  }

  if (w != image_info.width || h != image_info.height)
  {
    debug_ctx("non-matching DDS size: %dx%d (tex) != %dx%d (new)", image_info.width, image_info.height, w, h);
    logerr("non-matching DDS format");
    return 0;
  }

  if (len <= 0)
  {
    debug_ctx("invalid DDS data");
    logerr("invalid DDS data");
    return 0;
  }

  for (lev = 0; lev < levels; ++lev)
  {
    uint32_t ws = w >> 2;
    if (ws < 1)
      ws = 1;
    uint32_t hs = h >> 2;
    if (hs < 1)
      hs = 1;
    uint32_t sz;
    uint32_t pitch;
    if (dxt)
    {
      sz = ((ws * hs) << shift);
      pitch = ws << shift;
    }
    else
    {
      int align = sizeof(uint32_t) - 1;
      pitch = (((w * bitCount / 8) + align) & (~align));
      sz = h * pitch;
    }

    if (len < sz)
    {
      debug_ctx("invalid DDS data2 (%i < %i)", len, sz);
      logerr("invalid DDS data");
      return 0;
    }

    if (lev == 0)
      image_info.pitch = pitch;

    image_info.pixels[lev] = sp;

    sp += sz;
    w >>= 1;
    if (w < 1)
      w = 1;
    h >>= 1;
    if (h < 1)
      h = 1;
  }

  return true;
}

void *load_dds_file(const char *fn, int levels, int quality, IMemAlloc *mem, ImageInfoDDS &image_info, bool ignore_missing)
{
  FullFileLoadCB crd(fn, DF_READ | (ignore_missing ? DF_IGNORE_MISSING : 0));
  if (!crd.fileHandle)
    return NULL;

  int datalen = df_length(crd.fileHandle);
  void *p = mem->tryAlloc(datalen);
  if (p)
    crd.read(p, datalen);
  crd.close();

  if (!p || !load_dds(p, datalen, levels, quality, image_info))
  {
    memfree(p, mem);
    return NULL;
  }

  return p;
}

Texture *create_dds_texture(bool srgb, const ImageInfoDDS &image_info, const char *tex_name, int flags)
{
  Texture *tex = d3d::create_tex(NULL, image_info.width, image_info.height,
    (srgb ? TEXCF_SRGBREAD : 0) | image_info.format | TEXCF_LOADONCE | flags, image_info.nlevels, tex_name ? tex_name : "dds_texture");
  if (!tex)
    return NULL;

  for (unsigned int mipNo = 0; mipNo < image_info.nlevels; mipNo++)
  {
    char *dxtData;
    int dxtPitch;
    if (!tex->lockimg((void **)&dxtData, dxtPitch, mipNo,
          TEXLOCK_WRITE | ((mipNo != image_info.nlevels - 1) ? TEXLOCK_DONOTUPDATEON9EXBYDEFAULT : TEXLOCK_DELSYSMEMCOPY)))
    {
      logerr("%s lockimg failed '%s'", __FUNCTION__, d3d::get_last_error());
      continue;
    }


    unsigned mipH = image_info.height;
    unsigned pitch = image_info.pitch;

    if (image_info.dxt)
    {
      mipH >>= 2;
      if (mipH < 1)
        mipH = 1;
    }

    mipH >>= mipNo;
    pitch >>= mipNo;

    if (dxtPitch == pitch)
      memcpy(dxtData, image_info.pixels[mipNo], mipH * pitch);
    else
      for (int i = 0; i < mipH; ++i)
        memcpy(dxtData + i * dxtPitch, (char *)image_info.pixels[mipNo] + i * pitch, pitch);

    tex->unlockimg();
  }

  return tex;
}
