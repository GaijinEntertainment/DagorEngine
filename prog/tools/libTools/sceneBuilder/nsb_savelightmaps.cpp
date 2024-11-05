// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/ddsFormat.h>
#include <string.h>
#include <util/dag_bitArray.h>
#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_StdTonemapper.h>
#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_zlibIo.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <image/dag_dxtCompress.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_memIo.h>
#include <libTools/dtx/dtx.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <stdlib.h>


namespace StaticSceneBuilder
{
int save_lightmap_files(const char *fname_base, TexImage32 *img, int scene_format, int target_code, mkbindump::BinDumpSaveCB &cwr,
  bool keep_ref_files);

int save_lightmap_hdr(const char *fname_base, IPoint2 lm_sz, Color4 *img, StaticSceneBuilder::StdTonemapper &tonemapper,
  mkbindump::BinDumpSaveCB &cwr, bool keep_ref_files);

int save_lightmap_bump(const char *fname_base, TexImage32 *flat_img, TexImage32 *sunbump_color_img, mkbindump::BinDumpSaveCB &cwr,
  bool keep_ref_files);
} // namespace StaticSceneBuilder


static __forceinline E3DCOLOR blend23(E3DCOLOR c1, E3DCOLOR c2)
{
  return E3DCOLOR((c1.r * 2 + c2.r + 1) / 3, (c1.g * 2 + c2.g + 1) / 3, (c1.b * 2 + c2.b + 1) / 3, (c1.a * 2 + c2.a + 1) / 3);
}


static __forceinline E3DCOLOR convertToNormalizedColor(E3DCOLOR c)
{
  int l = c.r;
  if (c.g > l)
    l = c.g;
  if (c.b > l)
    l = c.b;

  c.a = l;

  if (l > 0)
  {
    c.r = c.r * 255 / l;
    c.g = c.g * 255 / l;
    c.b = c.b * 255 / l;
  }

  return c;
}


static __forceinline real colorDiff(const TexPixel32 &c1, const TexPixel32 &c2)
{
  real dr = c1.r - c2.r;
  real dg = c1.g - c2.g;
  real db = c1.b - c2.b;
  return (dr * dr + dg * dg + db * db) * (float(c1.a) * c1.a / (255 * 255)) / (255 * 255);
}


static __forceinline uint16_t make565Color(E3DCOLOR c)
{
  return (((c.r * 31 + 127) / 255) << 11) | (((c.g * 63 + 127) / 255) << 5) | ((c.b * 31 + 127) / 255);
}


static void killBlackPixels(TexPixel32 cell[16])
{
  int numBlack = 0, highBr = 0;
  E3DCOLOR highColor = 0;

  for (int i = 0; i < 16; ++i)
  {
    E3DCOLOR c = cell[i].c;
    int br = c.r + c.g + c.b;

    if (br == 0)
    {
      ++numBlack;
      cell[i].c = 0;
    }
    else
    {
      br *= c.a;
      if (br > highBr)
      {
        highBr = br;
        highColor = c;
      }
    }
  }

  if (numBlack > 0)
    for (int i = 0; i < 16 && numBlack > 0; ++i)
      if (cell[i].c == 0)
      {
        cell[i].c = highColor;
        cell[i].a = 0;
        --numBlack;
      }
}


