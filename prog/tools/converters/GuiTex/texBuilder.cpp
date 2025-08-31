// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/dtx.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/genericCache.h>
#include <libTools/util/fileUtils.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_findFiles.h>
#include <stdio.h>
#include <stdlib.h>
#include <3d/ddsxTex.h>
#include <debug/dag_debug.h>
#include <image/dag_texPixel.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_log.h>
#include <image/dag_tga.h>
#include <image/dag_loadImage.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_globalSettings.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_oaHashNameMap.h>
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

enum
{
  EMPTY = 0,
  ZERO_ALPHA,
  OCCUPIED
};

static bool premultAlpha = false;

extern int unpackTex(const char *coordblk, const char *dest_dir, bool no_stripes);

int makeAtlas(dag::ConstSpan<String> texNames, Tab<String> &outNames, short width, short height, const char *textureName,
  const char *coordblk, int margin, bool may_flip);

static void error(const char *er)
{
  fprintf(stderr, "GuiTex: FATAL ERROR: %s", er);
  exit(13);
}

static int __width = 0, __width32 = 0, __height = 0;
static bool __quietMode = false, __mkref = false;
static bool __mkbundle = false;
static unsigned __ddsx_target = 0;
static const char *__cmd_outdir = NULL, *__cmd_outblk = NULL;

static char *map = NULL;
static uint32_t *occupied_map = NULL; // bitmask

static TexImage32 *texOut = NULL;

static bool write_atlas_dds(FullFileSaveCB &fcwr, TexImage32 *im, const DataBlock &blk, unsigned ddsx_t, int &out_tex_size,
  int &out_file_size, bool quiet)
{
  ddstexture::Converter cvt;
  MemorySaveCB cwr;

  uint64_t tc_storage = 0;
  const char *fmt =
    blk.getStr(String::mk_str_cat("format_", mkbindump::get_target_str(ddsx_t, tc_storage)), blk.getStr("format", "rgba"));
  int mips = blk.getInt("mips", 1);

  if (strcmp(fmt, "rgba") == 0)
    cvt.format = cvt.fmtARGB;
  else if (strcmp(fmt, "dxt1") == 0)
    cvt.format = cvt.fmtDXT1;
  else if (strcmp(fmt, "dxt3") == 0)
    cvt.format = cvt.fmtDXT3;
  else if (strcmp(fmt, "dxt5") == 0)
    cvt.format = cvt.fmtDXT5;
  else if (strcmp(fmt, "4444") == 0)
    cvt.format = cvt.fmtRGBA4444;
  else if (strcmp(fmt, "5551") == 0)
    cvt.format = cvt.fmtRGBA5551;
  else if (strcmp(fmt, "565") == 0)
    cvt.format = cvt.fmtRGB565;
  else if (strcmp(fmt, "l8") == 0)
    cvt.format = cvt.fmtL8;
  else
  {
    printf("ERROR: unknown output format <%s>\n", fmt);
    return false;
  }

  if (mips == 0)
  {
    cvt.mipmapType = cvt.mipmapGenerate;
    cvt.mipmapCount = cvt.AllMipMaps;
  }
  else if (mips == 1)
  {
    cvt.mipmapType = cvt.mipmapNone;
    cvt.mipmapCount = 0;
  }
  else
  {
    cvt.mipmapType = cvt.mipmapGenerate;
    cvt.mipmapCount = mips - 1;
  }

  if (!quiet)
    printf("converting to DDS:%s format...\n", fmt);

  bool ret = cvt.convertImage(cwr, (TexPixel32 *)(im + 1), im->w, im->h, im->w * 4);
  out_tex_size = out_file_size = cwr.tell();
  if (!ret)
  {
    printf("ERROR: cannot convert image to DDS (%s)\n", fcwr.getTargetName());
    fcwr.close();
    dd_erase(fcwr.getTargetName());
    return false;
  }

  if (ddsx_t)
  {
    if (!quiet)
      printf("converting to DDSx, target: %c%c%c%c ...\n", _DUMP4C(ddsx_t));

    const int len = cwr.getSize();
    SmallTab<char, TmpmemAlloc> buf;
    clear_and_resize(buf, len);
    {
      MemoryLoadCB crd(cwr.getMem(), false);
      crd.read(buf.data(), len);
    }
    cwr.deleteMem();

    ddsx::Buffer b;
    ddsx::ConvertParams cp;
    cp.packSzThres = blk.getInt("ddsxPackThres", 16 << 10);
    cp.imgGamma = 1.0;
    cp.mipOrdRev = false;

    if (ddsx::convert_dds(ddsx_t, b, buf.data(), len, cp))
    {
      out_tex_size = mkbindump::le2be32_cond(((ddsx::Header *)b.ptr)->memSz, dagor_target_code_be(ddsx_t));
      out_file_size = b.len;
      fcwr.write(b.ptr, b.len);
      b.free();
    }
    else
    {
      printf("ERROR: cannot convert DDS to DDSx (%c%c%c%c): %s\n", _DUMP4C(ddsx_t), ddsx::get_last_error_text());
      fcwr.close();
      dd_erase(fcwr.getTargetName());
      return false;
    }
  }
  else
    cwr.copyDataTo(fcwr);

  return ret;
}

