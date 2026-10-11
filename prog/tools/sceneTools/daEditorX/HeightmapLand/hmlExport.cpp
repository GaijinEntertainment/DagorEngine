// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"

#include <EditorCore/ec_IEditorCore.h>

#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_memIo.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_cpuFreq.h>
#include <libTools/dtx/dtx.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dtx/makeDDS.h>
#include <coolConsole/coolConsole.h>

#include <oldEditor/pluginService/de_IDagorPhys.h>
#include <oldEditor/de_workspace.h>
#include <de3_hmapService.h>
#include <de3_interface.h>
#include <assets/asset.h>

#include <math/dag_capsule.h>

// the 3d/ddsFormat.h included by mokeDDS.h indirectly includes wingdi.h which defines ERROR with a macro
#undef ERROR

using editorcore_extapi::dagTools;

#if defined(USE_HMAP_ACES)
#include <landMesh/lmeshWeightAtlas.h>
#include <libTools/dtx/astcenc.h>
#include <convert/fastDXT/rygDXT.h>
#include <EASTL/unique_ptr.h>
#if !_TARGET_STATIC_LIB // a static link takes the definition from dtx.lib
ASTCENC_DECLARE_STATIC_DATA();
#endif
#endif

bool game_res_sys_v2 = false;
int exportImageAsDds(mkbindump::BinDumpSaveCB &cb, TexPixel32 *image, int size, int format, int mipmap_count, bool gamma1,
  int stride_bytes = 0);
static int exportDdsAsDDsX(mkbindump::BinDumpSaveCB &cb, void *data, int size, bool gamma1);


static void makeFilter(float *wt, int size)
{
  double sum = 0;

  float *ptr = wt;
  for (int y = -size; y <= size; ++y)
  {
    float yk = size - abs(y);

    for (int x = -size; x <= size; ++x, ++ptr)
    {
      float w = (size - abs(x)) * yk;
      *ptr = w;
      sum += w;
    }
  }

  ptr = wt;
  for (int y = -size; y <= size; ++y)
    for (int x = -size; x <= size; ++x, ++ptr)
      *ptr /= sum;
}


static float filterHeight(float *data, int size, int stride, float *wt)
{
  float res = 0;

  for (int y = size; y; --y, data += stride)
    for (int x = 0; x < size; ++x, ++wt)
      res += data[x] * (*wt);

  return res;
}


struct HeightMapExportLodMap
{
  int sizeX, sizeY;
  SmallTab<int, TmpmemAlloc> offsets;

  int tableOffset;


  void resize(int x, int y)
  {
    sizeX = x;
    sizeY = y;
    clear_and_resize(offsets, x * y);
    mem_set_0(offsets);
  }

  void saveTable(mkbindump::BinDumpSaveCB &cb)
  {
    tableOffset = cb.tell();
    cb.writeTabData32ex(offsets);
  }

  void makeOfsRelative(int base_ofs)
  {
    for (int i = 0; i < offsets.size(); i++)
      offsets[i] -= base_ofs;
  }

  void save(mkbindump::BinDumpSaveCB &cb)
  {
    cb.writeInt32e(sizeX);
    cb.writeInt32e(sizeY);

    saveTable(cb);
  }
};


static int packHeightAs16Bit(float h, float min_val, float scale)
{
  int v = real2int((h - min_val) * scale + 0.5f);
  if (v <= 0)
    return 0;
  if (v >= (1 << 16) - 1)
    return (1 << 16) - 1;
  return v;
}


static void writeHeightmapElem(mkbindump::BinDumpSaveCB &cb, float *data_ptr, int elem_size, int data_x, int data_stride)
{
  G_ASSERT(data_x > 0);

  if (data_x > elem_size)
    data_x = elem_size;

  float minVal, maxVal;
  minVal = maxVal = *data_ptr;

  float *ptr = data_ptr;
  for (int y = 0; y < elem_size; ++y, ptr += data_stride)
    for (int x = 0; x < data_x; ++x)
    {
      float v = ptr[x];
      if (v < minVal)
        minVal = v;
      else if (v > maxVal)
        maxVal = v;
    }

  cb.writeReal(minVal);
  cb.writeReal(maxVal);

  float scale = maxVal - minVal;
  if (float_nonzero(scale))
    scale = ((1 << 16) - 1) / scale;

  for (int y = 0; y < elem_size; ++y, data_ptr += data_stride)
  {
    for (int x = 0; x < data_x; ++x)
      cb.writeInt16e(packHeightAs16Bit(data_ptr[x], minVal, scale));

    int v = packHeightAs16Bit(data_ptr[data_x - 1], minVal, scale);
    for (int x = data_x; x < elem_size; ++x)
      cb.writeInt16e(v);
  }
}