static __forceinline void packDxtCellColor(TexPixel32 cell[16], uint8_t *&output)
{
  real bestErr = MAX_REAL;
  uint32_t bestCode = 0;
  E3DCOLOR bestC1, bestC2;

  static TexPixel32 palette[16];

  int numPalColors = 1;
  palette[0] = cell[0];
  palette[0].a = 0;

  for (int i = 1; i < 16; ++i)
  {
    E3DCOLOR c = cell[i].c;
    c.a = 0;

    int j;
    for (j = 0; j < numPalColors; ++j)
      if (palette[j].c == c)
        break;

    if (j >= numPalColors)
      palette[numPalColors++].c = c;
  }

  if (numPalColors > 2)
  {
    for (int c1 = 0; c1 < numPalColors - 1; ++c1)
    {
      for (int c2 = c1 + 1; c2 < numPalColors; ++c2)
      {
        real err = 0;

        static TexPixel32 ramp[4];
        ramp[0] = palette[c1];
        ramp[1] = palette[c2];
        ramp[2].c = blend23(ramp[0].c, ramp[1].c);
        ramp[3].c = blend23(ramp[1].c, ramp[0].c);

        uint32_t curCode = 0;

        for (int i = 0; i < 16; ++i)
        {
          uint32_t bestInd = 0;
          real bestDiff = colorDiff(cell[i], ramp[bestInd]);

          for (int j = 1; j < 4; ++j)
          {
            real d = colorDiff(cell[i], ramp[j]);
            if (d < bestDiff)
            {
              bestDiff = d;
              bestInd = j;
            }
          }

          curCode |= bestInd << (i * 2);

          err += bestDiff;

          if (err >= bestErr)
            break;
        }

        if (err < bestErr)
        {
          bestErr = err;
          bestCode = curCode;
          bestC1 = ramp[0].c;
          bestC2 = ramp[1].c;
        }
      }
    }
  }
  else
  {
    // map 1 or 2 colors to indices
    bestCode = 0;

    bestC1 = palette[0].c;
    bestC2 = palette[numPalColors > 1 ? 1 : 0].c;

    for (int i = 0; i < 16; ++i)
    {
      E3DCOLOR c = cell[i].c;
      c.a = 0;

      if (palette[0].c != c)
        bestCode |= 1 << (i * 2);
    }
  }

  // output packed color
  uint16_t pc1 = ::make565Color(bestC1);
  uint16_t pc2 = ::make565Color(bestC2);

  if (pc1 == pc2)
  {
    bestCode = 0;
  }
  else if (pc1 < pc2)
  {
    // swap
    uint16_t c = pc1;
    pc1 = pc2;
    pc2 = c;
    bestCode ^= 0x55555555;
  }

  *(uint16_t *)output = pc1;
  output += 2;
  *(uint16_t *)output = pc2;
  output += 2;

  *(uint32_t *)output = bestCode;
  output += 4;
}

static __forceinline void packDxtCellInterpAlpha(TexPixel32 cell[16], uint8_t *&output)
{
  uint8_t minAlpha = 255, maxAlpha = 0;
  bool useBlack = false;

  int i;
  for (i = 0; i < 16; ++i)
  {
    if (cell[i].a == 0)
      useBlack = true;
    else
    {
      if (cell[i].a < minAlpha)
        minAlpha = cell[i].a;
      if (cell[i].a > maxAlpha)
        maxAlpha = cell[i].a;
    }
  }

  static uint8_t alpha[8];

  if (useBlack)
  {
    // 6-alpha block.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    alpha[0] = minAlpha;
    alpha[1] = maxAlpha;
    alpha[2] = (4 * alpha[0] + 1 * alpha[1] + 2) / 5; // Bit code 010
    alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5; // Bit code 011
    alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5; // Bit code 100
    alpha[5] = (1 * alpha[0] + 4 * alpha[1] + 2) / 5; // Bit code 101
    alpha[6] = 0;                                     // Bit code 110
    alpha[7] = 255;                                   // Bit code 111
  }
  else
  {
    // 8-alpha block.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    alpha[0] = maxAlpha;
    alpha[1] = minAlpha;
    alpha[2] = (6 * alpha[0] + 1 * alpha[1] + 3) / 7; // bit code 010
    alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7; // bit code 011
    alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7; // bit code 100
    alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7; // bit code 101
    alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7; // bit code 110
    alpha[7] = (1 * alpha[0] + 6 * alpha[1] + 3) / 7; // bit code 111
  }

  __int64 code = 0;
  for (i = 0; i < 16; ++i)
  {
    int a = cell[i].a;

    int bestCode = 0;
    int bestDiff = abs(a - alpha[0]);

    for (int j = 1; j < 8; ++j)
    {
      int d = abs(a - alpha[j]);
      if (d < bestDiff)
      {
        bestDiff = d;
        bestCode = j;
      }
    }

    code |= __int64(bestCode) << (i * 3);
  }

  // output packed alpha
  output[0] = alpha[0];
  output[1] = alpha[1];
  output += 2;

  *(uint32_t *)output = code;
  output += 4;
  *(uint16_t *)output = code >> 32;
  output += 2;
}


