#include <libTools/dtx/dtxSampler.h>
// #include <debug/dag_debug.h>
#include "rgbe.h"
#include <3d/dag_tex3d.h>
#include <3d/ddsFormat.h>


#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
  ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif // defined(MAKEFOURCC)

inline static bool check_dds_signature(void *ptr) { return *(uint32_t *)ptr == 0x20534444; };


//------------------------------------------------------------------------------

DTXTextureSampler::DTXTextureSampler() :
  data_(NULL),
  ownData_(false),
  dataSize(0),
  addr_mode_u(TEXADDR_WRAP),
  addr_mode_v(TEXADDR_WRAP),
  format_(FMT_RGBA),
  width_(0),
  height_(0),
  pitch_(0),
  rgbOther({{0, 0, 0, 0}, {0, 0, 0, 0}})
{}


//------------------------------------------------------------------------------

DTXTextureSampler::~DTXTextureSampler() { destroy(); }


//------------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct PixelFormat
{
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwFourCC;
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwRGBAlphaBitMask;
};

struct SurfaceDesc2
{
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwHeight;
  uint32_t dwWidth;
  uint32_t dwPitchOrLinearSize;
  uint32_t dwDepth;
  uint32_t dwMipMapCount;
  uint32_t dwReserved1[11];
  PixelFormat pixelFormat;
  uint8_t caps2[16];
  uint32_t dwReserved2;
};
#pragma pack(pop)


//------------------------------------------------------------------------------
static void mask_to_ofs(int mask, int &ofs, int &shift, int target_sz)
{
  if (mask == 0)
  {
    ofs = 32;
    shift = 0;
    return;
  }
  DWORD o, s;
#if _TARGET_64BIT
  o = s = 32;
  __bit_scan_forward(o, mask);
  __bit_scan_reverse(s, mask);
#else
  asm
    {
    mov eax, 32
    mov ebx, mask
    bsf eax, ebx
    mov o, eax
    mov eax, 32
    bsr eax, ebx
    mov s, eax
    }
#endif
  ofs = o;
  shift = target_sz - (s - o + 1);
}

bool DTXTextureSampler::construct(unsigned char *data, size_t size)
{
  int offset;
  if (!ddstexture::getInfo(data, size, &addr_mode_u, &addr_mode_v, &offset))
    return false;
  size -= offset;
  data += offset;

  bool success = false;
  uint32_t *magic = (uint32_t *)data;
  SurfaceDesc2 *desc = (SurfaceDesc2 *)((uint8_t *)data + 4);

  if (check_dds_signature(magic))
  {
    dataSize = size;
    width_ = desc->dwWidth;
    height_ = desc->dwHeight;
    pitch_ = (desc->dwFlags & 0x00000008l) // DDSD_PITCH
               ? desc->dwPitchOrLinearSize
               : desc->dwWidth * (desc->pixelFormat.dwRGBBitCount >> 3);

    data_ = (uint8_t *)data;
    ownData_ = false;

    if (desc->pixelFormat.dwFlags & 0x00000004l) // DDPF_FOURCC
    {
      if (desc->pixelFormat.dwFourCC == 0x00000074) // A32B32G32R32F
      {
        pitch_ = width_ * 4 * sizeof(float);
        format_ = FMT_A32B32G32R32F;
        success = true;
      }
      else if (desc->pixelFormat.dwFourCC == 36) // D3DFMT_A16B16G16R16
      {
        pitch_ = width_ * 4 * 2;
        format_ = FMT_A16B16G16R16;
        success = true;
      }
      else if (desc->pixelFormat.dwFourCC == 113) // D3DFMT_A16B16G16R16F
      {
        pitch_ = width_ * 4 * 2;
        format_ = FMT_A16B16G16R16F;
        success = true;
      }
      else
      {
        success = true;
        switch (desc->pixelFormat.dwFourCC)
        {
          case MAKEFOURCC('D', 'X', 'T', '1'): format_ = FMT_DXT1; break;
          case MAKEFOURCC('D', 'X', 'T', '3'): format_ = FMT_DXT3; break;
          case MAKEFOURCC('D', 'X', 'T', '5'): format_ = FMT_DXT5; break;

          default:
          {
            success = false;
            const char *cc = (char *)&desc->pixelFormat.dwFourCC;
            DAG_FATAL("unsupported four-CC format (%c%c%c%c)", cc[0], cc[1], cc[2], cc[3]);
          }
        }
      }
    }
    else if (desc->pixelFormat.dwRGBBitCount == 32)
    {
      format_ = (desc->pixelFormat.dwFourCC == MAKEFOURCC('R', 'G', 'B', 'E')) ? FMT_RGBE : FMT_RGBA;
      success = true;
    }
    else if (desc->pixelFormat.dwFlags & 0x00000040) // DDPF_RGB
    {
      format_ = FMT_RGB_OTHERS;
      success = true;
    }

    memset(&rgbOther, 0, sizeof(rgbOther));
    if (format_ == FMT_RGB_OTHERS)
    {
      SurfaceDesc2 *desc = (SurfaceDesc2 *)((uint8_t *)data_ + 4);

      rgbOther.bpp = desc->pixelFormat.dwRGBBitCount >> 3;

      mask_to_ofs(desc->pixelFormat.dwRBitMask, rgbOther.ofs.r, rgbOther.shift.r, 8);
      mask_to_ofs(desc->pixelFormat.dwGBitMask, rgbOther.ofs.g, rgbOther.shift.g, 8);
      mask_to_ofs(desc->pixelFormat.dwBBitMask, rgbOther.ofs.b, rgbOther.shift.b, 8);
      if (desc->pixelFormat.dwFlags & 0x00000001)
      {
        mask_to_ofs(desc->pixelFormat.dwRGBAlphaBitMask, rgbOther.ofs.a, rgbOther.shift.a, 8);
      }
      else
      {
        rgbOther.ofs.a = 32;
        rgbOther.shift.a = 0;
      }

      pitch_ = (desc->dwFlags & 0x00000008l) // DDSD_PITCH
                 ? desc->dwPitchOrLinearSize
                 : desc->dwWidth * rgbOther.bpp;
    }
  }

  return success;
}


