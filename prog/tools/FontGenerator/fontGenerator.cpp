#include <windows.h>
#include "fontFileFormat.h"
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <io.h>
#include <libTools/util/genericCache.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/fileUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include "freeTypeFont.h"
#include <image/dag_texPixel.h>
#include <generic/dag_qsort.h>
#include <image/dag_tga.h>
#include <image/dag_loadImage.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_unicode.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <math/dag_color.h>
#include <util/dag_hierBitArray.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_cropMultiLineStr.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include "mutexAA.h"

enum
{
  TEX_LAYOUT_NUM = 10,
  TEX_SIDE_BASE = 4,
  TEX_MAX_SIZE = (TEX_SIDE_BASE << (TEX_LAYOUT_NUM))
};

struct GlyphImage
{
  short wd, ht;
  short dyn, wcharIdx;
  uint8_t *im;
  TexPixel32 *im32;

  GlyphImage()
  {
    im = NULL;
    im32 = NULL;
    wd = ht = 0;
    dyn = -1;
    wcharIdx = 0;
  }
  ~GlyphImage()
  {
    if (im)
      memfree(im, tmpmem);
    if (im32)
      memfree(im32, tmpmem);
  }
  void blend_pixel_32(uint8_t *rg, int s_index, int d_index, E3DCOLOR border_color)
  {
    if (rg[s_index])
    {
      float d_alpha = rg[s_index] / 255.0;
      im32[d_index].r = im32[d_index].r * (1.0 - d_alpha) + border_color.r * d_alpha;
      im32[d_index].g = im32[d_index].g * (1.0 - d_alpha) + border_color.g * d_alpha;
      im32[d_index].b = im32[d_index].b * (1.0 - d_alpha) + border_color.b * d_alpha;
      im32[d_index].a = im32[d_index].a * (1.0 - d_alpha) + rg[s_index] * d_alpha;
    }
  }
  void set(uint8_t *rg, int w, int h, int border, E3DCOLOR border_color, bool use_color, E3DCOLOR font_color)
  {
    if (!border && !use_color)
    {
      im = (uint8_t *)memalloc(w * h, tmpmem);
      memcpy(im, rg, w * h);
      wd = w;
      ht = h;
      return;
    }

    if (border)
    {
      wd = w + border * 2;
      ht = h + border * 2;
      im32 = (TexPixel32 *)memalloc(wd * ht * 4, tmpmem);
      memset((void *)im32, 0, wd * ht * 4);

      for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
        {
          int s_index = w * j + i;
          const int IND_COUNT = 4;
          int d_indixes[IND_COUNT];
          d_indixes[0] = wd * j + i;
          d_indixes[1] = wd * j + i + 2 * border;
          d_indixes[2] = wd * (j + 2 * border) + i;
          d_indixes[3] = wd * (j + 2 * border) + i + 2 * border;

          for (int k = 0; k < IND_COUNT; ++k)
            blend_pixel_32(rg, s_index, d_indixes[k], border_color);
        }
    }
    else
    {
      wd = w;
      ht = h;
      im32 = (TexPixel32 *)memalloc(wd * ht * 4, tmpmem);
      memset((void *)im32, 0, wd * ht * 4);
    }

    for (int j = 0; j < h; ++j)
      for (int i = 0; i < w; ++i)
      {
        int s_index = w * j + i;
        int d_index = wd * (j + border) + i + border;
        E3DCOLOR _color = use_color ? font_color : E3DCOLOR(rg[s_index], rg[s_index], rg[s_index], 0);
        blend_pixel_32(rg, s_index, d_index, _color);
      }
  }
  void set32(TexPixel32 *rg, int w, int h)
  {
    im32 = (TexPixel32 *)memalloc(w * h * 4, tmpmem);
    memcpy(im32, rg, w * h * 4);
    wd = w;
    ht = h;
  }
  void markDyn(int dyn_grp, int wchar_idx, bool remove_pregen)
  {
    dyn = dyn_grp;
    wcharIdx = wchar_idx;
    if (!remove_pregen)
      return;
    if (im)
      memfree(im, tmpmem);
    im = NULL;
    if (im32)
      memfree(im32, tmpmem);
    im32 = NULL;
  }
  bool hasImage() const { return im || im32; }
};
DAG_DECLARE_RELOCATABLE(GlyphImage);

struct GlyphOffset
{
  int font_index;
  int index;
};

class Tabint : public Tab<int>
{
public:
  Tabint() : Tab<int>(tmpmem) {}
};

static Tab<Tab<GlyphImage>> glimg(tmpmem_ptr());

// static int glimg_index = 0;


struct GlyphOffsetCmp
{
  static int compare(const GlyphOffset &a, const GlyphOffset &b)
  {
    int c = glimg[b.font_index][b.index].ht - glimg[a.font_index][a.index].ht;
    if (!c)
    {
      c = glimg[b.font_index][b.index].wd - glimg[a.font_index][a.index].wd;
      if (!c)
      {
        c = a.font_index - b.font_index;
        return c ? c : a.index - b.index;
      }
      return c;
    }
    return c;
  }
};

typedef HierConstSizeBitArray<8, ConstSizeBitArray<8>> unicode_range_t;
static int count_bits(const unicode_range_t &r)
{
  int cnt = 0;
  for (int i = 1, inc = 1; i < r.FULL_SZ; i += inc)
    if (r.getIter(i, inc))
      cnt++;
  return cnt;
};
static FastNameMap dynTTFUsed;
static const DataBlock *sizeRestrBlk = NULL;
static bool global_generateMissingGlyphBox = true;

struct FontInfo
{
  const char *file;
  String name;
  const char *atlas;
  const char *cpfile;
  const char *prefix;
  String defName;
  double size;
  bool monochrome;
  bool forceAutoHinting, checkExtents;
  Tab<int> hRes;
  dag::Vector<FontInfo> additional;
  Tab<unicode_range_t> monoWdGrp;
  FastNameMap extFilesUsed;
  unicode_range_t include, exclude;
  unicode_range_t dyn;
  bool storeDynGrp;
  bool includeCp, symbCharMap, remapPrivate;
  bool makeHtEven;
  bool symmEL;
  bool generateMissingGlyphBox;
  int charAsSpace;
  float spaceWidthMul;
  int border;
  int valve_upscale;
  E3DCOLOR border_color, font_color;
  bool caseSensitive, useFontColor;

  struct RasterGlyph
  {
    wchar_t code;
    TexImage32 *img;
    int l, t, r, b, dx, mx;
  };
  OAHashNameMap<true> rasterImgNames;
  Tab<TexImage32 *> rasterImg;
  Tab<RasterGlyph> rasterGlyphs;
  int rasterAscender, rasterDescender, rasterHt, rasterLineGap, rasterHorMargin, rasterBaseline;
  int rasterSpaceWidth;
  wchar_t rasterMissingCode;
  bool rgba32;

  bool wasCompiled;
  int extIdxRemap = -1;

  FontInfo() :
    hRes(tmpmem),
    includeCp(true),
    symbCharMap(false),
    remapPrivate(false),
    wasCompiled(false),
    rasterImg(tmpmem),
    rasterGlyphs(tmpmem),
    rgba32(false),
    border(0),
    border_color(0),
    caseSensitive(true),
    monoWdGrp(tmpmem),
    font_color(0xFFFFFFFF),
    useFontColor(false),
    rasterMissingCode(0),
    rasterSpaceWidth(0),
    valve_upscale(1),
    storeDynGrp(false)
  {}
  ~FontInfo()
  {
    clear_and_shrink(hRes);
    clear_all_ptr_items(rasterImg);
  }

  bool load(const DataBlock &blk, int i, const char *def_prefix, const char *def_cp, const char *def_atlas, float def_sz,
    int def_valve_upscale, bool symm_el, float size_multiplier = 1.0, int size_pixels_by_screenheight = 0, const char *suffix = "")
  {
    defName.printf(11, "%d", i);
    name.printf(0, "%s%s", blk.getStr("name", defName), suffix);
    atlas = blk.getStr("atlas", def_atlas ? def_atlas : name);
    valve_upscale = blk.getInt("vUpscale", def_valve_upscale);
    symmEL = blk.getBool("symmetricExtLeading", symm_el);
    generateMissingGlyphBox = blk.getBool("generateMissingGlyphBox", global_generateMissingGlyphBox);

    if (!::dgs_execute_quiet)
      printf("load atlas = %s\n", atlas);

    rgba32 = blk.getBool("colorFont", false);
    font_color = blk.getE3dcolor("font_color", 0xFFFFFFFF);
    border = blk.getInt("border", 0);
    border_color = blk.getE3dcolor("border_color", E3DCOLOR(0));
    caseSensitive = blk.getBool("caseSensitive", true);
    useFontColor = blk.getBool("useFontColor", false);
    prefix = blk.getStr("prefix", def_prefix);
    cpfile = blk.getStr("codepage", def_cp);
    symbCharMap = blk.getBool("symbCharMap", false);
    remapPrivate = blk.getBool("remapPrivate", false);
    file = blk.getStr("file", NULL);

    if (blk.getStr("subset", NULL))
    {
      printf("[SKIP] %s/%s subset format is changed!\n", file, name.c_str());
      return false;
    }

    charAsSpace = blk.getInt("charAsSpace", 0);
    if (!charAsSpace)
      charAsSpace = blk.getStr("charAsSpace", " ")[0];
    if (!charAsSpace)
      charAsSpace = ' ';

    spaceWidthMul = blk.getReal("spaceWidthMul", 1.0);
    makeHtEven = blk.getBool("makeHtEven", false);

    if (blk.getBool("ascii", false))
      cpfile = NULL;

    bool has_size_percents = blk.paramExists("size_percents") && (size_pixels_by_screenheight <= 0);
    bool has_size_pixels = blk.paramExists("size_pixels") || (blk.paramExists("size_percents") && size_pixels_by_screenheight > 0);
    if (!def_sz && !has_size_percents && !has_size_pixels && !blk.getBlockByName("rasterFont"))
    {
      printf("[SKIP] %s/%s neither size_percents:r nor size_pixels:r specified\n", file, name.c_str());
      return false;
    }
    if (!def_sz && has_size_percents && has_size_pixels)
    {
      printf("[SKIP] %s/%s both size_percents:r and size_pixels:r specified\n", file, name.c_str());
      return false;
    }

    if (!::dgs_execute_quiet)
      debug("font name = %s, def_sz = %f, size_multiplier = %f, size_percents = %f, size_pixels =  %f, size_pixels_by_screen = %d",
        name, def_sz, size_multiplier, blk.getReal("size_percents", -1), blk.getReal("size_pixels", -1), size_pixels_by_screenheight);
    if (has_size_percents)
    {
      size = blk.getReal("size_percents", 0) * size_multiplier;
      if (!::dgs_execute_quiet)
        debug("result percents_size = %f", size);

      if (size <= 0)
      {
        printf("[SKIP] %s/%s invalid size_percents:r=%.1f\n", file, name.str(), size);
        return false;
      }
    }
    else if (has_size_pixels)
    {
      size = blk.paramExists("size_pixels")
               ? blk.getReal("size_pixels", 0)
               : floor(0.5 + 0.01 * float(blk.getReal("size_percents", 0)) * size_multiplier * size_pixels_by_screenheight);

      if (!::dgs_execute_quiet)
        debug("result pixel_size = %f", size);
      if (size <= 0)
      {
        printf("[SKIP] %s/%s invalid size_pixels:r=%.1f\n", file, name.str(), size);
        return false;
      }
      size = -size;
    }
    else
      size = def_sz;

    if (!::dgs_execute_quiet)
      debug("result size = %f", size);

    monochrome = blk.getBool("bw", false);
    forceAutoHinting = blk.getBool("forceAutoHinting", true);
    checkExtents = blk.getBool("checkExtents", false);

    if (size < 0)
      clear_and_shrink(hRes);
    else
    {
      const DataBlock *resblk = blk.getBlockByName("hRes_ex");
      if (resblk)
        clear_and_shrink(hRes);
      else
        resblk = blk.getBlockByName("hRes");

      if (resblk)
        addHRes(*resblk);
    }

    const DataBlock *incblk = blk.getBlockByName("include_ex");
    if (incblk)
    {
      include.reset();
      includeCp = true;
    }
    else
      incblk = blk.getBlockByName("include");

    const DataBlock *excblk = blk.getBlockByName("exclude_ex");
    if (excblk)
      exclude.reset();
    else
      excblk = blk.getBlockByName("exclude");

    if (incblk)
      makeInclusions(*incblk);
    if (excblk)
      makeExclusions(*excblk);

    int nid = blk.getNameId("monoWidthGroup");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        bool icp = false;
        makeUnicodeRange(*blk.getBlock(i), monoWdGrp.push_back(), icp, *this);
      }