bool _checkPlace(int xPos, int yPos, TexImage32 *tex, const IPoint2 &lt_ofs, const IPoint2 &vis_sz, int transf_code)
{
  TexPixel32 *p = tex->getPixels() + lt_ofs.y * tex->w + lt_ofs.x;
  int a = 0, b = 0, c = 0;
  switch (transf_code)
  {
    case 0: a = 1, b = tex->w, c = 0; break;
    case 1: a = -1, b = tex->w, c = tex->w - 1; break;
    case 2: a = 1, b = -tex->w, c = (tex->h - 1) * tex->w; break;
    case 3: a = -1, b = -tex->w, c = (tex->h - 1) * tex->w + tex->w - 1; break;
    default: return false;
  }
  for (int y = 0; y < vis_sz.y; y += 16)
  {
    if (yPos + y < 0 || yPos + y >= __height)
      continue;
    for (int x = 0; x < vis_sz.x; x += 16)
    {
      if (xPos + x < 0 || xPos + x >= __width)
        continue;
      if ((map[(yPos + y) * __width + (xPos + x)] + (p[a * x + b * y + c].a > 0)) > 1)
        return false;
    }
  }

  for (int y = 0; y < vis_sz.y; y++)
  {
    if (yPos + y < 0 || yPos + y >= __height)
      continue;
    for (int x = 0; x < vis_sz.x; x++)
    {
      if (xPos + x < 0 || xPos + x >= __width)
        continue;
      if ((map[(yPos + y) * __width + (xPos + x)] + (p[a * x + b * y + c].a > 0)) > 1)
        return false;
    }
  }

  return true;
}

bool checkTransFrame(int x, int y, int w, int h)
{
  /*{
    const char *p0 = map + y*__width + x;
    const char *p1 = map + (y+h-1)*__width + x;
    for (int ix=x; ix<x+w; ix++, p0++, p1++)
      if (*p0==OCCUPIED || *p1==OCCUPIED )
        return false;
  }*/
  int x32 = (x + 31) & (~31);
  int xw32 = (x + w) & (~31);
  int pos0 = y * __width32 + x32;
  int pos1 = (y + h - 1) * __width32 + x32;
  const uint32_t *o0 = occupied_map + (pos0 >> 5);
  const uint32_t *o1 = occupied_map + (pos1 >> 5);

  if (x32 != x)
  {
    uint32_t start_mask = ((1 << (x32 - x)) - 1) << (x & 31);
    if ((o0[-1] | o1[-1]) & start_mask)
      return false;
  }
  if (xw32 != x + w)
  {
    uint32_t end_mask = (1 << (x + w - xw32)) - 1;
    int pos0 = y * __width32 + xw32;
    int pos1 = (y + h - 1) * __width32 + xw32;
    if ((occupied_map[pos0 >> 5] | occupied_map[pos1 >> 5]) & end_mask)
      return false;
  }
  for (int ix = x32; ix < xw32; ix += 32, o0++, o1++)
    if (*o0 | *o1)
      return false;

  {
    int pos0 = y * __width32 + x;
    int pos1 = pos0 + w - 1;
    const uint32_t *o0 = occupied_map + (pos0 >> 5);
    const uint32_t *o1 = occupied_map + (pos1 >> 5);
    int width32_ = __width32 >> 5;
    uint32_t mask0 = 1 << (x & 31), mask1 = 1 << ((x + w - 1) & 31);
    for (int iy = y; iy < y + h; iy++, o0 += width32_, o1 += width32_)
      if ((*o0 & mask0) | (*o1 & mask1))
        return false;
  }
  /*const char *p0 = map + y*__width + x;
  const char *p1 = map + y*__width + x+w-1;
  for (int iy=y; iy<y+h; iy++, p0+=__width, p1+=__width)
    if (*p0==OCCUPIED || *p1==OCCUPIED )
    {
      return false;
    }
  */

  return true;
}