static void exportHeightmapToGame(mkbindump::BinDumpSaveCB &cb, HeightMapStorage &heightmap, int gridStep, int elemSize, int numLods,
  int base_ofs)
{
  CoolConsole &con = DAGORED2->getConsole();

  int mapSizeX = heightmap.getMapSizeX();
  int mapSizeY = heightmap.getMapSizeY();

  int farCellSize = 1 << numLods;
  int farElemSize = elemSize << numLods;

  Tab<HeightMapExportLodMap> lodMaps(tmpmem);
  lodMaps.resize(numLods + 1);

  for (int i = 0; i < lodMaps.size(); ++i)
  {
    int esize = (elemSize * gridStep) << i;

    lodMaps[i].resize((mapSizeX + esize - 1) / esize, (mapSizeY + esize - 1) / esize);

    lodMaps[i].save(cb);
  }

  con.startProgress();
  con.setActionDesc("exporting heightmap...");
  con.setTotal(mapSizeY);

  int dataW = mapSizeX / gridStep + farCellSize * 2;

  SmallTab<float, TmpmemAlloc> data, buffer, filterWt;
  clear_and_resize(data, dataW * ((elemSize + 2) << numLods));

  int bufferW = (dataW + 1) * gridStep + 1;
  clear_and_resize(buffer, bufferW * (gridStep * 2 + 1));

  clear_and_resize(filterWt, (gridStep * 2 + 1) * (gridStep * 2 + 1));
  makeFilter(&filterWt[0], gridStep);

  for (int mapY = 0; mapY < mapSizeY; mapY += farElemSize * gridStep)
  {
    // get data from heightmap (downsample if necessary)
    int hmY = mapY - farCellSize * gridStep;

    float *ptr = &buffer[0];
    for (int y = 0; y <= gridStep; ++y)
      for (int x = 0; x < bufferW; ++x, ++ptr)
        *ptr = heightmap.getFinalData(x - gridStep - farCellSize * gridStep, hmY + y - gridStep);

    float *dataPtr = &data[0];
    for (int dataY = -farCellSize; dataY < farElemSize + farCellSize; ++dataY)
    {
      hmY = mapY + dataY * gridStep;

      ptr = &buffer[bufferW * (gridStep + 1)];
      for (int y = 1; y <= gridStep; ++y)
        for (int x = 0; x < bufferW; ++x, ++ptr)
          *ptr = heightmap.getFinalData(x - gridStep - farCellSize * gridStep, hmY + y);

      // downsample buffer to data line
      ptr = &buffer[0];
      for (int x = 0; x < dataW; ++x, ++dataPtr, ptr += gridStep)
        *dataPtr = filterHeight(ptr, gridStep * 2 + 1, bufferW, &filterWt[0]);

      // move buffer data
      memmove(&buffer[0], &buffer[gridStep * bufferW], bufferW * (gridStep + 1) * elem_size(buffer));
    }

    // save lods heightmap data
    for (int lodi = 0; lodi < lodMaps.size(); ++lodi)
    {
      int elemY = mapY / ((elemSize * gridStep) << lodi);
      int ofsi = elemY * lodMaps[lodi].sizeX;

      int numY = 1 << (numLods - lodi);

      if (numY + elemY > lodMaps[lodi].sizeY)
        numY = lodMaps[lodi].sizeY - elemY;

      int width = dataW >> lodi;

      int dataOfs = (farCellSize >> lodi) * (width + 1);

      for (int ey = 0; ey < numY; ++ey, ofsi += lodMaps[lodi].sizeX)
      {
        dataPtr = &data[width * elemSize * ey + dataOfs];

        for (int ex = 0; ex < lodMaps[lodi].sizeX; ++ex, dataPtr += elemSize)
        {
          lodMaps[lodi].offsets[ofsi + ex] = cb.tell();

          writeHeightmapElem(cb, dataPtr, elemSize, width - ex * elemSize, width);
        }
      }

      if (lodi == lodMaps.size() - 1)
        break;

      // downsample data for lower LOD
      dataPtr = &data[width * 2];
      float *dest = &data[width >> 1];

      numY = (elemSize + 2) << (numLods - lodi);

      for (int y = 2; y < numY - 1; y += 2, dataPtr += width * 2, dest += (width >> 1))
        for (int x = 2; x < width - 1; x += 2)
          dest[x >> 1] = dataPtr[x] * 0.25f + (dataPtr[x - 1] + dataPtr[x + 1] + dataPtr[x - width] + dataPtr[x + width]) * 0.125f +
                         (dataPtr[x - width - 1] + dataPtr[x - width + 1] + dataPtr[x + width - 1] + dataPtr[x + width + 1]) * 0.0625f;
    }

    heightmap.unloadUnchangedData(mapY + 1);

    con.incDone(farElemSize * gridStep);
  }

  // write tables
  int ofs = cb.tell();

  for (int i = 0; i < lodMaps.size(); ++i)
  {
    cb.seekto(lodMaps[i].tableOffset);
    lodMaps[i].makeOfsRelative(base_ofs);
    lodMaps[i].saveTable(cb);
  }

  cb.seekto(ofs);

  con.endProgress();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void buildColorTexture(TexPixel32 *image, MapStorage<E3DCOLOR> &colormap, int tex_data_sizex, int tex_data_sizey, int stride,
  int map_x, int map_y)
{
  for (int y = 0; y < tex_data_sizey; ++y, ++map_y, image += stride)
    for (int x = 0; x < tex_data_sizex; ++x)
      image[x].c = colormap.getData(map_x + x, map_y);
}


#if 0
void buildLightTexture(TexPixel32 *image, MapStorage<uint32_t> &lightmap, int tex_data_sizex, int tex_data_sizey, int stride,
  int map_x, int map_y, bool use_normal_map)
{
  for (int y = 0; y < tex_data_sizey; ++y, ++map_y, image += stride)
    for (int x = 0; x < tex_data_sizex; ++x)
    {
      unsigned lt = lightmap.getData(map_x + x, map_y);

      if (use_normal_map)
      {
        unsigned nx = (lt >> 16) & 0xFF;
        unsigned nz = (lt >> 24) & 0xFF;

        image[x].c = E3DCOLOR(0, nx, 0, nz);
      }
      else
      {
        unsigned sunLight = (lt >> 0) & 0xFF;
        unsigned skyLight = (lt >> 8) & 0xFF;

        image[x].c = E3DCOLOR(skyLight, skyLight, skyLight, sunLight);
      }
    }
}


static void expandImageBorder(TexPixel32 *image, int size, int data_size)
{
  int y;
  for (y = 0; y < data_size; ++y, image += size)
  {
    TexPixel32 p = image[data_size - 1];
    for (int x = data_size; x < size; ++x)
      image[x] = p;
  }

  TexPixel32 *lastLine = image - size;
  for (; y < size; ++y, image += size)
    memcpy(image, lastLine, size * sizeof(*image));
}


static void exportColorAndLightMaps(mkbindump::BinDumpSaveCB &cb, MapStorage<E3DCOLOR> &colormap, MapStorage<uint32_t> &lightmap,
  int elem_size, int num_lods, int lightmapScaleFactor, int base_ofs, bool use_normal_map)
{
  CoolConsole &con = DAGORED2->getConsole();
  bool exp_ltmap = lightmapScaleFactor > 0;
  if (!exp_ltmap)
    lightmapScaleFactor = 1;

  int mapSizeX = colormap.getMapSizeX();
  int mapSizeY = colormap.getMapSizeY();

  int texElemSize = elem_size << num_lods;

  int texSize;
  for (texSize = 1; texSize < texElemSize; texSize <<= 1)
    ;

  int texDataSize = texElemSize;
  if (texDataSize < texSize)
    texDataSize++;

  int numElemsX = (mapSizeX + texElemSize - 1) / texElemSize;
  int numElemsY = (mapSizeY + texElemSize - 1) / texElemSize;

  con.startProgress();
  con.setActionDesc(exp_ltmap ? "exporting color and light maps..." : "exporting color maps...");
  con.setTotal(numElemsX * numElemsY);

  cb.writeInt32e(numElemsX);
  cb.writeInt32e(numElemsY);
  cb.writeInt32e(texSize);
  cb.writeInt32e(exp_ltmap ? texSize * lightmapScaleFactor : 0);
  cb.writeInt32e(texElemSize);

  SmallTab<int, TmpmemAlloc> offsets;

  clear_and_resize(offsets, numElemsX * numElemsY);
  mem_set_0(offsets);

  int tableOffset = cb.tell();
  cb.writeTabDataRaw(offsets); // reserve space with zeroes here

  SmallTab<TexPixel32, TmpmemAlloc> image;
  clear_and_resize(image, texSize * texSize * lightmapScaleFactor * lightmapScaleFactor);

  for (int ey = 0, mapY = 0, index = 0; ey < numElemsY; ++ey, mapY += texElemSize)
  {
    for (int ex = 0, mapX = 0; ex < numElemsX; ++ex, mapX += texElemSize, ++index)
    {
      offsets[index] = cb.tell();
      int colorTexSz = 0, lightTexSz = 0;

      cb.writeInt32e(0);
      cb.writeInt32e(0);

      buildColorTexture(&image[0], colormap, texDataSize, texDataSize, texSize, mapX, mapY);
      expandImageBorder(&image[0], texSize, texDataSize);

      ddstexture::Converter::Format fmt = ddstexture::Converter::fmtDXT1;
      if (HmapLandPlugin::useASTC(cb.getTarget()))
        fmt = ddstexture::Converter::fmtASTC8;
      colorTexSz = exportImageAsDds(cb, &image[0], texSize, fmt, ddstexture::Converter::AllMipMaps, false);

      if (exp_ltmap)
      {
        buildLightTexture(&image[0], lightmap, texDataSize * lightmapScaleFactor, texDataSize * lightmapScaleFactor,
          texSize * lightmapScaleFactor, mapX * lightmapScaleFactor, mapY * lightmapScaleFactor, use_normal_map);
        expandImageBorder(&image[0], texSize * lightmapScaleFactor, texDataSize * lightmapScaleFactor);

        ddstexture::Converter::Format fmt = ddstexture::Converter::fmtDXT5;
        if (HmapLandPlugin::useASTC(cb.getTarget()))
          fmt = ddstexture::Converter::fmtASTC4;
        lightTexSz = exportImageAsDds(cb, &image[0], texSize * lightmapScaleFactor, fmt, ddstexture::Converter::AllMipMaps, true);
      }
      int endOfs = cb.tell();

      cb.seekto(offsets[index]);
      cb.writeInt32e(colorTexSz);
      cb.writeInt32e(lightTexSz);

      cb.seekto(endOfs);

      con.incDone();
    }
    colormap.unloadUnchangedData(mapY + 1);
    lightmap.unloadUnchangedData(mapY + 1);
  }

  int ofs = cb.tell();

  cb.seekto(tableOffset);

  for (int i = 0; i < offsets.size(); i++)
    offsets[i] -= base_ofs;
  cb.writeTabData32ex(offsets);

  cb.seekto(ofs);

  con.endProgress();
}
#endif

int exportImageAsDds(mkbindump::BinDumpSaveCB &cb, TexPixel32 *image, int size, int format, int mipmap_count, bool gamma1,
  int stride_bytes)
{
  // ddsConvertImage() takes a pitch but reads the rows as tight (dtxConvertImage.cpp), so any padding the
  // locked texture reports has to be removed here; 0 means the caller already has tight rows
  const int tightStride = size * sizeof(*image);
  Tab<TexPixel32> tightImage(tmpmem);
  if (stride_bytes > 0 && stride_bytes != tightStride)
  {
    tightImage.resize(size * size);
    for (int y = 0; y < size; y++)
      memcpy(&tightImage[y * size], (const char *)image + y * stride_bytes, tightStride);
    image = tightImage.data();
  }

  ddstexture::Converter cnv;
  cnv.format = (ddstexture::Converter::Format)format;
  cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
  cnv.mipmapCount = mipmap_count;

  DynamicMemGeneralSaveCB memcwr(tmpmem, 0, size * size * (4 * 3 / 2));
  if (!dagTools->ddsConvertImage(cnv, memcwr, image, size, size, tightStride))
    throw IGenSave::SaveException("Error converting image to DDS", cb.tell());

  return exportDdsAsDDsX(cb, memcwr.data(), memcwr.size(), gamma1);
}

// the caller owns b on success: write it out, then dagTools->ddsxFreeBuffer(b)
static bool convertDdsToDdsx(unsigned target, ddsx::Buffer &b, void *data, int size, bool gamma1, bool allow_non_pow2)
{
  ddsx::ConvertParams cp;
  cp.allowNonPow2 = allow_non_pow2;
  cp.packSzThres = 8 << 10;
  if (gamma1)
    cp.imgGamma = 1.0;
  cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
  cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
  cp.mipOrdRev = HmapLandPlugin::defMipOrdRev;

  if (!dagTools->ddsxConvertDds(target, b, data, size, cp))
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.startLog();
    con.addMessage(ILogWriter::ERROR, "Can't export image: %s", dagTools->ddsxGetLastErrorText());
    con.endLog();
    return false;
  }
  return true;
}

