// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <libTools/util/strUtil.h>
#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <generic/dag_tab.h>
#include <coolConsole/coolConsole.h>

#include "hmlPlugin.h"
#include <de3_bitMaskMgr.h>


class HeightmapImporter
{
public:
  float hMin, hScale;

  HeightmapImporter() { hMin = 0, hScale = 1; }

  virtual bool importHeightmap(const char *filename, HeightMapStorage &heightmap, CoolConsole &con, HmapLandPlugin &plugin, int x0,
    int y0, int x1, int y1) = 0;
};


class Raw32fHeightmapImporter : public HeightmapImporter
{
  bool sizesHeader;
#pragma pack(push)
#pragma pack(1)
  struct Header
  {
    int w, h;
  } header;
#pragma pack(pop)
public:
  Raw32fHeightmapImporter(bool sizes_header) : sizesHeader(sizes_header) {}
  virtual bool importHeightmap(const char *filename, HeightMapStorage &heightmap, CoolConsole &con, HmapLandPlugin &plugin, int x0,
    int y0, int x1, int y1)
  {
    file_ptr_t handle = ::df_open(filename, DF_READ);

    if (!handle)
    {
      con.addMessage(ILogWriter::ERROR, "Can't open file '%s'", filename);
      return false;
    }

    int length = ::df_length(handle);
    if (sizesHeader)
    {
      if (::df_read(handle, &header, sizeof(header)) != sizeof(header))
      {
        con.addMessage(ILogWriter::ERROR, "Error reading file '%s'", filename);
        ::df_close(handle);
        return false;
      }
      length -= sizeof(header);
    }
    int mapSizeX = int(floor(sqrt((double)(length / 4)) + 0.0001));
    int mapSizeY = mapSizeX;
    if (sizesHeader)
    {
      if (header.w * header.h * 4 != length)
        con.addMessage(ILogWriter::WARNING, "File's header (%d, %d) doesn't match file size=%d '%s'", header.w, header.h,
          length - sizeof(header), filename);
      mapSizeX = header.w;
      mapSizeY = header.h;
    }
    else
    {
      header.w = mapSizeX;
      header.h = mapSizeY;
    }
    if (mapSizeX != x1 - x0 || mapSizeY != y1 - y0)
      con.addMessage(ILogWriter::WARNING, "File dim (%d, %d) doesn't match destination %dx%d '%s'", header.w, header.h, x1 - x0,
        y1 - y0, filename);

    SmallTab<float, TmpmemAlloc> buf;
    clear_and_resize(buf, mapSizeX);

    con.startProgress();
    con.setActionDesc("importing heightmap...");
    con.setTotal(mapSizeY);

    for (int y = mapSizeY - 1, cnt = 0; y >= 0; --y)
    {
      if (::df_read(handle, buf.data(), data_size(buf)) != data_size(buf))
      {
        con.addMessage(ILogWriter::ERROR, "Error reading file '%s'", filename);
        ::df_close(handle);
        return false;
      }

      if (y >= y1 - y0)
        continue;
      for (int x = 0; x < mapSizeX && x < x1 - x0; ++x)
      {
        heightmap.setInitialData(x + x0, y + y0, buf[x]);
        heightmap.setFinalData(x + x0, y + y0, buf[x]);
      }

      con.incDone();
    }

    heightmap.flushData();

    con.endProgress();

    ::df_close(handle);

    return true;
  }
};


