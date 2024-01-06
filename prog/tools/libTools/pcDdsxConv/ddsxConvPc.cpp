#include <util/dag_stdint.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <3d/ddsxTex.h>
#include <3d/ddsFormat.h>
#include <3d/dag_tex3d.h>
#if _TARGET_PC_WIN
#include <d3d9.h>
#include <DXGIFormat.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <math/dag_adjpow2.h>
#include <3d/ddsxTexMipOrder.h>

#define OUT_DEBUG (_TARGET_PC_WIN)

#undef debug
#if OUT_DEBUG
#include <stdlib.h>
#include <stdarg.h>
static void debug(const char *fmt, ...)
{
  va_list ap;
  static char buf[8 * 1024];

  va_start(ap, fmt);
  _vsnprintf(buf, sizeof(buf) - 3, fmt, ap);
  buf[sizeof(buf) - 3] = '\0';
  strcat(buf, "\n");
  OutputDebugString(buf);
  va_end(ap);
}
#else
static inline void debug(const char *fmt, ...) {}
#endif

static const int dxgi_format_bc4_unorm = 80; // DXGI_FORMAT_BC4_UNORM
static const int dxgi_format_bc5_unorm = 83; // DXGI_FORMAT_BC5_UNORM

static char errBuf[256] = "";
static IDdsxCreatorPlugin::IAlloc *alloc = NULL;

#define ERR_PRINTF(...) snprintf(errBuf, sizeof(errBuf), __VA_ARGS__)

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
  {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8},
#if BUILD_FOR_MOBILE_TEXFMT
  {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5},
  {16, 0x00000001, 0x00f800, 0x07c0, 0x3e, D3DFMT_A1R5G5B5},
  {16, 0x0000000f, 0x00f000, 0x0f00, 0xf0, D3DFMT_A4R4G4B4},
#else
  {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8},
  {32, 0xff000000, 0xff, 0xff00, 0xff0000, D3DFMT_A8B8G8R8},
  {32, 0x00000000, 0xff, 0xff00, 0xff0000, D3DFMT_X8B8G8R8},
  {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5},
  {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5},
  {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5},
  {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4},
  {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4},
  {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2},
  {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2},
#endif
  {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8},
  {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8},
  {16, 0x00000000, 0x0000ff, 0xff00, 0x00, (D3DFORMAT)TEXFMT_R8G8},
  {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8},
  {32, 0x00000000, 0x0000FFFF, 0xFFFF0000, 0x00, D3DFMT_V16U16},
  {16, 0x00000000, 0x0000FFFF, 0x00000000, 0x00, D3DFMT_L16},
};

struct SwizzledBitMaskFormat
{
  uint32_t bitCount;
  uint32_t mask[4];
  D3DFORMAT format;
  int shift[4];
  uint32_t andm, orm;

  void swizzle(void *data, int sz) const;
  unsigned sh(unsigned v, int i) const
  {
    v &= mask[i];
    return shift[i] >= 0 ? (v << shift[i]) : (v >> (-shift[i]));
  }
};
SwizzledBitMaskFormat bitMaskFormatSw[] = {
#if BUILD_FOR_MOBILE_TEXFMT
  {32, {0xff000000, 0xff0000, 0xff00, 0xff}, D3DFMT_A8B8G8R8, {0, -16, 0, 16}, 0xFFFFFFFF, 0},
  {32, {0x00000000, 0xff0000, 0xff00, 0xff}, D3DFMT_A8B8G8R8, {0, -16, 0, 16}, 0x00FFFFFF, 0xFF000000},
  {32, {0x00000000, 0xff, 0xff00, 0xff0000}, D3DFMT_A8B8G8R8, {0, 0, 0, 0}, 0x00FFFFFF, 0xFF000000},
  {32, {0xff000000, 0xff, 0xff00, 0xff0000}, D3DFMT_A8B8G8R8, {0, 0, 0, 0}, 0xFFFFFFFF, 0},
  {16, {0x0000f000, 0x000f00, 0x00f0, 0x0f}, (D3DFORMAT)D3DFMT_R4G4B4A4, {-12, 4, 4, 4}, 0xFFFF, 0},
  {16, {0x00000000, 0x000f00, 0x00f0, 0x0f}, (D3DFORMAT)D3DFMT_R4G4B4A4, {0, 4, 4, 4}, 0xFFF0, 0xF},
  {16, {0x00008000, 0x007c00, 0x03e0, 0x1f}, (D3DFORMAT)D3DFMT_R5G5B5A1, {-15, 1, 1, 1}, 0xFFFF, 0},
  {16, {0x00000000, 0x007c00, 0x03e0, 0x1f}, (D3DFORMAT)D3DFMT_R5G5B5A1, {0, 1, 1, 1}, 0xFFFE, 1},
#else
  {16, {0x0000000f, 0x00f000, 0x0f00, 0xf0}, D3DFMT_A4R4G4B4, {12, -4, -4, -4}, 0xFFFF, 0},
  {16, {0x00000001, 0x00f800, 0x07c0, 0x3e}, D3DFMT_A1R5G5B5, {15, -1, -1, -1}, 0xFFFF, 0},
#endif
};