static int exportDdsAsDDsX(mkbindump::BinDumpSaveCB &cb, void *data, int size, bool gamma1)
{
  ddsx::Buffer b;
  if (!convertDdsToDdsx(cb.getTarget(), b, data, size, gamma1, /*allow_non_pow2*/ false))
    return 0;
  cb.writeRaw(b.ptr, b.len);
  const int ret = b.len;
  dagTools->ddsxFreeBuffer(b);
  return ret;
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static bool validate_texture_name(String &name)
{
  DagorAsset *a = DAEDITOR3.getAssetByName(name, DAEDITOR3.getAssetTypeId("tex"));
  if (!a)
    return false;
  name.printf(128, "%s*", a->getName());
  dd_strlwr(name);
  return true;
}

bool hmap_export_tex(mkbindump::BinDumpSaveCB &cb, const char *tex_name, const char *fname, bool clamp, bool gamma1)
{
  String name(128, "%s", tex_name);
  if (!validate_texture_name(name))
  {
    cb.writeDwString("");
    return false;
  }
  cb.writeDwString(name);
  return true;
}


static bool validate_blk_texture_name(DataBlock &blk, const char *param_name)
{
  if (!blk.getStr(param_name, NULL))
    return false;
  String texture_name(128, "%s", blk.getStr(param_name));
  validate_texture_name(texture_name);
  blk.setStr(param_name, texture_name);
  return true;
}

bool hmap_export_land(mkbindump::BinDumpSaveCB &cb, const char *land_name, int editorId)
{
  DagorAsset *a = DAEDITOR3.getAssetByName(land_name, DAEDITOR3.getAssetTypeId("land"));
  String ref(128, "%s", a ? a->getName() : "");
  dd_strlwr(ref);
  cb.beginBlock();
  cb.writeDwString(ref);
  // saves for now only!
  DataBlock *blk = a ? a->props.getBlockByName("detail") : NULL;
  if (!a || !blk)
  {
    cb.endBlock();
    if (a)
      DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "Land class <%s> - has no detail block", land_name);
    else
      DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "No land class <%s>", land_name);
    return false;
  }

  if (!blk->getStr("texture", NULL))
    DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "Land class <%s> - has no texture in detail block", land_name);
  DataBlock saveBlk = *blk;
  validate_blk_texture_name(saveBlk, "texture");
  saveBlk.setInt("editorId", editorId);
  saveBlk.saveToStream(cb.getRawWriter());
  cb.endBlock();
  return true;
}