void markFrame(int x, int y, int w, int h)
{
  for (int ix = x; ix < x + w; ix++)
  {
    char *p = map + (y * __width + ix);
    if (*p == EMPTY)
      *p = ZERO_ALPHA;

    p += (h - 1) * __width;
    if (*p == EMPTY)
      *p = ZERO_ALPHA;
  }

  for (int iy = y; iy < y + h; iy++)
  {
    char *p = map + (iy * __width + x);
    if (*p == EMPTY)
      *p = ZERO_ALPHA;

    p += w - 1;
    if (*p == EMPTY)
      *p = ZERO_ALPHA;
  }
}

void cropTex(TexImage32 *tex, IPoint2 &lt_ofs, IPoint2 &vis_sz)
{
  lt_ofs.zero();
  vis_sz.set(tex->w, tex->h);
}

IPoint2 placeTex(TexImage32 *tex, const IPoint2 &lt_ofs, const IPoint2 &vis_sz, int MARGIN, int &transf_code, bool may_flip)
{
  IPoint2 res(-1, -1);
  transf_code = -1;

  for (int tc = 0; tc < 4; tc++)
  {
    for (int y = 0; y <= texOut->h - vis_sz.y; y++)
    {
      for (int x = 0; x <= texOut->w - vis_sz.x; x++)
      {
        int l0 = max(x - MARGIN, 0), w0 = min(x + vis_sz.x + MARGIN, __width) - l0;
        int t0 = max(y - MARGIN, 0), h0 = min(y + vis_sz.y + MARGIN, __height) - t0;

        if (checkTransFrame(l0, t0, w0, h0) && _checkPlace(x, y, tex, lt_ofs, vis_sz, tc))
        {
          if (transf_code < 0 || y < res.y || (y == res.y && x < res.x))
          {
            transf_code = tc;
            res = IPoint2(x, y);
          }
          goto next_transf_code;
        }
      }
    }
  next_transf_code:;
    if (!may_flip)
      break;
  }

  if (res.x < 0 || res.y < 0)
    return res;

  debug("pos found = %d, %d (%d)", res.x, res.y, transf_code);

  for (int i = 1; i <= MARGIN; i++)
  {
    int l0 = max(res.x - i, 0), w0 = min(res.x + vis_sz.x + i, __width) - l0;
    int t0 = max(res.y - i, 0), h0 = min(res.y + vis_sz.y + i, __height) - t0;
    markFrame(l0, t0, w0, h0);
  }

  // put to image and to map
  TexPixel32 *p = tex->getPixels() + lt_ofs.y * tex->w + lt_ofs.x;
  int a = 0, b = 0, c = 0;
  switch (transf_code)
  {
    case 0: a = 1, b = tex->w, c = 0; break;
    case 1: a = -1, b = tex->w, c = tex->w - 1; break;
    case 2: a = 1, b = -tex->w, c = (tex->h - 1) * tex->w; break;
    case 3: a = -1, b = -tex->w, c = (tex->h - 1) * tex->w + tex->w - 1; break;
  }

  for (int y = 0; y < vis_sz.y; y++)
  {
    for (int x = 0; x < vis_sz.x; x++)
    {
      TexPixel32 &pd = texOut->getPixels()[(res.y + y) * __width + res.x + x];
      const TexPixel32 &ps = p[a * x + b * y + c];

      map[(res.y + y) * __width + res.x + x] = ps.a ? OCCUPIED : ZERO_ALPHA;
      if (ps.a)
      {
        int pos = (res.y + y) * __width32 + res.x + x;
        occupied_map[pos >> 5] |= (1 << (pos & 31));
      }

      if (premultAlpha)
      {
        pd.r = ((float)ps.r) * ((float)ps.a / 255);
        pd.g = ((float)ps.g) * ((float)ps.a / 255);
        pd.b = ((float)ps.b) * ((float)ps.a / 255);
        pd.a = ps.a;
      }
      else
        pd = ps;
    }
  }

  return res;
}

static int cmp_area(const IPoint2 &a, const IPoint2 &b)
{
  int diff = a.x * a.y - b.x * b.y;
  if (!diff)
    diff = a.y - b.y;
  if (!diff)
    diff = a.x - b.x;
  return diff;
}

static int cmp_vert(const IPoint2 &a, const IPoint2 &b)
{
  int diff = a.y - b.y;
  if (!diff)
    diff = a.x - b.x;
  return diff;
}

static int cmp_horz(const IPoint2 &a, const IPoint2 &b)
{
  int diff = a.x - b.x;
  if (!diff)
    diff = a.y - b.y;
  return diff;
}