static DDSURFACEDESC2 *unsupported_dx10(const DDSHDR_DXT10 &dx10hdr, int pidx)
{
  ERR_PRINTF("unsupported DX10: dxgiFormat=%d%s resourceDimension=%d%s arraySize=%d%s miscFlag=%08X%s miscFlag2=%08X%s",
    (int)dx10hdr.dxgiFormat, pidx == 0 ? "*" : "", (int)dx10hdr.resourceDimension, pidx == 1 ? "*" : "", (int)dx10hdr.arraySize,
    pidx == 2 ? "*" : "", (int)dx10hdr.miscFlag, pidx == 3 ? "*" : "", (int)dx10hdr.miscFlags2, pidx == 4 ? "*" : "");
  return NULL;
}

static DDSURFACEDESC2 *parse_dds_header(void *dds_data, int dds_len, int &out_data_ofs, int &out_fmt, int &out_bpp)
{
  if (!dds_data)
  {
    strcpy(errBuf, "dds_data=NULL");
    return NULL;
  }
  if (dds_len < sizeof(DDSURFACEDESC2) + 4)
  {
    ERR_PRINTF("dds_len=%d is too short", dds_len);
    return NULL;
  }
  if (*(uint32_t *)dds_data != MAKEFOURCC('D', 'D', 'S', ' '))
  {
    strcpy(errBuf, "no DDS signature");
    return NULL;
  }
  DDSURFACEDESC2 &dsc = *(DDSURFACEDESC2 *)((uint32_t *)dds_data + 1);
  out_data_ofs = sizeof(DDSURFACEDESC2) + 4;
  out_fmt = D3DFMT_UNKNOWN;
  out_bpp = 0;
  if (dsc.dwWidth == 0 && dsc.dwHeight == 0 && dsc.dwMipMapCount == 0) // empty DDS
    return &dsc;

  if ((dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && dsc.ddpfPixelFormat.dwFourCC == FOURCC_DX10)
  {
    out_data_ofs += sizeof(DDSHDR_DXT10);
    DDSHDR_DXT10 &dx10hdr = *(DDSHDR_DXT10 *)(&dsc + 1);
    if (dx10hdr.arraySize > 1)
      return unsupported_dx10(dx10hdr, 3);
    switch (dx10hdr.resourceDimension)
    {
      case DDSHDR_DXT10::RESOURCE_DIMENSION_TEXTURE2D: dsc.ddsCaps.dwCaps2 &= (DDSCAPS2_CUBEMAP | DDSCAPS2_VOLUME); break;
      case DDSHDR_DXT10::RESOURCE_DIMENSION_TEXTURE3D:
        dsc.ddsCaps.dwCaps2 &= DDSCAPS2_CUBEMAP;
        dsc.ddsCaps.dwCaps2 |= DDSCAPS2_VOLUME;
        break;
      default: return unsupported_dx10(dx10hdr, 2);
    }
    switch (dx10hdr.dxgiFormat)
    {
      case DXGI_FORMAT_R32G32B32A32_UINT:
        out_fmt = TEXFMT_A32B32G32R32UI;
        out_bpp = 128;
        break;
      case DXGI_FORMAT_R16G16B16A16_SNORM:
        out_fmt = TEXFMT_A16B16G16R16S;
        out_bpp = 64;
        break;
      case DXGI_FORMAT_R16G16B16A16_UINT:
        out_fmt = TEXFMT_A16B16G16R16UI;
        out_bpp = 64;
        break;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        out_fmt = D3DFMT_A16B16G16R16F;
        out_bpp = 64;
        break;
      case DXGI_FORMAT_R32_UINT:
        out_fmt = TEXFMT_R32UI;
        out_bpp = 32;
        break;
      case DXGI_FORMAT_R11G11B10_FLOAT:
        out_fmt = TEXFMT_R11G11B10F;
        out_bpp = 32;
        break;
      case DXGI_FORMAT_R10G10B10A2_UNORM:
        out_fmt = D3DFMT_A2B10G10R10;
        out_bpp = 32;
        break;
      case DXGI_FORMAT_R16G16_FLOAT:
        out_fmt = D3DFMT_G16R16F;
        out_bpp = 32;
        break;
      case DXGI_FORMAT_R16G16_UNORM:
        out_fmt = D3DFMT_G16R16;
        out_bpp = 32;
        break;
      case DXGI_FORMAT_R16_FLOAT:
        out_fmt = D3DFMT_R16F;
        out_bpp = 16;
        break;
      case DXGI_FORMAT_R8G8_UNORM:
        out_fmt = TEXFMT_R8G8;
        out_bpp = 16;
        break;

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

      default: return unsupported_dx10(dx10hdr, 0);
    }
    dsc.ddpfPixelFormat.dwFlags &= ~(DDPF_RGB | DDPF_ALPHA);
  }

  if (dsc.ddpfPixelFormat.dwFlags & (DDPF_RGB | DDPF_ALPHA | DDPF_LUMINANCE))
  {
    for (const auto &f : bitMaskFormat)
      if (f.bitCount == dsc.ddpfPixelFormat.dwRGBBitCount && f.alphaMask == dsc.ddpfPixelFormat.dwRGBAlphaBitMask &&
          f.redMask == dsc.ddpfPixelFormat.dwRBitMask && f.greenMask == dsc.ddpfPixelFormat.dwGBitMask &&
          f.blueMask == dsc.ddpfPixelFormat.dwBBitMask)
      {
        out_fmt = f.format;
        break;
      }

    if (out_fmt == D3DFMT_UNKNOWN)
      for (const auto &f : bitMaskFormatSw)
        if (f.bitCount == dsc.ddpfPixelFormat.dwRGBBitCount && f.mask[0] == dsc.ddpfPixelFormat.dwRGBAlphaBitMask &&
            f.mask[1] == dsc.ddpfPixelFormat.dwRBitMask && f.mask[2] == dsc.ddpfPixelFormat.dwGBitMask &&
            f.mask[3] == dsc.ddpfPixelFormat.dwBBitMask)
        {
          out_fmt = f.format;
          break;
        }

    if (out_fmt == D3DFMT_UNKNOWN)
    {
      ERR_PRINTF("Unknown DDPF_RGB format: %d bit, %08X %08X %08X %08X", (int)dsc.ddpfPixelFormat.dwRGBBitCount,
        (int)dsc.ddpfPixelFormat.dwRGBAlphaBitMask, (int)dsc.ddpfPixelFormat.dwRBitMask, (int)dsc.ddpfPixelFormat.dwGBitMask,
        (int)dsc.ddpfPixelFormat.dwBBitMask);
      return NULL;
    }

    if (out_fmt == D3DFMT_V16U16 && (dsc.ddpfPixelFormat.dwFlags & DDPF_RGB))
      out_fmt = D3DFMT_G16R16;

    out_bpp = dsc.ddpfPixelFormat.dwRGBBitCount;
  }
  else if ((dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && dsc.ddpfPixelFormat.dwFourCC != FOURCC_DX10)
    out_fmt = dsc.ddpfPixelFormat.dwFourCC;

  if (out_fmt == D3DFMT_UNKNOWN)
  {
    ERR_PRINTF("Unknown format (neither RGB nor FOURCC) flg=%08X", (int)dsc.ddpfPixelFormat.dwFlags);
    return NULL;
  }

  int w = (dsc.dwFlags & DDSD_WIDTH) ? dsc.dwWidth : 0, h = (dsc.dwFlags & DDSD_HEIGHT) ? dsc.dwHeight : 0,
      depth = (dsc.dwFlags & DDSD_DEPTH) ? dsc.dwDepth : 1,
      mip_cnt = (dsc.dwFlags & DDSD_MIPMAPCOUNT) ? (dsc.dwMipMapCount & 0xFFFF) : 1;
  if ((!is_pow_of2(w) || !is_pow_of2(h) || !is_pow_of2(depth)))
  {
    int align = 4;
    for (int i = 1; i < mip_cnt; i++)
      align *= 2;
    if (align && (((w % align) && !is_pow_of2(w)) || ((h % align) && !is_pow_of2(h)) || ((depth % align) && !is_pow_of2(depth))) &&
        (out_fmt == D3DFMT_DXT1 || out_fmt == D3DFMT_DXT2 || out_fmt == D3DFMT_DXT3 || out_fmt == D3DFMT_DXT4 ||
          out_fmt == D3DFMT_DXT5 || out_fmt == _MAKE4C('ATI1') || out_fmt == _MAKE4C('ATI2') || out_fmt == dxgi_format_bc4_unorm ||
          out_fmt == dxgi_format_bc5_unorm || out_fmt == _MAKE4C('BC6H') || out_fmt == _MAKE4C('BC7 ')))
    {
      ERR_PRINTF("bad image size %dx%d, should be %d-aligned for fmt=0x%08X, %d mips", w, h, align, out_fmt, mip_cnt - 1);
      return NULL;
    }
  }
  return &dsc;
}

static uint32_t get_bpp_for_format(uint32_t fmt, uint32_t bpp)
{
  switch (fmt)
  {
    case D3DFMT_A16B16G16R16:
    case D3DFMT_A16B16G16R16F:
    case TEXFMT_A16B16G16R16S:
    case TEXFMT_A16B16G16R16UI: return 64;

    case D3DFMT_A32B32G32R32F:
    case TEXFMT_A32B32G32R32UI: return 128;

    case D3DFMT_R32F:
    case D3DFMT_A2B10G10R10:
    case D3DFMT_G16R16F:
    case D3DFMT_G16R16:
    case TEXFMT_R32UI:
    case TEXFMT_R11G11B10F: return 32;

    case D3DFMT_R16F:
    case D3DFMT_L16:
    case TEXFMT_R8G8: return 16;
  }
  return bpp;
}

static const SwizzledBitMaskFormat *resolve_swizzle(int &fmt, const DDSURFACEDESC2 &dsc)
{
  for (const auto &f : bitMaskFormat)
    if (f.format == fmt && f.bitCount == dsc.ddpfPixelFormat.dwRGBBitCount && f.alphaMask == dsc.ddpfPixelFormat.dwRGBAlphaBitMask &&
        f.redMask == dsc.ddpfPixelFormat.dwRBitMask && f.greenMask == dsc.ddpfPixelFormat.dwGBitMask &&
        f.blueMask == dsc.ddpfPixelFormat.dwBBitMask)
      return nullptr;

  for (const auto &f : bitMaskFormatSw)
    if (f.format == fmt && f.bitCount == dsc.ddpfPixelFormat.dwRGBBitCount && f.mask[0] == dsc.ddpfPixelFormat.dwRGBAlphaBitMask &&
        f.mask[1] == dsc.ddpfPixelFormat.dwRBitMask && f.mask[2] == dsc.ddpfPixelFormat.dwGBitMask &&
        f.mask[3] == dsc.ddpfPixelFormat.dwBBitMask)
      return &f;
  return NULL;
}

void SwizzledBitMaskFormat::swizzle(void *data, int sz) const
{
  if (bitCount == 16)
  {
    for (unsigned short *p = (unsigned short *)data, *pe = p + sz / 2; p < pe; p++)
      *p = ((sh(*p, 0) | sh(*p, 1) | sh(*p, 2) | sh(*p, 3)) & andm) | orm;
  }
  else if (bitCount == 32)
  {
    for (unsigned *p = (unsigned *)data, *pe = p + sz / 4; p < pe; p++)
      *p = ((sh(*p, 0) | sh(*p, 1) | sh(*p, 2) | sh(*p, 3)) & andm) | orm;
  }
}

static inline uint32_t get_surface_sz(int w, int h, int dxt_shift, int bpp)
{
  if (dxt_shift >= 5 && dxt_shift <= 7) // ASTC 4x4, 8x8, 12x12 case
  {
    int bsz = 4 * (dxt_shift - 5 + 1);
    uint32_t bw = (w + bsz - 1) / bsz, bh = (h + bsz - 1) / bsz;
    if (bw < 1)
      bw = 1;
    if (bh < 1)
      bh = 1;
    return bh * bw * 128 / 8;
  }

  if (!dxt_shift)
    return h * (w * bpp / 8);

  uint32_t bw = w >> 2, bh = h >> 2;
  if (bw < 1)
    bw = 1;
  if (bh < 1)
    bh = 1;
  return (bw * bh) << dxt_shift;
}


static inline bool is_pow_2(int i) { return (i & (i - 1)) == 0; }

static bool process_split(int &inout_skip_levels, int &inout_levels, int w, int h, const ddsx::ConvertParams &cp, int &out_hq_lev)
{
  if (!cp.splitAt)
    return true;
  // debug("process_split(inout_skip_levels=%d, inout_levels=%d, w=%d, h=%d, cp=%d,%d) out_hq_lev=%d",
  //   inout_skip_levels, inout_levels, w, h, cp.splitAt, cp.splitHigh, out_hq_lev);

  if (cp.splitHigh)
  {
    if ((w >> inout_skip_levels) <= cp.splitAt && (h >> inout_skip_levels) <= cp.splitAt)
      return false;

    if ((w >> (inout_levels - 1)) > cp.splitAt || (h >> (inout_levels - 1)) > cp.splitAt)
      return false;

    for (int i = 0; i < inout_levels; i++)
    {
      if (w <= cp.splitAt && h <= cp.splitAt)
      {
        inout_levels = i;
        break;
      }
      w = w >> 1;
      h = h >> 1;
      if (w < 1)
        w = 1;
      if (h < 1)
        h = 1;
    }
  }
  else
  {
    for (int i = 0; i < inout_skip_levels; i++)
    {
      w = w >> 1;
      h = h >> 1;
      if (w < 1)
        w = 1;
      if (h < 1)
        h = 1;
    }
    if ((w >> (inout_levels - 1)) > cp.splitAt || (h >> (inout_levels - 1)) > cp.splitAt)
      return true;
    int lev = inout_levels;
    while (inout_levels > 1)
    {
      if (w > cp.splitAt || h > cp.splitAt)
      {
        inout_skip_levels++;
        out_hq_lev++;
        inout_levels--;
      }
      else
        break;
      w = w >> 1;
      h = h >> 1;
      if (w < 1)
        w = 1;
      if (h < 1)
        h = 1;
    }
    inout_levels = lev;
  }
  // debug("  -> inout_skip_levels=%d, inout_levels=%d, out_hq_lev=%d", inout_skip_levels, inout_levels, out_hq_lev);
  return true;
}

static bool make_empty_ddsx(ddsx::Buffer &dest, int flg)
{
  dest.len = sizeof(ddsx::Header);
  dest.ptr = alloc->alloc(dest.len);

  ddsx::Header &hdr = *(ddsx::Header *)dest.ptr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.label = 'xSDD'; // DDSx label
  hdr.flags = flg;
  return true;
}

static bool convert_dds_voltex(ddsx::Buffer &dest, DDSURFACEDESC2 &dsc, uint8_t *sp, int dds_len, int fmt, int bpp,
  const ddsx::ConvertParams &params)
{
  int w = dsc.dwWidth, h = dsc.dwHeight, depth = dsc.dwDepth;
  int levels = (dsc.dwFlags & DDSD_MIPMAPCOUNT) ? dsc.dwMipMapCount : 1;

  if (!(dsc.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME))
  {
    strcpy(errBuf, "not voltex");
    return false;
  }
  if (!is_pow_2(w) || !is_pow_2(h) || depth < 0 || !is_pow_2(depth))
  {
    ERR_PRINTF("non-pow-2 voltex size : %dx%dx%d", w, h, depth);
    return false;
  }
  if (levels <= 0)
  {
    ERR_PRINTF("invalid number of mipmaps in DDS: %d", levels);
    return false;
  }
  if (params.hQMip != 0)
  {
    ERR_PRINTF("invalid number hqMip: %d, must be 0", params.hQMip);
    return false;
  }
  if (params.splitAt != 0 && params.splitHigh || (w == 0 && h == 0 && depth == 0))
    return make_empty_ddsx(dest, ddsx::Header::FLG_VOLTEX);

  auto *swizzle = resolve_swizzle(fmt, dsc);

  int dxt_shift = 0;
  int flg = ddsx::Header::FLG_VOLTEX;
  if (fmt == _MAKE4C('BC4U'))
    fmt = _MAKE4C('ATI1');
  if (fmt == D3DFMT_DXT1 || fmt == _MAKE4C('ATI1') || fmt == dxgi_format_bc4_unorm)
    dxt_shift = 3;
  else if (fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5 || fmt == _MAKE4C('ATI2') ||
           fmt == dxgi_format_bc5_unorm || fmt == _MAKE4C('BC6H') || fmt == _MAKE4C('BC7 '))
    dxt_shift = 4;
#if BUILD_FOR_MOBILE_TEXFMT
  if (fmt == MAKEFOURCC('A', 'S', 'T', '4'))
    dxt_shift = 5, bpp = 8, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', '8'))
    dxt_shift = 6, bpp = 2, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', 'C'))
    dxt_shift = 7, bpp = 0, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
#endif
  bpp = get_bpp_for_format(fmt, bpp);
  if (!bpp && !dxt_shift)
  {
    ERR_PRINTF("format 0x%08X not supported: bits-per-pixel is not determined", fmt);
    return false;
  }

  ddsx::Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.label = 'xSDD'; // DDSx label
  hdr.d3dFormat = fmt;
  hdr.flags = (params.addrU & 0xF) | ((params.addrV & 0xF) << 4) | flg;
  hdr.w = w;
  hdr.h = h;

  if (fabsf(params.imgGamma - 1.0f) < 1e-3f)
    hdr.flags |= hdr.FLG_GAMMA_EQ_1;
  if (params.needSysMemCopy)
    hdr.flags |= hdr.FLG_HOLD_SYSMEM_COPY;

  hdr.levels = levels;
  hdr.depth = depth;
  hdr.bitsPerPixel = bpp;
  hdr.dxtShift = dxt_shift;

  hdr.mQmip = (params.mQMip > 0) ? params.mQMip : 0;
  hdr.lQmip = (params.lQMip > 0) ? params.lQMip : 0;
  hdr.uQmip = hdr.levels - 1;

  if (hdr.dxtShift)
    while ((hdr.w >> hdr.uQmip) < 4 && hdr.uQmip > 0)
      hdr.uQmip--;

  if (hdr.mQmip > hdr.uQmip)
    hdr.mQmip = hdr.uQmip;
  if (hdr.lQmip > hdr.uQmip)
    hdr.lQmip = hdr.uQmip;
  if (hdr.lQmip < hdr.mQmip)
    hdr.lQmip = hdr.mQmip;

  hdr.memSz = 0;

  // calculate full volume size
  w = hdr.w;
  h = hdr.h;
  depth = hdr.depth;
  for (int i = 0; i < hdr.levels; i++)
  {
    uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

    hdr.memSz += sz * depth;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;
    if (depth > 1)
      depth >>= 1;
  }

  if (hdr.memSz > dds_len)
  {
    ERR_PRINTF("invalid DDS: need %d bytes, only %d available", hdr.memSz, dds_len);
    return false;
  }

  // allocate buffer
  dest.len = sizeof(ddsx::Header) + hdr.memSz;
  dest.ptr = alloc->alloc(dest.len);
  memcpy(dest.ptr, &hdr, sizeof(hdr));
  memcpy((uint8_t *)dest.ptr + sizeof(ddsx::Header), sp, hdr.memSz);
  if (swizzle)
    swizzle->swizzle((uint8_t *)dest.ptr + sizeof(ddsx::Header), hdr.memSz);

  if (params.mipOrdRev)
    ddsx_reverse_mips_inplace(dest.ptr, dest.len);
  return true;
}

static bool convert_dds_cubetex(ddsx::Buffer &dest, DDSURFACEDESC2 &dsc, uint8_t *sp, int dds_len, int fmt, int bpp,
  const ddsx::ConvertParams &params)
{
  int w = dsc.dwWidth, h = dsc.dwHeight;
  int levels = (dsc.dwFlags & DDSD_MIPMAPCOUNT) ? (dsc.dwMipMapCount & 0xFFFF) : 1,
      skip_levels = (params.hQMip < levels) ? params.hQMip : levels - 1;
  int hq_lev = dsc.dwMipMapCount >> 16;
  if (!process_split(skip_levels, levels, w, h, params, hq_lev) || (w == 0 && h == 0))
    return make_empty_ddsx(dest, ddsx::Header::FLG_CUBTEX);

  if (!(dsc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
  {
    strcpy(errBuf, "not cubetex");
    return false;
  }
  if (w != h)
  {
    ERR_PRINTF("non-square DDS cubemap: %dx%d", w, h);
    return false;
  }
  if (levels <= 0)
  {
    ERR_PRINTF("invalid number of mipmaps in DDS: %d", levels);
    return false;
  }

  auto *swizzle = resolve_swizzle(fmt, dsc);

  if ((dsc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) != DDSCAPS2_CUBEMAP_ALLFACES)
  {
    ERR_PRINTF("Cannot create partial cubtex: %08X", unsigned(dsc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES));
    return false;
  }

  int dxt_shift = 0;
  int flg = ddsx::Header::FLG_CUBTEX;
#if BUILD_FOR_MOBILE_TEXFMT
  if (fmt == MAKEFOURCC('A', 'S', 'T', '4'))
    dxt_shift = 5, bpp = 8, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', '8'))
    dxt_shift = 6, bpp = 2, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', 'C'))
    dxt_shift = 7, bpp = 0, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
#endif
#ifdef IOS_EXP
  else if (fmt == D3DFMT_DXT1 || fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5)
  {
    ERR_PRINTF("Cannot transcode cubemap DXT format=%08X %c%c%c%c", fmt, _DUMP4C(fmt));
    return false;
  }
  else if (fmt == D3DFMT_R32F || fmt == D3DFMT_R16F)
    ; // ok
  else if (dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
  {
    ERR_PRINTF("Unknown FOURCC format=%08X %c%c%c%c", fmt, _DUMP4C(fmt));
    return false;
  }
#else
  if (fmt == _MAKE4C('BC4U'))
    fmt = _MAKE4C('ATI1');
  if (fmt == D3DFMT_DXT1 || fmt == _MAKE4C('ATI1') || fmt == dxgi_format_bc4_unorm)
    dxt_shift = 3;
  else if (fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5 || fmt == _MAKE4C('ATI2') ||
           fmt == dxgi_format_bc5_unorm || fmt == _MAKE4C('BC6H') || fmt == _MAKE4C('BC7 '))
    dxt_shift = 4;
#endif
  bpp = get_bpp_for_format(fmt, bpp);
  if (!bpp && !dxt_shift)
  {
    ERR_PRINTF("format 0x%08X not supported: bits-per-pixel is not determined", fmt);
    return false;
  }

  ddsx::Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.label = 'xSDD'; // DDSx label
  hdr.d3dFormat = fmt;
  hdr.flags = flg;
  hdr.w = max(w >> skip_levels, 1);
  hdr.h = max(h >> skip_levels, 1);

  if (fabsf(params.imgGamma - 1.0f) < 1e-3f)
    hdr.flags |= hdr.FLG_GAMMA_EQ_1;
  if (params.needSysMemCopy)
    hdr.flags |= hdr.FLG_HOLD_SYSMEM_COPY;
  if (params.splitHigh)
    hdr.flags |= hdr.FLG_HQ_PART;

  hdr.levels = levels - skip_levels;
  hdr.hqPartLevels = hq_lev;
  hdr.depth = 0;
  hdr.bitsPerPixel = bpp;
  hdr.dxtShift = dxt_shift;

  hdr.mQmip = (params.mQMip > skip_levels) ? params.mQMip - skip_levels : 0;
  hdr.lQmip = (params.lQMip > skip_levels) ? params.lQMip - skip_levels : 0;
  hdr.uQmip = hdr.levels - 1;

  if (hdr.dxtShift)
    while ((hdr.w >> hdr.uQmip) < 4 && hdr.uQmip > 0)
      hdr.uQmip--;

  if (hdr.mQmip > hdr.uQmip)
    hdr.mQmip = hdr.uQmip;
  if (hdr.lQmip > hdr.uQmip)
    hdr.lQmip = hdr.uQmip;
  if (hdr.lQmip < hdr.mQmip)
    hdr.lQmip = hdr.mQmip;

  hdr.memSz = 0;

  // calculate one face size
  w = hdr.w;
  h = hdr.h;
  for (int i = 0; i < hdr.levels; i++)
  {
    uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

    hdr.memSz += sz;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;
  }

  // allocate buffer
  hdr.memSz *= 6;
  dest.len = sizeof(ddsx::Header) + hdr.memSz;
  dest.ptr = alloc->alloc(dest.len);
  memcpy(dest.ptr, &hdr, sizeof(hdr));

  // copy 6 faces
  for (int j = 0; j < 6; j++)
  {
    uint8_t *dp = (uint8_t *)dest.ptr + sizeof(ddsx::Header) + hdr.memSz / 6 * j;
    w = hdr.w << skip_levels;
    h = hdr.h << skip_levels;
    for (int i = 0; i < skip_levels; i++)
    {
      uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

      sp += sz;
      dds_len -= sz;
      if (w > 1)
        w >>= 1;
      if (h > 1)
        h >>= 1;
    }
    for (int i = 0; i < hdr.levels; i++)
    {
      uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

      memcpy(dp, sp, sz);
      if (swizzle)
        swizzle->swizzle(dp, sz);
      dp += sz;
      sp += sz;
      dds_len -= sz;
      if (w > 1)
        w >>= 1;
      if (h > 1)
        h >>= 1;

      if (dds_len < 0)
      {
        strcpy(errBuf, "Invalid DDS");
        return false;
      }
    }
  }

  if (params.mipOrdRev)
    ddsx_reverse_mips_inplace(dest.ptr, dest.len);
  return true;
}

static bool convert_dds_tex(ddsx::Buffer &dest, DDSURFACEDESC2 &dsc, uint8_t *sp, int dds_len, int fmt, int bpp,
  const ddsx::ConvertParams &params)
{
  int w = dsc.dwWidth, h = dsc.dwHeight;
  int levels = (dsc.dwFlags & DDSD_MIPMAPCOUNT) ? (dsc.dwMipMapCount & 0xFFFF) : 1,
      skip_levels = (params.hQMip < levels) ? params.hQMip : levels - 1;
  int hq_lev = dsc.dwMipMapCount >> 16;
  if (!process_split(skip_levels, levels, w, h, params, hq_lev) || (w == 0 && h == 0))
    return make_empty_ddsx(dest, 0);

  if (dsc.ddsCaps.dwCaps2 & (DDSCAPS2_CUBEMAP | DDSCAPS2_VOLUME))
  {
    strcpy(errBuf, "not plain tex");
    return false;
  }

  auto *swizzle = resolve_swizzle(fmt, dsc);

  int dxt_shift = 0;
  int flg = 0;
#if BUILD_FOR_MOBILE_TEXFMT
  if (fmt == MAKEFOURCC('A', 'S', 'T', '4'))
    dxt_shift = 5, bpp = 8, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', '8'))
    dxt_shift = 6, bpp = 2, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
  else if (fmt == MAKEFOURCC('A', 'S', 'T', 'C'))
    dxt_shift = 7, bpp = 0, flg |= ddsx::Header::FLG_MOBILE_TEXFMT;
#endif
#ifdef IOS_EXP
  else if (fmt == D3DFMT_DXT1 || fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5)
  {
    ERR_PRINTF("Cannot transcode DXT format=%08X %c%c%c%c", fmt, _DUMP4C(fmt));
    return false;
  }
  else if (fmt == D3DFMT_R32F || fmt == D3DFMT_R16F || fmt == D3DFMT_A32B32G32R32F)
    ; // ok
  else if (dsc.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
  {
    ERR_PRINTF("Unknown FOURCC format=%08X %c%c%c%c", fmt, _DUMP4C(fmt));
    return false;
  }
#else
  if (fmt == _MAKE4C('BC4U'))
    fmt = _MAKE4C('ATI1');
  if (fmt == D3DFMT_DXT1 || fmt == _MAKE4C('ATI1') || fmt == dxgi_format_bc4_unorm)
    dxt_shift = 3;
  else if (fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5 || fmt == _MAKE4C('ATI2') ||
           fmt == dxgi_format_bc5_unorm || fmt == _MAKE4C('BC6H') || fmt == _MAKE4C('BC7 '))
    dxt_shift = 4;
#endif

  bpp = get_bpp_for_format(fmt, bpp);
  if (!bpp && !dxt_shift)
  {
    ERR_PRINTF("format 0x%08X not supported: bits-per-pixel is not determined", fmt);
    return false;
  }

  ddsx::Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.label = 'xSDD'; // DDSx label
  hdr.d3dFormat = fmt;
  hdr.flags = flg | (params.addrU & 0xF) | ((params.addrV & 0xF) << 4);
  hdr.w = max(w >> skip_levels, 1);
  hdr.h = max(h >> skip_levels, 1);

  hdr.levels = levels - skip_levels;
  hdr.hqPartLevels = hq_lev;
  hdr.depth = 0;
  hdr.bitsPerPixel = bpp;

  if (fabsf(params.imgGamma - 1.0f) < 1e-3f)
    hdr.flags |= hdr.FLG_GAMMA_EQ_1;
  if (params.needSysMemCopy)
    hdr.flags |= hdr.FLG_HOLD_SYSMEM_COPY;
  if (params.splitHigh)
    hdr.flags |= hdr.FLG_HQ_PART;

  int levels_to_store = hdr.levels, suffix_sz = 0;
  hdr.dxtShift = dxt_shift;

  hdr.mQmip = (params.mQMip > skip_levels) ? params.mQMip - skip_levels : 0;
  hdr.lQmip = (params.lQMip > skip_levels) ? params.lQMip - skip_levels : 0;
  hdr.uQmip = hdr.levels - 1;
  if (hdr.dxtShift)
    while (((hdr.w >> hdr.uQmip) < 4 || (hdr.h >> hdr.uQmip) < 4) && hdr.uQmip > 0)
      hdr.uQmip--;

  if (hdr.mQmip > hdr.uQmip)
    hdr.mQmip = hdr.uQmip;
  if (hdr.lQmip > hdr.uQmip)
    hdr.lQmip = hdr.uQmip;
  if (hdr.lQmip < hdr.mQmip)
    hdr.lQmip = hdr.mQmip;

#if !BUILD_FOR_MOBILE_TEXFMT
  if ((levels_to_store > 1 || params.needSysMemCopy || params.needBaseTex) &&
      (fmt == D3DFMT_DXT1 || fmt == D3DFMT_DXT5 || fmt == D3DFMT_A8R8G8B8 || fmt == D3DFMT_X8R8G8B8 || fmt == _MAKE4C('ATI1') ||
        fmt == _MAKE4C('ATI2') || fmt == dxgi_format_bc4_unorm || fmt == dxgi_format_bc5_unorm))
  {
    if (params.rtGenMipsBox)
    {
      hdr.flags |= hdr.FLG_GENMIP_BOX;
      levels_to_store = 1;
      suffix_sz = sizeof(float) * 1;
    }
    else if (params.rtGenMipsKaizer)
    {
      hdr.flags |= hdr.FLG_GENMIP_KAIZER;
      levels_to_store = 1;
      suffix_sz = sizeof(float) * 3;
    }
  }
#endif

  if (params.needBaseTex)
  {
    if (!params.rtGenMipsBox && !params.rtGenMipsKaizer)
    {
      ERR_PRINTF("bad combo rtGenMipsBox=%d rtGenMipsKaizer=%d needBaseTex=%d", params.rtGenMipsBox, params.rtGenMipsKaizer,
        params.needBaseTex);
      return false;
    }
    hdr.flags |= hdr.FLG_NEED_PAIRED_BASETEX;
    hdr.memSz = sizeof(float) * 3 + dds_len;

    dest.len = sizeof(ddsx::Header) + hdr.memSz;
    dest.ptr = alloc->alloc(dest.len);

    char *dp = (char *)dest.ptr;

    memcpy(dp, &hdr, sizeof(hdr));
    dp += sizeof(hdr);

    float *prefix_data = (float *)dp;
    ((float *)dp)[0] = params.imgGamma;
    ((float *)dp)[1] = params.kaizerAlpha, ((float *)dp)[2] = params.kaizerStretch;
    dp += 3 * sizeof(float);

    memcpy(dp, sp, hdr.memSz - sizeof(float) * 3);
    dp += hdr.memSz - sizeof(float) * 3;

    if (dp - (char *)dest.ptr != hdr.memSz + sizeof(hdr))
    {
      ERR_PRINTF("IE: dp-dest.ptr=%d hdr.memSz=%d", int(dp - (char *)dest.ptr), hdr.memSz);
      return false;
    }
    return true;
  }

  hdr.memSz = suffix_sz;

  for (int i = 0; i < skip_levels; i++)
  {
    uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

    sp += sz;
    dds_len -= sz;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;
  }
  for (int i = 0; i < levels_to_store; i++)
  {
    uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);

    hdr.memSz += sz;
    dds_len -= sz;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;

    if (dds_len < 0)
    {
      strcpy(errBuf, "Invalid DDS");
      return false;
    }
  }

  dest.len = sizeof(ddsx::Header) + hdr.memSz;
  dest.ptr = alloc->alloc(dest.len);

  memcpy(dest.ptr, &hdr, sizeof(hdr));
  memcpy((char *)dest.ptr + sizeof(hdr), sp, hdr.memSz - suffix_sz);

  if (swizzle)
  {
    char *pix_data = (char *)dest.ptr + sizeof(hdr);
    w = hdr.w, h = hdr.h;
    for (int i = 0; i < levels_to_store; i++)
    {
      uint32_t sz = get_surface_sz(w, h, dxt_shift, bpp);
      swizzle->swizzle(pix_data, sz);
      pix_data += sz;
      if (w > 1)
        w >>= 1;
      if (h > 1)
        h >>= 1;
    }
  }

  float *suffix_data = (float *)((char *)dest.ptr + sizeof(hdr) + hdr.memSz - suffix_sz);

  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
    suffix_data[0] = params.imgGamma;

  if (hdr.flags & hdr.FLG_GENMIP_KAIZER)
  {
    suffix_data[1] = params.kaizerAlpha, suffix_data[2] = params.kaizerStretch;
  }
  if (params.mipOrdRev)
    ddsx_reverse_mips_inplace(dest.ptr, dest.len);
  return true;
}

class PcDdsxExport : public IDdsxCreatorPlugin
{
public:
  virtual bool __stdcall startup() { return true; }

  virtual void __stdcall shutdown() {}

  virtual unsigned __stdcall targetCode()
  {
#define _MAKE4C(x) MAKE4C((int(x) >> 24) & 0xFF, (int(x) >> 16) & 0xFF, (int(x) >> 8) & 0xFF, int(x) & 0xFF)
#ifdef IOS_EXP
    return _MAKE4C('iOS');
#elif defined(TEGRA_EXP)
    return _MAKE4C('and');
#else
    return _MAKE4C('PC');
#endif
  }

  virtual bool __stdcall convertDds(ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &p)
  {
    dest.zero();

    int data_ofs = 0, fmt = 0, bpp = 0;
    DDSURFACEDESC2 *dsc = parse_dds_header(dds_data, dds_len, data_ofs, fmt, bpp);

    if (!dsc)
      return false;

    if (dsc->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME)
      return convert_dds_voltex(dest, *dsc, ((uint8_t *)dds_data) + data_ofs, dds_len - data_ofs, fmt, bpp, p);
    if (dsc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
      return convert_dds_cubetex(dest, *dsc, ((uint8_t *)dds_data) + data_ofs, dds_len - data_ofs, fmt, bpp, p);
    else
      return convert_dds_tex(dest, *dsc, ((uint8_t *)dds_data) + data_ofs, dds_len - data_ofs, fmt, bpp, p);
  }

  virtual int __stdcall getTexDataHeaderSize(const void * /*data*/) const { return sizeof(ddsx::Header); }
  virtual bool __stdcall checkCompressionSupport(int compr_flag) const
  {
    return compr_flag == ddsx::Header::FLG_ZLIB || compr_flag == ddsx::Header::FLG_7ZIP || compr_flag == ddsx::Header::FLG_OODLE ||
           compr_flag == ddsx::Header::FLG_ZSTD;
  }
  virtual void __stdcall setTexDataHeaderCompression(void *data, int compr_sz, int compr_flag) const
  {
    if (!checkCompressionSupport(compr_flag) || (compr_flag & ddsx::Header::FLG_COMPR_MASK) != compr_flag)
      return;
    ddsx::Header *hdr = (ddsx::Header *)data;
    hdr->flags &= ~hdr->FLG_COMPR_MASK;
    hdr->flags |= compr_flag;
    hdr->packedSz = compr_sz;
  }

  virtual const char *__stdcall getLastErrorText() { return errBuf[0] ? errBuf : NULL; }
};

static PcDdsxExport plugin;

extern "C"
#if _TARGET_PC_WIN
  __declspec(dllexport)
#elif _TARGET_PC_LINUX
  __attribute__((visibility("default")))
#endif
    IDdsxCreatorPlugin *__stdcall get_plugin(IDdsxCreatorPlugin::IAlloc *a)
{
  alloc = a;
  return &plugin;
}
