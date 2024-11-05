// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <math/dag_adjpow2.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_stdint.h>


inline int integer_sqrt(int n)
{
  if (n < 0)
    return -1;
  if (n < 2)
    return n;
  const int smallCandidate = integer_sqrt(n >> 2) << 1;
  const int largeCandidate = smallCandidate + 1;
  return largeCandidate * largeCandidate > n ? smallCandidate : largeCandidate;
}

static inline TexPtr create_tex_from_raw_hmap_file(const char *name, int &width)
{
  FullFileLoadCB cb(name);
  if (!cb.fileHandle)
  {
    logerr("file not found %s", name);
    return {};
  }
  int fsize = df_length(cb.fileHandle);
  width = integer_sqrt(fsize / 2);
  G_ASSERTF(width * width * 2 == fsize, "%d fsize %d", width, fsize);
  ShaderGlobal::set_color4(get_shader_variable_id("tex_hmap_inv_sizes"), 1.0 / width, 1.0 / width, 0, 0);

  uint32_t texFMT = TEXFMT_L16;
  if ((d3d::get_texformat_usage(TEXFMT_L16) & d3d::USAGE_VERTEXTEXTURE))
    texFMT = TEXFMT_L16; //-V1048
  else if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_VERTEXTEXTURE))
    texFMT = TEXFMT_R32F;
  else if ((d3d::get_texformat_usage(TEXFMT_R16F) & d3d::USAGE_VERTEXTEXTURE)) /*better fast, than accurate*/
    texFMT = TEXFMT_R16F;
  else
    logerr("no suitable vertex texture found");
  int level_count = 1; // get_log2i(width);

  TexPtr tex = dag::create_tex(nullptr, width, width, texFMT, level_count, "heightmap");
  if (!tex)
    return {};

  tex->texaddr(TEXADDR_CLAMP);
  char *data;
  int stride;
  if (!tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    return {};
  }

  Tab<uint16_t> hdata;
  hdata.resize(width);
  if (texFMT == TEXFMT_L16)
  {
#if _TARGET_C3





#else
    for (int i = 0; i < width; ++i, data += stride)
      cb.read(data, width * 2);
#endif
  }
  else if (texFMT == TEXFMT_R32F)
  {
    for (int y = 0; y < width; ++y, data += stride)
    {
      cb.read(hdata.data(), data_size(hdata));
      for (int x = 0; x < width; ++x)
        ((float *)data)[x] = hdata[x] / 65535.0;
    }
  }
  else if (texFMT == TEXFMT_R16F) //-V547
  {
    for (int y = 0; y < width; ++y, data += stride)
    {
      cb.read(hdata.data(), data_size(hdata));
      for (int x = 0; x < width; ++x)
        ((uint16_t *)data)[x] = float_to_half(hdata[x] / 65535.0);
    }
  }
  tex->unlockimg();
  return tex;
}

static inline TexPtr create_tex_from_raw_hmap_file(const char *name)
{
  int width = 0;
  return create_tex_from_raw_hmap_file(name, width);
}


