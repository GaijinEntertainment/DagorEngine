// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/ddsxTex.h>
#include <image/dag_tga.h>
#include <debug/dag_debug3d.h>

#include <image/dag_texPixel.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_debug.h>


void save_rt_image_as_tga(Texture *tex, int level, const char *filename)
{
  G_ASSERT(tex);

  TextureInfo tinfo;
  d3d_err(tex->getinfo(tinfo));
  tinfo.w >>= level;
  tinfo.h >>= level;
  void *ptr;
  int stride;
  d3d_err(tex->lockimg(&ptr, stride, level, TEXLOCK_READ));
  if ((tinfo.cflg & TEXFMT_MASK) == TEXFMT_L16)
  {
    SmallTab<TexPixel8a, TmpmemAlloc> pixels;
    clear_and_resize(pixels, tinfo.w * tinfo.h);
    TexPixel8a *pixptr = pixels.data();
    unsigned short *halfs = (unsigned short *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j, halfs += (stride - tinfo.w * 2) / 2)
      for (int i = 0; i < tinfo.w; ++i, halfs++, pixptr++)
      {
        float pix = *halfs / 65535.0f;
        average += pix;
        pixptr->l = real2uchar(pix);
        pixptr->a = *halfs > 0 ? 255 : 0;
      }
    debug("filename=%s , average = %g", filename, average / (tinfo.w * tinfo.h));
    save_tga8a(filename, pixels.data(), tinfo.w, tinfo.h, tinfo.w * 2);
  }
  else if ((tinfo.cflg & TEXFMT_MASK) == TEXFMT_R16F)
  {
    SmallTab<TexPixel8a, TmpmemAlloc> pixels;
    clear_and_resize(pixels, tinfo.w * tinfo.h);
    TexPixel8a *pixptr = pixels.data();
    unsigned short *halfs = (unsigned short *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j, halfs += (stride - tinfo.w * 2) / 2)
      for (int i = 0; i < tinfo.w; ++i, halfs++, pixptr++)
      {
        float pix = half_to_float(*halfs);
        average += pix;
        pixptr->l = real2uchar(pix);
        pixptr->a = 0;
      }
    debug("filename=%s , average = %g", filename, average / (tinfo.w * tinfo.h));
    save_tga8a(filename, pixels.data(), tinfo.w, tinfo.h, tinfo.w * 2);
  }
  else if ((tinfo.cflg & TEXFMT_MASK) == TEXFMT_L8)
  {
    SmallTab<TexPixel32, TmpmemAlloc> pixels;
    clear_and_resize(pixels, tinfo.w * tinfo.h);
    TexPixel32 *pixptr = pixels.data();
    unsigned char *chars = (unsigned char *)ptr;
    for (int j = 0; j < tinfo.h; ++j, chars += stride - tinfo.w)
      for (int i = 0; i < tinfo.w; ++i, pixptr++, chars++)
      {
        pixptr->r = pixptr->g = pixptr->b = *chars;
        pixptr->a = 255;
      }
    save_tga32(filename, pixels.data(), tinfo.w, tinfo.h, tinfo.w * 4);
  }
  else
    save_tga32(filename, (TexPixel32 *)ptr, tinfo.w, tinfo.h, stride);
  d3d_err(tex->unlockimg());
}

void save_rt_image_as_tga(Texture *tex, const char *filename) { save_rt_image_as_tga(tex, 0, filename); }