#if defined(USE_HMAP_ACES)
// the mobile class takes the atlas as ASTC 4x4, the rest as DXT1; useASTC()
// serves the legacy exports, which never shipped Android ASTC
static bool weight_atlas_astc(unsigned target) { return HmapLandPlugin::useASTC(target) || target == _MAKE4C('and'); }

bool aces_export_detail_maps(mkbindump::BinDumpSaveCB &cb, int mapSizeX, int mapSizeY, int tex_elem_size,
  const Tab<SimpleString> &land_class_names, int base_ofs, bool tools_internal = false)
{
  static const int MAX_DET_TEX_NUM = HmapLandPlugin::HMAX_DET_TEX_NUM;
  G_STATIC_ASSERT(MAX_DET_TEX_NUM == LandWeightAtlas::DET_NUM);
  CoolConsole &con = DAGORED2->getConsole();

  const int texElemSize = tex_elem_size;

  int numElemsX = (mapSizeX + texElemSize - 1) / texElemSize;
  int numElemsY = (mapSizeY + texElemSize - 1) / texElemSize;

  int numDetTex = HmapLandPlugin::self->getNumDetailTextures();
  SmallTab<int, TmpmemAlloc> detTexRemap;
  // each cell's landclass selection, made once here and reused by the packing
  // pass below - the selection rule must not fork between the two
  SmallTab<uint8_t, TmpmemAlloc> cellDetIds;
  clear_and_resize(cellDetIds, numElemsX * numElemsY * MAX_DET_TEX_NUM);
  {
    SmallTab<bool, TmpmemAlloc> usedDetTex;
    clear_and_resize(usedDetTex, numDetTex);
    mem_set_0(usedDetTex);

    SmallTab<uint8_t, TmpmemAlloc> typeRemap;
    clear_and_resize(typeRemap, 256);
    uint8_t detIds[MAX_DET_TEX_NUM];
    for (int ey = 0, mapY = 0, index = 0; ey < numElemsY; ++ey, mapY += texElemSize)
      for (int ex = 0, mapX = 0; ex < numElemsX; ++ex, mapX += texElemSize, ++index)
      {
        mem_set_ff(typeRemap);
        memset(detIds, 0xFF, MAX_DET_TEX_NUM); // a slot the selection leaves alone is free, not landclass 0
        HmapLandPlugin::self->getCellDetTex(mapX, mapY, texElemSize, detIds, typeRemap.data(), MAX_DET_TEX_NUM);
        // an unpainted cell keeps 0xFF ids, as the legacy export and the
        // editor's own atlas ship it: the renderer draws the identity tile
        memcpy(&cellDetIds[index * MAX_DET_TEX_NUM], detIds, MAX_DET_TEX_NUM);
        for (int di = 0; di < MAX_DET_TEX_NUM; di++)
          if (detIds[di] != 0xFFU && detIds[di] < usedDetTex.size() && !usedDetTex[detIds[di]])
            usedDetTex[detIds[di]] = true;
      }

    // A slot with a registered landclass asset but no pixel reaching the
    // 1/255 quantized weight threshold gets dropped from lmDump, which is
    // fine for the exported payload but means the editor-time
    // blendDetTex(slot, wt) lookup later hits lcRemap[slot]=0xFF. Report
    // such slots as errors so the BLK author knows the landclass is
    // effectively unused (masks / thresholds / overlapping layers drove
    // its contribution to zero everywhere). Layers with writeDetTex
    // disabled never emit detTex weights in the first place -- that is a
    // supported land-only configuration, not a failure -- so filter them
    // out via isDetTexSlotWritten(i).
    clear_and_resize(detTexRemap, numDetTex);
    mem_set_ff(detTexRemap);
    int ord = 0;
    for (int i = 0; i < numDetTex; ++i)
    {
      if (usedDetTex[i])
        detTexRemap[i] = ord++;
      else if (
        !tools_internal && i < land_class_names.size() && !land_class_names[i].empty() && HmapLandPlugin::self->isDetTexSlotWritten(i))
        con.addMessage(ILogWriter::ERROR, "Landclass <%s> final weights are all 0!", land_class_names[i].c_str());
      debug("detTex[%d] %s is %s, remapped to %d", i, i < land_class_names.size() ? land_class_names[i].str() : NULL,
        usedDetTex[i] ? "used" : "UNUSED", detTexRemap[i]);
    }
    debug("used %d detTex of %d", ord, numDetTex);
    numDetTex = ord;
  }

  int time0 = dagTools->getTimeMsec();

  con.startProgress();
  con.setActionDesc("exporting detail textures...");
  con.setTotal(numDetTex);

  cb.beginBlock();

  cb.writeInt32e(numDetTex);

  int customLandClassesCount = 0; // with LandClassType::LC_CUSTOM type
  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppBlkPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppBlkPath());
  int customLandClassesLimit = app_blk.getBlockByNameEx("heightMap")->getInt("customLandClassesLimit", -1);

  for (int i = 0, ie = HmapLandPlugin::self->getNumDetailTextures(); i < ie; ++i)
  {
    if (detTexRemap[i] < 0)
      continue;
    const char *landClassName = i < land_class_names.size() ? land_class_names[i].str() : NULL;
    const bool exported = ::hmap_export_land(cb, landClassName, i);
    DagorAsset *la = DAEDITOR3.getAssetByName(landClassName, DAEDITOR3.getAssetTypeId("land"));
    // hmap_export_land warns and carries on, which suits the tools; the level
    // export owns the stricter bar the per-cell loop used to apply: the causes
    // it just named are fatal here
    if (!tools_internal && (!exported || !la || !la->props.getBlockByNameEx("detail")->getStr("texture", NULL)))
    {
      DAEDITOR3.conError("cannot export the level: landclass <%s> is unresolved or has no detail texture",
        landClassName ? landClassName : "");
      con.endProgress();
      return false;
    }
    if (la && la->props.getBlockByNameEx("detail")->getStr("shader", nullptr))
      customLandClassesCount++;
    con.incDone();
  }
  if (customLandClassesLimit >= 0 && customLandClassesCount > customLandClassesLimit)
  {
    DAEDITOR3.conError("Map has more than %d landclasses with custom shader", customLandClassesLimit);
  }

  debug("exported %d detail tex", numDetTex);

  con.endProgress();
  con.addMessage(ILogWriter::REMARK, " in %g seconds", (dagTools->getTimeMsec() - time0) / 1000.0f);
  time0 = dagTools->getTimeMsec();

  cb.endBlock();

  // The atlas the level ships, packed here from the painted map and encoded
  // for the target; the loader creates the texture from it as is.
  cb.writeInt32e(numElemsX);
  cb.writeInt32e(numElemsY);
  cb.writeInt32e(LandWeightAtlas::PACKED_TEX_SIZE); // no per-cell weight textures follow, the packed atlas does
  cb.writeInt32e(texElemSize);
  if (tools_internal) // daEditor paints its own atlas and reads nothing past the header
    return true;

  con.startProgress();
  con.setActionDesc("packing land weight atlas: %dx%d cells of %d texels...", numElemsX, numElemsY, texElemSize);
  con.setTotal(numElemsX * numElemsY);

  LandWeightAtlasBuilder builder(numElemsX, numElemsY, texElemSize);
  const int cellTexels = texElemSize * texElemSize;
  SmallTab<uint8_t, TmpmemAlloc> weights; // a cell's texels, one plane per slot
  clear_and_resize(weights, MAX_DET_TEX_NUM * cellTexels);
  const uint8_t *planes[MAX_DET_TEX_NUM];
  for (int ch = 0; ch < MAX_DET_TEX_NUM; ch++)
    planes[ch] = &weights[ch * cellTexels];
  SmallTab<uint8_t, TmpmemAlloc> typeRemap;
  clear_and_resize(typeRemap, 256);
  for (int ey = 0, mapY = 0, index = 0; ey < numElemsY; ++ey, mapY += texElemSize)
    for (int ex = 0, mapX = 0; ex < numElemsX; ++ex, mapX += texElemSize, ++index)
    {
      carray<uint8_t, MAX_DET_TEX_NUM> detIds;
      memcpy(detIds.data(), &cellDetIds[index * MAX_DET_TEX_NUM], MAX_DET_TEX_NUM);
      mem_set_ff(typeRemap);
      for (int ch = 0; ch < MAX_DET_TEX_NUM; ch++) // landclass id -> the cell's slot
        if (detIds[ch] != 0xFF)
          typeRemap[detIds[ch]] = ch;
      // the cell's own texels only: the packer takes the borders from the neighbours
      for (int y = 0, t = 0; y < texElemSize; y++)
        for (int x = 0; x < texElemSize; x++, t++)
        {
          uint8_t wt[MAX_DET_TEX_NUM];
          HmapLandPlugin::self->readLandDetailWeights(mapX + x, mapY + y, make_span_const(typeRemap), wt);
          for (int ch = 0; ch < MAX_DET_TEX_NUM; ch++)
            weights[ch * cellTexels + t] = wt[ch];
        }
      // one selection feeds both passes, so a stored id has its remap entry
      // unless it names a landclass no detail slot registers (a layer map with
      // an empty slot table votes for id 0): such a level cannot ship
      for (int di = 0; di < MAX_DET_TEX_NUM; di++)
        if (detIds[di] != 0xFFU)
        {
          if (detIds[di] >= detTexRemap.size() || detTexRemap[detIds[di]] < 0)
          {
            DAEDITOR3.conError("cannot export the level: cell (%d,%d) refers to landclass id %d, which no detail slot registers", ex,
              ey, detIds[di]);
            con.endProgress();
            return false;
          }
          detIds[di] = detTexRemap[detIds[di]];
        }
      builder.addCellWeights(index, detIds.data(), planes);
      con.incDone();
    }

  const bool astc = weight_atlas_astc(cb.getTarget());
  // the loader checks the texture against the records it ships with, so the
  // cap must hold on every driver of the target
  eastl::unique_ptr<LandWeightAtlas> atlas(builder.finishForExport(LandWeightAtlas::MAX_TEX_SIDE));
  if (!atlas) // refused; the packer logged the cause
  {
    con.addMessage(ILogWriter::ERROR, "land weight atlas: the map does not pack, see the log");
    con.endProgress();
    return false;
  }
  int atlasW = 0, atlasH = 0;
  atlas->getAtlasSize(atlasW, atlasH);
  Tab<TexPixel32> img(tmpmem);
  atlas->composeRawImage(img);

  // the encoded atlas as a DDS; the ddsx conversion takes it from there
  Tab<uint8_t> dds(tmpmem);
  const int hdrSz = dds_header_size();
  if (astc)
  {
    ASTCEncoderHelperContext::setupAstcEncExePathname();
    ASTCEncoderHelperContext astcenc;
    Tab<char> packed(tmpmem);
    if (!astcenc.prepareTmpFilenames("land_weight_atlas") || !astcenc.writeTga(img.data(), atlasW, atlasH) ||
        !astcenc.buildOneSurface(packed, "4x4"))
    {
      con.addMessage(ILogWriter::ERROR, "land weight atlas: ASTC encoding of %dx%d failed", atlasW, atlasH);
      con.endProgress();
      return false;
    }
    dds.resize(hdrSz + packed.size());
    create_dds_header(dds.data(), hdrSz, atlasW, atlasH, 8, 1, TEXFMT_ASTC4, false);
    memcpy(dds.data() + hdrSz, packed.data(), packed.size());
  }
  else
  {
    dds.resize(hdrSz + (atlasW / 4) * (atlasH / 4) * 8);
    create_dds_header(dds.data(), hdrSz, atlasW, atlasH, 4, 1, TEXFMT_DXT1, false);
    // the codec of the runtime CPU conversion; TexPixel32 is the BGRA it reads
    rygDXT::CompressImageDXT1((const uint8_t *)img.data(), dds.data() + hdrSz, atlasW, atlasH, rygDXT::STB_DXT_HIGHQUAL,
      (atlasW / 4) * 8);
  }
  ddsx::Buffer ddsxBuf;
  // the atlas is page-granular, almost never pow2; a rescale would break the
  // placement the records ship, so non-pow2 passes through as is
  if (!convertDdsToDdsx(cb.getTarget(), ddsxBuf, dds.data(), dds.size(), /*gamma1*/ true, /*allow_non_pow2*/ true))
  {
    con.endProgress();
    return false;
  }
  atlas->savePacked(cb.getRawWriter(), base_ofs, make_span_const((const uint8_t *)ddsxBuf.ptr, ddsxBuf.len));
  const int ddsxLen = ddsxBuf.len;
  dagTools->ddsxFreeBuffer(ddsxBuf);

  con.endProgress();
  con.addMessage(ILogWriter::REMARK, "land weight atlas: %d pages, %dx%d %s, %dK as ddsx, in %g seconds", atlas->getPageCount(),
    atlasW, atlasH, astc ? "ASTC 4x4" : "DXT1", ddsxLen >> 10, (dagTools->getTimeMsec() - time0) / 1000.0f);
  return true;
}
#else
bool aces_export_detail_maps(mkbindump::BinDumpSaveCB &cb, int mapSizeX, int mapSizeY, int tex_elem_size,
  const Tab<SimpleString> &land_class_names, int base_ofs, bool tools_internal = false)
{
  return false;
}
#endif


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

bool HmapLandPlugin::exportLand(mkbindump::BinDumpSaveCB &cb) { return true; }

bool HmapLandPlugin::exportLand(String &filename)
{
  CoolConsole &con = DAGORED2->getConsole();
  con.addMessage(ILogWriter::NOTE, "Exporting heightmap is no longer supported...");
  return true;
}