static inline TexPtr create_tex_from_mtw(const char *name, float *out_cell_sz = NULL, float *out_max_ht = NULL,
  float *out_scale = NULL, float *out_h0 = NULL)
{
#pragma pack(push, 1)
  struct MtwHeader
  {
    uint32_t label; // 0x0057544D MTW
    uint32_t ver;   // 0x200 | 0x201
    uint32_t fileLen;
    uint32_t thumbCopyOfs;
    uint32_t userLabel;
    char ansiName[32];
    uint32_t bitsPerElem;                  // 8, 16, 32, 64
    uint32_t mapH, mapW;                   // map resolution (in elems)
    uint32_t blkW, blkH;                   // map dimensions (in blocks)
    uint32_t elemPerBlockH, elemPerBlockW; // size of block (in elems)
    uint32_t lastRowElemH, lastColElemW;
    uint32_t frameRecOfs, frameRecLen;
    uint32_t palleteRecOfs, palleteRecLen;
    uint32_t blkDescOfs, blkDescLen;
    uint32_t _resv[3];
    uint32_t mapType;
    uint32_t projType;
    uint32_t epsgCode;
    double scaleDiv;
    double elemsPerMapMeter;
    double metersPerElem;
    double leftBottomX, leftBottomY;
    double mainLat1Rad;
    double mainLat2Rad;
    double axisLonRad;
    double mainPtLatRad;
    uint8_t packType, shadowMaskType, shadowMaskStep, frameRecUsed;
    uint32_t blkFlgOfs, blkFlgLen;
    uint32_t file0Len, file1Len;
    uint8_t _resv2[8];
    uint32_t layerDescOfs, layerDescLen;
    uint8_t _resv3[4];
    uint8_t invisColorFlags[32];
    double minHt, maxHt, missingCode;
    uint32_t htUnitsType;
    uint8_t sumHt;
    uint8_t mtrBuildMethod[3];
    uint32_t addDescOfs, addDescLen;
  };
#pragma pack(pop)

  FullFileLoadCB crd(name);
  if (!crd.fileHandle)
    return {};

  MtwHeader hdr;
  crd.read(&hdr, sizeof(hdr));
  debug("create HMAP tex from MTW data: %s", name);
  DEBUG_DUMP_VAR(sizeof(hdr));
  DEBUG_DUMP_VAR(hdr.bitsPerElem);
  DEBUG_DUMP_VAR(hdr.mapW);
  DEBUG_DUMP_VAR(hdr.mapH);
  DEBUG_DUMP_VAR(hdr.blkW);
  DEBUG_DUMP_VAR(hdr.blkH);
  DEBUG_DUMP_VAR(hdr.elemPerBlockW);
  DEBUG_DUMP_VAR(hdr.elemPerBlockH);
  DEBUG_DUMP_VAR(hdr.mapType);
  DEBUG_DUMP_VAR(hdr.projType);
  DEBUG_DUMP_VAR(hdr.epsgCode);
  DEBUG_DUMP_VAR(hdr.scaleDiv);
  DEBUG_DUMP_VAR(hdr.elemsPerMapMeter);
  DEBUG_DUMP_VAR(hdr.metersPerElem);
  DEBUG_DUMP_VAR(hdr.leftBottomX);
  DEBUG_DUMP_VAR(hdr.leftBottomY);
  DEBUG_DUMP_VAR(hdr.mainLat1Rad * RAD_TO_DEG);
  DEBUG_DUMP_VAR(hdr.mainLat2Rad * RAD_TO_DEG);
  DEBUG_DUMP_VAR(hdr.axisLonRad * RAD_TO_DEG);
  DEBUG_DUMP_VAR(hdr.mainPtLatRad * RAD_TO_DEG);
  DEBUG_DUMP_VAR(hdr.minHt);
  DEBUG_DUMP_VAR(hdr.maxHt);
  DEBUG_DUMP_VAR(hdr.missingCode);

  int tex_w = get_bigger_pow2(hdr.blkW * hdr.elemPerBlockW);
  int tex_h = get_bigger_pow2(hdr.blkH * hdr.elemPerBlockH);
  ShaderGlobal::set_color4(get_shader_variable_id("tex_hmap_inv_sizes"), 1.0 / tex_w, 1.0 / tex_h, 0, 0);

  uint32_t texFMT = 0;
  if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_VERTEXTEXTURE))
    texFMT = TEXFMT_R32F;
  else
  {
    logerr("no suitable vertex texture found");
    return {};
  }
  int level_count = 1; // get_log2i(tex_w);

  TexPtr tex = dag::create_tex(NULL, tex_w, tex_h, texFMT, level_count, "heightmap");
  if (!tex)
    return {};

  tex->texaddr(TEXADDR_CLAMP);
  char *data;
  int stride;
  if (!tex->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
  {
    return {};
  }

  Tab<char> buf;
  for (float *data_p = (float *)data, *data_e = data_p + (stride / sizeof(*data_p)) * tex_h; data_p < data_e; data_p++)
    *data_p = -1024.0f;
  int dest_x0 = (tex_w - (hdr.blkW * hdr.elemPerBlockW)) / 2, dest_y0 = (tex_h - (hdr.blkH * hdr.elemPerBlockH)) / 2;
  for (int b = 0; b < hdr.blkW * hdr.blkH; b++)
  {
    crd.seekto(hdr.blkDescOfs + b * 8);
    int ofs = crd.readInt(), len = crd.readInt();
    int blk_x = b % hdr.blkW, blk_y = b / hdr.blkW;
    crd.seekto(ofs);
    buf.resize(len);
    crd.readTabData(buf);
    int dest_x = blk_x * hdr.elemPerBlockW + dest_x0, dest_y = blk_y * hdr.elemPerBlockH + dest_y0,
        dest_w = (blk_x == hdr.blkW - 1) ? hdr.lastColElemW : hdr.elemPerBlockW,
        dest_h = (blk_y == hdr.blkH - 1) ? hdr.lastRowElemH : hdr.elemPerBlockH;
    // debug("block %d:%d ofs=0x%X len=%d -> %d,%d  %dx%d", blk_x, blk_y, ofs, len, dest_x, dest_y, dest_w, dest_h);
    if (hdr.bitsPerElem == 64)
    {
      double *ht = (double *)buf.data();
      float *dest = (float *)(data + stride * dest_y + dest_x * 4);
      for (int y = 0; y < dest_h; y++, dest += stride / sizeof(*dest))
        for (int x = 0; x < dest_w; x++, ht++)
          dest[x] = (*ht == hdr.missingCode) ? -1024.0f : *ht;
    }
  }
  tex->unlockimg();
  if (out_cell_sz)
    *out_cell_sz = hdr.metersPerElem;
  if (out_max_ht)
    *out_max_ht = hdr.maxHt;
  if (out_scale)
    *out_scale = 1.0;
  if (out_h0)
    *out_h0 = 0;
  return tex;
}