class Raw16HeightmapImporter : public HeightmapImporter
{
public:
  virtual bool importHeightmap(const char *filename, HeightMapStorage &heightmap, CoolConsole &con, HmapLandPlugin &plugin, int x0,
    int y0, int x1, int y1)
  {
    file_ptr_t handle = ::df_open(filename, DF_READ);

    if (!handle)
    {
      con.addMessage(ILogWriter::ERROR, "Can't open file '%s'", filename);
      return false;
    }

    int length = ::df_length(handle);

    int mapSizeX = int(floor(sqrt((double)(length / 2)) + 0.0001));
    int mapSizeY = mapSizeX;

    if (mapSizeX != x1 - x0 || mapSizeY != y1 - y0)
      con.addMessage(ILogWriter::WARNING, "File dim (%d, %d) doesn't match destination %dx%d '%s'", mapSizeX, mapSizeY, x1 - x0,
        y1 - y0, filename);

    SmallTab<uint16_t, TmpmemAlloc> buf;
    clear_and_resize(buf, mapSizeX);

    con.startProgress();
    con.setActionDesc("importing heightmap...");
    con.setTotal(mapSizeY);

    for (int y = mapSizeY - 1, cnt = 0; y >= 0; --y)
    {
      if (::df_read(handle, buf.data(), data_size(buf)) != data_size(buf))
      {
        con.addMessage(ILogWriter::ERROR, "Error reading file '%s'", filename);
        ::df_close(handle);
        return false;
      }

      if (y >= y1 - y0)
        continue;
      for (int x = 0; x < mapSizeX && x < x1 - x0; ++x)
      {
        float v = buf[x] / float((1 << 16) - 1) * hScale + hMin;
        heightmap.setInitialData(x + x0, y + y0, v);
        heightmap.setFinalData(x + x0, y + y0, v);
      }

      con.incDone();
    }

    heightmap.flushData();
    heightmap.unloadAllUnchangedData();

    con.endProgress();

    ::df_close(handle);

    return true;
  }
};


class TgaHeightmapImporter : public HeightmapImporter
{
public:
  virtual bool importHeightmap(const char *filename, HeightMapStorage &heightmap, CoolConsole &con, HmapLandPlugin &plugin, int x0,
    int y0, int x1, int y1)
  {
    con.startProgress();
    con.setActionDesc("loading TGA image...");
    con.setTotal(2);
    con.incDone();

    TexImage8 *img = load_tga8(filename, tmpmem);

    if (!img)
    {
      con.endProgress();
      con.addMessage(ILogWriter::ERROR, "Can't load TGA file '%s'", filename);
      return false;
    }

    int mapSizeX = img->w;
    int mapSizeY = img->h;

    if (mapSizeX != x1 - x0 || mapSizeY != y1 - y0)
      con.addMessage(ILogWriter::WARNING, "File dim (%d, %d) doesn't match destination %dx%d '%s'", mapSizeX, mapSizeY, x1 - x0,
        y1 - y0, filename);

    uint8_t *ptr = (uint8_t *)(img + 1);

    con.setActionDesc("importing heightmap...");
    con.setTotal(mapSizeY);

    for (int y = 0, cnt = 0; y < mapSizeY; y++)
    {
      if (y >= y1 - y0)
        break;
      for (int x = 0; x < mapSizeX && x < x1 - x0; ++x, ++ptr)
      {
        float v = *ptr / float((1 << 8) - 1) * hScale + hMin;
        heightmap.setInitialData(x + x0, y + y0, v);
        heightmap.setFinalData(x + x0, y + y0, v);
      }

      con.incDone();
    }

    heightmap.flushData();

    con.endProgress();

    memfree(img, tmpmem);

    return true;
  }
};