    if (blk.getBlockByName("rasterFont"))
      return loadRasterFont(blk);
    return true;
  }

  bool loadRasterFont(const DataBlock &blk)
  {
    int nid = blk.getNameId("rasterFont");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        const DataBlock &b = *blk.getBlock(i);
        const char *img_fn = b.getStr("img", blk.getStr("img", ""));
        int texId = rasterImgNames.addNameId(img_fn);
        TexImage32 *im = NULL;

        if (texId < 0)
        {
          printf("ERR: bad img for raster font: <%s>\n", img_fn);
          return false;
        }

        if (texId >= rasterImg.size())
        {
          im = load_image(img_fn, tmpmem);
          if (!im)
          {
            printf("ERR: failed to load raster font img <%s>\n", img_fn);
            return false;
          }
          G_ASSERT(texId == rasterImg.size());
          rasterImg.push_back(im);
        }
        else
          im = rasterImg[texId];

        const char *symbols_str = b.getStr("symbols", NULL);

        if (!symbols_str)
        {
          // read settings in form of bmfont
          int nid2 = blk.getNameId("char");
          for (int j = 0; j < b.paramCount(); j++)
            if (b.getParamNameId(j) == nid2)
            {
              int id, x, y, w, h, xo, yo, dx, pg, tmp;
              if (sscanf(b.getStr(j),
                    "id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d "
                    "xadvance=%d page=%d chnl=%d",
                    &id, &x, &y, &w, &h, &xo, &yo, &dx, &pg, &tmp) != 10)
              {
                printf("ERR: bad syntax in char=<%s>", b.getStr(j));
                continue;
              }

              RasterGlyph &rg = rasterGlyphs.push_back();
              rg.code = id;
              rg.img = im;
              rg.l = x - xo;
              rg.t = y - yo;
              rg.r = x + w;
              rg.b = y + h;
              rg.dx = dx;
              rg.mx = 0;
              if (id == ' ')
                rasterSpaceWidth = dx;
            }
          continue;
        }

        IPoint2 ofs = b.getIPoint2("ofs", blk.getIPoint2("ofs", IPoint2(0, 0)));
        IPoint2 step = b.getIPoint2("step", blk.getIPoint2("step", IPoint2(1, 1)));
        int cpr = b.getInt("cellsPerRow", blk.getInt("cellsPerRow", 1));
        int def_dx = b.getBool("autoCanvaSz", blk.getBool("autoCanvaSz", false)) ? -2 : -1;
        int mod_dx = b.getInt("widthMod", blk.getInt("widthMod", 0));

        if (cpr <= 0)
        {
          printf("ERR: bad cellsPerRow=%d\n", cpr);
          return false;
        }

        wchar_t *sym = convert_utf8_to_u16(symbols_str, -1);
        int sym_cnt = (int)wcslen(sym);
        rasterGlyphs.reserve(sym_cnt);
        for (int j = 0; j < sym_cnt; j++)
        {
          RasterGlyph &rg = rasterGlyphs.push_back();
          rg.code = sym[j];
          rg.img = im;
          rg.l = ofs.x + (j % cpr) * step.x;
          rg.t = ofs.y + (j / cpr) * step.y;
          rg.r = rg.l + step.x;
          rg.b = rg.t + step.y;
          rg.dx = def_dx;
          rg.mx = mod_dx;
        }

#if 0
        TexImage32 *im2 = TexImage32::create(im->w, im->h);
        memcpy(im2->getPixels(), im->getPixels(), im->w*im->h*4);
        for (int x = ofs.x; x < im->w; x += step.x)
          for (int j = 0; j < im->h; j ++)
            im2->getPixels()[j*im->w+x].c = 0xFF000000;
        for (int y = ofs.y; y < im->h; y += step.y)
          for (int j = 0; j < im->w; j ++)
            im2->getPixels()[y*im->w+j].c = 0xFF000000;
        save_tga32(String(128,"%s_%s_%d.tga", dd_get_fname(name), img_fn, i),im2);
        delete im2;
#endif
      }
    sort(rasterGlyphs, &cmp_glyph_code);
    rasterAscender = blk.getInt("ascender", 0);
    rasterDescender = blk.getInt("descender", 0);
    rasterHt = blk.getInt("height", rasterGlyphs.size() ? rasterGlyphs[0].b - rasterGlyphs[0].t : 0);
    rasterLineGap = blk.getInt("lineGap", rasterHt * 12 / 10);
    rasterHorMargin = blk.getInt("horMargin", 1);
    rasterBaseline = blk.getInt("baseline", 0);
    rasterMissingCode = blk.getInt("missingCode", blk.getStr("missingChar", "")[0]);
    rasterSpaceWidth = blk.getInt("spaceWidth", rasterSpaceWidth);

    return true;
  }
  static int cmp_glyph_code(const RasterGlyph *a, const RasterGlyph *b) { return a->code - b->code; }

  bool hasRasterSymbol(int c)
  {
    for (int i = 0; i < rasterGlyphs.size(); i++)
      if (rasterGlyphs[i].code == c)
        return true;
    return false;
  }

  bool rasterize(int c, TexPixel32 *dest, int &out_w, int &out_h, int &out_x0, int &out_y0, short &out_dx)
  {
    for (int i = 0; i < rasterGlyphs.size(); i++)
      if (rasterGlyphs[i].code == c)
      {
        RasterGlyph &rg = rasterGlyphs[i];
        TexPixel32 *p = rg.img->getPixels();
        int imgw = rg.img->w;

        if (rg.dx < -1)
          updateGlyphBounds(rg);
        out_w = rg.r - rg.l;
        out_h = rg.b - rg.t;
        out_x0 = 0;
        out_y0 = 0;
        out_dx = rg.dx;

        while (out_h > 0)
          if (isScanRowBlank(p + rg.l + out_x0 + (rg.t + out_y0) * imgw, out_w))
          {
            out_y0++;
            out_h--;
          }
          else if (isScanRowBlank(p + rg.l + out_x0 + (rg.t + out_y0 + out_h - 1) * imgw, out_w))
            out_h--;
          else
            break;

        while (out_w > 0)
          if (isScanColBlank(p + rg.l + out_x0 + (rg.t + out_y0) * imgw, out_h, imgw))
          {
            out_x0++;
            out_w--;
          }
          else if (isScanColBlank(p + rg.l + out_x0 + out_w - 1 + (rg.t + out_y0) * imgw, out_h, imgw))
            out_w--;
          else
            break;

        TexPixel32 *src = p + rg.l + out_x0 + (rg.t + out_y0) * imgw;
        for (int y = 0; y < out_h; y++, src += imgw)
          for (int x = 0; x < out_w; x++, dest++)
            *dest = src[x];

        if (!monochrome && rg.dx < 0)
        {
          out_x0 = rasterHorMargin;
          out_dx = out_w + rasterHorMargin * 2;
        }
        out_y0 = rasterBaseline - out_y0;
        if (c == ' ')
          out_dx = rasterSpaceWidth;
        return true;
      }
    return false;
  }
  void updateGlyphBounds(RasterGlyph &rg)
  {
    TexPixel32 *p = rg.img->getPixels();
    int imgw = rg.img->w;

    // debug_("u%04X: rg=(%d,%d)-(%d,%d) -> ", rg.code, rg.l, rg.t, rg.r, rg.b);

    while (rg.t < rg.b)
      if (isScanRowBlankC(p + rg.l + rg.t * imgw, rg.r - rg.l))
        rg.t++;
      else if (isScanRowBlankC(p + rg.l + (rg.b - 1) * imgw, rg.r - rg.l))
        rg.b--;
      else
        break;

    while (rg.l < rg.r)
      if (isScanColBlankC(p + rg.l + rg.t * imgw, rg.b - rg.t, imgw))
        rg.l++;
      else if (isScanColBlankC(p + rg.r - 1 + rg.t * imgw, rg.b - rg.t, imgw))
        rg.r--;
      else
        break;

    for (int y = rg.t; y < rg.b; y++)
      for (TexPixel32 *pix = p + rg.l + y * imgw, *pe = pix + rg.r - rg.l; pix < pe; pix++)
        if (!pix->a)
          pix->c = 0;

    rg.dx = rg.r - rg.l + rg.mx;
    if (rg.dx < 0)
      rg.dx = 0;
    // debug("(%d,%d)-(%d,%d) dx=%d", rg.l, rg.t, rg.r, rg.b, rg.dx);
  }
  static bool isScanRowBlank(TexPixel32 *pix, int c)
  {
    for (TexPixel32 *pe = pix + c; pix < pe; pix++)
      if (pix->a)
        return false;
    return true;
  }
  static bool isScanColBlank(TexPixel32 *pix, int c, int pitch)
  {
    for (TexPixel32 *pe = pix + c * pitch; pix < pe; pix += pitch)
      if (pix->a)
        return false;
    return true;
  }
  static bool isScanRowBlankC(TexPixel32 *pix, int c)
  {
    for (TexPixel32 *pe = pix + c; pix < pe; pix++)
      if (pix->c)
        return false;
    return true;
  }
  static bool isScanColBlankC(TexPixel32 *pix, int c, int pitch)
  {
    for (TexPixel32 *pe = pix + c * pitch; pix < pe; pix += pitch)
      if (pix->c)
        return false;
    return true;
  }

  void addHRes(const DataBlock &blk)
  {
    for (int k = 0; k < blk.paramCount(); ++k)
      if (blk.getParamType(k) == DataBlock::TYPE_INT)
        hRes.push_back(blk.getInt(k));
  }

  static void processRanges(const DataBlock &blk, unicode_range_t &urange)
  {
    int nid_range = blk.getNameId("range"), nid_char = blk.getNameId("char");

    for (int k = 0; k < blk.paramCount(); ++k)
      if (blk.getParamType(k) == DataBlock::TYPE_INT && blk.getParamNameId(k) == nid_char)
      {
        int id = blk.getInt(k);
        if (id >= 0 && id < urange.FULL_SZ)
          urange.set(id);
        else
          printf("ERR: invalid char %d (0x%04X)\n", id, id);
      }
      else if (blk.getParamType(k) == DataBlock::TYPE_IPOINT2 && blk.getParamNameId(k) == nid_range)
      {
        IPoint2 r = blk.getIPoint2(k);
        if (r[1] < r[0] || r[0] < 0 || r[1] >= (1 << 16))
          printf("ERR: invalid range %d..%d\n", r[0], r[1]);
        else
          urange.setRange(r[0], r[1]);
      }
  }

  bool isSymbolMonowidth(int symbol)
  {
    for (int i = 0, ie = monoWdGrp.size(); i < ie; i++)
    {
      unicode_range_t &ur = monoWdGrp[i];
      int maxw = 0;
      for (int c = 0, inc = 0; c < ur.FULL_SZ; c += inc)
        if (ur.getIter(c, inc))
          if (symbol == c)
            return true;
    }

    return false;
  }


  static void makeUnicodeRange(const DataBlock &blk, unicode_range_t &urange, bool &includeCp, FontInfo &info)
  {
    Tab<char> utf8(tmpmem);
    Tab<wchar_t> utf16(tmpmem);

    int nid_subset = blk.getNameId("subset");
    int nid_subset_utf8 = blk.getNameId("subset_utf8");
    int nid_utf8 = blk.getNameId("utf8_file");
    int nid_utf16 = blk.getNameId("utf16_file");

    for (int k = 0; k < blk.paramCount(); ++k)
      if (blk.getParamType(k) == DataBlock::TYPE_STRING && blk.getParamNameId(k) == nid_subset)
      {
        const char *str = blk.getStr(k);

        while (*str)
        {
          urange.set(*str);
          str++;
        }

        includeCp = false;
      }
      else if (blk.getParamType(k) == DataBlock::TYPE_STRING && blk.getParamNameId(k) == nid_subset_utf8)
      {
        const char *str = blk.getStr(k);
        utf16.resize(strlen(str));
        int used = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, (int)strlen(str), utf16.data(), utf16.size());

        G_ASSERT(used <= utf16.size());
        utf16.resize(used);

        for (int i = 0; i < utf16.size(); i++)
          urange.set(utf16[i]);

        includeCp = false;
      }
      else if (blk.getParamType(k) == DataBlock::TYPE_STRING && blk.getParamNameId(k) == nid_utf8)
      {
        const char *fname = blk.getStr(k);
        FILE *fp = fopen(fname, "rb");
        if (fp)
        {
          info.extFilesUsed.addNameId(fname);
          fseek(fp, 0, SEEK_END);
          utf8.resize(ftell(fp));
          fseek(fp, 0, SEEK_SET);
          utf8.resize(fread(utf8.data(), 1, utf8.size(), fp));
          fclose(fp);

          utf16.resize(utf8.size());
          int used = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), utf8.size(), utf16.data(), utf16.size());
          G_ASSERT(used <= utf16.size());
          utf16.resize(used);

          for (int i = 0; i < utf16.size(); i++)
            urange.set(utf16[i]);
        }
        else
          printf("ERR: cant open utf8 file <%s>", fname);
      }
      else if (blk.getParamType(k) == DataBlock::TYPE_STRING && blk.getParamNameId(k) == nid_utf16)
      {
        const char *fname = blk.getStr(k);
        FILE *fp = fopen(fname, "rb");
        if (fp)
        {
          info.extFilesUsed.addNameId(fname);
          fseek(fp, 0, SEEK_END);
          utf16.resize(ftell(fp) / 2);
          fseek(fp, 0, SEEK_SET);
          utf16.resize(fread(utf16.data(), 2, utf16.size(), fp));
          fclose(fp);

          for (int i = 0; i < utf16.size(); i++)
            urange.set(utf16[i]);
        }
        else
          printf("ERR: cant open utf16 file <%s>", fname);
      }

    includeCp = blk.getBool("includeCp", includeCp);

    processRanges(blk, urange);
  }

  void makeInclusions(const DataBlock &blk) { makeUnicodeRange(blk, include, includeCp, *this); }

  void makeExclusions(const DataBlock &blk) { processRanges(blk, exclude); }
  void markDynamic(const DataBlock &blk)
  {
    bool icp = false;
    makeUnicodeRange(blk, dyn, icp, *this);
    storeDynGrp = blk.getBool("storeDynGrpForAllGlyphs", false);
  }
};


