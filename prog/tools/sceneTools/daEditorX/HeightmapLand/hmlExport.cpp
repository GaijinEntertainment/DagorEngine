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

bool game_res_sys_v2 = false;
int exportImageAsDds(mkbindump::BinDumpSaveCB &cb, TexPixel32 *image, int size, int format, int mipmap_count, bool gamma1);
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

int exportImageAsDds(mkbindump::BinDumpSaveCB &cb, TexPixel32 *image, int size, int format, int mipmap_count, bool gamma1)
{
  ddstexture::Converter cnv;
  cnv.format = (ddstexture::Converter::Format)format;
  cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
  cnv.mipmapCount = mipmap_count;

  DynamicMemGeneralSaveCB memcwr(tmpmem, 0, size * size * (4 * 3 / 2));
  if (!dagTools->ddsConvertImage(cnv, memcwr, image, size, size, size * sizeof(*image)))
    throw IGenSave::SaveException("Error converting image to DDS", cb.tell());

  return exportDdsAsDDsX(cb, memcwr.data(), memcwr.size(), gamma1);
}

static int exportDdsAsDDsX(mkbindump::BinDumpSaveCB &cb, void *data, int size, bool gamma1)
{
  ddsx::Buffer b;
  ddsx::ConvertParams cp;
  cp.allowNonPow2 = false;
  cp.packSzThres = 8 << 10;
  if (gamma1)
    cp.imgGamma = 1.0;
  cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
  cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
  cp.mipOrdRev = HmapLandPlugin::defMipOrdRev;

  if (!dagTools->ddsxConvertDds(cb.getTarget(), b, data, size, cp))
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.startLog();
    con.addMessage(ILogWriter::ERROR, "Can't export image: %s", dagTools->ddsxGetLastErrorText());
    con.endLog();
    return 0;
  }
  cb.writeRaw(b.ptr, b.len);
  // debug("DDS(%d)->DDSx(%d)= %+d b  rev=%d", size, b.len, b.len-size, cp.mipOrdRev);
  int ret = b.len;
  dagTools->ddsxFreeBuffer(b);
  return ret;
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


