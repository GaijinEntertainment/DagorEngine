#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_zlibIo.h>
#include <libTools/util/strUtil.h>
#include <math/dag_bounds2.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_logSys.h>

#include "mgr.h"

void __cdecl ctrl_break_handler(int) { quit_game(0); }


struct GridCellRec
{
  Point2 pos;
  real density;
};


IBitMaskImageMgr *mgr = NULL;


void fillDensityCells(DataBlock &blk, Tab<GridCellRec> &cells)
{
  DataBlock *cb = blk.getBlockByName("DensityCells");
  if (!cb)
    return;

  int nid = cb->getNameId("DensityCell");

  for (int i = 0; i < cb->blockCount(); i++)
  {
    if (cb->getBlock(i)->getBlockNameId() == nid)
    {
      DataBlock &g = *cb->getBlock(i);

      GridCellRec r;
      r.pos = g.getPoint2("position", Point2(0, 0));
      r.density = g.getReal("density", 0);

      cells.push_back(r);
    }
  }
}


void makeTifMask(const char *fname, int w, int h, int bpp, real cell_size, Tab<GridCellRec> &c, Point2 offs, bool hmap)
{
  int width = w / cell_size;
  int height = h / cell_size;

  int cw = width % 8;
  if (cw)
    width += 8 - cw;

  int ch = height % 8;
  if (ch)
    height += 8 - ch;

  printf("creating TIF mask %s (%dx%dx%dbits)...", fname, width, height, bpp);

  IBitMaskImageMgr::BitmapMask img;
  mgr->createBitMask(img, width, height, bpp);

  if (hmap)
  {
    offs /= cell_size;
    Point2 realHmap = Point2(width, height) + offs;

    for (int i = 0; i < c.size(); i++)
    {
      real x = c[i].pos.x / cell_size;
      real y = c[i].pos.y / cell_size;

      if (x >= offs.x && y >= offs.y && x < realHmap.x && y < realHmap.y)
        img.setMaskPixel1((int)(x - offs.x), (int)(height - (y - offs.y) + 1), c[i].density ? 255 : 0);
    }
  }
  else
  {
    for (int i = 0; i < c.size(); i++)
    {
      real x = (c[i].pos.x - offs.x) / cell_size;
      real y = (c[i].pos.y - offs.y) / cell_size;

      img.setMaskPixel8((int)x, (int)(height - y + 1), c[i].density * 255.0);
    }
  }

  mgr->saveImage(img, fname);
  mgr->destroyImage(img);

  printf("OK\n");
}


void uncompressHmap(const char *in, const char *out)
{
  file_ptr_t fileHandle = df_open(in, DF_RDWR);
  int fsize = df_length(fileHandle);

  LFileGeneralLoadCB crd(fileHandle);
  FullFileSaveCB cwr(out);
  if (crd.readInt() != _MAKE4C('ZLIB'))
  {
    crd.seekto(0);
    copy_stream_to_stream(crd, cwr, fsize);
    df_close(fileHandle);
    cwr.close();
    return;
  }

  fsize = crd.readInt();

  if (!cwr.fileHandle)
  {
    df_close(fileHandle);
    return;
  }

  ZlibLoadCB z_crd(crd, fsize);
  copy_stream_to_stream(z_crd, cwr, fsize);
  z_crd.close();
  cwr.close();

  df_close(fileHandle);
}

void fillObjectsBlk(DataBlock &in, DataBlock &out)
{
  DataBlock *cb = in.getBlockByName("brushObjects");
  if (!cb)
    return;

  int ni = cb->getNameId("object");

  for (int i = 0; i < cb->blockCount(); i++)
  {
    if (cb->getBlock(i)->getBlockNameId() == ni)
    {
      DataBlock &g = *cb->getBlock(i);
      DataBlock *o = out.addNewBlock("object");

      o->addStr("orientation", "world");
      o->addStr("name", g.getStr("name", ""));
      o->addReal("weight", g.getReal("weight", 1));
      o->addPoint2("rot_x", Point2(0, 0));
      o->addPoint2("rot_y", Point2(0, g.getReal("disperse_orientation", 0)));
      o->addPoint2("rot_z", Point2(0, 0));
      o->addPoint2("scale", g.getPoint2("sizeDSize", Point2(1, 0)));
      o->addPoint2("yScale", g.getPoint2("heightDHeight", Point2(1, 0)));
    }
  }
}