static bool parseCodepage(const char *filename, uint16_t cmap[256])
{
  DataBlock blk;

  if (!blk.load(filename))
  {
    printf("can't load codepage '%s'\n", filename);
    return false;
  }

  int nameId = blk.getNameId("cmap");

  int ci = 0;

  for (int pi = 0; pi < blk.paramCount() && ci < 256; ++pi)
  {
    if (blk.getParamNameId(pi) != nameId)
      continue;
    if (blk.getParamType(pi) != DataBlock::TYPE_INT)
      continue;

    cmap[ci] = blk.getInt(pi);
    ++ci;
  }

  if (ci != 256)
    return false;

  return true;
}

static int getHistMinY(uint16_t *hist, int x0, int x1)
{
  int ymin = hist[x0];
  for (x0++; x0 < x1; x0++)
    if (hist[x0] < ymin)
      ymin = hist[x0];
  return ymin;
}

static int getHistMaxY(uint16_t *hist, int x0, int x1)
{
  int ymax = hist[x0];
  for (x0++; x0 < x1; x0++)
    if (hist[x0] > ymax)
      ymax = hist[x0];
  return ymax;
}

static void setHist(uint16_t *hist, int x0, int x1, int y)
{
  for (; x0 < x1; x0++)
    hist[x0] = y;
}

template <class T>
static bool packGlyphsAlg(T &alg, int texw, int &out_texh, int &gl0, BinFormat &bin, int fonts_count, dag::ConstSpan<Tabint> gli,
  int &opt, bool force_pow2)
{
  Tab<BinFormat::FontData> &fonts = bin.fonts;
  int i, x, y;
  bool ret = true;
  uint16_t hist[TEX_MAX_SIZE];
  int x0 = 0, dx = 1;
  memset(hist, 0, texw * sizeof(*hist));

  opt = 0;

  Tab<GlyphOffset> all_glyph(tmpmem);
  for (int font_index = 0; font_index < fonts_count; font_index++)
  {
    all_glyph.reserve(gli[font_index].size());
    for (i = 0; i < gli[font_index].size(); i++)
    {
      if (glimg[font_index][gli[font_index][i]].dyn >= 0 && !glimg[font_index][gli[font_index][i]].hasImage())
        continue;
      GlyphOffset &g = all_glyph.push_back();
      g.font_index = font_index;
      g.index = gli[font_index][i];
    }
  }
  all_glyph.shrink_to_fit();
  SimpleQsort<GlyphOffset, GlyphOffsetCmp>::sort(all_glyph.data(), all_glyph.size());

  {
    // try to fill texture with rows of images, starting at gl0
    for (i = gl0, y = 0; i < all_glyph.size();)
    {
      int ht = glimg[all_glyph[i].font_index][gli[all_glyph[i].font_index][all_glyph[i].index]].ht + 1;
      if (ht == 1)
        break;

      if (y + ht > TEX_MAX_SIZE)
      {
        ret = false;
        break;
      }

      int last_i = i;

      for (x = x0; i < all_glyph.size(); ++i)
      {
        int wd = glimg[all_glyph[i].font_index][gli[all_glyph[i].font_index][all_glyph[i].index]].wd + 1;
        if (wd == 1)
          continue;

        if (dx > 0 && x + wd > texw)
          break;
        else if (dx < 0 && x - wd < 0)
          break;
        int maxy = getHistMaxY(hist, dx > 0 ? x : x + wd * dx, dx > 0 ? x + wd * dx : x);
        int h = glimg[all_glyph[i].font_index][gli[all_glyph[i].font_index][all_glyph[i].index]].ht + 1;
        if (maxy + h > TEX_MAX_SIZE)
          break;

        setHist(hist, dx > 0 ? x : x + wd * dx, dx > 0 ? x + wd * dx : x, maxy + h);

        if (alg.NEED_OPT)
          opt += wd * h;
        else
        {
          // printf("current step: %i\n",i);
          alg.setPos(bin.fonts[all_glyph[i].font_index].glyph.data(), all_glyph[i].font_index,
            gli[all_glyph[i].font_index][all_glyph[i].index], dx > 0 ? x : x - wd + 1, maxy);
        }
        x += wd * dx;
      }


      if (i == all_glyph.size())
        break;

      if (i == last_i)
      {
        ret = false;
        break;
      }

      y = getHistMinY(hist, 0, texw);
      if (dx > 0)
      {
        x0 = texw - 1;
        dx = -1;
      }
      else
      {
        x0 = 0;
        dx = 1;
      }
    }
  }

  if (!alg.NEED_OPT)
    return true;

  if (ret)
  {
    y = getHistMaxY(hist, 0, texw);
    if (force_pow2 || y > 1024)
      out_texh = get_bigger_pow2(y);
    else
      out_texh = y ? (y + 31) & ~31 : 4;
    // printf ( "sz=%dx%d, y=%d\n", texw, out_texh, y );

    opt = texw * out_texh - opt + TEX_MAX_SIZE + texw + abs(texw - out_texh);
    return true;
  }
  opt = (i > gl0) ? (texw * TEX_MAX_SIZE - opt) / (i - gl0) : 0x7FFFFFFF;
  gl0 = i < all_glyph.size() ? i : bin.getGlyphsCount();
  out_texh = TEX_MAX_SIZE;
  return false;
}

struct AlgJustCalcOpt
{
  static const int NEED_OPT = 1;
  void setPos(BinFormat::Glyph *glyph, int font_index, int idx, int x, int y) {}
};

struct AlgFinalPack
{
  AlgFinalPack(TexPixel32 *_tex, int _ti, int _texw, int _texh)
  {
    tex = _tex;
    ti = _ti;
    texw = _texw;
    texh = _texh;
  }