static void *packFlatLightmapDxt(TexPixel32 *pix, int img_w, int img_h, int &data_size)
{
  int dataSize = 4 + sizeof(DDSURFACEDESC2) + img_w * img_h;
  void *data = memalloc(dataSize, tmpmem);

  uint8_t *ptr = (uint8_t *)data;

  *((DWORD *)ptr) = MAKEFOURCC('D', 'D', 'S', ' ');
  ptr += 4;

  DDSURFACEDESC2 &dsd = *(DDSURFACEDESC2 *)ptr;

  memset(&dsd, 0, sizeof(DDSURFACEDESC2));

  G_ASSERT(sizeof(DDSURFACEDESC2) == 124);

  dsd.dwSize = sizeof(DDSURFACEDESC2);
  dsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE;
  dsd.dwWidth = img_w;
  dsd.dwHeight = img_h;
  dsd.dwLinearSize = dsd.dwWidth * dsd.dwHeight;
  dsd.dwMipMapCount = 0;

  dsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;

  dsd.ddpfPixelFormat.dwSize = 32;
  dsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
  dsd.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;

  ptr += sizeof(DDSURFACEDESC2);

  // pack DXT image
  for (int y = 0, ofs = 0; y < dsd.dwHeight; y += 4, ofs += dsd.dwWidth * 4)
  {
    for (int x = 0, o = ofs; x < dsd.dwWidth; x += 4, o += 4)
    {
      static TexPixel32 cell[16];

      TexPixel32 *p = &pix[o];
      int i;
      for (i = 0; i < 4; ++i, p += dsd.dwWidth)
      {
        cell[i * 4 + 0].c = p[0].c;
        cell[i * 4 + 1].c = p[1].c;
        cell[i * 4 + 2].c = p[2].c;
        cell[i * 4 + 3].c = p[3].c;
      }

      ::killBlackPixels(cell);
      ::packDxtCellInterpAlpha(cell, ptr);
      ::packDxtCellColor(cell, ptr);
    }
  }

  G_ASSERT(ptr == ((uint8_t *)data + dataSize));

  data_size = dataSize;
  return data;
}


