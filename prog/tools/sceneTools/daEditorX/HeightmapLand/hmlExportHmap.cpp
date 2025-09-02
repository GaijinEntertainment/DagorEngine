// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <coolConsole/coolConsole.h>
#include <de3_bitMaskMgr.h>


class HeightmapExporter
{
public:
  virtual ~HeightmapExporter() {}
  virtual bool exportHeightmap(const char *filename, HeightMapStorage &heightmap, real min_height, real height_range, CoolConsole &con,
    int x0, int y0, int x1, int y1) = 0;
};


class Raw32fHeightmapExporter : public HeightmapExporter
{
#pragma pack(push)
#pragma pack(1)
  struct Header
  {
    int w, h;
  };
#pragma pack(pop)
  bool writeHeader;

public:
  Raw32fHeightmapExporter(bool write_header) : writeHeader(write_header) {}
  bool exportHeightmap(const char *filename, HeightMapStorage &heightmap, real, real, CoolConsole &con, int x0, int y0, int x1,
    int y1) override
  {
    file_ptr_t handle = ::df_open(filename, DF_WRITE | DF_CREATE);

    if (!handle)
    {
      con.addMessage(ILogWriter::ERROR, "Can't create file '%s'", filename);
      return false;
    }

    if (writeHeader)
    {
      Header header;
      header.w = x1 - x0;
      header.h = y1 - y0;
      if (::df_write(handle, &header, sizeof(header)) != sizeof(header))
      {
        con.addMessage(ILogWriter::ERROR, "Error writing file '%s'", filename);
        ::df_close(handle);
        return false;
      }
    }

    SmallTab<float, TmpmemAlloc> buf;
    clear_and_resize(buf, x1 - x0);

    con.startProgress();
    con.setActionDesc("exporting heightmap...");
    con.setTotal(y1 - y0);

    for (int y = y1 - 1, cnt = 0; y >= y0; --y)
    {
      for (int x = x0; x < x1; ++x)
        buf[x - x0] = heightmap.getFinalData(x, y);

      if (::df_write(handle, buf.data(), data_size(buf)) != data_size(buf))
      {
        con.addMessage(ILogWriter::ERROR, "Error writing file '%s'", filename);
        ::df_close(handle);
        return false;
      }

      con.incDone();
    }

    heightmap.unloadAllUnchangedData();

    con.endProgress();

    ::df_close(handle);

    return true;
  }
};


class Raw16HeightmapExporter : public HeightmapExporter
{
public:
  bool exportHeightmap(const char *filename, HeightMapStorage &heightmap, real min_height, real height_range, CoolConsole &con, int x0,
    int y0, int x1, int y1) override
  {
    file_ptr_t handle = ::df_open(filename, DF_WRITE | DF_CREATE);

    if (!handle)
    {
      con.addMessage(ILogWriter::ERROR, "Can't create file '%s'", filename);
      return false;
    }

    SmallTab<uint16_t, TmpmemAlloc> buf;
    clear_and_resize(buf, x1 - x0);

    con.startProgress();
    con.setActionDesc("exporting heightmap...");
    con.setTotal((y1 - y0) * 2);

    float minH = min_height;
    float maxH = min_height + height_range;

    heightmap.unloadAllUnchangedData();

    float scale = maxH - minH;
    if (float_nonzero(scale))
      scale = ((1 << 16) - 1) / scale;

    for (int y = y1 - 1, cnt = 0; y >= y0; --y)
    {
      for (int x = x0; x < x1; ++x)
      {
        int h = real2int((heightmap.getFinalData(x, y) - minH) * scale + 0.5);

        if (h < 0)
          h = 0;
        else if (h >= (1 << 16))
          h = (1 << 16) - 1;

        buf[x - x0] = h;
      }

      if (::df_write(handle, buf.data(), data_size(buf)) != data_size(buf))
      {
        con.addMessage(ILogWriter::ERROR, "Error writing file '%s'", filename);
        ::df_close(handle);
        return false;
      }

      con.incDone();
    }

    heightmap.unloadAllUnchangedData();

    con.endProgress();

    ::df_close(handle);

    return true;
  }
};