void sortTextures(const DataBlock &blk, Tab<String> &names, int width, int height)
{
  TexImage32 *img;
  Tab<IPoint2> size(tmpmem);

  if (!blk.paramCount())
    error("empty input block!");

  OAHashNameMap<true> *includeNames = NULL;
  if (const DataBlock *b = blk.getBlockByName("includePics"))
  {
    includeNames = new OAHashNameMap<true>;
    for (int i = 0, nid = b->getNameId("pic"); i < b->paramCount(); i++)
      if (b->getParamNameId(i) == nid && b->getParamType(i) == blk.TYPE_STRING)
        includeNames->addNameId(b->getStr(i));
    if (!dgs_execute_quiet)
      printf("includePics list set: %d names\n", includeNames->nameCount());
  }

  int nid = blk.getNameId("dir");
  int skipped_pic_count = 0;
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == blk.TYPE_STRING)
    {
      String directory(blk.getStr(i));
      if (!dd_dir_exist(directory))
      {
        printf("source icons folder <%s> doesn't exist", directory.str());
        exit(13);
      }

      Tab<SimpleString> files;
      if (!find_files_in_folder(files, directory, "", false, true, false))
        error("There are no textures of necessary format in input directory");
      for (auto &fn : files)
      {
        if (includeNames)
        {
          char *p = strchr(fn.str(), '.');
          if (p)
            *p = '\0';
          if (includeNames->getNameId(dd_get_fname(fn)) < 0)
          {
            skipped_pic_count++;
            continue;
          }
          if (p)
            *p = '.';
        }
        names.push_back() = fn;
      }
    }

  nid = blk.getNameId("file");
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == blk.TYPE_STRING)
      names.push_back() = blk.getStr(i);

  del_it(includeNames);
  nid = blk.getNameId("exclude");

  int excluded_pic_count = 0;
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == blk.TYPE_STRING)
    {
      const char *s = blk.getStr(i);
      for (int j = names.size() - 1; j >= 0; j--)
        if (stricmp(names[j], s) == 0)
        {
          erase_items(names, j, 1);
          excluded_pic_count++;
        }
    }

  if (!dgs_execute_quiet || !names.size())
    printf("gathered %d pics, while skipped %d (due to includePics{}) and %d (due to exclude{})\n", names.size(), skipped_pic_count,
      excluded_pic_count);

  for (int i = 0; i < names.size(); i++)
  {
    debug("enum file: %s", names[i].str());
    img = load_image(names[i], tmpmem);
    if (!img)
    {
      printf("texture %s%s", names[i].str(), " has wrong format\n");
      erase_items(names, i, 1);
      i--;
      continue;
    }

    if (img->w > width || img->h > height)
      printf("Size of %s%s", names[i].str(), " is more then size of output texture\n");
    size.push_back(IPoint2(img->w, img->h));
    memfree(img, tmpmem);
  }

  if (!names.size())
    error("no pictures found!");

  debug("%d files found", names.size());
  int (*cmp)(const IPoint2 &a, const IPoint2 &b) = NULL;
  const char *s_heur = blk.getStr("sort", "area");
  if (strcmp(s_heur, "area") == 0)
    cmp = &cmp_area;
  else if (strcmp(s_heur, "vert") == 0)
    cmp = &cmp_vert;
  else if (strcmp(s_heur, "horz") == 0)
    cmp = &cmp_horz;
  else
  {
    printf("Warning: unknown sort heuristic <%s>, using <area>\n", s_heur);
    cmp = &cmp_area;
  }

  for (int i = 0; i < (int)names.size() - 1; i++)
    for (int j = i + 1; j < names.size(); j++)
      if (cmp(size[i], size[j]) < 0)
      {
        String a(names[i]);
        names[i] = names[j];
        names[j] = a;
        IPoint2 b = size[i];
        size[i] = size[j];
        size[j] = b;
      }

  // debug_flush(true);
}

static void printTitle()
{
  printf("GUI image atlas creator, v2.32\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n\n");
}