#if _TARGET_PC
void save_cube_rt_image_as_tga(CubeTexture *tex, int id, const char *filename, int level)
{
  G_ASSERT(tex);

  TextureInfo tinfo;
  d3d_err(tex->getinfo(tinfo, level));

  void *ptr;
  int stride;
  d3d_err(tex->lockimg(&ptr, stride, id, level, TEXLOCK_READ));
  if ((tinfo.cflg & TEXFMT_R16F) == TEXFMT_R16F)
  {
    SmallTab<TexPixel8a, TmpmemAlloc> pixels;
    clear_and_resize(pixels, tinfo.w * tinfo.h);
    TexPixel8a *pixptr = pixels.data();
    unsigned short *halfs = (unsigned short *)ptr;
    double average = 0;
    for (int j = 0; j < tinfo.h; ++j)
      for (int i = 0; i < tinfo.w; ++i, halfs++, pixptr++)
      {
        float pix = half_to_float(*halfs);
        average += pix;
        pixptr->l = real2uchar(pix);
        pixptr->a = 0;
      }
    debug("filename=%s , average = %g", filename, average / (tinfo.w * tinfo.h));
    save_tga8a(filename, pixels.data(), tinfo.w, tinfo.h, stride);
  }
  else if ((tinfo.cflg & TEXFMT_R11G11B10F) == TEXFMT_R11G11B10F)
  {
    if (stride != tinfo.w * sizeof(int))
    {
      d3d_err(tex->unlockimg());
      return;
    }

    SmallTab<TexPixel32, TmpmemAlloc> pixels;
    clear_and_resize(pixels, tinfo.w * tinfo.h);
    TexPixel32 *pixptr = pixels.data();
    unsigned int *rgbs = (unsigned int *)ptr;
    Color3 average = Color3(0.f, 0.f, 0.f);
    for (int j = 0; j < tinfo.h; ++j)
      for (int i = 0; i < tinfo.w; ++i, rgbs++, pixptr++)
      {
        Color3 color = Color3(half_to_float((*rgbs & 0x7ff) << 4), half_to_float((*rgbs & 0x3ff800) >> 7),
          half_to_float((*rgbs & 0xffc00000) >> 17));

        if (check_nan(color.r) || check_nan(color.g) || check_nan(color.b))
        {
          debug("@%d#%d(%d,%d): %f %f %f", level, id, i, j, color.r, color.g, color.b);
          static int numNans = 0;
          if (numNans == 0)
            G_ASSERT(0);
          numNans++;
          color = Color3(1, 0, 0);
        }
        else
          average += color;

        pixptr->r = real2uchar(color.r);
        pixptr->g = real2uchar(color.g);
        pixptr->b = real2uchar(color.b);
        pixptr->a = 255;
      }
    average /= (tinfo.w * tinfo.h);
    debug("filename=%s , average = (%g, %g, %g)", filename, average.r, average.g, average.b);
    save_tga32(filename, pixels.data(), tinfo.w, tinfo.h, stride);
  }
  else
    save_tga32(filename, (TexPixel32 *)ptr, tinfo.w, tinfo.h, stride);
  d3d_err(tex->unlockimg());
}
#endif

#if _TARGET_PC
enum
{
  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_X8B8G8R8 = 33,
  D3DFMT_G16R16 = 34,
  D3DFMT_A2R10G10B10 = 35,
  D3DFMT_A16B16G16R16 = 36,

  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_L16 = 81,

  D3DFMT_V8U8 = 60,

  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,

  DXGI_FORMAT_BC4_UNORM = 80,
  DXGI_FORMAT_BC5_UNORM = 83,

  D3DFMT_DXT1 = _MAKE4C('DXT1'),
  D3DFMT_DXT2 = _MAKE4C('DXT2'),
  D3DFMT_DXT3 = _MAKE4C('DXT3'),
  D3DFMT_DXT4 = _MAKE4C('DXT4'),
  D3DFMT_DXT5 = _MAKE4C('DXT5'),
  D3DFMT_ATI1 = TEXFMT_ATI1N,
  D3DFMT_ATI2 = TEXFMT_ATI2N,
};