  static const int NEED_OPT = 0;
  void setPos(BinFormat::Glyph *glyph, int font_index, int idx, int x, int y)
  {
    BinFormat::Glyph &g = glyph[idx];
    TexPixel32 *dp = tex + y * texw + x;
    int wd = glimg[font_index][idx].wd, ht = glimg[font_index][idx].ht;
    G_ASSERT(x + wd <= texw && y + ht <= texh);

    g.tex = ti;
    g.u0 = float(x) / texw;
    g.u1 = float(x + wd) / texw;
    g.v0 = float(y) / texh;
    g.v1 = float(y + ht) / texh;

    if (glimg[font_index][idx].im)
    {
      uint8_t *s = glimg[font_index][idx].im;
      for (; ht; --ht, dp += texw)
      {
        TexPixel32 *d = dp;
        for (int w = wd; w; --w, ++d, ++s)
          d->r = d->g = d->b = d->a = *s;
      }
    }
    else if (glimg[font_index][idx].im32)
    {
      TexPixel32 *s = glimg[font_index][idx].im32;
      for (; ht; --ht, dp += texw)
      {
        TexPixel32 *d = dp;
        for (int w = wd; w; --w, ++d, ++s)
          *d = *s;
      }
    }
  }

  TexPixel32 *tex;
  int ti, texw, texh;
};

static bool packGlyphs(int texw, int &out_texh, int &gl0, BinFormat &bin, int fonts_count, dag::ConstSpan<Tabint> gli, int &opt,
  bool force_pow2)
{
  AlgJustCalcOpt alg;
  return packGlyphsAlg(alg, texw, out_texh, gl0, bin, fonts_count, gli, opt, force_pow2);
}

static void writeGlyphs(TexPixel32 *tex, int texw, int texh, int gl0, BinFormat &bin, int fonts_count, Tab<Tabint> &gli, int ti,
  bool force_pow2)
{
  AlgFinalPack alg(tex, ti, texw, texh);
  int unused_opt;
  packGlyphsAlg(alg, texw, texh, gl0, bin, fonts_count, gli, unused_opt, force_pow2);
}

static bool loadFreeTypeFont(const FontInfo &font_info, int ht, FreeTypeFont &f)
{
  if (!font_info.file)
    return false;

  if (!dd_file_exist(font_info.file))
  {
    printf("[SKIP] File \"%s\" not exist\n", font_info.file);
    return false;
  }

  if (!f.import_ttf(font_info.file, font_info.symbCharMap))
  {
    fatal("TrueType can't load font from \"%s\"", font_info.file);
    return false;
  }
  int original_ht = ht;
  for (int i = 0; i < sizeRestrBlk->blockCount(); i++)
    if (stricmp(sizeRestrBlk->getBlock(i)->getBlockName(), dd_get_fname(font_info.file)) == 0)
    {
      int hmin = sizeRestrBlk->getBlock(i)->getInt("minHt", 1);
      int hmax = sizeRestrBlk->getBlock(i)->getInt("maxHt", 256);
      if (ht < hmin)
        ht = hmin;
      else if (ht > hmax)
        ht = hmax;
      else
        while (ht > hmin && !sizeRestrBlk->getBlock(i)->getBool(String(0, "use%d", ht), true))
          ht--;
    }

  if (!f.resize_font(ht - 2 * font_info.border, font_info.valve_upscale))
  {
    printf("[SKIP] can't set height=%d pixels for font \"%s\" \n", ht, font_info.file);
    return false;
  }
  if (ht != original_ht)
    printf(" using font height %d pix (instead of %d)\n", ht, original_ht);

  f.srcDesc.printf(0, "%s?%d?%d?%d", font_info.file, ht - 2 * font_info.border, font_info.valve_upscale, font_info.forceAutoHinting);
  strlwr(f.srcDesc);
  simplify_fname(f.srcDesc);
  return true;
}

