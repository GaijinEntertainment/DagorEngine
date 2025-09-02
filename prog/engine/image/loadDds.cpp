// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_dds.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_debug.h>
#include <3d/ddsFormat.h>
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

static BitMaskFormat bitMaskFormat[] = {
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
  {16, 0x00000000, 0x0000FFFF, 0x00000000, 0x00, D3DFMT_L16},
};

#define UNKNOWN_DDS_FORMAT (0xFFFFFFFFU)

static uint32_t get_texformat(D3DFORMAT f)
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
    case D3DFMT_L8: return TEXFMT_R8;
    case D3DFMT_A8: return TEXFMT_A8;
    case D3DFMT_A8L8: return TEXFMT_R8G8;
    case D3DFMT_A4R4G4B4: return TEXFMT_A4R4G4B4;
    default: return UNKNOWN_DDS_FORMAT;
  }

  return UNKNOWN_DDS_FORMAT;
}

static uint32_t get_texformat_bits(D3DFORMAT f)
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

  unsigned dds_hdr_sz = sizeof(DDSURFACEDESC2) + 4;
  if (len < dds_hdr_sz)
  {
    logerr("invalid DDS format: len=%d < dds_hdr_sz=%d", len, dds_hdr_sz);
    return false;
  }

  if (*(uint32_t *)ptr != MAKEFOURCC('D', 'D', 'S', ' '))
  {
    logerr("invalid DDS format");
    return false;
  }

  bool dds_hdr_only = (len == dds_hdr_sz);
  DDSURFACEDESC2 &dsc = *(DDSURFACEDESC2 *)((uint32_t *)ptr + 1);
  if ((dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && dsc.ddpfPixelFormat.dwFourCC == FOURCC_DX10)
  {
    dds_hdr_sz += sizeof(DDSHDR_DXT10);
    if (len < dds_hdr_sz)
    {
      logerr("invalid DDS format: len=%d < dds_hdr_sz=%d", len, dds_hdr_sz);
      return false;
    }
    dds_hdr_only = (len == dds_hdr_sz);
    DDSHDR_DXT10 &dx10hdr = *(DDSHDR_DXT10 *)(void *)(&dsc + 1);
    if (dx10hdr.arraySize > 1)
    {
      logerr("invalid DDS format: DX10 with arraySize=%d", dx10hdr.arraySize);
      return false;
    }
    switch (dx10hdr.resourceDimension)
    {
      case DDSHDR_DXT10::RESOURCE_DIMENSION_TEXTURE2D: dsc.ddsCaps.dwCaps2 &= (DDSCAPS2_CUBEMAP | DDSCAPS2_VOLUME); break;
      case DDSHDR_DXT10::RESOURCE_DIMENSION_TEXTURE3D:
        dsc.ddsCaps.dwCaps2 = (dsc.ddsCaps.dwCaps2 & ~DDSCAPS2_CUBEMAP) | DDSCAPS2_VOLUME;
        break;
      default: logerr("invalid DDS format: DX10 with resourceDimension=%d", dx10hdr.resourceDimension); return false;
    }
    unsigned out_fmt = 0;
    switch (dx10hdr.dxgiFormat)
    {
      case DXGI_FORMAT_R32G32B32A32_UINT: out_fmt = TEXFMT_A32B32G32R32UI; break;
      case DXGI_FORMAT_R16G16B16A16_SNORM: out_fmt = TEXFMT_A16B16G16R16S; break;
      case DXGI_FORMAT_R16G16B16A16_UINT: out_fmt = TEXFMT_A16B16G16R16UI; break;
      case DXGI_FORMAT_R16G16B16A16_FLOAT: out_fmt = D3DFMT_A16B16G16R16F; break;
      case DXGI_FORMAT_R32_UINT: out_fmt = TEXFMT_R32UI; break;
      case DXGI_FORMAT_R11G11B10_FLOAT: out_fmt = TEXFMT_R11G11B10F; break;
      case DXGI_FORMAT_R10G10B10A2_UNORM: out_fmt = D3DFMT_A2B10G10R10; break;
      case DXGI_FORMAT_R16G16_FLOAT: out_fmt = D3DFMT_G16R16F; break;
      case DXGI_FORMAT_R16G16_UNORM: out_fmt = D3DFMT_G16R16; break;
      case DXGI_FORMAT_R16_FLOAT: out_fmt = D3DFMT_R16F; break;
      case DXGI_FORMAT_R8G8_UNORM: out_fmt = TEXFMT_R8G8; break;

      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC1_UNORM: out_fmt = D3DFMT_DXT1; break;

      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC2_UNORM: out_fmt = D3DFMT_DXT3; break;

      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC3_UNORM: out_fmt = D3DFMT_DXT5; break;

      case DXGI_FORMAT_BC4_UNORM: out_fmt = _MAKE4C('ATI1'); break;
      case DXGI_FORMAT_BC5_UNORM: out_fmt = _MAKE4C('ATI2'); break;
      case DXGI_FORMAT_BC6H_UF16: out_fmt = _MAKE4C('BC6H'); break;

      case DXGI_FORMAT_BC7_UNORM_SRGB:
      case DXGI_FORMAT_BC7_UNORM: out_fmt = _MAKE4C('BC7 '); break;

      default: logerr("invalid DDS format: DX10 with dxgiFormat=%d (0x%x)", dx10hdr.dxgiFormat, dx10hdr.dxgiFormat); return false;
    }
    dsc.ddpfPixelFormat.dwFourCC = out_fmt;
    dsc.ddpfPixelFormat.dwFlags &= ~(DDPF_RGB | DDPF_ALPHA);
  }

  image_info.cube_map = (dsc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP) ? true : false;
  image_info.volume_tex = (dsc.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) ? true : false;
  if (!dds_hdr_only && image_info.cube_map)
  {
    logerr("cubemap in DDS not supported");
    return false;
  }
  if (!dds_hdr_only && image_info.volume_tex)
  {
    logerr("voltex in DDS not supported");
    return false;
  }

  int w = dsc.dwWidth;
  int h = dsc.dwHeight;
  int d = dsc.dwDepth;
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
    if (image_info.volume_tex)
      d = max(d >> 1, 1);
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
    if (fmt == (D3DFORMAT)_MAKE4C('BC4U'))
      fmt = (D3DFORMAT)FOURCC_ATI1N;
    bitCount = get_texformat_bits(fmt);
    if (!bitCount)
      bitCount = dsc.ddpfPixelFormat.dwRGBBitCount;

    if (!bitCount)
    {
      logerr("Unknown DDPF_FOURCC format: %c%c%c%c", _DUMP4C(dsc.ddpfPixelFormat.dwFourCC));
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
  }

  image_info.nlevels = levels;
  image_info.format = get_texformat(fmt);
  image_info.d3dFormat = fmt;
  image_info.width = w;
  image_info.height = h;
  image_info.depth = d ? d : 1;
  image_info.dxt = dxt;
  image_info.shift = shift;

  if (image_info.format == UNKNOWN_DDS_FORMAT)
  {
    logerr("Unknown dds texture format: d3dFormat=0x%x", fmt);
    return false;
  }

  w = dsc.dwWidth;
  h = dsc.dwHeight;
  d = dsc.dwDepth;

  uint8_t *sp = (uint8_t *)ptr + dds_hdr_sz;
  len -= dds_hdr_sz;
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
    DEBUG_CTX("non-matching DDS size: %dx%d (tex) != %dx%d (new)", image_info.width, image_info.height, w, h);
    logerr("non-matching DDS format");
    return 0;
  }

  if (dds_hdr_only) // only header is passed, so clear pointers and return success
  {
    memset(image_info.pixels, 0, sizeof(image_info.pixels));
    return true;
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
      DEBUG_CTX("invalid DDS data2 (%i < %i)", len, sz);
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

bool read_dds_info(const char *fn, ImageInfoDDS &out_image_info)
{
  FullFileLoadCB crd(fn, DF_READ | DF_IGNORE_MISSING);
  if (!crd.fileHandle)
    return false;
  static const unsigned DDS_HDR_SZ = sizeof(DDSURFACEDESC2) + 4;
  static const unsigned DX10_HDR_SZ = sizeof(DDSHDR_DXT10);
  char hdr[DDS_HDR_SZ + DX10_HDR_SZ];

  unsigned hdr_sz = DDS_HDR_SZ;
  if (crd.tryRead(hdr, DDS_HDR_SZ) < DDS_HDR_SZ)
    return false;

  DDSURFACEDESC2 &dsc = *(DDSURFACEDESC2 *)(void *)(&hdr[4]);
  if ((dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && dsc.ddpfPixelFormat.dwFourCC == FOURCC_DX10)
  {
    if (crd.tryRead(hdr + DDS_HDR_SZ, DX10_HDR_SZ) < DX10_HDR_SZ)
      return false;
    hdr_sz += DX10_HDR_SZ;
  }
  return load_dds(&hdr, hdr_sz, 0, 0, out_image_info);
}