static bool fill_ddsx_hdr(ddsx::Header &hdr, int cflg, int w, int h, int d, int levels)
{
  memset(&hdr, 0, sizeof(hdr));
  switch (cflg & TEXFMT_MASK)
  {
    case TEXFMT_A2R10G10B10:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_A2R10G10B10;
      break;
    case TEXFMT_A2B10G10R10:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_A2B10G10R10;
      break;
    case TEXFMT_A16B16G16R16:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = D3DFMT_A16B16G16R16;
      break;
    case TEXFMT_A16B16G16R16F:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = D3DFMT_A16B16G16R16F;
      break;
    case TEXFMT_A32B32G32R32F:
      hdr.bitsPerPixel = 128;
      hdr.d3dFormat = D3DFMT_A32B32G32R32F;
      break;
    case TEXFMT_G16R16:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_G16R16;
      break;
    case TEXFMT_L16:
      hdr.bitsPerPixel = 16;
      hdr.d3dFormat = D3DFMT_L16;
      break;
    case TEXFMT_A8:
      hdr.bitsPerPixel = 8;
      hdr.d3dFormat = D3DFMT_A8;
      break;
    case TEXFMT_L8:
      hdr.bitsPerPixel = 8;
      hdr.d3dFormat = D3DFMT_L8;
      break;
    case TEXFMT_G16R16F:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_G16R16F;
      break;
    case TEXFMT_G32R32F:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = D3DFMT_G32R32F;
      break;
    case TEXFMT_R16F:
      hdr.bitsPerPixel = 16;
      hdr.d3dFormat = D3DFMT_R16F;
      break;
    case TEXFMT_R32F:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_R32F;
      break;
    case TEXFMT_DXT1:
      hdr.bitsPerPixel = 0;
      hdr.d3dFormat = D3DFMT_DXT1;
      break;
    case TEXFMT_DXT3:
      hdr.bitsPerPixel = 0;
      hdr.d3dFormat = D3DFMT_DXT3;
      break;
    case TEXFMT_DXT5:
      hdr.bitsPerPixel = 0;
      hdr.d3dFormat = D3DFMT_DXT5;
      break;
    case TEXFMT_ATI1N:
      hdr.bitsPerPixel = 0;
      hdr.d3dFormat = D3DFMT_ATI1;
      break;
    case TEXFMT_ATI2N:
      hdr.bitsPerPixel = 0;
      hdr.d3dFormat = D3DFMT_ATI2;
      break;
    case TEXFMT_A8R8G8B8:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = D3DFMT_A8R8G8B8;
      break;
    case TEXFMT_A4R4G4B4:
      hdr.bitsPerPixel = 16;
      hdr.d3dFormat = D3DFMT_A4R4G4B4;
      break;
    case TEXFMT_A1R5G5B5:
      hdr.bitsPerPixel = 16;
      hdr.d3dFormat = D3DFMT_A1R5G5B5;
      break;
    case TEXFMT_R5G6B5:
      hdr.bitsPerPixel = 16;
      hdr.d3dFormat = D3DFMT_R5G6B5;
      break;
    case TEXFMT_A16B16G16R16S:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_A16B16G16R16UI:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_A32B32G32R32UI:
      hdr.bitsPerPixel = 128;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_R32G32UI:
      hdr.bitsPerPixel = 64;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_R32UI:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_R32SI:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_R11G11B10F:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;
    case TEXFMT_R9G9B9E5:
      hdr.bitsPerPixel = 32;
      hdr.d3dFormat = (cflg & TEXFMT_MASK);
      break;

    default: return false;
  }
  hdr.w = w;
  hdr.h = h;
  hdr.depth = d;
  hdr.levels = levels;

  if (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_ATI1)
    hdr.dxtShift = 3;
  else if (hdr.d3dFormat == D3DFMT_DXT2 || hdr.d3dFormat == D3DFMT_DXT3 || hdr.d3dFormat == D3DFMT_DXT4 ||
           hdr.d3dFormat == D3DFMT_DXT5 || hdr.d3dFormat == D3DFMT_ATI2)
    hdr.dxtShift = 4;
  return true;
}
#endif

bool save_tex_as_ddsx(Texture *tex, const char *filename, bool srgb)
{
#if _TARGET_PC
  G_ASSERT(tex);
  TextureInfo ti;
  d3d_err(tex->getinfo(ti));

  ddsx::Header hdr;
  if (!fill_ddsx_hdr(hdr, ti.cflg, ti.w, ti.h, 0, tex->level_count()))
  {
    logerr("unsupported tex with cflags=%08X (%s)", ti.cflg, filename);
    return false;
  }
  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
  {
    logerr("failed to write %s", filename);
    return false;
  }
  cwr.write(&hdr, sizeof(hdr));
  hdr.label = _MAKE4C('DDSx');
  hdr.flags |= (srgb ? 0 : hdr.FLG_GAMMA_EQ_1);

  Texture *texCopy = NULL;
  if (!(ti.cflg & (TEXCF_RTARGET | TEXCF_SYSMEM | TEXCF_UNORDERED | TEXCF_READABLE)))
  {
    ti.cflg &= ~TEXCF_SYSTEXCOPY;
    texCopy = d3d::create_tex(NULL, ti.w, ti.h, ti.cflg | TEXCF_SYSMEM, tex->level_count(), "tmp");
    d3d_err(texCopy->update(tex));
    tex = texCopy;
  }

  for (int level = 0; level < hdr.levels; level++)
  {
    void *ptr;
    int stride, ddsx_stride = hdr.getSurfacePitch(level);
    d3d_err(tex->lockimg(&ptr, stride, level, TEXLOCK_READ));
    G_ASSERT(ddsx_stride <= stride);
    for (int y = hdr.getSurfaceScanlines(level); y > 0; y--, ptr = (char *)ptr + stride)
      cwr.write(ptr, ddsx_stride);
    d3d_err(tex->unlockimg());
  }
  del_d3dres(texCopy);

  hdr.memSz = cwr.tell() - sizeof(hdr);
  cwr.seekto(0);
  cwr.write(&hdr, sizeof(hdr));
  return true;
#else
  G_UNUSED(tex);
  G_UNUSED(filename);
  G_UNUSED(srgb);
  logerr("%s() not implemented", "save_tex_as_ddsx");
  return false;
#endif
}