int StaticSceneBuilder::save_lightmap_files(const char *fname_base, TexImage32 *img, int scene_format, int target_code,
  mkbindump::BinDumpSaveCB &cwr, bool keep_ref_files)
{
  if (keep_ref_files)
  {
    String fname(260, "%s.tga", fname_base);
    save_tga32(fname, img);
  }

  SmallTab<TexPixel32, TmpmemAlloc> pix;
  pix = make_span_const((TexPixel32 *)(img + 1), img->w * img->h);

  // Normalize colors.
  if (scene_format != SCENE_FORMAT_AO_DXT1)
    for (int i = 0; i < img->w * img->h; ++i)
      pix[i].c = ::convertToNormalizedColor(pix[i].c);


  // save tga
  int pos;
  if (scene_format == SCENE_FORMAT_LdrTgaDds || scene_format == SCENE_FORMAT_LdrTga)
  {
    if (keep_ref_files)
    {
      String fname(260, "%sscn.tga", fname_base);
      save_tga32(fname, pix.data(), img->w, img->h, img->w * 4);
    }

    pos = cwr.tell();

    ddstexture::Converter cnv;
    cnv.format = ddstexture::Converter::fmtARGB;
    cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
    cnv.mipmapCount = ddstexture::Converter::AllMipMaps;

    DynamicMemGeneralSaveCB memcwr(tmpmem, img->w * img->h * (4 * 3 / 2) + 256);
    if (!cnv.convertImage(memcwr, pix.data(), img->w, img->h, img->w * 4))
      throw IGenSave::SaveException("Error converting image to DDS", cwr.tell());

    ddsx::Buffer b;
    ddsx::ConvertParams cp;
    cp.allowNonPow2 = false;
    cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
    cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
    cp.packSzThres = 0;
    cp.canPack = true;
    cp.mQMip = 1;
    cp.lQMip = 2;

    if (!ddsx::convert_dds(target_code, b, memcwr.data(), memcwr.size(), cp))
      debug("error converting to DDSx: %s", ddsx::get_last_error_text());

    cwr.beginBlock();
    cwr.writeRaw(b.ptr, b.len);
    cwr.endBlock();

    b.free();
  }

  if (scene_format == SCENE_FORMAT_LdrTga)
  {
    return pos;
  }

  // make DDS
  int dataSize = 0;
  void *data = nullptr;

  if (scene_format == SCENE_FORMAT_AO_DXT1)
  {
    ddstexture::Converter cnv;
    cnv.format = ddstexture::Converter::fmtDXT1;
    cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
    cnv.mipmapCount = ddstexture::Converter::AllMipMaps;

    DynamicMemGeneralSaveCB memcwr(tmpmem, img->w * img->h * 3 / 2 + 256);
    if (!cnv.convertImage(memcwr, pix.data(), img->w, img->h, img->w * 4))
      throw IGenSave::SaveException("Error converting image to DXT1", cwr.tell());
    data = memalloc(memcwr.size(), tmpmem);
    memcpy(data, memcwr.data(), memcwr.size());
  }
  else
    data = ::packFlatLightmapDxt(pix.data(), img->w, img->h, dataSize);

  // save DDS
  if (keep_ref_files)
  {
    file_ptr_t file = df_open(String(fname_base) + "c.dds", DF_WRITE | DF_CREATE);
    if (!file)
      return -1;

    if (df_write(file, data, dataSize) != dataSize)
    {
      df_close(file);
      return -1;
    }

    df_close(file);
  }

  ddsx::Buffer buf;
  ddsx::ConvertParams cp;
  cp.allowNonPow2 = false;
  cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
  cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
  cp.packSzThres = 0;
  cp.canPack = true;
  cp.mQMip = 1;
  cp.lQMip = 2;

  if (!ddsx::convert_dds(target_code, buf, data, dataSize, cp))
    debug("error converting to DDSx: %s", ddsx::get_last_error_text());

  memfree(data, tmpmem);

  // copy lightmap to scene stream
  pos = cwr.tell();

  cwr.beginBlock();
  cwr.writeRaw(buf.ptr, buf.len);
  cwr.endBlock();

  buf.free();

  return pos;
}