//------------------------------------------------------------------------------

void DTXTextureSampler::destroy()
{
  if (ownData_)
  {
    delete[] data_;
    data_ = NULL;
  }
}


//------------------------------------------------------------------------------

static void applyAddressing(float &u, int mode)
{
  if (mode == TEXADDR_CLAMP || mode == TEXADDR_BORDER)
  {
    if (u < 0)
      u = 0;
    if (u >= 1)
      u = 1;
  }
  else
  {
    float iter;
    u = modff(u, &iter);
    if (u < 0)
    {
      if (mode == TEXADDR_MIRROR)
        u = fabsf(u);
      else
        u += 1.0f;
    }
  }
}

static void applyAddressing(int &u, int limit, int mode)
{
  if (mode == TEXADDR_CLAMP || mode == TEXADDR_BORDER)
  {
    if (u < 0)
      u = 0;
    if (u >= limit)
      u = limit - 1;
  }
  else if (limit > 0)
  {
    u %= limit;
    if (u < 0)
    {
      if (mode == TEXADDR_MIRROR)
        u = -u;
      else
        u += limit;
    }
  }
}

void DTXTextureSampler::applyAddressing(float &u, float &v) const
{
  /*
    const DTXFooter *footer = (const DTXFooter *)(data_ + dataSize- sizeof(DTXFooter) - 4);
    int addru = footer->addr_mode_u&7;
    int addrv = (footer->addr_mode_v>>3)&7;
    if (!addrv)
      addrv = addru;
    ::applyAddressing(u, addru);
    ::applyAddressing(v, addrv);
  */
  ::applyAddressing(u, addr_mode_u & 7);
  ::applyAddressing(v, addr_mode_v & 7);
}

void DTXTextureSampler::applyAddressing(int &u, int &v) const
{
  /*
    const DTXFooter *footer = (const DTXFooter *)(data_ + dataSize - sizeof(DTXFooter) - 4);
    int addru = footer->addr_mode_x&7;
    int addrv = (footer->addr_mode_x>>3)&7;
    if (!addrv)
      addrv = addru;
    ::applyAddressing(u, width(), addru);
    ::applyAddressing(v, height(), addrv);
  */
  ::applyAddressing(u, width(), addr_mode_u & 7);
  ::applyAddressing(v, height(), addr_mode_v & 7);
}

E3DCOLOR
DTXTextureSampler::e3dcolorValueAt(float u, float v) const
{
  int texelU = u * width_;
  int texelV = v * height_;
  applyAddressing(texelU, texelV);
  return e3dcolorValueAt(texelU, texelV);
}

Color4 DTXTextureSampler::floatValueAt(float u, float v) const
{
  int texelU = u * width_;
  int texelV = v * height_;
  applyAddressing(texelU, texelV);
  return floatValueAt(texelU, texelV);
}