int DagorWinMain(bool debugmode)
{
  ::register_tga_tex_load_factory();
  ::register_jpeg_tex_load_factory();
  ::register_psd_tex_load_factory();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();

  if (dgs_argc <= 1)
  {
    printTitle();

    printf("Packing:\n"
           "    GuiTex-dev.exe <config.blk> [options]\n");
    printf("  where\n");
    printf("    <config.blk>  name of blk configuration file\n");
    printf("  options are:\n");
    printf("    -q            enable quiet mode\n");
    printf("    -odir:dir     use dir to override Output{outdir:t param\n");
    printf("    -cache:dir    use specified dir for cache (to be specified AFTER -odir: )\n");
    printf("    -oblkdir:dir  use dir to prefix Output{coord:t param (instead of oudir)\n");
    printf("    -ddsx:target  save atlas textures as DDSx, target=PC, X360, PS3, etc.\n");
    printf("    -mkbundle     writes single (BLK+DDSx) file in combined format to output dir\n\n");
    printf("    -mkref        writes texture reference TGA image\n\n");
    printf("  BLK configuration file sample:\n");
    printf("    conva:b=false\t//premultiplying alpha \n");
    printf("    mkbundle:b=false\t//overrides -mkbundle option\n");
    printf("    Input {\n");
    printf("      dir:t=hud\\icons\t//path to folder with source icons to atlas\n");
    printf("      sort:t=area\t//sort heuristic: area (default), vert, horz\n");
    printf("    }\n");
    printf("    Output {\n");
    printf("      width:r=1024\t//file output width\n");
    printf("      height:r=1024\t//file output heigh\n");
    printf("      //width_PC:r=1024\t//optional width override for target\n");
    printf("      //width_X360:r=1024\n");
    printf("      //height_PC:r=1024\t//optional height override for target\n\n");
    printf("      //height_X360:r=992\n");
    printf("      outdir:t=.\t//path to dir where texture/blk will be created (.=default)\n");
    printf("      texture:t=gameuiskin\t//name of DDS file with result atlas image output\n");
    printf("      coord:t=gameuiskin\t//name of atlas BLK file (# perfix means abs path)\n");
    printf("      format:t=rgba\t//output image format: rgba (default), dxt3, dxt5\n");
    printf("      mips:i=1\t\t//number of mipmaps to generate: 0=all, 1 (default), ...\n");
    printf("      autocrop:t=auto\t//autocrop. How to crop texture if used space is less than height, multiple of: auto (default), "
           "4^mips, 4, 16, 32, pow2\n");
    printf("    }\n");
    printf("\n");
    printf("Unpacking: \n    GuiTex-dev.exe <guiskin.blk> -unpack:<dir> [-nostripes]\n");
    printf("  where\n");
    printf("    <guiskin.blk>\tname of blk file created by GUI image atlas creator\n");
    printf("    <dir>\t\tfolder for separated files\n");
    exit(0);
  }

  String textureName, coordblk;
  short width, height;
  if (!dd_file_exist(dgs_argv[1]))
    error("config file doesn't exist");

  bool noStripes = false, quietMode = false, mkref = false;
  unsigned ddsx_target = 0;
  const char *unpackDir = NULL;
  const char *cmd_outdir = NULL, *cmd_outblk = NULL;
  const char *cmd_copyto = NULL;

  // get flags commandline flags
  for (int i = 2; i < dgs_argc; i++)
  {
    if (stricmp(dgs_argv[i], "-conva") == 0)
      premultAlpha = true;
    else if (strnicmp(dgs_argv[i], "-unpack:", strlen("-unpack:")) == 0)
      unpackDir = dgs_argv[i] + strlen("-unpack:"); // unpack texture
    else if (stricmp(dgs_argv[i], "-nostripes") == 0)
      noStripes = true;
    else if (stricmp(dgs_argv[i], "-q") == 0)
      quietMode = true;
    else if (stricmp(dgs_argv[i], "-mkbundle") == 0)
      __mkbundle = true;
    else if (stricmp(dgs_argv[i], "-mkref") == 0)
      mkref = true;
    else if (strnicmp(dgs_argv[i], "-ddsx:", 6) == 0)
    {
      for (int j = 6; j < 10; j++)
        if (dgs_argv[i][j] == '\0')
          break;
        else
          ddsx_target = (ddsx_target >> 8) | (dgs_argv[i][j] << 24);
    }
    else if (strnicmp(dgs_argv[i], "-odir:", 6) == 0)
      cmd_outdir = dgs_argv[i] + 6;
    else if (strnicmp(dgs_argv[i], "-cache:", 7) == 0)
    {
      if (!cmd_outdir)
      {
        printTitle();
        printf("ERR: -cache specified before (or without) -odir\n");
        return 1;
      }
      cmd_copyto = cmd_outdir;
      cmd_outdir = dgs_argv[i] + 7;
      dd_mkdir(cmd_copyto);
    }
    else if (strnicmp(dgs_argv[i], "-oblkdir:", 9) == 0)
      cmd_outblk = dgs_argv[i] + 9;
  }

  if (!quietMode)
    printTitle();

  if (unpackDir)
    return unpackTex(dgs_argv[1], unpackDir, noStripes);

  // if ( !quietMode && ddsx_target )
  //   printf ( "convert to DDSx, target: %c%c%c%c\n", _DUMP4C(ddsx_target));

  DataBlock d(dgs_argv[1]);
  uint64_t tc_storage = 0;
  const char *ddsx_tc_str = mkbindump::get_target_str(ddsx_target, tc_storage);

  premultAlpha |= d.getBool("conva", false);
  __mkbundle = d.getBool("mkbundle", __mkbundle);

  DataBlock *dd = d.getBlockByName("Input");
  if (dd == NULL)
    error("input block wasn't found");
  dd = d.getBlockByName("Output");
  if (dd == NULL)
    error("output block wasn't found");
  width = dd->getReal(String::mk_str_cat("width_", ddsx_tc_str), dd->getReal("width", 0));
  if (width <= 0)
    error("width wasn't found or <=0");
  height = dd->getReal(String::mk_str_cat("height_", ddsx_tc_str), dd->getReal("height", 0));
  if (height <= 0)
    error("height wasn't found or <=0");
  String fname_base(dd_get_fname(dgs_argv[1]));
  remove_trailing_string(fname_base, ".tiles.blk");

  textureName = dd->getStr("texture", "def_atlas");
  if (ddsx_target)
    textureName += ".ddsx";
  else
  {
    if (strstr(textureName, ".tga") == NULL)
      textureName += ".dds";
    else if (strcmp(strstr(textureName, ".tga"), ".tga"))
      textureName += ".dds";
  }

  coordblk = dd->getStr("coord", get_file_name_wo_ext(textureName));
  if (__mkbundle)
    coordblk += ".ta.bin";
  else if (strstr(coordblk, ".blk") == NULL)
    coordblk += ".blk";
  else if (strcmp(strstr(coordblk, ".blk"), ".blk"))
    coordblk += ".blk";

  __width = width;
  __height = height;

  dgs_execute_quiet = __quietMode = quietMode;
  __mkref = mkref;
  __ddsx_target = ddsx_target;
  if (__mkbundle && !__ddsx_target)
    __mkbundle = false;
  __cmd_outdir = cmd_outdir;
  __cmd_outblk = cmd_outblk;

  Tab<String> texNames(tmpmem), outNames(tmpmem);
  sortTextures(*d.getBlockByNameEx("Input"), texNames, width, height);

  String target_fname(0, "%s/%s", __cmd_outdir ? __cmd_outdir : d.getBlockByNameEx("Output")->getStr("outdir", "."), coordblk);
  String cache_fname(0, "%s.%s.c4.bin", cmd_copyto ? target_fname : dgs_argv[1], ddsx_tc_str);
  GenericBuildCache c4;
  bool uptodate = true;
  if (__mkbundle)
  {
    if (!c4.load(cache_fname) || c4.checkTargetFileChanged(target_fname))
    {
      uptodate = false;
      if (dd_file_exist(cache_fname))
      {
        logwarn("removed outdated cache: %s", cache_fname.str());
        unlink(cache_fname);
      }
    }

    if (c4.checkFileChanged(dgs_argv[0])) //== depend on EXE revision
      uptodate = false;

    DataBlock srcBlk;
    srcBlk = d;
    for (int i = 0; i < texNames.size(); i++)
      srcBlk.addStr("_pic", texNames[i]);

    if (c4.checkDataBlockChanged(dgs_argv[1], srcBlk))
      uptodate = false;
    for (int i = 0; i < texNames.size(); i++)
      if (c4.checkFileChanged(texNames[i]))
        uptodate = false;

    if (uptodate)
    {
      if (c4.isTimeChanged())
        c4.save(cache_fname);
      printf("up-to-date\n");
      if (cmd_copyto)
        return dag_copy_file(target_fname, String(0, "%s/%s", cmd_copyto, dd_get_fname(target_fname))) ? 0 : -1;
      return 0;
    }
  }

  int texNum = 0;
  String texName(textureName);
  String blkName(coordblk);
  while (texNames.size())
  {
    makeAtlas(texNames, outNames, width, height, texName, blkName,
      d.getBlockByNameEx("Output")->getInt(String::mk_str_cat("margin_", ddsx_tc_str),
        d.getBlockByNameEx("Output")->getInt("margin", 2)),
      d.getBlockByNameEx("Output")->getBool("mayFlip", false));
    texNames = eastl::move(outNames);

    if (texNames.size())
    {
      texNum++;

      texName = textureName;
      String texExt(dd_get_fname_ext(texName));
      remove_trailing_string(texName, texExt);
      texName.aprintf(260, "_%02d%s", texNum, texExt.str());

      blkName = coordblk;
      remove_trailing_string(blkName, ".blk");
      blkName.aprintf(260, "_%02d%s", texNum, ".blk");
    }
  }

  if (__mkbundle)
  {
    c4.setTargetFileHash(target_fname);
    c4.save(cache_fname);
    if (cmd_copyto)
      return dag_copy_file(target_fname, String(0, "%s/%s", cmd_copyto, dd_get_fname(target_fname))) ? 0 : -1;
  }
  return 0;
}