static bool dimCompile(Tab<FontInfo *> &info, int hres_id, bool genTga, int target, DataBlock &cfgBlk, bool verbose)
{
  int fonts_count = info.size();
  BinFormat bin;
  Tab<Tabint> gli(tmpmem);
  uint8_t *rg = (uint8_t *)memalloc(256 * 256, tmpmem);
  TexPixel32 *rg32 = (TexPixel32 *)memalloc(256 * 256 * 4, tmpmem);
  bool rgba32 = false;
  const char *copyto_dir = cfgBlk.getStr("copyto", NULL);

  append_items(gli, fonts_count);
  append_items(glimg, fonts_count);
  append_items(bin.fonts, fonts_count);

  bool has_classic = false, has_valve = false;
  for (int i = 0; i < info.size(); i++)
    if (info[i]->valve_upscale > 1)
      has_valve = true;
    else
      has_classic = true;

  if (has_valve && has_classic)
  {
    printf("ERR: cannot mix classic and valve font texture generation\n");
    return false;
  }

  String target_fname;
  if (hres_id == -1)
    target_fname.printf(0, "%s%s.bin", info[0]->prefix, info[0]->atlas);
  else
    target_fname.printf(0, "%s%s.%d.bin", info[0]->prefix, info[0]->atlas, info[0]->hRes[hres_id]);

  String cache_fname;
  String dest_target_fname;
  if (copyto_dir)
  {
    cache_fname.printf(0, "%s.%s.c4.bin", target_fname, mkbindump::get_target_str(target));
    dest_target_fname.printf(0, "%s/%s", copyto_dir, target_fname + strlen(info[0]->prefix));
    dd_mkpath(dest_target_fname);
  }
  else
    cache_fname.printf(0, "%s-%s.%s.c4.bin", cfgBlk.resolveFilename(), dd_get_fname(target_fname), mkbindump::get_target_str(target));
  GenericBuildCache c4;
  bool uptodate = true;

  MutexAutoAcquire mutexAa(cache_fname);

  if (!c4.load(cache_fname) || c4.checkTargetFileChanged(target_fname))
  {
    uptodate = false;
    if (dd_file_exist(cache_fname))
    {
      logwarn("removed outdated cache: %s", cache_fname.str());
      unlink(cache_fname);
    }
  }

  if (c4.checkFileChanged(__argv[0])) //== depend on EXE revision
    uptodate = false;
  if (c4.checkDataBlockChanged(cfgBlk.resolveFilename(), cfgBlk))
    uptodate = false;
  for (int i = 0; i < info.size(); i++)
  {
    if (c4.checkFileChanged(info[i]->file))
      uptodate = false;
    iterate_names(info[i]->extFilesUsed, [&](int, const char *name) {
      if (c4.checkFileChanged(name))
        uptodate = false;
    });
    for (int j = 0; j < info[i]->additional.size(); j++)
    {
      if (c4.checkFileChanged(info[i]->additional[j].file))
        uptodate = false;
      iterate_names(info[i]->additional[j].extFilesUsed, [&](int, const char *name) {
        if (c4.checkFileChanged(name))
          uptodate = false;
      });
    }
  }
  char dest_dir[512];
  dd_get_fname_location(dest_dir, target_fname);
  size_t dest_dir_len = strlen(dest_dir);
  if (uptodate)
    iterate_names(c4.getNameList(), [&](int, const char *name) {
      if (strncmp(name, dest_dir, dest_dir_len) == 0)
        if (const char *ext = dd_get_fname_ext(name))
          if (stricmp(ext, ".bin") != 0)
            if (c4.checkFileChanged(name))
              uptodate = false;
    });


  if (uptodate)
  {
    if (c4.isTimeChanged() || !c4.checkAllTouched())
    {
      c4.removeUntouched();
      c4.save(cache_fname);
    }
    printf("up-to-date: %s\n", target_fname.str());
    if (copyto_dir)
    {
      MutexAutoAcquire mutexAa2(copyto_dir);
      iterate_names(c4.getNameList(), [&](int, const char *name) {
        if (strncmp(name, dest_dir, dest_dir_len) == 0)
          if (const char *ext = dd_get_fname_ext(name))
            if (stricmp(ext, ".bin") != 0 && stricmp(ext, ".txt") != 0 && stricmp(ext, ".utf8") != 0)
              dag_copy_file(name, String(0, "%s/%s", copyto_dir, dd_get_fname(name)));
      });
      return dag_copy_file(target_fname, dest_target_fname);
    }
    return true;
  }

  for (int current_font = 0; current_font < fonts_count; current_font++)
  {
    double src_ht = info[current_font]->size;
    int char_as_space = info[current_font]->charAsSpace;
    int pix_ht = 0;

    printf("Compile '%s' '%s' '%s' %.2f%s (upscale=%d)", info[current_font]->file ? info[current_font]->file : "RASTER FONT",
      info[current_font]->name.str(), info[current_font]->cpfile, fabs(src_ht), src_ht > 0 ? "%" : " pix",
      info[current_font]->valve_upscale);

    if (hres_id < 0)
    {
      if (src_ht > 0)
      {
        printf("[SKIP] %s/%s size=%.1f (in percents) but target hRes not specified\n", info[current_font]->file,
          info[current_font]->name.str(), src_ht);
        return false;
      }
      src_ht = -src_ht;
    }
    else
    {
      if (src_ht < 0)
      {
        printf("[SKIP] %s/%s size=%.1f (in pixels) but target hRes is used - meaningless\n", info[current_font]->file,
          info[current_font]->name.str(), -src_ht);
        return false;
      }
      src_ht = src_ht * info[current_font]->hRes[hres_id] / 100.0f;
    }

    if (info[current_font]->makeHtEven)
      pix_ht = int(floor(src_ht / 2 + 0.5)) * 2;
    else
      pix_ht = int(floor(src_ht + 0.5));

    if (hres_id > -1)
      printf(" (%d pix) for target hRes: %d lines\n", pix_ht, info[current_font]->hRes[hres_id]);
    else
      printf("\n");


    if (!info[current_font]->file && !info[current_font]->rasterGlyphs.size())
    {
      printf("ERR: no font data specified\n");
      return false;
    }
    if (info[current_font]->rgba32 && !rgba32)
    {
      printf("generating RGBA32 textures\n");
      rgba32 = true;
    }

    FreeTypeFont generalFont;
    bool hasGenFont = false;

    if (info[current_font]->file)
    {
      if (!loadFreeTypeFont(*info[current_font], pix_ht, generalFont))
        return false;
      else
        hasGenFont = true;
    }

    Tab<FreeTypeFont> additionalFonts(tmpmem);
    const int numAdditional = info[current_font]->additional.size();
    for (int addNo = 0; addNo < numAdditional; ++addNo)
    {
      FontInfo &af = info[current_font]->additional[addNo];
      FreeTypeFont &addFnt = additionalFonts.push_back();
      double add_ht;

      if (hres_id < 0)
        add_ht = -af.size;
      else
        add_ht = af.size * info[current_font]->hRes[hres_id] / 100.0f;

      if (!loadFreeTypeFont(af, int(floor(add_ht / 2 + 0.5f)) * 2, addFnt))
      {
        additionalFonts.pop_back();
        return false;
      }
    }

    const int CP_SIZE = 256;

    Tab<uint16_t> cmap(tmpmem);
    unicode_range_t usedSym;

    cmap.resize(CP_SIZE);

    if (!info[current_font]->cpfile)
    {
      if (!info[current_font]->symbCharMap)
      {
        printf("No codepage specified in %s font, using direct ASCIItable\n", info[current_font]->name.c_str());
      }
      if (info[current_font]->remapPrivate)
        for (int i = 0; i < CP_SIZE; i++)
        {
          cmap[i] = 0xF000 + i;
          usedSym.set(0xF000 + i);
        }
      else
        for (int i = 0; i < CP_SIZE; i++)
        {
          cmap[i] = i;
          usedSym.set(i);
        }
    }
    else if (strcmp(info[current_font]->cpfile, "none") == 0)
    {
      for (int i = 0; i < CP_SIZE; i++)
        cmap[i] = 0;
      usedSym.set(0);
    }
    else
    {
      if (!dd_file_exist(info[current_font]->cpfile))
      {
        printf("[SKIP] File \"%s\" not exist\n", info[current_font]->cpfile);
        return false;
      }

      if (!::parseCodepage(info[current_font]->cpfile, cmap.data()))
      {
        printf("[SKIP] Invalid codepage file %s for %s font\n", info[current_font]->cpfile, info[current_font]->name.c_str());
        return false;
      }
      for (int i = 0; i < CP_SIZE; ++i)
        usedSym.set(cmap[i]);
    }

    cmap[' '] = ' ';
    cmap['\t'] = '\t';

    for (int i = 0, inc = 1; i < usedSym.FULL_SZ; i += inc)
      if (info[current_font]->include.getIter(i, inc))
        if (!info[current_font]->exclude.get(i) && !usedSym.get(i))
        {
          cmap.push_back(i);
          usedSym.set(i);
        }
    for (int j = 0; j < numAdditional; j++)
      for (int i = 0, inc = 1; i < usedSym.FULL_SZ; i += inc)
        if (info[current_font]->additional[j].include.getIter(i, inc))
          if (!info[current_font]->additional[j].exclude.get(i) && !usedSym.get(i))
          {
            cmap.push_back(i);
            usedSym.set(i);
          }
    usedSym.reset();

    Tab<int> cmap_fid;
    cmap_fid.resize(cmap.size());
    mem_set_0(cmap_fid);
    for (int gi = 0; gi < cmap.size(); ++gi)
    {
      unsigned c = cmap[gi];
      if (!c)
        continue;
      int fid = 0;
      for (int j = 0; j < numAdditional; j++)
        if (info[current_font]->additional[j].include.get(c) && !info[current_font]->additional[j].exclude.get(c))
        {
          fid = j + 1;
          break;
        }
      if (gi < CP_SIZE && cmap[gi] != ' ')
      {
        if (fid == 0 && (!info[current_font]->include.get(cmap[gi]) || info[current_font]->exclude.get(cmap[gi])))
          cmap[gi] = 0;
        else if (fid > 0 && (!info[current_font]->additional[fid - 1].include.get(cmap[gi]) ||
                              info[current_font]->additional[fid - 1].exclude.get(cmap[gi])))
          cmap[gi] = 0;
      }
      cmap_fid[gi] = fid;
    }


    bin.prepareUnicodeTable(current_font, cmap.data(), cmap.size());

    if (hasGenFont)
      generalFont.metrics(&bin.fonts[current_font].ascent, &bin.fonts[current_font].descent, &bin.fonts[current_font].fontht,
        &bin.fonts[current_font].linegap); // read metrics
    else
    {
      bin.fonts[current_font].ascent = info[current_font]->rasterAscender;
      bin.fonts[current_font].descent = info[current_font]->rasterDescender;
      bin.fonts[current_font].fontht = info[current_font]->rasterHt;
      bin.fonts[current_font].linegap = info[current_font]->rasterLineGap;
    }

    bin.fonts[current_font].btbd = bin.fonts[current_font].ascent + bin.fonts[current_font].descent + bin.fonts[current_font].linegap;
    bin.fonts[current_font].name = info[current_font]->name;

    if (hasGenFont && info[current_font]->symmEL)
    {
      BinFormat::FontData &f = bin.fonts[current_font];
      int char_idx = generalFont.get_char_index('A');
      int wd = 0, capsHt = 0, x0, y0;
      short dx;

      if (char_idx && generalFont.render_glyph(rg, &wd, &capsHt, &x0, &y0, &dx, char_idx, false, false, false, false))
      {
        if (verbose)
          debug("font metrics: ht=%d ascent=%d descent=%d capsHt=%d", f.fontht, f.ascent, f.descent, capsHt);
        f.descent = max(f.descent, int(f.ascent - capsHt));
        f.ascent = f.descent + capsHt;
        if (verbose)
          debug("  -> ascent=%d descent=%d", f.ascent, f.descent);
      }
    }

    glimg[current_font].resize(0);
    glimg[current_font].reserve(CP_SIZE);
    bin.fonts[current_font].glyph.reserve(CP_SIZE);
    int ident_sym_cnt = 0;

    Tab<Tabint> glyph_idx(tmpmem_ptr());
    glyph_idx.resize(32);
    glyph_idx[0].reserve(4096);

    // add 3 predefined symbols: MISING, SPACE, TAB
    G_ASSERT(bin.fonts[current_font].glyph.size() == 0);
    bin.fonts[current_font].glyph.resize(3);
    bin.fonts[current_font].ut[0]->idx['\0'] = 0;
    bin.fonts[current_font].ut[0]->idx[' '] = 1;
    bin.fonts[current_font].ut[0]->idx['\t'] = 2;

    // rasterize MISSING symbol
    {
      int wd = 0, ht = 0, x0, y0;
      short dx;
      int char_idx = 0;
      BinFormat::Glyph &g = bin.fonts[current_font].glyph[0];

      if (!info[current_font]->generateMissingGlyphBox &&
          generalFont.render_glyph(rg, &wd, &ht, &x0, &y0, &dx, 0, info[current_font]->monochrome,
            info[current_font]->forceAutoHinting, info[current_font]->checkExtents, true))
      {
        g.x0 = x0;
        g.y0 = -y0;
        g.x1 = x0 + wd;
        g.y1 = -y0 + ht;
        g.dx = dx;
        glimg[current_font].push_back().set(rg, wd, ht, info[current_font]->border, info[current_font]->border_color,
          info[current_font]->useFontColor, info[current_font]->font_color);
      }
      else if (info[current_font]->rasterGlyphs.size() && info[current_font]->rasterMissingCode &&
               info[current_font]->rasterize(info[current_font]->rasterMissingCode, rg32, wd, ht, x0, y0, dx))
      {
        char_idx = info[current_font]->rasterMissingCode;
        g.x0 = x0;
        g.y0 = -y0;
        g.x1 = x0 + wd;
        g.y1 = -y0 + ht;
        g.dx = dx;
        glimg[current_font].push_back().set32(rg32, wd, ht);
      }
      else
      {
        ht = max(bin.fonts[current_font].ascent - bin.fonts[current_font].descent, 2);
        wd = max(ht / 2, 2);

        g.x0 = 1;
        g.y0 = -ht;
        g.x1 = wd + 1;
        g.y1 = 0;
        g.dx = wd + 2;
        memset(rg, 0, wd * ht);
        memset(rg, 0xA0, wd);
        memset(rg + wd * (ht - 1), 0xA0, wd);
        for (int i = 0; i < ht; i++)
          rg[i * wd + 0] = rg[i * wd + wd - 1] = 0xA0;

        glimg[current_font].push_back().set(rg, wd, ht, info[current_font]->border, info[current_font]->border_color,
          info[current_font]->useFontColor, info[current_font]->font_color);
      }

      g.tex = g.dynGrp = -1;
      g.pkOfs = 0;
      g.noPk = 1;

      glyph_idx[0].resize(char_idx + 1);
      mem_set_ff(make_span(glyph_idx[0]).first(char_idx));
      glyph_idx[0][char_idx] = 0;
    }

    // setup SPACE and TAB glyphs
    for (int i = 1; i < 3; i++)
    {
      BinFormat::Glyph &g = bin.fonts[current_font].glyph[i];
      g.tex = g.dynGrp = -1;
      g.u0 = g.v0 = g.u1 = g.v1 = 0;
      g.x0 = g.y0 = g.x1 = g.y1 = 0;
      g.dx = 0;
      g.pkOfs = 0;
      g.noPk = 1;

      glimg[current_font].push_back().set(rg, 0, 0, info[current_font]->border, info[current_font]->border_color,
        info[current_font]->useFontColor, info[current_font]->font_color);
    }
    bin.fonts[current_font].glyph[1].dx = info[current_font]->rasterSpaceWidth;

    // rasterize font symbols
    Tab<uint32_t> &glyphPk = bin.fonts[current_font].glyphPk;
    int missing_glyphs = 0, total_char = 0;
    for (int gi = 0; gi < cmap.size(); ++gi)
    {
      if (cmap[gi])
      {
        total_char++;

        int wd, ht, x0, y0;
        short dx;
        int char_idx = 0;
        int fontId = cmap_fid[gi];
        FreeTypeFont *f = &generalFont;
        bool isMonochrome = info[current_font]->monochrome;
        bool forceAutoHinting = info[current_font]->forceAutoHinting;
        bool checkExtents = info[current_font]->checkExtents;
        if (fontId > 0)
        {
          f = &additionalFonts[fontId - 1];
          isMonochrome = info[current_font]->additional[fontId - 1].monochrome;
          forceAutoHinting = info[current_font]->additional[fontId - 1].forceAutoHinting;
          checkExtents = info[current_font]->additional[fontId - 1].checkExtents;
        }
        char_idx = f->get_char_index(cmap[gi]);
        if (!char_idx && !info[current_font]->hasRasterSymbol(cmap[gi]))
        {
          char utf8_buf[8];
          missing_glyphs++;
          if (verbose)
            printf("  symbol u%04X [%s] not found font #%d (%s)\n", cmap[gi],
              gi >= 0x20 ? wchar_to_utf8(cmap[gi], utf8_buf, sizeof(utf8_buf)) : " ", fontId, f->srcDesc.str());
          cmap[gi] = 0;
        }


        int s_pair_cnt = glyphPk.size();
        if (f->has_pair_kerning(cmap[gi]) && !info[current_font]->isSymbolMonowidth(cmap[gi]))
          for (int sgi = 0; sgi < cmap.size(); ++sgi)
          {
            unsigned c2 = cmap[sgi];
            if (c2 && fontId == cmap_fid[sgi])
            {
              if (!info[current_font]->isSymbolMonowidth(c2))
              {
                if (int add_w = f->get_pair_kerning(cmap[gi], c2))
                  glyphPk.push_back((uint16_t(add_w & 0xFFFF) << 16) | c2);
              }
            }
          }
        G_ASSERT(s_pair_cnt < 0xFFFF);
        G_ASSERT(glyphPk.size() < 0xFFFF);

        if (info[current_font]->rasterize(cmap[gi], rg32, wd, ht, x0, y0, dx))
        {
          // raster font branch
          if (cmap[gi] == char_as_space)
          {
            float spaceWidthMul = info[current_font]->spaceWidthMul;
            if ((char_as_space != ' ' || spaceWidthMul != 1) && verbose)
              printf("replaced TTF's space width to reference '%c'(u%04X), mul=%.1f width (%d)\n", char_as_space, char_as_space,
                spaceWidthMul, int(dx * spaceWidthMul));
            bin.fonts[current_font].glyph[1].dx = dx * spaceWidthMul;
          }
          if (gi == ' ' || gi == '\t')
            continue;
          if (cmap[gi] == info[current_font]->rasterMissingCode)
          {
            int gidx = cmap[gi] >> 8;
            G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
            bin.fonts[current_font].ut[gidx]->idx[cmap[gi] & 0xFF] = 0;
            if (gi < CP_SIZE)
              bin.fonts[current_font].at[gi] = 0;
            continue;
          }

          if (gi < CP_SIZE)
            bin.fonts[current_font].at[gi] = bin.fonts[current_font].glyph.size();

          int gidx = cmap[gi] >> 8;
          G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
          bin.fonts[current_font].ut[gidx]->idx[cmap[gi] & 0xFF] = bin.fonts[current_font].glyph.size();

          BinFormat::Glyph &g = bin.fonts[current_font].glyph.push_back();
          g.tex = g.dynGrp = -1;
          g.x0 = x0;
          g.y0 = -y0;
          g.x1 = x0 + wd;
          g.y1 = -y0 + ht;
          g.dx = dx;
          g.pkOfs = s_pair_cnt;
          g.noPk = (s_pair_cnt == glyphPk.size()) ? 1 : 0;

          glimg[current_font].push_back().set32(rg32, wd, ht);
          continue;
        }

        if (!char_idx)
          continue;

        if (char_idx < glyph_idx[fontId].size() && glyph_idx[fontId][char_idx] != -1)
        {
          if (gi < CP_SIZE)
            bin.fonts[current_font].at[gi] = glyph_idx[fontId][char_idx];

          int gidx = cmap[gi] >> 8;
          G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
          bin.fonts[current_font].ut[gidx]->idx[cmap[gi] & 0xFF] = glyph_idx[fontId][char_idx];
          ident_sym_cnt++;
          continue;
        }

        bool dyn_glyph = info[current_font]->dyn.get(cmap[gi]);
        if (!f->render_glyph(rg, &wd, &ht, &x0, &y0, &dx, char_idx, isMonochrome, forceAutoHinting, checkExtents, !dyn_glyph))
        {
          char utf8_buf[8];
          if (wd > 256 || ht > 256)
            printf("u%04X [%s] glyph[%d] too big sz=%dx%d in %s\n", cmap[gi],
              cmap[gi] >= 0x20 ? wchar_to_utf8(cmap[gi], utf8_buf, sizeof(utf8_buf)) : " ", char_idx, wd, ht, f->srcDesc.str());
          else if (verbose)
            printf("u%04X [%s] glyph[%d] render failed for %s!\n", cmap[gi],
              cmap[gi] >= 0x20 ? wchar_to_utf8(cmap[gi], utf8_buf, sizeof(utf8_buf)) : " ", char_idx, f->srcDesc.str());
          continue;
        }

        if (char_idx >= glyph_idx[fontId].size())
        {
          int st = glyph_idx[fontId].size();
          glyph_idx[fontId].resize(char_idx + 1);
          mem_set_ff(make_span(glyph_idx[fontId]).subspan(st, char_idx - st));
        }
        glyph_idx[fontId][char_idx] = bin.fonts[current_font].glyph.size();

        if (cmap[gi] == char_as_space)
        {
          float spaceWidthMul = info[current_font]->spaceWidthMul;
          if (char_as_space != ' ' || spaceWidthMul != 1)
            printf("replaced TTF's space width to reference '%c'(u%04X), mul=%.1f width (%d)\n", char_as_space, char_as_space,
              spaceWidthMul, int(dx * spaceWidthMul));
          bin.fonts[current_font].glyph[1].dx = dx * spaceWidthMul;
        }
        if (gi == ' ' || gi == '\t')
          continue;

        if (gi < CP_SIZE)
          bin.fonts[current_font].at[gi] = bin.fonts[current_font].glyph.size();

        int gidx = cmap[gi] >> 8;
        G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
        bin.fonts[current_font].ut[gidx]->idx[cmap[gi] & 0xFF] = bin.fonts[current_font].glyph.size();

        BinFormat::Glyph &g = bin.fonts[current_font].glyph.push_back();
        g.tex = g.dynGrp = -1;
        g.x0 = x0;
        g.y0 = -y0;
        g.x1 = x0 + wd;
        g.y1 = -y0 + ht;
        g.dx = dx;
        g.pkOfs = s_pair_cnt;
        g.noPk = (s_pair_cnt == glyphPk.size()) ? 1 : 0;
        if (g.dx < 0)
        {
          printf("WARN: u%04X; (%d,%d)-(%d,%d) dx=%d, changed to 0\n", cmap[gi], x0, y0, x0 + wd, y0 + ht, dx);
          g.dx = 0;
        }

        GlyphImage &gl_img = glimg[current_font].push_back();
        if (!dyn_glyph)
          gl_img.set(rg, wd, ht, info[current_font]->border, info[current_font]->border_color, info[current_font]->useFontColor,
            info[current_font]->font_color);

        if (info[current_font]->storeDynGrp || dyn_glyph)
          gl_img.markDyn(g.dynGrp = dynTTFUsed.addNameId(f->srcDesc), cmap[gi], dyn_glyph);
        G_ASSERT(dynTTFUsed.nameCount() < 128);
        // printf("%04X->u%04X; (%d,%d)-(%d,%d) dx=%d\n", gi, cmap[gi], x0, y0, x0+wd, y0+ht, dx);
      }
    }
    if (!::dgs_execute_quiet)
      printf("%d chars (%d glyphs) produced %d kerning pairs (and %d glyphs missing)\n", total_char,
        bin.fonts[current_font].glyph.size(), glyphPk.size(), missing_glyphs);
    glyphPk.shrink_to_fit();

    if (bin.fonts[current_font].glyph.size() == 3 && !ident_sym_cnt)
    {
      printf("no symbols rasterized!\n");
      return false;
    }
    bin.fonts[current_font].at['\0'] = 0;
    bin.fonts[current_font].at[' '] = 1;
    bin.fonts[current_font].at['\t'] = 2;

    //-------------------------------------------------------
    // remap case insensitive fonts
    if (!info[current_font]->caseSensitive)
    {
      Tab<uint16_t> add_cmap(tmpmem);
      Tab<uint16_t> add_glind(tmpmem);

      for (int c_ind = 0; c_ind < cmap.size(); ++c_ind)
        if (cmap[c_ind])
        {
          uint16_t up_case = LOWORD(CharUpperW((LPWSTR)(uintptr_t)cmap[c_ind]));                              //-V:542 LCMapString
          uint16_t low_case = LOWORD(CharLowerW((LPWSTR)(uintptr_t)cmap[c_ind]));                             //-V:542
          uint16_t search_case = up_case != cmap[c_ind] ? up_case : (low_case != cmap[c_ind] ? low_case : 0); //-V:542

          if (search_case)
          {
            bool _found = false;
            int gidx = cmap[c_ind] >> 8;
            G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
            uint16_t gl_index = bin.fonts[current_font].ut[gidx]->idx[cmap[c_ind] & 0xFF];

            for (int c2_ind = 0; c2_ind < cmap.size(); ++c2_ind)
              if ((c2_ind != c_ind) && (search_case == cmap[c2_ind]))
              {
                int gidx_new = cmap[c2_ind] >> 8;
                G_ASSERT(gidx_new < 256 && bin.fonts[current_font].ut[gidx_new]);
                uint16_t gl_index_new = bin.fonts[current_font].ut[gidx_new]->idx[cmap[c2_ind] & 0xFF];

                if (gl_index_new && gl_index_new != 0xFFFF)
                  _found = true;
                break;
              }

            if (!_found && gl_index && gl_index != 0xFFFF)
            {
              add_cmap.push_back(search_case);
              add_glind.push_back(gl_index);
              continue;
            }
          }
        }

      bin.prepareUnicodeTable(current_font, add_cmap.data(), add_cmap.size());

      for (int c_ind = 0; c_ind < add_cmap.size(); ++c_ind)
      {
        int gidx = add_cmap[c_ind] >> 8;
        G_ASSERT(gidx < 256 && bin.fonts[current_font].ut[gidx]);
        bin.fonts[current_font].ut[gidx]->idx[add_cmap[c_ind] & 0xFF] = add_glind[c_ind];

        if (add_cmap[c_ind] < CP_SIZE)
          bin.fonts[current_font].at[add_cmap[c_ind]] = add_glind[c_ind];
      }
    }
    //-------------------------------------------------------

    bin.shrinkUnicodeTable(current_font);

    gli[current_font].resize(bin.fonts[current_font].glyph.size());
    for (int gi = 0; gi < gli[current_font].size(); ++gi)
      gli[current_font][gi] = gi;

    for (int i = 0, ie = info[current_font]->monoWdGrp.size(); i < ie; i++)
    {
      unicode_range_t &ur = info[current_font]->monoWdGrp[i];
      int maxw = 0;
      for (int c = 0, inc = 0; c < ur.FULL_SZ; c += inc)
        if (ur.getIter(c, inc))
        {
          int idx = bin.fonts[current_font].ut[c >> 8] ? bin.fonts[current_font].ut[c >> 8]->idx[c & 0xFF] : -1;
          if (idx >= 0)
            if (bin.fonts[current_font].glyph[idx].dx > maxw)
              maxw = bin.fonts[current_font].glyph[idx].dx;
        }

      for (int c = 0, inc = 0; c < ur.FULL_SZ; c += inc)
        if (ur.getIter(c, inc))
        {
          int idx = bin.fonts[current_font].ut[c >> 8] ? bin.fonts[current_font].ut[c >> 8]->idx[c & 0xFF] : -1;
          if (idx >= 0)
          {
            int dx = bin.fonts[current_font].glyph[idx].dx;
            if (dx < maxw)
            {
              bin.fonts[current_font].glyph[idx].dx = maxw;
              bin.fonts[current_font].glyph[idx].x0 += (maxw - dx) / 2;
              bin.fonts[current_font].glyph[idx].x1 += (maxw - dx) / 2;
            }
          }
        }
    }
  }

  memfree(rg, tmpmem);
  memfree(rg32, tmpmem);

  int gl0 = 0;
  bool force_pow2 = (target == _MAKE4C('iOS')) || cfgBlk.getBool("forcePow2", false);

  for (; gl0 < bin.getGlyphsCount();)
  {
    int opt = 0x7FFFFFFF, opt_i = -1, opt_texh = 0;
    int opt_part = 0x7FFFFFFF, opt_part_i = -1, opt_part_glnext = 0;

    for (int layout_idx = 0; layout_idx < TEX_LAYOUT_NUM; layout_idx++)
    {
      int optx, next_gl0 = gl0, texh;
      if (packGlyphs(TEX_SIDE_BASE << layout_idx, texh, next_gl0, bin, bin.fonts.size(), gli, optx, force_pow2))
      {
        // printf ( "%dx%d: opt=%d\n", TEX_SIDE_BASE << layout_idx, texh, optx );
        if (optx < opt)
        {
          opt = optx;
          opt_i = layout_idx;
          opt_texh = texh;
        }
      }
      else
      {
        // printf ( "p: %dx%d: opt=%d\n", TEX_SIDE_BASE << layout_idx, 2048, optx );
        if (optx < opt_part)
        {
          opt_part = optx;
          opt_part_i = layout_idx;
          opt_part_glnext = next_gl0;
        }
      }
    }

    // printf ( "opt_i=%d, opt_part_i=%d\n", opt_i, opt_part_i );
    if (opt_i == -1)
    {
      if (opt_part_i == -1)
        fatal("glyph is too big to fit in %dx%d", TEX_MAX_SIZE, TEX_MAX_SIZE);
      opt_i = opt_part_i;
      opt_texh = TEX_MAX_SIZE;
      // printf ( "partial, opt_part_glnext=%d\n", opt_part_glnext );
    }

    int texw = TEX_SIDE_BASE << opt_i, texh = opt_texh;

    TexPixel32 *im = (TexPixel32 *)memalloc(texw * texh * sizeof(TexPixel32), tmpmem);
    memset(im, 0, texw * texh * sizeof(TexPixel32));
    writeGlyphs(im, texw, texh, gl0, bin, bin.fonts.size(), gli, bin.ims.size(), force_pow2);
    if (genTga)
    {
      String fn(100, "%s%s.%d_%d.tga", info[0]->prefix, info[0]->atlas, hres_id >= 0 ? info[0]->hRes[hres_id] : 0, bin.ims.size());
      dd_mkpath(fn);
      save_tga32(fn, im, texw, texh, texw * 4);
    }

    bin.ims.push_back(im);
    bin.texws.push_back(texw);
    bin.texhs.push_back(texh);

    if (opt_i != opt_part_i)
      break;
    gl0 = opt_part_glnext;
  }

  printf("  %d symbols in %d texture(s): ", bin.getGlyphsCount(), bin.ims.size());
  for (int i = 0; i < bin.ims.size(); i++)
    printf("%dx%d ", bin.texws[i], bin.texhs[i]);
  printf("\n");

  ::dd_mkpath(target_fname);
  if (!bin.saveBinary(target_fname, target, rgba32, info[0]->valve_upscale > 1, false, dynTTFUsed))
    return false;

  MutexAutoAcquire mutexAa2(copyto_dir ? copyto_dir : target_fname);
  FastNameMap ttfs;
  for (int i = 0; i < dynTTFUsed.nameCount(); i++)
  {
    const char *nm = dynTTFUsed.getName(i);
    G_ASSERT(nm != nullptr);
    const char *sz_p = strchr(nm, '?');
    ttfs.addNameId(String(0, "%.*s", sz_p - nm, nm));
  }
  for (int j = 0; j < ttfs.nameCount(); j++)
  {
    const char *fn = dd_get_fname(ttfs.getName(j));
    String dest_fn(0, "%s/%s", dest_dir, fn);
    dag_copy_file(ttfs.getName(j), dest_fn);
    if (c4.checkFileChanged(dest_fn))
      c4.updateFileHash(dest_fn);
    if (copyto_dir)
      dag_copy_file(ttfs.getName(j), String(0, "%s/%s", copyto_dir, fn));
  }

  for (int i = 0; i < glimg.size(); i++)
    clear_and_shrink(glimg[i]);
  clear_and_shrink(glimg);

  c4.setTargetFileHash(target_fname);
  c4.removeUntouched();
  c4.save(cache_fname);
  if (copyto_dir)
    return dag_copy_file(target_fname, dest_target_fname);
  return true;
}