bool save_cubetex_as_ddsx(CubeTexture *tex, const char *filename, bool srgb)
{
#if _TARGET_PC
  G_ASSERT(tex);
  TextureInfo ti;
  d3d_err(tex->getinfo(ti));

  ddsx::Header hdr;
  if (!fill_ddsx_hdr(hdr, ti.cflg, ti.w, ti.h, 0, ti.mipLevels))
  {
    logerr("unsupported tex with cflags=%08X (%s)", ti.cflg, filename);
    return false;
  }

  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
  {
    logerr("failed to write %s", filename);
    return false;
  }
  cwr.write(&hdr, sizeof(hdr));
  hdr.label = _MAKE4C('DDSx');
  hdr.flags |= hdr.FLG_CUBTEX | (srgb ? hdr.FLG_GAMMA_EQ_1 : 0);

  CubeTexture *texCopy = NULL;
  if (!(ti.cflg & (TEXCF_RTARGET | TEXCF_SYSMEM | TEXCF_UNORDERED | TEXCF_READABLE)))
  {
    ti.cflg &= ~TEXCF_SYSTEXCOPY;
    texCopy = d3d::create_cubetex(ti.w, ti.cflg | TEXCF_SYSMEM, ti.mipLevels, "tmp");
    d3d_err(texCopy->update(tex));
    tex = texCopy;
  }

  for (int ci = 0; ci < 6; ci++)
    for (int level = 0; level < hdr.levels; level++)
    {
      void *ptr;
      int stride, ddsx_stride = hdr.getSurfacePitch(level);
      d3d_err(tex->lockimg(&ptr, stride, ci, level, TEXLOCK_READ));
      G_ASSERT(ddsx_stride <= stride);
      for (int y = hdr.getSurfaceScanlines(level); y > 0; y--, ptr = (char *)ptr + stride)
        cwr.write(ptr, ddsx_stride);
      d3d_err(tex->unlockimg());
    }
  del_d3dres(texCopy);

  hdr.memSz = cwr.tell() - sizeof(hdr);
  cwr.seekto(0);
  cwr.write(&hdr, sizeof(hdr));
  return true;
#else
  G_UNUSED(tex);
  G_UNUSED(filename);
  G_UNUSED(srgb);
  logerr("%s() not implemented", "save_cube_tex_as_ddsx");
  return false;
#endif
}

bool save_voltex_as_ddsx(VolTexture *tex, const char *filename, bool srgb)
{
#if _TARGET_PC
  G_ASSERT(tex);
  TextureInfo ti;
  d3d_err(tex->getinfo(ti));

  ddsx::Header hdr;
  if (!fill_ddsx_hdr(hdr, ti.cflg, ti.w, ti.h, ti.d, tex->level_count()))
  {
    logerr("unsupported tex with cflags=%08X (%s)", ti.cflg, filename);
    return false;
  }

  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
  {
    logerr("failed to write %s", filename);
    return false;
  }
  cwr.write(&hdr, sizeof(hdr));
  hdr.label = _MAKE4C('DDSx');
  hdr.flags |= hdr.FLG_VOLTEX | (srgb ? 0 : hdr.FLG_GAMMA_EQ_1);

  VolTexture *texCopy = NULL;
  if ((ti.cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC)))
  {
    ti.cflg &= ~(TEXCF_RTARGET | TEXCF_DYNAMIC | TEXCF_SYSTEXCOPY);
    texCopy = d3d::create_voltex(ti.w, ti.h, ti.d, ti.cflg | TEXCF_SYSMEM, tex->level_count(), "tmp");
    d3d_err(texCopy->update(tex));
    tex = texCopy;
  }

  for (int level = 0; level < hdr.levels; level++)
  {
    void *ptr;
    int stride, slice_pitch, ddsx_stride = hdr.getSurfacePitch(level);
    d3d_err(tex->lockbox(&ptr, stride, slice_pitch, level, TEXLOCK_READ | TEXLOCK_DELSYSMEMCOPY));

    G_ASSERT(ddsx_stride <= stride);
    for (int d = 0; d < max(hdr.depth >> level, 1); d++)
    {
      char *p = (char *)ptr + d * slice_pitch;
      for (int y = hdr.getSurfaceScanlines(level); y > 0; y--, p += stride)
        cwr.write(p, ddsx_stride);
    }
    d3d_err(tex->unlockbox());
  }
  del_d3dres(texCopy);

  hdr.memSz = cwr.tell() - sizeof(hdr);
  cwr.seekto(0);
  cwr.write(&hdr, sizeof(hdr));
  return true;
#else
  G_UNUSED(tex);
  G_UNUSED(filename);
  G_UNUSED(srgb);
  logerr("%s() not implemented", "save_vol_tex_as_ddsx");
  return false;
#endif
}