E3DCOLOR
DTXTextureSampler::e3dcolorValueAt(int texelU, int texelV) const
{
  switch (format_)
  {
    case FMT_RGB_OTHERS:
    {
      unsigned at;
      void *p = data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + texelV * pitch_ + texelU * rgbOther.bpp;
      if (rgbOther.bpp == 2)
        at = *(uint16_t *)p;
      else
        at = *(uint32_t *)p;
      E3DCOLOR col;
      col.r = ((at >> rgbOther.ofs.r) << rgbOther.shift.r) & 0xFF;
      col.g = ((at >> rgbOther.ofs.g) << rgbOther.shift.g) & 0xFF;
      col.b = ((at >> rgbOther.ofs.b) << rgbOther.shift.b) & 0xFF;
      col.a = ((at >> rgbOther.ofs.a) << rgbOther.shift.a) & 0xFF;
      return col;
    }
    break;
    case FMT_RGBA:
    case FMT_RGBE:
      return *((uint32_t *)(data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + texelV * pitch_ + texelU * sizeof(uint32_t)));
      break;

    case FMT_DXT1:
    case FMT_DXT3:
    case FMT_DXT5:
    {
      enum
      {
        dxt_BlockWidth = 4,
        dxt_Blockheight = 4,
      };

      uint32_t x = texelU;
      uint32_t y = texelV;
      uint32_t block_sz = (format_ == FMT_DXT1) ? 8 : 8 + 8;
      long stride = (width_ / dxt_BlockWidth) * block_sz;
      uint8_t *block =
        data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + (y / dxt_Blockheight) * stride + (x / dxt_BlockWidth) * block_sz;
      uint8_t *color_block = (format_ == FMT_DXT1) ? block : block + 8;
      uint32_t bx = x & 0x3;
      uint32_t by = y & 0x3;
      uint16_t clr_0 = *((uint16_t *)(color_block + 0));
      uint16_t clr_1 = *((uint16_t *)(color_block + 2));
      uint8_t *color_bmp = color_block + 4;
      uint8_t mask[4] = {0x03, 0x0C, 0x30, 0xC0};
      uint8_t shift[4] = {0, 2, 4, 6};
      uint8_t color_code = ((*(color_bmp + by)) & mask[bx]) >> shift[bx];

      uint32_t color_0 =
        ((((clr_0 & 0x001F) >> 0) << 0) << 3) | ((((clr_0 & 0x07E0) >> 5) << 8) << 2) | ((((clr_0 & 0xF800) >> 11) << 16) << 3);
      uint32_t color_1 =
        ((((clr_1 & 0x001F) >> 0) << 0) << 3) | ((((clr_1 & 0x07E0) >> 5) << 8) << 2) | ((((clr_1 & 0xF800) >> 11) << 16) << 3);
      E3DCOLOR color;


      // get color

      if (clr_0 > clr_1 || format_ != FMT_DXT1)
      {
        switch (color_code)
        {
          case 0x0: color.u = color_0; break;

          case 0x1: color.u = color_1; break;

          case 0x02:
          {
            // (2 * color_0 + color_1 + 1) / 3

            uint32_t r0 = (color_0 & 0x00FF0000) >> 16;
            uint32_t r1 = (color_1 & 0x00FF0000) >> 16;
            uint32_t g0 = (color_0 & 0x0000FF00) >> 8;
            uint32_t g1 = (color_1 & 0x0000FF00) >> 8;
            uint32_t b0 = (color_0 & 0x000000FF);
            uint32_t b1 = (color_1 & 0x000000FF);

            color.u = ((((2 * r0 + r1 + 1) / 3) & 0xFF) << 16) | ((((2 * g0 + g1 + 1) / 3) & 0xFF) << 8) |
                      ((((2 * b0 + b1 + 1) / 3) & 0xFF) << 0);
          }
          break;

          case 0x03:
          {
            // (color_0 + 2 * color_1 + 1) / 3

            uint32_t r0 = (color_0 & 0x00FF0000) >> 16;
            uint32_t r1 = (color_1 & 0x00FF0000) >> 16;
            uint32_t g0 = (color_0 & 0x0000FF00) >> 8;
            uint32_t g1 = (color_1 & 0x0000FF00) >> 8;
            uint32_t b0 = (color_0 & 0x000000FF);
            uint32_t b1 = (color_1 & 0x000000FF);

            color.u = ((((2 * r1 + r0 + 1) / 3) & 0xFF) << 16) | ((((2 * g1 + g0 + 1) / 3) & 0xFF) << 8) |
                      ((((2 * b1 + b0 + 1) / 3) & 0xFF) << 0);
          }
          break;
        } // switch( color_code )
      }
      else
      {
        switch (color_code)
        {
          case 0x0: color = color_0; break;

          case 0x1: color = color_1; break;

          case 0x2:
          {
            // (color_0 + color_1) / 2

            uint32_t r0 = (color_0 & 0x00FF0000) >> 16;
            uint32_t r1 = (color_1 & 0x00FF0000) >> 16;
            uint32_t g0 = (color_0 & 0x0000FF00) >> 8;
            uint32_t g1 = (color_1 & 0x0000FF00) >> 8;
            uint32_t b0 = (color_0 & 0x000000FF);
            uint32_t b1 = (color_1 & 0x000000FF);

            color.u = ((((r1 + r0) / 2) & 0xFF) << 16) | ((((g1 + g0) / 2) & 0xFF) << 8) | ((((b1 + b0) / 2) & 0xFF) << 0);
          }
          break;

          case 0x3: color = 0; break;
        }
      }


      // get alpha

      if (format_ == FMT_DXT1)
      {
        color.a = 0xFF;
      }
      else if (format_ == FMT_DXT3)
      {
        unsigned alpha = *(block + by * 2 + (bx / 2));

        if (bx & 0x1)
          alpha = (alpha & 0x0F) << 4;
        else
          alpha &= 0xF0;

        color.a = alpha;
      }
      else if (format_ == FMT_DXT5) //-V547
      {
        uint32_t alpha_0 = *(block + 0);
        uint32_t alpha_1 = *(block + 1);
        uint32_t alpha = 0x80;
        uint32_t offset[4] = {0, 1, 3, 4};
        uint16_t shift[4] = {0, 4, 0, 4};
        uint16_t bmp = *((uint16_t *)(block + 2 + offset[by])) >> shift[by];
        uint32_t alpha_code = (bmp & (0x0007 << (bx * 3))) >> (bx * 3);

        if (alpha_0 > alpha_1)
        {
          switch (alpha_code)
          {
            case 0: alpha = alpha_0; break;
            case 1: alpha = alpha_1; break;
            case 2: alpha = (6 * alpha_0 + 1 * alpha_1 + 3) / 7; break;
            case 3: alpha = (5 * alpha_0 + 2 * alpha_1 + 3) / 7; break;
            case 4: alpha = (4 * alpha_0 + 3 * alpha_1 + 3) / 7; break;
            case 5: alpha = (3 * alpha_0 + 4 * alpha_1 + 3) / 7; break;
            case 6: alpha = (2 * alpha_0 + 5 * alpha_1 + 3) / 7; break;
            case 7: alpha = (1 * alpha_0 + 6 * alpha_1 + 3) / 7; break;
          }
        }
        else
        {
          switch (alpha_code)
          {
            case 0: alpha = alpha_0; break;
            case 1: alpha = alpha_1; break;
            case 2: alpha = (4 * alpha_0 + 1 * alpha_1 + 2) / 5; break;
            case 3: alpha = (3 * alpha_0 + 2 * alpha_1 + 2) / 5; break;
            case 4: alpha = (2 * alpha_0 + 3 * alpha_1 + 2) / 5; break;
            case 5: alpha = (1 * alpha_0 + 4 * alpha_1 + 2) / 5; break;
            case 6: alpha = 0; break;
            case 7: alpha = 255; break;
          }
        }

        color.a = alpha;
      }

      return color;
    }
    break;
    default: return 0xDEADBEAF;
  }
}