static void showUsage()
{
  printf("\nUsage:\n"
         "  fontgen2-dev <BLK file> [options]\n\n"
         "Options are:\n"
         "  -tga     - generate refernce *.tga files\n"
         "  -xbox360 - generate font for XBOX360 target\n"
         "  -ps3     - generate font for PS3 target\n"
         "  -ps4     - generate font for PS4 target\n"
         "  -iOS     - generate font for iOS target\n"
         "  -and     - generate font for and target\n"
         "  -verbose - output all font generation warnings\n"
         "  -quiet   - do not show message boxes on errors\n"
         "  -dbgfile:<fname>  - output debug to specified filename\n"
         "  -fullDynamic - build BLK for new fully dynamic fonts\n");
}
SimpleString pvrtexOpt;

static void __cdecl ctrl_break_handler(int)
{
  printf("\nInterrupted by user!\n");
  fflush(stdout);
  MutexAutoAcquire::release_mutexes();
  debug_flush(true);
  ExitProcess(13);
}
static void on_shutdown_handler() { MutexAutoAcquire::release_mutexes(); }
static void stdout_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  const char *stk_end = get_multi_line_str_end(call_stack, 4 * 2 + 1, 4096);
  G_ASSERT(stk_end != nullptr);
  printf("%s:  %s\n\n%.*s\n", title, msg, int(stk_end - call_stack), call_stack);
  fflush(stdout);
}