int convertFile(const char *in_leveldir, const char *out_filename, bool hmap_size_gen)
{
  if (!in_leveldir || !*in_leveldir || !out_filename || !*out_filename)
    return -1;

  String inLevelDir(in_leveldir);
  append_slash(inLevelDir);

  String inGridFile(inLevelDir + "Plant/grids.blk.bin");

  Point2 hmapSize, hmapOffset;
  real hmapCellSize;

  if (hmap_size_gen)
  {
    String inHmapFile(inLevelDir + "HeightmapLand/HeightmapLand.plugin.blk");
    DataBlock hcb(inHmapFile);
    hmapOffset = hcb.getPoint2("heightMapOffset", Point2(12, 0));
    hmapCellSize = hcb.getReal("gridCellSize", 0);

    inHmapFile = inLevelDir + "HeightmapLand/heightmap.dat";

    if (!dd_file_exist(inHmapFile))
    {
      printf("warning: heightmap.dat wasn't found!\n");
      hmap_size_gen = false;
    }
    else
    {
      const char *outhmapfname = "hmaptmp.dat";
      uncompressHmap(inHmapFile, outhmapfname);
      FullFileLoadCB crd(outhmapfname);
      crd.readInt();

      hmapSize.x = (real)crd.readInt();
      hmapSize.y = (real)crd.readInt();

      hmapSize *= hmapCellSize;

      crd.close();
      dd_erase(outhmapfname);
    }
  }

  DataBlock blk;
  if (!blk.load(inGridFile))
  {
    printf("ERROR: can't load DataBlock %s\n", (char *)inGridFile);
    return -1;
  }

  DataBlock *cb = blk.getBlockByName("grids");
  if (!cb)
    return -1;

  DataBlock out;
  out.addStr("className", "landClass");

  int ni = cb->getNameId("grid");

  for (int i = 0; i < cb->blockCount(); i++)
  {
    if (cb->getBlock(i)->getBlockNameId() == ni)
    {
      DataBlock &g = *cb->getBlock(i);
      DataBlock *o = out.addNewBlock("obj_plant_generate");

      o->addBool("place_on_collision", true);
      o->addReal("density", g.getReal("density", 0) / 100.0);
      o->addReal("intersection", g.getReal("intersection", 0));
      o->addInt("rseed", g.getInt("rseed", 0));

      const char *gridName = g.getStr("name", "");
      String tifName(512, "./%s_mask", gridName);

      real cellSize = g.getReal("cell_size", 1);

      if (!hmap_size_gen)
        o->addStr("densityMap", tifName);

      Tab<GridCellRec> gridCells(midmem);
      fillDensityCells(g, gridCells);
      if (!gridCells.size())
        continue;

      if (hmap_size_gen)
      {
        makeTifMask(tifName + ".tif", hmapSize.x, hmapSize.y, 1, cellSize, gridCells, hmapOffset, true);
      }
      else
      {
        BBox2 box;
        box.setempty();
        for (int j = 0; j < gridCells.size(); j++)
          box += gridCells[j].pos;

        Point2 offs = box[0];

        box.lim[1] -= box.lim[0];
        box.lim[0] = Point2(0, 0);

        o->addPoint2("mapSize", box[1]);
        o->addPoint2("mapOffset", offs);

        makeTifMask(tifName + ".tif", box[1].x, box[1].y, 8, cellSize, gridCells, offs, false);
      }

      fillObjectsBlk(g, *o);
    }
  }

  out.saveToTextFile(out_filename);
  printf("\nprocessed: %s", (char *)inGridFile);
  return 0;
}


int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  printf("Plants Grid to daEditor3 TIFF mask converter\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  if (__argc < 3)
  {
    printf("usage: <source level dir> <destination asset file>"
           "[-hmap (1bits mask with hmap-size)]\n");
    flush_debug_file();
    terminate_process(0);
    return -1;
  }

  bool hmapSizeGen = false;
  if (__argc == 4 && stricmp(__argv[3], "-hmap") == 0)
    hmapSizeGen = true;

  mgr = get_tiff_bit_mask_image_mgr();
  convertFile(__argv[1], __argv[2], hmapSizeGen);

  flush_debug_file();
  terminate_process(0);
  return 0;
}