class TgaHeightmapExporter : public HeightmapExporter
{
public:
  bool exportHeightmap(const char *filename, HeightMapStorage &heightmap, real min_height, real height_range, CoolConsole &con, int x0,
    int y0, int x1, int y1) override
  {
    file_ptr_t handle = ::df_open(filename, DF_WRITE | DF_CREATE);

    if (!handle)
    {
      con.addMessage(ILogWriter::ERROR, "Can't create file '%s'", filename);
      return false;
    }

    int mapSizeX = x1 - x0;
    int mapSizeY = y1 - y0;

    SmallTab<E3DCOLOR, TmpmemAlloc> buf;
    clear_and_resize(buf, mapSizeX);

    con.startProgress();
    con.setActionDesc("exporting heightmap...");
    con.setTotal(mapSizeY * 2);

    ::df_write(handle, "\0\0\2\0\0\0\0\0\0\0\0\0", 12);
    ::df_write(handle, &mapSizeX, 2);
    ::df_write(handle, &mapSizeY, 2);
    ::df_write(handle, "\x20\x08", 2);

    float minH = min_height;
    float maxH = min_height + height_range;

    heightmap.unloadAllUnchangedData();

    float scale = maxH - minH;
    if (float_nonzero(scale))
      scale = ((1 << 8) - 1) / scale;

    for (int y = y0, cnt = 0; y < y1; ++y)
    {
      for (int x = x0; x < x1; ++x)
      {
        int h = real2int((heightmap.getFinalData(x, y) - minH) * scale);

        if (h < 0)
          h = 0;
        else if (h >= (1 << 8))
          h = (1 << 8) - 1;

        buf[x - x0] = E3DCOLOR(h, h, h);
      }

      if (::df_write(handle, buf.data(), data_size(buf)) != data_size(buf))
      {
        con.addMessage(ILogWriter::ERROR, "Error writing file '%s'", filename);
        ::df_close(handle);
        return false;
      }

      heightmap.unloadUnchangedData(y + 1);

      con.incDone();
    }

    heightmap.unloadAllUnchangedData();

    con.endProgress();

    ::df_close(handle);

    return true;
  }
};


class TiffHeightmapExporter : public HeightmapExporter
{
public:
  bool exportHeightmap(const char *filename, HeightMapStorage &heightmap, real min_height, real height_range, CoolConsole &con, int x0,
    int y0, int x1, int y1) override
  {
    int mapSizeX = x1 - x0;
    int mapSizeY = y1 - y0;
    IBitMaskImageMgr::BitmapMask img;
    static const int MAX_VAL = 0xFFFF;
    HmapLandPlugin::bitMaskImgMgr->createBitMask(img, mapSizeX, mapSizeX, 16);

    con.startProgress();
    con.setActionDesc("exporting heightmap...");
    con.setTotal(mapSizeY * 2);

    float minH = min_height;
    float maxH = min_height + height_range;

    heightmap.unloadAllUnchangedData();

    float scale = maxH - minH;
    if (float_nonzero(scale))
      scale = MAX_VAL / scale;

    for (int y = y0, cnt = 0; y < y1; ++y)
    {
      for (int x = x0; x < x1; ++x)
      {
        int h = clamp(int((heightmap.getFinalData(x, y) - minH) * scale), 0, int(MAX_VAL));
        img.setMaskPixel16(x - x0, y1 - 1 - y, h);
      }
      heightmap.unloadUnchangedData(y + 1);
      con.incDone();
    }

    heightmap.unloadAllUnchangedData();

    bool ret = HmapLandPlugin::bitMaskImgMgr->saveImage(img, NULL, filename);
    HmapLandPlugin::bitMaskImgMgr->destroyImage(img);

    con.endProgress();

    if (!ret)
      con.addMessage(ILogWriter::ERROR, "Can't write file '%s'", filename);

    return ret;
  }
};


bool HmapLandPlugin::exportHeightmap(String &filename, real min_height, real height_range, bool exp_det_hmap)
{
  const char *ext = dd_get_fname_ext(filename);
  if (!ext)
    return false;

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  HeightmapExporter *exporter = NULL;

  if (stricmp(ext, ".r32") == 0)
    exporter = new Raw32fHeightmapExporter(false);
  else if (stricmp(ext, ".height") == 0)
    exporter = new Raw32fHeightmapExporter(true);
  else if (stricmp(ext, ".r16") == 0 || stricmp(ext, ".raw") == 0)
    exporter = new Raw16HeightmapExporter;
  else if (stricmp(ext, ".tif") == 0 || stricmp(ext, ".tiff") == 0)
    exporter = new TiffHeightmapExporter;
  else
    exporter = new TgaHeightmapExporter;

  applyHmModifiers(false);

  if (!exporter->exportHeightmap(filename, exp_det_hmap ? heightMapDet : heightMap, min_height, height_range, con,
        exp_det_hmap ? detRectC[0].x : 0, exp_det_hmap ? detRectC[0].y : 0, exp_det_hmap ? detRectC[1].x : heightMap.getMapSizeX(),
        exp_det_hmap ? detRectC[1].y : heightMap.getMapSizeY()))
  {
    con.endLog();
    delete exporter;
    return false;
  }

  con.addMessage(ILogWriter::NOTE, "Exported heightmap to '%s'", (const char *)filename);

  con.endLog();

  delete exporter;
  return true;
}