//------------------------------------------------------------------------------

Color4 DTXTextureSampler::floatValueAt(int u, int v) const
{
  Color4 value;

  switch (format_)
  {
    case FMT_RGB_OTHERS:
    case FMT_RGBA:
    case FMT_DXT1:
    case FMT_DXT3:
    case FMT_DXT5:
    {
      value = color4(e3dcolorValueAt(u, v));
    }
    break;
    case FMT_RGBE:
    {
      E3DCOLOR color = e3dcolorValueAt(u, v);
      Color3 fltrgbe = rgbe2float(color);
      return Color4(fltrgbe.r, fltrgbe.g, fltrgbe.b, 0); // swapped
    }
    break;

    case FMT_A32B32G32R32F:
    {

      float *f = (float *)(data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + v * pitch_ + u * 4 * sizeof(float));

      value.r = f[0];
      value.g = f[1];
      value.b = f[2];
      value.a = f[3];
    }
    break;

    case FMT_A16B16G16R16:
    {
      unsigned short int *pixel = (unsigned short int *)(data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + v * pitch_ + u * 4 * 2);

      value.r = pixel[0] / 65535.f;
      value.g = pixel[1] / 65535.f;
      value.b = pixel[2] / 65535.f;
      value.a = pixel[3] / 65535.f;
    }
    break;

    case FMT_A16B16G16R16F:
    {
      unsigned short int *half = (unsigned short int *)(data_ + sizeof(uint32_t) + sizeof(SurfaceDesc2) + v * pitch_ + u * 4 * 2);

      value.r = half_to_float(half[0]);
      value.g = half_to_float(half[1]);
      value.b = half_to_float(half[2]);
      value.a = half_to_float(half[3]);
    }
    break;
  }

  return value;
}