int StaticSceneBuilder::save_lightmap_bump(const char *fname_base, TexImage32 *flat_img, TexImage32 *sunbump_color_img,
  mkbindump::BinDumpSaveCB &cwr, bool keep_ref_files)
{
  TexImage32 *lightmaps[] = {flat_img, sunbump_color_img};
  const char *fileSuffixes[] = {"_flat", "_sun"};

  if (keep_ref_files)
  {
    for (unsigned int lightmapNo = 0; lightmapNo < sizeof(lightmaps) / sizeof(lightmaps[0]); lightmapNo++)
    {
      String fname(260, "%s%s.tga", fname_base, fileSuffixes[lightmapNo]);
      save_tga32(fname, lightmaps[lightmapNo]);
    }
  }


  // Normalize colors.
  SmallTab<TexPixel32, TmpmemAlloc> original_pix[2];

  for (unsigned int lightmapNo = 0; lightmapNo < 2; lightmapNo++)
  {
    original_pix[lightmapNo] =
      make_span_const((TexPixel32 *)(lightmaps[lightmapNo] + 1), lightmaps[lightmapNo]->w * lightmaps[lightmapNo]->h);
  }

  for (unsigned int lightmapNo = 0; lightmapNo < 1; lightmapNo++)
  {
    TexPixel32 *pix = (TexPixel32 *)(lightmaps[lightmapNo] + 1);
    for (int i = 0; i < lightmaps[lightmapNo]->w * lightmaps[lightmapNo]->h; ++i)
      pix[i].c = ::convertToNormalizedColor(pix[i].c);
  }

  int pos;

  pos = cwr.tell();
  for (unsigned int lightmapNo = 0; lightmapNo < 2; lightmapNo++)
  {
    int dataSize;
    void *data;
    int data2Size = 0;
    void *data2 = 0;
    if (lightmapNo == 0)
    {
      data =
        ::packFlatLightmapDxt((TexPixel32 *)(lightmaps[lightmapNo] + 1), lightmaps[lightmapNo]->w, lightmaps[lightmapNo]->h, dataSize);
    }
    else
    {
      TexPixel32 *chrominance = (TexPixel32 *)memalloc(sunbump_color_img->h * sunbump_color_img->w * sizeof(TexPixel32), tmpmem);
      TexPixel32 *p = (TexPixel32 *)(sunbump_color_img + 1);
      TexPixel32 *cp = chrominance;

      TexPixel32 *p1 = p;
      for (int y = 0; y < lightmaps[lightmapNo]->h; y++)
        for (int x = 0; x < lightmaps[lightmapNo]->w; x++, ++p1, ++cp)
        {
          cp->c = ::convertToNormalizedColor(p1->c);
          p1 = (p + x + y * lightmaps[lightmapNo]->w);
          p1->g = p1->a;
          p1->r = p1->b = p1->a;
          p1->a = 0;
        }

      if (keep_ref_files)
      {
        String fname(260, "%s%s_amsun.tga", fname_base, fileSuffixes[lightmapNo]);
        save_tga32(fname, lightmaps[lightmapNo]);
        fname.printf(260, "%s%s_acr.tga", fname_base, fileSuffixes[lightmapNo]);
        save_tga32(fname, chrominance, lightmaps[lightmapNo]->w, lightmaps[lightmapNo]->h, lightmaps[lightmapNo]->w);
      }

      ddstexture::Converter cnv;
      cnv.format = ddstexture::Converter::fmtL8;
      cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
      cnv.mipmapCount = ddstexture::Converter::AllMipMaps;
      {
        DynamicMemGeneralSaveCB memcwr(tmpmem, lightmaps[lightmapNo]->w * lightmaps[lightmapNo]->h * 3 / 2 + 256);

        if (!cnv.convertImage(memcwr, (TexPixel32 *)(sunbump_color_img + 1), lightmaps[lightmapNo]->w, lightmaps[lightmapNo]->h,
              lightmaps[lightmapNo]->w * 4))
          throw IGenSave::SaveException("Error converting image to DDS", cwr.tell());

        data = memcwr.copy();
        dataSize = memcwr.size();
      }

      data2 = ::packFlatLightmapDxt(chrominance, lightmaps[lightmapNo]->w, lightmaps[lightmapNo]->h, data2Size);

      memfree(chrominance, tmpmem);
    }
    // save DDS
    if (keep_ref_files)
    {
      String fname(260, "%s%s_c.dds", fname_base, fileSuffixes[lightmapNo]);
      file_ptr_t file = df_open(fname, DF_WRITE | DF_CREATE);
      if (!file)
        return -1;

      if (df_write(file, data, dataSize) != dataSize)
      {
        df_close(file);
        return -1;
      }

      df_close(file);
      if (data2)
      {
        String fname(260, "%s%s_cc.dds", fname_base, fileSuffixes[lightmapNo]);
        file_ptr_t file = df_open(fname, DF_WRITE | DF_CREATE);
        if (!file)
          return -1;

        if (df_write(file, data2, data2Size) != data2Size)
        {
          df_close(file);
          return -1;
        }

        df_close(file);
      }
    }

    ddsx::Buffer buf;
    ddsx::ConvertParams cp;
    cp.allowNonPow2 = false;
    cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
    cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
    cp.packSzThres = 0;
    cp.canPack = true;
    cp.mQMip = 1;
    cp.lQMip = 2;

    if (!ddsx::convert_dds(cwr.getTarget(), buf, data, dataSize, cp))
      debug("error converting to DDSx: %s", ddsx::get_last_error_text());

    // copy lightmap to scene stream
    cwr.beginBlock();
    cwr.writeRaw(buf.ptr, buf.len);
    cwr.endBlock();

    if (buf.len)
      debug("DDSx lightmap compression: %.2f", float(dataSize) / buf.len);
    buf.free();
    memfree(data, tmpmem);
    if (data2)
    {
      ddsx::Buffer buf;
      cp.mQMip = 1;
      cp.lQMip = 1;

      if (!ddsx::convert_dds(cwr.getTarget(), buf, data2, data2Size, cp))
        debug("error converting chrominance to DDSx: %s", ddsx::get_last_error_text());

      // copy lightmap to scene stream
      cwr.beginBlock();
      cwr.writeRaw(buf.ptr, buf.len);
      cwr.endBlock();

      memfree(data2, tmpmem);
      if (buf.len)
        debug("DDSx lightmap color compression: %.2f", float(data2Size) / buf.len);
      debug("DDSx lightmap color compression size: %d", buf.len);
      buf.free();
    }
  }

  // restore original data
  for (unsigned int lightmapNo = 0; lightmapNo < 2; lightmapNo++)
    mem_copy_to(original_pix[lightmapNo], (TexPixel32 *)(lightmaps[lightmapNo] + 1));

  return pos;
}