int hmap_export_dds_as_ddsx_raw(mkbindump::BinDumpSaveCB &cb, char *data, int size, int w, int h, int bpp, int levels, int fmt,
  bool gamma1)
{
  Tab<char> dds_data;
  char buf[1024];
  if (!create_dds_header(buf, sizeof(buf), w, h, bpp, levels, fmt, HmapLandPlugin::useASTC(cb.getTarget())))
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.startLog();
    con.addMessage(ILogWriter::ERROR, "Can't convert to DDS - unknown format %08X", fmt);
    con.endLog();
    return 0;
  }

  dds_data.resize(::dds_header_size() + size);
  memcpy(&dds_data[0], buf, ::dds_header_size());
  memcpy(&dds_data[::dds_header_size()], data, size);
  return exportDdsAsDDsX(cb, dds_data.data(), data_size(dds_data), gamma1);
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
__forceinline uint8_t convert_4bit_to8bit(unsigned a)
{
  a &= 0xF;
  return (a << 4) | a;
}
bool aces_export_detail_maps(mkbindump::BinDumpSaveCB &cb, int mapSizeX, int mapSizeY, int tex_elem_size,
  const Tab<SimpleString> &land_class_names, int base_ofs, bool optimize_size = true, bool tools_internal = false)
{
  static const int MAX_DET_TEX_NUM = HmapLandPlugin::HMAX_DET_TEX_NUM;
  CoolConsole &con = DAGORED2->getConsole();

  int texElemSize = tex_elem_size;

  int texSize;
  for (texSize = 1; texSize < texElemSize; texSize <<= 1)
    ;

  int texDataSize = texElemSize;
  if (texDataSize < texSize)
    texDataSize++;

  int numElemsX = (mapSizeX + texElemSize - 1) / texElemSize;
  int numElemsY = (mapSizeY + texElemSize - 1) / texElemSize;

  int numDetTex = HmapLandPlugin::self->getNumDetailTextures();
  SmallTab<int, TmpmemAlloc> detTexRemap;
  {
    SmallTab<bool, TmpmemAlloc> usedDetTex;
    clear_and_resize(usedDetTex, numDetTex);
    mem_set_0(usedDetTex);

    SmallTab<uint8_t, TmpmemAlloc> typeRemap;
    clear_and_resize(typeRemap, 256);
    uint8_t detIds[MAX_DET_TEX_NUM];
    int texDataSize = min(texElemSize + 4, texSize);
    for (int ey = 0, mapY = 0, index = 0; ey < numElemsY; ++ey, mapY += texElemSize)
      for (int ex = 0, mapX = 0; ex < numElemsX; ++ex, mapX += texElemSize, ++index)
      {
        mem_set_ff(typeRemap);
        memset(detIds, 0, MAX_DET_TEX_NUM);
        int cnt =
          HmapLandPlugin::self->getMostUsedDetTex(mapX, mapY, texDataSize, detIds, typeRemap.data(), HmapLandPlugin::LMAX_DET_TEX_NUM);
        for (int di = 0; di < MAX_DET_TEX_NUM; di++)
          if (detIds[di] != 0xFFU && detIds[di] < usedDetTex.size() && !usedDetTex[detIds[di]])
            usedDetTex[detIds[di]] = true;
      }

    clear_and_resize(detTexRemap, numDetTex);
    mem_set_ff(detTexRemap);
    int ord = 0;
    for (int i = 0; i < numDetTex; ++i)
    {
      if (usedDetTex[i])
        detTexRemap[i] = ord++;
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
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  int customLandClassesLimit = app_blk.getBlockByNameEx("heightMap")->getInt("customLandClassesLimit", -1);

  for (int i = 0, ie = HmapLandPlugin::self->getNumDetailTextures(); i < ie; ++i)
  {
    if (detTexRemap[i] < 0)
      continue;
    const char *landClassName = i < land_class_names.size() ? land_class_names[i].str() : NULL;
    ::hmap_export_land(cb, landClassName, i);
    if (DagorAsset *a = DAEDITOR3.getAssetByName(landClassName, DAEDITOR3.getAssetTypeId("land")))
    {
      if (a->props.getBlockByNameEx("detail")->getStr("shader", nullptr))
        customLandClassesCount++;
    }
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

  /*for (int i=0; i < numDetTex; ++i)
  {
    cb.writeReal(i < det_tex_offset.size() ? det_tex_offset[i].x : 0.f);
    cb.writeReal(i < det_tex_offset.size() ? det_tex_offset[i].y : 0.f);
  }*/

  cb.endBlock();

  con.endProgress();
  con.addMessage(ILogWriter::REMARK, " in %g seconds", (dagTools->getTimeMsec() - time0) / 1000.0f);
  time0 = dagTools->getTimeMsec();

  con.startProgress();
  con.setActionDesc("exporting %d detail maps: %dx%d (texSize=%d)...", numElemsX * numElemsY, numElemsX, numElemsY, texSize);
  con.setTotal(numElemsX * numElemsY);

  cb.writeInt32e(numElemsX);
  cb.writeInt32e(numElemsY);
  cb.writeInt32e(texSize);
  cb.writeInt32e(texElemSize);

  SmallTab<int, TmpmemAlloc> offsets;

  clear_and_resize(offsets, numElemsX * numElemsY);
  mem_set_0(offsets);

  int tableOffset = cb.tell();
  cb.writeTabDataRaw(offsets); // reserve space with zeroes here

  SmallTab<uint16_t, TmpmemAlloc> image1;
  clear_and_resize(image1, texSize * texSize);
  mem_set_0(image1);

  SmallTab<uint8_t, TmpmemAlloc> image2;
  clear_and_resize(image2, texSize * texSize / 2);
  mem_set_0(image2);
  int optimizedSize = 0;

  for (int ey = 0, mapY = 0, index = 0; ey < numElemsY; ++ey, mapY += texElemSize)
  {
    for (int ex = 0, mapX = 0; ex < numElemsX; ++ex, mapX += texElemSize, ++index)
    {
      offsets[index] = cb.tell();
      if (tools_internal)
      {
        cb.writeZeroes(MAX_DET_TEX_NUM);
        cb.writeInt32e(0);
        cb.writeInt32e(0);
        continue;
      }
      carray<uint8_t, MAX_DET_TEX_NUM> detIds;
      mem_set_0(detIds);

      bool doneMark;
      int cnt = HmapLandPlugin::self->loadLandDetailTexture(cb.getTarget(), ex, ey, (char *)image1.data(), texSize * 2,
        (char *)image2.data(), texSize * 2, detIds, &doneMark, texSize, texElemSize, false, false);
      for (int di = 0; di < MAX_DET_TEX_NUM; di++)
        if (detIds[di] != 0xFFU)
        {
          DagorAsset *a = detIds[di] >= 0 && detIds[di] < land_class_names.size()
                            ? DAEDITOR3.getAssetByName(land_class_names[detIds[di]], DAEDITOR3.getAssetTypeId("land"))
                            : NULL;
          if (!a)
            DAEDITOR3.conError("cannot export with unresolved reference to landclass <%s>, id=%d [%d]",
              detIds[di] >= 0 && detIds[di] < land_class_names.size() ? land_class_names[detIds[di]] : "", detIds[di], di);
          else if (!a->props.getBlockByName("detail"))
            DAEDITOR3.conError("Land class <%s> - has no detail block", land_class_names[detIds[di]]);
          else if (!a->props.getBlockByName("detail")->getStr("texture", NULL))
            DAEDITOR3.conError("Land class <%s> - has no texture in detail block", land_class_names[detIds[di]]);
          else if (detIds[di] >= detTexRemap.size() || detTexRemap[detIds[di]] < 0)
            DAEDITOR3.conError("Internal error: unexpected detIds[%d]=%d detTexRemap.count=%d", di, detIds[di], detTexRemap.size());
          else
          {
            detIds[di] = detTexRemap[detIds[di]];
            continue;
          }
          return false;
        }

      cb.writeRaw(detIds.data(), MAX_DET_TEX_NUM);
      cb.writeInt32e(0);
      cb.writeInt32e(0);

      int texOfs = cb.tell();

      // exportImageAsDds(cb, image2.data(), texSize,
      //   ddstexture::Converter::fmtDXT1a, 1);
      if (!optimize_size || cnt > 1)
      {
        unsigned fmt =
          ((!optimize_size || cnt > 4) && cb.getTarget() != _MAKE4C('iOS')) ? TEXFMT_A4R4G4B4 : (cnt > 3 ? TEXFMT_DXT5 : TEXFMT_DXT1);
        char *srcData = (char *)image1.data();
        int srcDataSize = data_size(image1);
        SmallTab<unsigned char, TmpmemAlloc> cnvData;
        if (fmt != TEXFMT_A4R4G4B4)
        {
          ddstexture::Converter cnv;
          cnv.format = (fmt == TEXFMT_DXT5) ? ddstexture::Converter::fmtDXT5 : ddstexture::Converter::fmtDXT1;
          if (HmapLandPlugin::useASTC(cb.getTarget()))
            cnv.format = (fmt == TEXFMT_DXT5) ? ddstexture::Converter::fmtASTC4 : ddstexture::Converter::fmtASTC8;
          cnv.mipmapType = ddstexture::Converter::mipmapNone;
          cnv.mipmapCount = 0;
          clear_and_resize(cnvData, texSize * texSize * 4);
          uint16_t *srcPix = (uint16_t *)srcData;
          TexPixel32 *pixel = (TexPixel32 *)cnvData.data();
          for (int i = 0; i < texSize * texSize; ++i, srcPix++, pixel++)
          {
            pixel->a = pixel->r = pixel->g = pixel->b = 0;
            pixel->a = convert_4bit_to8bit((*srcPix) >> 12);
            pixel->r = convert_4bit_to8bit(((*srcPix) >> 8));
            pixel->g = convert_4bit_to8bit(((*srcPix) >> 4));
            pixel->b = convert_4bit_to8bit(((*srcPix) >> 0));
            if (cnt == 3 || cnt == 4)
              pixel->b = 0;
            else if (cnt == 2)
              pixel->r = 0;
          }
          DynamicMemGeneralSaveCB cwr(tmpmem, data_size(cnvData) + 256, 64 << 10);
          if (!dagTools->ddsConvertImage(cnv, cwr, (TexPixel32 *)cnvData.data(), texSize, texSize, texSize * 4))
          {
            DAEDITOR3.conError("cannot convert %dx%d image at (%d,%d)", texSize, texSize, ex, ey);
            fmt = TEXFMT_A4R4G4B4;
          }
          else
          {
            srcDataSize = (fmt == TEXFMT_DXT5) ? (texSize * texSize) : (texSize / 4) * (texSize * 2);
            if (cb.getTarget() == _MAKE4C('iOS'))
              srcDataSize = (fmt == TEXFMT_DXT5) ? srcDataSize : srcDataSize / 2;
            clear_and_resize(cnvData, srcDataSize);
            memcpy(cnvData.data(), cwr.data() + 128, srcDataSize);
            srcData = (char *)cnvData.data();
          }
        }
        if (!hmap_export_dds_as_ddsx_raw(cb, srcData, srcDataSize, texSize, texSize,
              (fmt == TEXFMT_A4R4G4B4) ? 16 : ((fmt == TEXFMT_DXT5) ? 8 : 4), 1, fmt, true))
        {
          con.addMessage(ILogWriter::ERROR, "Can't write DDSX");
        }
        optimizedSize += texSize * texSize * 2 - srcDataSize;
      }
      else
      {
        if (optimize_size)
          optimizedSize += texSize * texSize * 2;
      }
      int endOfs1 = cb.tell();
      if (!optimize_size || cnt > 5)
      {
        hmap_export_dds_as_ddsx_raw(cb, (char *)image2.data(), data_size(image2), texSize, texSize, 4, 1, TEXFMT_DXT1, true);
      }
      {
        if (optimize_size)
          optimizedSize += texSize * texSize / 2;
      }
      int endOfs2 = cb.tell();

      cb.seekto(offsets[index] + MAX_DET_TEX_NUM);
      cb.writeInt32e(endOfs2 - texOfs);
      cb.writeInt32e(endOfs1 - texOfs);

      cb.seekto(endOfs2);

      con.incDone();
    }
  }

  int ofs = cb.tell();

  cb.seekto(tableOffset);

  for (int i = 0; i < offsets.size(); i++)
    offsets[i] -= base_ofs;
  cb.writeTabData32ex(offsets);

  cb.seekto(ofs);
  con.addMessage(ILogWriter::REMARK, " in %g seconds, optimizedSize=%dbytes", (dagTools->getTimeMsec() - time0) / 1000.0f,
    optimizedSize);
  time0 = dagTools->getTimeMsec();

  con.endProgress();
  return true;
}
#else
bool aces_export_detail_maps(mkbindump::BinDumpSaveCB &cb, int mapSizeX, int mapSizeY, int tex_elem_size,
  const Tab<SimpleString> &land_class_names, int base_ofs, bool optimize_size = true, bool tools_internal = false)
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