class TiffHeightmapImporter : public HeightmapImporter
{
public:
  virtual bool importHeightmap(const char *filename, HeightMapStorage &heightmap, CoolConsole &con, HmapLandPlugin &plugin, int x0,
    int y0, int x1, int y1)
  {
    con.startProgress();
    con.setActionDesc("loading TIFF image...");
    con.setTotal(2);
    con.incDone();

    IBitMaskImageMgr::BitmapMask bm;
    if (!HmapLandPlugin::bitMaskImgMgr->loadImage(bm, NULL, filename))
    {
      con.endProgress();
      con.addMessage(ILogWriter::ERROR, "Can't load TIFF file '%s'", filename);
      return false;
    }
    if (bm.getBitsPerPixel() < 4)
    {
      con.endProgress();
      con.addMessage(ILogWriter::ERROR, "TIFF file '%s' has too low BPP=%d", filename, bm.getBitsPerPixel());
      return false;
    }


    int mapSizeX = bm.getWidth();
    int mapSizeY = bm.getHeight();

    if (mapSizeX != x1 - x0 || mapSizeY != y1 - y0)
      con.addMessage(ILogWriter::WARNING, "File dim (%d, %d) doesn't match destination %dx%d '%s'", mapSizeX, mapSizeY, x1 - x0,
        y1 - y0, filename);

    con.setActionDesc("importing heightmap (%dx%d  %dbpp)...", bm.getWidth(), bm.getHeight(), bm.getBitsPerPixel());
    con.setTotal(mapSizeY);

    double mScale = 0xFFu;
    if (bm.getBitsPerPixel() == 4)
      mScale = 0xFu;
    else if (bm.getBitsPerPixel() == 16)
      mScale = 0xFFFFu;
    else if (bm.getBitsPerPixel() == 32)
      mScale = 0xFFFFFFFFu;
    for (int y = 0, cnt = 0; y < mapSizeY; y++)
    {
      if (y >= y1 - y0)
        break;
      for (int x = 0; x < mapSizeX && x < x1 - x0; ++x)
      {
        unsigned pix = bm.getMaskPixelAnyBpp(x, mapSizeY - 1 - y);
        float v = pix / mScale * hScale + hMin;
        heightmap.setInitialData(x + x0, y + y0, v);
        heightmap.setFinalData(x + x0, y + y0, v);
      }

      con.incDone();
    }

    heightmap.flushData();
    HmapLandPlugin::bitMaskImgMgr->destroyImage(bm);

    con.endProgress();

    return true;
  }
};


bool HmapLandPlugin::importHeightmap(String &filename, HeightmapTypes type)
{
  const char *ext = get_file_ext(filename);
  if (!ext)
    return false;

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  HeightmapImporter *importer = NULL;

  if (stricmp(ext, "r32") == 0)
    importer = new Raw32fHeightmapImporter(false);
  else if (stricmp(ext, "height") == 0)
    importer = new Raw32fHeightmapImporter(true);
  else if (stricmp(ext, "r16") == 0 || stricmp(ext, "raw") == 0)
    importer = new Raw16HeightmapImporter;
  else if (stricmp(ext, "tif") == 0)
    importer = new TiffHeightmapImporter;
  else
  {
    // importer=new TgaHeightmapImporter;
    return false;
  }

  HeightMapStorage *hms = &heightMap;
  float hMin = lastMinHeight[0];
  float hScale = lastHeightRange[0];
  IBBox2 importRect(IPoint2(0, 0), IPoint2(heightMap.getMapSizeX(), heightMap.getMapSizeY()));
  bool importWater = false;
  if (type == HeightmapTypes::HEIGHTMAP_DET)
  {
    hms = &heightMapDet;
    hMin = lastMinHeight[1];
    hScale = lastHeightRange[1];
    importRect = detRectC;
  }
  else if (type == HeightmapTypes::HEIGHTMAP_WATER_DET)
  {
    hms = &waterHeightmapDet;
    importWater = true;
  }
  else if (type == HeightmapTypes::HEIGHTMAP_WATER_MAIN)
  {
    hms = &waterHeightmapMain;
    importWater = true;
    importRect[1] = IPoint2(hms->getMapSizeX(), hms->getMapSizeY());
  }
  if (importWater)
  {
    hMin = 0.0f;
    hScale = 1.0f;
  }

  importer->hMin = hMin;
  importer->hScale = hScale;

  bool useDetRect = type == HeightmapTypes::HEIGHTMAP_DET;
  if (!importer->importHeightmap(filename, *hms, con, *this, importRect[0].x, importRect[0].y, importRect[1].x, importRect[1].y))
  {
    con.endLog();
    return false;
  }

  con.addMessage(ILogWriter::NOTE, "Imported heightmap from '%s'", (const char *)filename);

  if (!hms->flushData())
  {
    con.addMessage(ILogWriter::ERROR, "Error saving heightmap data");
    con.endLog();
    return false;
  }
  if (importWater)
    applyHmModifiers(false);

  con.endLog();

  return true;
}