struct rgbe_header_info;
extern int RGBE_WritePixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines);
extern int RGBE_WriteHeader(file_ptr_t fp, int width, int height, rgbe_header_info *info);
#define RGBE_RETURN_FAILURE -1

int StaticSceneBuilder::save_lightmap_hdr(const char *fname_base, IPoint2 lm_sz, Color4 *img,
  StaticSceneBuilder::StdTonemapper &tonemapper, mkbindump::BinDumpSaveCB &cwr, bool keep_ref_files)
{
  bool saveToRgbe = false;
  if (saveToRgbe)
  {
    // Save hdr.
    Tab<Color3> pixelList(tmpmem);
    pixelList.resize(lm_sz.x * lm_sz.y);

    for (int i = 0; i < pixelList.size(); i++, img++)
      pixelList[i].set_rgb(tonemapper.transformColorHdr(*img));

    String fname(260, "%sh.hdr", fname_base);
    file_ptr_t file = df_open(fname, DF_WRITE | DF_CREATE);
    if (!file)
      return -1;

    if (RGBE_WriteHeader(file, lm_sz.x, lm_sz.y, NULL) == RGBE_RETURN_FAILURE)
    {
      df_close(file);
      return -1;
    }

    if (RGBE_WritePixels_RLE(file, (float *)&pixelList[0], lm_sz.x, lm_sz.y) == RGBE_RETURN_FAILURE)
    {
      df_close(file);
      return -1;
    }
    df_close(file);

    // copy lightmap to scene stream
    int pos = cwr.tell();
    file_ptr_t fp = df_open(fname, DF_READ);
    cwr.writeInt32e(df_length(fp));
    cwr.beginBlock();
    ZlibSaveCB zlibCb(cwr.getRawWriter(), ZlibSaveCB::CL_BestSpeed + 5);
    copy_file_to_stream(fp, zlibCb);
    zlibCb.finish();
    cwr.endBlock();
    debug("HDR RGBE lightmap compression: %.2f", zlibCb.getCompressionRatio());
    df_close(fp);

    if (!keep_ref_files)
      dd_erase(fname);
    return pos;
  }
  else // saveToRgbe
  {
    Tab<TexPixel32> pixelList(tmpmem);
    pixelList.resize(lm_sz.x * lm_sz.y);

    // convert to RGBM
    for (int i = 0; i < pixelList.size(); i++, img++)
      pixelList[i].c = tonemapper.mapHdrToRgbmE3dcolor(*img);

    int dataSize;
    void *data = CompressDXT(MODE_DXT5, &pixelList[0], lm_sz.x * 4, lm_sz.x, lm_sz.y, 0, &dataSize);

    // save DDS
    if (keep_ref_files)
    {
      file_ptr_t file = df_open(String(fname_base) + "h.dds", DF_WRITE | DF_CREATE);
      if (!file)
        return -1;

      if (df_write(file, data, dataSize) != dataSize)
      {
        df_close(file);
        return -1;
      }

      df_close(file);
    }

    // copy lightmap to scene stream
    int pos = cwr.tell();
    cwr.writeInt32e(dataSize);
    cwr.beginBlock();
    ZlibSaveCB zlibCb(cwr.getRawWriter(), ZlibSaveCB::CL_BestSpeed + 5);
    zlibCb.write(data, dataSize);
    zlibCb.finish();
    cwr.endBlock();
    memfree(data, tmpmem);
    debug("HDR DDS lightmap compression: %.2f", zlibCb.getCompressionRatio());

    return pos;
  }
}