static void build_config_for_fully_dynamic_fonts(const char *dest_fn, dag::Span<FontInfo> fontPack,
  const DataBlock *fallback_fonts_blk, const char *copyto_dir)
{
  DataBlock dynFontBlk;

  for (auto &f : fontPack)
  {
    if (fallback_fonts_blk)
    {
      FreeTypeFont generalFont;
      unicode_range_t missingSym;
      if (!loadFreeTypeFont(f, 10, generalFont))
      {
        logerr("failed to open font: %s", f.file);
        return;
      }
      for (int i = 1, inc = 1; i < missingSym.FULL_SZ; i += inc)
        if (f.include.getIter(i, inc))
          if (!f.exclude.get(i) && generalFont.get_char_index(i) == 0)
            missingSym.set(i);
      for (auto &af : f.additional)
      {
        if (!loadFreeTypeFont(af, 10, generalFont))
        {
          logerr("failed to open font: %s", af.file);
          return;
        }
        for (int i = 1, inc = 1; i < missingSym.FULL_SZ; i += inc)
          if (af.include.getIter(i, inc))
            if (!af.exclude.get(i))
            {
              if (generalFont.get_char_index(i) == 0)
                missingSym.set(i);
              else
                missingSym.clr(i);
            }
      }
      int missing_cnt = count_bits(missingSym);
      printf("\nNOTE: found %d missing unicode chars for '%s'\n", missing_cnt, f.name.str());

      int last_fallback_font_idx = -1;
      auto setup_fallback = [&](const char *af_name, const DataBlock &af_props) {
        unicode_range_t fbIncludeSym, fbExcludeSym;
        bool explicit_ranges_used = false;
        FontInfo::processRanges(af_props, fbIncludeSym);
        FontInfo::processRanges(*af_props.getBlockByNameEx("exclude"), fbExcludeSym);
        if (count_bits(fbIncludeSym))
        {
          explicit_ranges_used = true;
          for (int i = 1, inc = 1; i < fbIncludeSym.FULL_SZ; i += inc)
            if (fbIncludeSym.getIter(i, inc) && (!missingSym.get(i) || fbExcludeSym.get(i)))
              fbIncludeSym.clr(i);
        }
        else
        {
          for (int i = 1, inc = 1; i < missingSym.FULL_SZ; i += inc)
            if (missingSym.getIter(i, inc) && !fbExcludeSym.get(i))
              fbIncludeSym.set(i);
        }
        if (!count_bits(fbIncludeSym))
          return;

        FontInfo &af = f.additional.push_back();
        af.file = af_name;
        af.size = f.size;
        af.include = fbIncludeSym;
        if (!loadFreeTypeFont(af, 10, generalFont))
        {
          logerr("failed to open font: %s", af.file);
          return;
        }
        for (int i = 1, inc = 1; i < missingSym.FULL_SZ; i += inc)
          if (af.include.getIter(i, inc))
          {
            if (generalFont.get_char_index(i) == 0)
              af.include.clr(i);
            else
              missingSym.clr(i), missing_cnt--;
          }
        int subst_cnt = count_bits(af.include);
        if (!subst_cnt)
          f.additional.pop_back();
        else
        {
          printf("NOTE: reusing %d unicode chars from fallback font: %s\n", subst_cnt, af.file);
          if (!explicit_ranges_used)
            last_fallback_font_idx = f.additional.size() - 1;
        }
      };
      dblk::iterate_child_blocks_by_name(*fallback_fonts_blk, "font", [&](const DataBlock &b) {
        if (missing_cnt)
          setup_fallback(b.getStr("file", "?"), b);
      });
      dblk::iterate_params_by_name_and_type(*fallback_fonts_blk, "font", DataBlock::TYPE_STRING, [&](int pidx) {
        if (missing_cnt)
          setup_fallback(fallback_fonts_blk->getStr(pidx), DataBlock::emptyBlock);
      });

      if (missing_cnt && last_fallback_font_idx != -1)
      {
        FontInfo &af = f.additional[last_fallback_font_idx];
        printf("NOTE: '%s': unresolved %d missing unicode chars left; redirecting to last font: %s\n", f.name.str(), missing_cnt,
          af.file);
        for (int i = 1, inc = 1; i < missingSym.FULL_SZ; i += inc)
          if (missingSym.getIter(i, inc))
            af.include.set(i);
      }
      else
        printf("NOTE: '%s': unresolved %d missing unicode chars left\n", f.name.str(), missing_cnt);
    }

    DataBlock b;
    b.setStr("baseFont", f.file);
    if (copyto_dir)
    {
      MutexAutoAcquire mutexAa2(copyto_dir);
      dag_copy_file(f.file, String(0, "%s/%s", copyto_dir, dd_get_fname(f.file)));
    }
    if (!f.forceAutoHinting)
      b.setBool("forceAutoHint", f.forceAutoHinting);

    unicode_range_t usedSym;
    for (auto &af : f.additional)
      for (int i = 0, inc = 1; i < usedSym.FULL_SZ; i += inc)
        if (af.include.getIter(i, inc))
          if (!af.exclude.get(i) && !usedSym.get(i))
            usedSym.set(i);

    Tab<IPoint3> sym_ranges;
    sym_ranges.push_back().set(-1, 0, 0);
    int scnt = 0;
    for (int i = 0, inc = 1; i < usedSym.FULL_SZ; i += inc)
      if (usedSym.getIter(i, inc))
      {
        scnt++;
        for (auto &af : f.additional)
          if (af.include.get(i) && !af.exclude.get(i))
          {
            int idx = &af - f.additional.data();
            if (sym_ranges.back().x == idx && sym_ranges.back().z + 1 == i)
              sym_ranges.back().z++;
            else
              sym_ranges.push_back().set(idx, i, i);
            break;
          }
      }
    usedSym.reset();

    const int CP_SIZE = 256;
    wchar_t cmap[CP_SIZE + 1];
    memset(cmap, 0, sizeof(cmap));
    if (!f.cpfile)
    {
      if (f.remapPrivate)
        for (int i = 0; i < CP_SIZE; i++)
          cmap[i] = 0xF000 + i;
      else
        for (int i = 0; i < CP_SIZE; i++)
          cmap[i] = i;
    }
    else if (strcmp(f.cpfile, "none") == 0)
    {
      for (int i = 0; i < CP_SIZE; i++)
        cmap[i] = 0;
    }
    else
    {
      if (!dd_file_exist(f.cpfile))
      {
        printf("[SKIP] File \"%s\" not exist\n", f.cpfile);
        f.cpfile = "none";
      }

      if (!::parseCodepage(f.cpfile, (uint16_t *)cmap))
      {
        printf("[SKIP] Invalid codepage file %s for %s font\n", f.cpfile, f.name.str());
        f.cpfile = "none";
      }
    }
    if (f.cpfile && strcmp(f.cpfile, "none") != 0)
      printf("WARNING: codepage is not supported for full dynamic fonts (%s, cp=%s)\n", f.name.str(), f.cpfile);

    for (auto &af : f.additional)
    {
      double sz_scale = af.size / f.size;
      if (sz_scale < 0)
      {
        printf("WARNING: mixed font size (%g and %g) in %s, skipped\n", f.size, af.size, f.name.str());
        continue;
      }
      DataBlock &b2 = *b.addNewBlock("addFont");
      b2.setStr("addFont", af.file);
      if (copyto_dir)
      {
        MutexAutoAcquire mutexAa2(copyto_dir);
        dag_copy_file(af.file, String(0, "%s/%s", copyto_dir, dd_get_fname(af.file)));
      }
      b2.setReal("htScale", sz_scale);
      if (!af.forceAutoHinting)
        b2.setBool("forceAutoHint", af.forceAutoHinting);

      for (int i = 0, idx = &af - f.additional.data(); i < sym_ranges.size(); i++)
        if (sym_ranges[i].x == idx)
          b2.addIPoint2("range", IPoint2::yz(sym_ranges[i]));
    }

    for (int i = 0; i < dynFontBlk.blockCount(); i++)
      if (equalDataBlocks(b, *dynFontBlk.getBlock(i)))
      {
        f.extIdxRemap = i;
        break;
      }
    if (f.extIdxRemap == -1)
    {
      f.extIdxRemap = dynFontBlk.blockCount();
      dynFontBlk.addNewBlock(&b, "baseFontData");
    }
  }

  for (auto &f : fontPack)
  {
    DataBlock &b = *dynFontBlk.addNewBlock("font");
    b.setStr("name", f.name);
    b.setInt("baseFontData", f.extIdxRemap);
    if (f.size < 0)
      b.setInt("ht", -f.size);
    else if (!f.hRes.size())
    {
      b.setReal("htRel", f.size);
      if (f.makeHtEven)
        b.setBool("htEven", f.makeHtEven);
    }
  }

  for (auto &f : fontPack)
    for (int hres : f.hRes)
    {
      DataBlock &b = *dynFontBlk.addBlock(String(0, "hRes.%d", hres));
      if (f.size > 0)
      {
        int pix_ht = int(floorf(f.size * hres / 100 + 0.5));
        if (f.makeHtEven)
          pix_ht = int(floorf(f.size * hres / 100 / 2 + 0.5)) * 2;
        b.addBlock(f.name)->setInt("ht", pix_ht);
      }
    }

  dd_mkpath(dest_fn);
  dynFontBlk.saveToTextFile(dest_fn);
  printf("\nFully dynamic: %d font(s) -> %s\n", fontPack.size(), dest_fn);
}