int makeAtlas(dag::ConstSpan<String> texNames, Tab<String> &outNames, short width, short height, const char *textureName,
  const char *coordblk, int MARGIN, bool may_flip)
{
  int i;
  TexImage32 *imIn, *imOut;
  real S = 0; // Total square of putting textures (for quality estimate)

  map = new char[width * height];
  memset(map, 0, width * height);
  __width32 = (width + 31) & (~31);
  occupied_map = (uint32_t *)memalloc(4 * (__width32 >> 5) * height);
  memset(occupied_map, 0, (__width32 >> 5) * height * 4);

  String saveTextureName(textureName);
  imOut = TexImage32::create(width, height, tmpmem);
  memset(imOut->getPixels(), 0, width * height * sizeof(*imOut->getPixels()));
  texOut = imOut;


  DataBlock d;
  DataBlock *dd = d.addNewBlock("tex");
  if (!__mkbundle)
  {
    String name(textureName);
    dd->addStr("name", dd_strlwr(name));
  }

  int usedH = 0;
  OAHashNameMap<true> nmap;
  for (i = 0; i < texNames.size(); i++)
  {
    if (!__quietMode)
      printf("processing texture '%s'\n", texNames[i].str());

    bool hs = false, vs = false, ss = false;
    imIn = load_image(texNames[i], tmpmem);
    // TexPixel32 *imgIn = (TexPixel32 *)(imIn+1);
    if (!__quietMode)
      printf("loaded file %d x %d - looking for place\n", imIn->w, imIn->h);

    debug("placing %s", texNames[i].str());

    IPoint2 lt_ofs, vis_sz;
    cropTex(imIn, lt_ofs, vis_sz);
    int transf_code = -1;
    IPoint2 place = placeTex(imIn, lt_ofs, vis_sz, MARGIN, transf_code, may_flip);
    if (place.x == -1) // no place on this texture
    {
      printf("Not enough space for texture %s\n", texNames[i].str());

      d.saveToTextFile(coordblk);
      save_tga32(saveTextureName + ".tga", imOut);

      char buf[80];
      SNPRINTF(buf, sizeof(buf), "Not enough space in atlas '%s'.", textureName);
      error(buf);
    }
    if (usedH < place.y + imIn->h + 1)
      usedH = place.y + imIn->h + 1;


    S += real(imIn->w) * imIn->h;

    DataBlock *ddd;
    String s(dd_get_fname(texNames[i]));

    s[int(strlen(s) - strlen(dd_get_fname_ext(s)))] = 0; // cut extention
    s.resize(strlen(s) + 1);

    char *st, *sss;

    if (!(st = strstr(s, "@@hs")))
      if (!(st = strstr(s, "@@vs")))
        st = strstr(s, "@@ss");

    if (st)
    {
      switch (st[2])
      {
        case 'h': hs = true; break;
        case 'v': vs = true; break;
        case 's': ss = true; break;
      }
      memmove(st, st + 4, strlen(st) - 3);
    }
    if ((st = strstr(s, "@@")) != nullptr)
    {
      ddd = dd->addNewBlock("stripe");
      st[0] = 0;
      ddd->addStr("prefix", s);
      st += 2;
      sss = strstr(st, "@@");
      st[2] = 0;
      int count = 1;
      if (atoi(st) > 0)
        count = atoi(st);
      G_ASSERT(count > 0);

      if (sss)
      {
        sss += 2;
        ddd->addInt("base", atoi(sss));
      }
      ddd->addInt("count", count);
      ddd->addReal("left", place.x + (hs || ss));
      ddd->addReal("top", place.y + (vs || ss));
      ddd->addReal("wd", (imIn->w - (hs || ss) * 2) / count);
      ddd->addReal("ht", imIn->h - (vs || ss) * 2);
    }
    else
    {
      dd_strlwr(s);
      int ncnt = nmap.nameCount();
      if (nmap.addNameId(s) < ncnt)
      {
        printf("duplicate picture: %s (%d %d)", s.str(), nmap.addNameId(s), nmap.nameCount());
        exit(13);
      }

      ddd = dd->addNewBlock("pic");
      ddd->addStr("name", s);
      ddd->addReal("left", place.x + (hs || ss));
      ddd->addReal("top", place.y + (vs || ss));
      ddd->addReal("wd", imIn->w - (hs || ss) * 2);
      ddd->addReal("ht", imIn->h - (vs || ss) * 2);
      if (transf_code)
        ddd->setInt("transf", transf_code);
    }
    memfree(imIn, tmpmem);
    if (!__quietMode)
      printf("%d of %d textures were passed\n", i + 1, texNames.size());
  }

  if (!__quietMode)
    printf("\nAtlas %dx%d: used %d%% of texture area (%d%% of vertical extent used)\n", imOut->w, imOut->h,
      (int)(S / (imOut->w * imOut->h) * 100), usedH * 100 / imOut->h);

  {
    if (__ddsx_target)
    {
      char startdir[260];
      dag_get_appmodule_dir(startdir, sizeof(startdir));
      int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", startdir));
      if (!__quietMode && !pc)
        printf("loaded %d DDSx export plugin(s) (%s)\n", pc, startdir);
    }

    DataBlock cfg(dgs_argv[1]);
    DataBlock &out = *cfg.getBlockByName("Output");

    const char *crop_opt = out.getStr("autocrop", "auto");
    int mips = out.getInt("mips", 1);
    uint64_t tc_storage = 0;
    const char *fmt =
      out.getStr(String::mk_str_cat("format_", mkbindump::get_target_str(__ddsx_target, tc_storage)), out.getStr("format", "rgba"));

    if (strcmp(crop_opt, "auto") == 0)
    {
      if (strstr(fmt, "dxt"))
        crop_opt = (mips == 1) ? "4" : "4^mips";
      else
        crop_opt = "4";
    }

    int totalH = imOut->h;
    if (!crop_opt)
      ;
    else if (strcmp(crop_opt, "pow2") == 0)
    {
      while (imOut->h / 2 > usedH)
        imOut->h /= 2;
    }
    else if (strcmp(crop_opt, "4^mips") == 0)
    {
      int align = 4;
      for (int i = 1; i < mips; i++)
        align *= 2;
      imOut->h = (usedH + align - 1) & ~(align - 1);
    }
    else if (strcmp(crop_opt, "32") == 0)
      imOut->h = (usedH + 31) & ~31;
    else if (strcmp(crop_opt, "16") == 0)
      imOut->h = (usedH + 15) & ~15;
    else if (strcmp(crop_opt, "4") == 0)
      imOut->h = (usedH + 3) & ~3;
    else if (!*crop_opt)
      ;
    else
      printf("ERR: unsupported autocrop=%s", crop_opt);

    if (imOut->h != totalH)
    {
      float min_err_y = 0, max_err_y = 0;
      for (int i = 0, nid = dd->getNameId("pic"); i < dd->blockCount(); i++)
      {
        int y0 = dd->getBlock(i)->getReal("top", 0);
        int y1 = y0 + dd->getBlock(i)->getReal("ht", 0);
        float tc0y = float(y0) / float(imOut->h);
        float tc1y = float(y1) / float(imOut->h);

        inplace_min(min_err_y, tc0y * imOut->h - y0);
        inplace_min(min_err_y, tc1y * imOut->h - y1);
        inplace_max(max_err_y, tc0y * imOut->h - y0);
        inplace_max(max_err_y, tc1y * imOut->h - y1);
      }
      if (!__quietMode || min_err_y < -1e-2 || max_err_y > 1e-2)
        printf("Cropped texture to %dx%d (%d scanlines used), TC inaccuracy [%g .. %g] pix\n", imOut->w, imOut->h, usedH, min_err_y,
          max_err_y);
    }

    dd->addReal("wd", width);
    dd->addReal("ht", imOut->h);

    String outdir(__cmd_outdir ? __cmd_outdir : out.getStr("outdir", "."));
    String path;

    if (coordblk[0] == '#')
      path = coordblk;
    else
      path.printf(260, "%s/%s", (__cmd_outblk && !__mkbundle) ? __cmd_outblk : outdir.str(), coordblk);

    dd_mkpath(path);
    dd_strlwr(path);

    if (!__mkbundle)
    {
      d.saveToTextFile(path);

      path = outdir + "/" + saveTextureName;
      dd_mkpath(path);
    }

    int tex_mem_sz = 0, tex_file_sz = 0;
    {
      FullFileSaveCB cwr(path);
      if (!cwr.fileHandle)
        error("failed to write output file");

      if (__mkbundle)
      {
        cwr.writeInt(_MAKE4C('TA.'));
        cwr.beginBlock();
        d.saveToStream(cwr);
        if (int pos = cwr.tell() % 16)
        {
          int zero[4] = {0, 0, 0, 0};
          cwr.write(zero, 16 - pos);
        }
        cwr.endBlock();
      }
      if (write_atlas_dds(cwr, imOut, out, __ddsx_target, tex_mem_sz, tex_file_sz, __quietMode))
      {
        cwr.close();
        if (__mkref)
          save_tga32(get_file_name_wo_ext(textureName) + ".ref.tga", imOut);
        if (!__quietMode)
          printf("\nTexture size is %dK (filesize is %dK)\n", tex_mem_sz >> 10, tex_file_sz >> 10);
        fflush(stdout);
      }
    }
  }

  memfree(imOut, tmpmem);

  delete[] map;
  memfree_anywhere(occupied_map);

  return 0;
}