int DagorWinMain(bool debugmode)
{
  if (!::dgs_execute_quiet)
    printf("Dagor Font Generator v3.16 (based on freetype-%d.%d.%d)\n"
           "Copyright (c) Gaijin Games KFT, 2023\n"
           "All rights reserved\n",
      FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);
  ::register_tga_tex_load_factory();
  ::register_jpeg_tex_load_factory();
  ::register_psd_tex_load_factory();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();

  signal(SIGINT, ctrl_break_handler);
  atexit(&MutexAutoAcquire::release_mutexes);
  dgs_pre_shutdown_handler = &on_shutdown_handler;
  if (::dgs_execute_quiet)
    dgs_report_fatal_error = &stdout_report_fatal_error;

  // get options
  if (__argc < 2)
  {
    showUsage();
    return 1;
  }
  const char *filename = __argv[1];
  const char *ext = dd_get_fname_ext(filename);
  if (!ext || strcmp(ext, ".blk") != 0)
  {
    printf("\n[FATAL ERROR] '%s' is not font generator rules BLK\n", filename);
    showUsage();
    return 1;
  }

  bool genTga = false;
  bool verbose = false;
  bool fullDynamic = false;
  int target = _MAKE4C('PC');
  pvrtexOpt = "-fOGL4444 -nt";

  for (int i = 2; i < __argc; i++)
    if (stricmp(__argv[i], "-tga") == 0)
      genTga = true;
    else if (stricmp(__argv[i], "-ps4") == 0)
      target = _MAKE4C('PS4');
    else if (stricmp(__argv[i], "-iOS") == 0)
      target = _MAKE4C('iOS');
    else if (stricmp(__argv[i], "-and") == 0)
      target = _MAKE4C('and');
    else if (strnicmp(__argv[i], "-pvrtex:", 8) == 0)
      pvrtexOpt = __argv[i] + 8;
    else if (stricmp(__argv[i], "-verbose") == 0)
      verbose = true;
    else if (stricmp(__argv[i], "-fullDynamic") == 0)
      fullDynamic = true;
    else if (stricmp(__argv[i], "-?") == 0 || stricmp(__argv[i], "-help") == 0)
    {
      showUsage();
      return 0;
    }
    else if (stricmp(__argv[i], "-noeh") == 0 || stricmp(__argv[i], "/noeh") == 0 || stricmp(__argv[i], "-debug") == 0 ||
             stricmp(__argv[i], "/debug") == 0)
      continue;
    else if (stricmp(__argv[i], "-quiet") == 0)
      continue;
    else if (strnicmp(__argv[i], "-dbgfile:", 9) == 0)
      continue;
    else
    {
      printf("\n[FATAL ERROR] unknown option <%s>\n", __argv[i]);
      return 13;
    }

  // read data block with files
  DataBlock blk;
  if (!blk.load(filename))
  {
    printf("\n[FATAL ERROR] Cannot open BLK file '%s' (file not found or syntax error, see log for details)\n", filename);
    showUsage();
    return 1;
  }
  dag::Vector<FontInfo> fontPack;

  if (blk.getStr("pvrtexOpt", NULL))
    pvrtexOpt = blk.getStr("pvrtexOpt");

  int fontNameId = blk.getNameId("font");
  const char *cp = blk.getStr("codepage", NULL);
  const char *prefix = blk.getStr("prefix", "");
  const char *suffix = blk.getStr("suffix", "");
  const char *masterAtlas = blk.getStr("atlas", NULL);

  float size_multiplier = blk.getReal("sizeMultiplier", 1.0);
  int size_pixels_by_screenheight = blk.getInt("sizeByScreenHeight", 0);
  bool symmEL = blk.getBool("symmetricExtLeading", false);
  DataBlock *resblk = blk.getBlockByName("hRes");
  DataBlock *incblk = blk.getBlockByName("include");
  DataBlock *excblk = blk.getBlockByName("exclude");
  int valve_upscale = blk.getInt("vUpscale", 1);
  sizeRestrBlk = blk.getBlockByNameEx("sizeRestrictions");
  global_generateMissingGlyphBox = blk.getBool("generateMissingGlyphBox", true);

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    DataBlock &fblk = *blk.getBlock(i);

    if (fblk.getBlockNameId() != fontNameId)
      continue;

    FontInfo &f = fontPack.push_back();

    if (resblk)
      f.addHRes(*resblk);
    if (incblk)
      f.makeInclusions(*incblk);
    if (excblk)
      f.makeExclusions(*excblk);
    if (const DataBlock *b = blk.getBlockByName("dynamic"))
      f.markDynamic(*b);

    if (!f.load(fblk, i, prefix, cp, masterAtlas, 0, valve_upscale, symmEL, size_multiplier, size_pixels_by_screenheight, suffix))
      fontPack.pop_back();

    for (int j = 0; j < fblk.blockCount(); ++j)
    {
      DataBlock &addBlk = *fblk.getBlock(j);

      if (addBlk.getBlockNameId() != fontNameId)
        continue;
      FontInfo &addFont = f.additional.push_back();

      addFont.include = f.include;
      addFont.exclude = f.exclude;
      if (!addFont.load(addBlk, i, prefix, cp, masterAtlas, f.size, valve_upscale, symmEL, size_multiplier,
            size_pixels_by_screenheight, suffix))
        f.additional.pop_back();
    }
  }

  if (!fontPack.size())
  {
    printf("\n[FATAL ERROR] no font generator rules in %s\n", filename);
    return 1;
  }

  if (fullDynamic)
  {
    build_config_for_fully_dynamic_fonts(
      String(0, "%s/%s.dynFont.blk", blk.getStr("copyto", prefix), masterAtlas ? masterAtlas : "all"), make_span(fontPack),
      blk.getBlockByName("fallback_fonts"), blk.getStr("copyto", prefix));
    fontPack.clear();
    return 0;
  }

  printf("\nTarget platform: %c%c%c%c\n", _DUMP4C(target));

  Tab<FontInfo *> atlas(tmpmem_ptr());
  int ret = 0;
  for (int i = 0; i < fontPack.size() && ret == 0; i++)
  {
    if (fontPack[i].wasCompiled)
      continue;
    clear_and_shrink(atlas);
    atlas.push_back(&fontPack[i]);
    if (fontPack[i].atlas)
    {
      for (int j = i + 1; j < fontPack.size(); j++)
      {
        if (!fontPack[j].wasCompiled && fontPack[j].atlas && !strcmp(fontPack[i].atlas, fontPack[j].atlas))
        {
          fontPack[j].wasCompiled = true;
          atlas.push_back(&fontPack[j]);
        }
      }
    }

    dynTTFUsed.reset();
    if (atlas[0]->hRes.empty())
    {
      if (!dimCompile(atlas, -1, genTga, target, blk, verbose))
        ret = 1;
    }
    else
      for (int j = 0; j < atlas[0]->hRes.size(); j++)
      {
        dynTTFUsed.reset();
        if (!dimCompile(atlas, j, genTga, target, blk, verbose))
          ret = 1;
      }
  }

  fontPack.clear();
  dynTTFUsed.clear();
  clear_and_shrink(glimg);
  return ret;
}
