// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/dag_materialData.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_platform.h>
#include <generic/dag_staticTab.h>
#include "guiRenderCache.h"
#include "font.h"
#include "stdGuiRenderPrivate.h"
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_direct.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_adjpow2.h>
#include <util/dag_fastStrMap.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdio.h>
#include <wchar.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_critSec.h>
#include <shaders/dag_shaderBlock.h>
#include <gui/dag_guiStartup.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/utility.h>
#include <math/dag_polyUtils.h>
#include <memory/dag_framemem.h>
#include <supp/dag_alloca.h>

#define CVT_TO_UTF16_ON_STACK(tmpU16, s, len)                       \
  if (len < 0)                                                      \
    len = i_strlen(s);                                              \
  else                                                              \
  {                                                                 \
    const char *e = (const char *)memchr(s, 0, len);                \
    if (e)                                                          \
      len = e - s;                                                  \
  }                                                                 \
  if (len > 8000)                                                   \
  {                                                                 \
    LOGERR_CTX("len=%d str=%.512s...", len, s);                     \
    len = 8000;                                                     \
  }                                                                 \
  wchar_t *tmpU16 = (wchar_t *)alloca((len + 1) * sizeof(wchar_t)); \
  {                                                                 \
    int used = utf8_to_wcs_ex(s, len, tmpU16, len + 1);             \
    G_ASSERT(used <= len);                                          \
    tmpU16[used] = 0;                                               \
    len = used;                                                     \
  }

static inline int dag_wcsnlen(const wchar_t *w, int len)
{
  const wchar_t *w0 = w;
  while (*w && len)
    w++, len--;
  return w - w0;
}

//************************************************************************
//* helper math functions
//************************************************************************


// return maximum of n values
template <typename T>
inline T max4(const T &a, const T &b, const T &c, const T &d)
{
  T maximum = max(a, b);
  maximum = max(maximum, c);
  maximum = max(maximum, d);

  return maximum;
}

// return minimum of n values
template <typename T>
inline T min4(const T &a, const T &b, const T &c, const T &d)
{
  T minimum = min(a, b);
  minimum = min(minimum, c);
  minimum = min(minimum, d);

  return minimum;
}

// align in order a <= b
template <typename T>
inline void alignValues(T &a, T &b)
{
  if (a > b)
    eastl::swap(a, b);
}

// align pairs (a, pa) and (b, pb) in order a <= b
template <typename T>
inline void alignValues(T &a, T &b, T &pa, T &pb)
{
  if (a > b)
  {
    eastl::swap(a, b);
    eastl::swap(pa, pb);
  }
}

// align points x1 <= x2, y1 <= y2
inline void alignPoints(Point2 &p1, Point2 &p2)
{
  alignValues(p1.x, p2.x);
  alignValues(p1.y, p2.y);
}

namespace StdGuiRender
{
static DECLSPEC_ALIGN(16) char stdgui_context_buf[sizeof(GuiContext)] ATTRIBUTE_ALIGN(16);
static StdGuiShader stdgui_default_shader;
short dyn_font_atlas_reset_generation = 0;

static DeviceResetFilter device_reset_filter = nullptr;

//************************************************************************
//* rendering parameters
//************************************************************************

static int max_dynbuf_quad_num = 8192;
static int max_extra_indices = 1024;

static Point2 g_screen_size;
static const Point2 correctCord = Point2(0.0f, 0.0f);

static GuiContext &get_stdgui_context_ref() { return *(GuiContext *)stdgui_context_buf; }

GuiContext *get_stdgui_context() { return &get_stdgui_context_ref(); }

#define stdgui_context get_stdgui_context_ref()

template <typename T, size_t N>
using TmpVec = dag::RelocatableFixedVector<T, N, true, framemem_allocator, uint32_t, false>;

//************************************************************************
//* point2 constants
//************************************************************************
namespace P2
{
const Point2 ZERO = Point2(0, 0);
const Point2 ONE = Point2(1, 1);
const Point2 AXIS_X = Point2(1, 0);
const Point2 AXIS_Y = Point2(0, 1);
} // namespace P2


#define FILL_VERTICES  \
  MAKEDATA(0, lt, lt); \
  MAKEDATA(1, rb, lt); \
  MAKEDATA(2, rb, rb); \
  MAKEDATA(3, lt, rb);

//************************************************************************
//* fonts
//************************************************************************

static Tab<DagorFontBinDump> rfont(inimem_ptr());
static FastStrMap font_names;
static WinCritSec font_critsec;


void acquire() { font_critsec.lock(); }
void release() { font_critsec.unlock(); }

// init fonts - get font cfg section & codepage filename (some as T_init())
void init_fonts(const DataBlock &blk)
{
  debug("init fonts");

  // load & init fonts

  int screenWidth, screenHeight;
  d3d::get_screen_size(screenWidth, screenHeight);

  ScopedAcquire l;

  if (const DataBlock *b = blk.getBlockByName("dynamicGen"))
    DagorFontBinDump::initDynFonts(b->getInt("texCount", 1), b->getInt("texSz", 256), b->getStr("prefix", "."));
  DagorFontBinDump::initInscriptions(blk);

  if (blk.paramExists("fontSizeByWidthCap"))
  {
    // triplehead
    if (screenWidth >= screenHeight * 3)
      screenWidth = screenWidth / 3;

    float capMul = blk.getReal("fontSizeByWidthCap", 1.f); // 0.75 makes 4:5 resolutions look better
    if (screenHeight > int(screenWidth * capMul) && int(screenWidth * capMul) >= blk.getInt("minFontSizeByWidthCap", 0))
      screenHeight = int(screenWidth * capMul);
  }

  // get dir
  const DataBlock *binblk = blk.getBlockByName("fontbins");
  if (!binblk)
  {
    DagorFontBinDump::loadFonts(rfont, "gui/system", screenHeight);
    return;
  }
  Tab<const DataBlock *> aliasBlk(tmpmem);
  bool needSomeFont = false;
  for (int i = 0; i < binblk->paramCount(); ++i)
    if (binblk->getParamType(i) == DataBlock::TYPE_STRING)
    {
      if (const DataBlock *fontProps = binblk->getBlockByName(binblk->getStr(i)))
      {
        IPoint2 hResRange = fontProps->getIPoint2("hResRange", IPoint2(0, 16384));
        const DataBlock *addAliases = NULL;
        if (screenHeight < hResRange[0])
          addAliases = fontProps->getBlockByNameEx("aliasForLowerRes");
        else if (screenHeight > hResRange[1])
          addAliases = fontProps->getBlockByNameEx("aliasForHigherRes");
        if (addAliases)
        {
          debug("skip loading %s due to hRes=%d is out of range [%d,%d], will use %d aliases instead", binblk->getStr(i), screenHeight,
            hResRange[0], hResRange[1], addAliases->paramCount());
          if (addAliases->paramCount())
            aliasBlk.push_back(addAliases);
          continue;
        }
      }
      needSomeFont = true;
      String dyn_font_fn(0, "%s.dynFont.blk", binblk->getStr(i));
      if (dd_file_exists(dyn_font_fn))
      {
        DataBlock fblk(dyn_font_fn);
        DagorFontBinDump::loadDynFonts(rfont, fblk, screenHeight);
      }
      else
        DagorFontBinDump::loadFonts(rfont, binblk->getStr(i), screenHeight);
    }

  for (int i = 0; i < rfont.size(); i++)
    if (font_names.addStrId(rfont[i].name, i) != i)
      logerr("duplicate font name \"%s\": id=%d and id=%d(used)", rfont[i].name, i, font_names.getStrId(rfont[i].name));

  aliasBlk.push_back(blk.getBlockByNameEx("alias"));
  for (int j = 0; j < aliasBlk.size(); j++)
  {
    binblk = aliasBlk[j];
    for (int i = 0; i < binblk->paramCount(); ++i)
      if (binblk->getParamType(i) == DataBlock::TYPE_STRING)
      {
        const char *name = binblk->getParamName(i);
        const char *alias = binblk->getStr(i);
        if (get_font_id(name) < 0)
        {
          int alias_id = get_font_id(alias);
          if (alias_id >= 0)
          {
            font_names.addStrId(name, alias_id);
            debug("alias <%s> to font_id=%d (%s)", name, alias_id, alias);
          }
          else
            logerr("cannot alias virtual name <%s> to non-existing font <%s>", name, alias);
        }
      }
    if (j + 1 < aliasBlk.size())
      debug("---");
  }

  if (needSomeFont && rfont.size() == 0)
    DAG_FATAL("No fonts found.");
  debug("init fonts OK");
}

void init_dynamics_fonts(int atlas_count, int atlas_size, const char *path_prefix)
{
  ScopedAcquire fontGuard;
  DagorFontBinDump::initDynFonts(atlas_count, atlas_size, path_prefix);
}

void load_dynamic_font(const DataBlock &font_description, int screen_height)
{
  ScopedAcquire fontGuard;

  int fontCount = rfont.size();
  DagorFontBinDump::loadDynFonts(rfont, font_description, screen_height);

  for (int i = fontCount; i < rfont.size(); i++)
    if (font_names.addStrId(rfont[i].name, i) != i)
      logerr("duplicate font name \"%s\": id=%d and id=%d(used)", rfont[i].name, i, font_names.getStrId(rfont[i].name));
}

// close fonts
void close_fonts()
{
  ScopedAcquire l;
  dyn_font_atlas_reset_generation++;
  for (DagorFontBinDump &f : rfont)
    f.clear();
  clear_and_shrink(rfont);
  font_names.reset();
  DagorFontBinDump::termDynFonts();
  DagorFontBinDump::termInscriptions();
}

// this call should be protected by ScopedAcquire
static inline void update_dyn_fonts()
{
  int64_t reft = profile_ref_ticks();
  bool allow_raster_glyphs = !dgs_gui_may_rasterize_glyphs_cb || dgs_gui_may_rasterize_glyphs_cb();
  bool allow_load_ttfs = !dgs_gui_may_load_ttf_for_rasterization_cb || dgs_gui_may_load_ttf_for_rasterization_cb();

  if (allow_raster_glyphs && allow_load_ttfs)
    debug("reqCharGenChanged");

  bool added_glyph = false;
  for (DagorFontBinDump &f : rfont)
  {
    if (f.updateGen(reft, allow_raster_glyphs, allow_load_ttfs))
      added_glyph = true;
    if (DagorFontBinDump::reqCharGenReset || profile_usec_passed(reft, 100000))
      break;
  }
  for (DagorFontBinDump &f : DagorFontBinDump::add_font)
  {
    if (f.updateGen(reft, allow_raster_glyphs, allow_load_ttfs))
      added_glyph = true;
    if (DagorFontBinDump::reqCharGenReset || profile_usec_passed(reft, 100000))
      break;
  }
  DagorFontBinDump::reqCharGenChanged = profile_usec_passed(reft, 100000);
  if (!allow_raster_glyphs)
    return;

  if (DagorFontBinDump::reqCharGenReset)
  {
    logwarn("out of area, trying to reset font cache");
    dyn_font_atlas_reset_generation++;
    DagorFontBinDump::reqCharGenReset = false;
    for (DagorFontBinDump &f : rfont)
      f.resetGen();
    for (DagorFontBinDump &f : DagorFontBinDump::add_font)
      f.resetGen();
    DagorFontBinDump::resetDynFontsCache();
  }

  if (added_glyph)
    DagorFontBinDump::dumpDynFontUsage();
  if (profile_usec_passed(reft, 20000))
    debug("%s %d ms", __FUNCTION__, profile_time_usec(reft) / 1000);
}

void set_device_reset_filter(DeviceResetFilter callback) { device_reset_filter = callback; }

void after_device_reset()
{
  if (device_reset_filter && !device_reset_filter())
    return;
  ScopedAcquire fontGuard;

  debug("resetting font cache");
  dyn_font_atlas_reset_generation++;
  DagorFontBinDump::reqCharGenReset = false;
  for (DagorFontBinDump &f : rfont)
  {
    f.resetGen();
    f.discard();
  }
  for (DagorFontBinDump &f : DagorFontBinDump::add_font)
  {
    f.resetGen();
    f.discard();
  }
  DagorFontBinDump::resetDynFontsCache();
  DagorFontBinDump::after_device_reset();
}

static bool update_internals_per_act_is_used = false;
void update_internals_per_act()
{
  update_internals_per_act_is_used = true;
  if (DagorFontBinDump::reqCharGenChanged)
  {
    ScopedAcquire fontGuard;
    update_dyn_fonts();
  }
}

static float dynfont_compute_str_width_u_cached(const StdGuiFontContext &fctx, const wchar_t *str, int len, BBox2 &out_bb)
{
#if STRBOXCACHE_DEBUG
  int64_t reft = profile_ref_ticks();
#endif
  unsigned font_id = fctx.font - rfont.data(), font_ht = fctx.fontHt ? fctx.fontHt : fctx.font->getFontHt();
  strboxcache::hash_t hash = strboxcache::build_hash(str, len, font_id, font_ht, fctx.spacing);
  uint32_t hash2 = strboxcache::build_hash2_fnv1a(str, len, font_id, font_ht, fctx.spacing);

  if (const BBox2 *b = strboxcache::find_bbox(hash, len == 1 ? strboxcache::H2I_CHAR : strboxcache::H2I_STR, hash2))
  {
    out_bb = *b;
#if STRBOXCACHE_DEBUG
    strboxcache::cacheTimeTicks += profile_ref_ticks() - reft;
#endif
    return out_bb[1].x;
  }

#if STRBOXCACHE_DEBUG
  strboxcache::calcCount++;
  int64_t reft2 = profile_ref_ticks();
#endif
  float dx = fctx.font->dynfont_compute_str_width_u(fctx.fontHt, str, len, 1.0f, fctx.spacing, out_bb);
#if STRBOXCACHE_DEBUG
  int calc_time = profile_ref_ticks() - reft2;
  strboxcache::calcTimeTicks += calc_time;
#endif
  strboxcache::add_bbox(hash, len == 1 ? strboxcache::H2I_CHAR : strboxcache::H2I_STR, out_bb, hash2);
#if STRBOXCACHE_DEBUG
  strboxcache::cacheTimeTicks += profile_ref_ticks() - reft - calc_time;
#endif
  return dx;
}

static BBox2 get_char_bboxS(DagorFontBinDump *font, const DagorFontBinDump::GlyphData &g)
{
  BBox2 box;
  font->getGlyphBounds(g, box[0].x, box[0].y, box[1].x, box[1].y);
  return box;
}

static BBox2 get_char_vis_bboxS(DagorFontBinDump *font, const DagorFontBinDump::GlyphData &g)
{
  BBox2 box;

  if (g.texIdx < font->tex.size() || g.isDynGrpValid())
    font->getGlyphVisBounds(g, box[0].x, box[0].y, box[1].x, box[1].y);
  else
    font->getGlyphBounds(g, box[0].x, box[0].y, box[1].x, box[1].y);

  return box;
}

float get_char_width_u(wchar_t ch, const StdGuiFontContext &fctx)
{
  if (fctx.monoW)
    return fctx.monoW + fctx.spacing;
  if (!fctx.font->isFullyDynamicFont())
    return fctx.font->getGlyphU(ch).dx + fctx.spacing;

  BBox2 box;
  return dynfont_compute_str_width_u_cached(fctx, &ch, 1, box);
}

// get character bounding box
BBox2 get_char_bbox_u(wchar_t c, const StdGuiFontContext &fctx)
{
  if (!fctx.font->isFullyDynamicFont())
    return get_char_bboxS(fctx.font, fctx.font->getGlyphU(c));

  BBox2 box;
  dynfont_compute_str_width_u_cached(fctx, &c, 1, box);
  return box;
}

// get character bounding box
BBox2 get_char_visible_bbox_u(wchar_t c, const StdGuiFontContext &fctx)
{
  if (!fctx.font->isFullyDynamicFont())
    return get_char_vis_bboxS(fctx.font, fctx.font->getGlyphU(c));

  BBox2 box;
  fctx.font->dynfont_compute_str_visbox_u(fctx.fontHt, &c, 1, 1.0f, fctx.spacing, box);
  return box;
}


// get string bounding box
BBox2 get_str_bbox_u(const wchar_t *str, int len, const StdGuiFontContext &fctx)
{
  G_ASSERT(fctx.font);

  if (!str)
    return BBox2(0, 0, 0, 0);

  BBox2 box;
  if (fctx.monoW)
  {
    len = dag_wcsnlen(str, len);
    box[0].set(0, 0);
    box[1].set(len * fctx.monoW, 0);
  }
  if (fctx.font->isFullyDynamicFont())
  {
    if (fctx.monoW)
      fctx.font->dynfont_update_bbox_y(fctx.fontHt, box, false);
    else
      dynfont_compute_str_width_u_cached(fctx, str, len, box);
    return box;
  }

  Point2 pos = P2::ZERO;
  BBox2 b;
  while (*str && len)
  {
    const DagorFontBinDump::GlyphData &g = fctx.font->getGlyphU(*str);
    int dx2 = 0;
    pos.x += fctx.font->getDx2(dx2, fctx.monoW, g, str[1]);

    b = get_char_bboxS(fctx.font, g);
    b[0] += pos;
    b[1] += pos;
    box += b;

    pos.x += dx2 + fctx.spacing;

    ++str;
    len--;
  }

  return box;
}

BBox2 get_str_bbox_u_ex(const wchar_t *str, int len, const StdGuiFontContext &fctx, float max_w, bool left_align, int &out_start_idx,
  int &out_count, const wchar_t *break_sym, float ellipsis_resv_w)
{
  G_ASSERT(fctx.font);
  if (len < 0)
    len = str ? wcslen(str) : 0;

  out_start_idx = out_count = 0;
  if (!str || !len)
    return BBox2(0, 0, 0, 0);

  BBox2 box;
  if (fctx.font->isFullyDynamicFont())
  {
    if (fctx.monoW)
      len = dag_wcsnlen(str, len);
    return fctx.font->dynfont_compute_str_width_u_ex(fctx.fontHt, str, len, 1.0f, fctx.spacing, fctx.monoW, max_w, left_align,
      break_sym, ellipsis_resv_w, out_start_idx, out_count);
  }

  BBox2 full_bb = get_str_bbox_u(str, len, fctx);
  if (full_bb.width().x <= max_w)
  {
    out_count = len;
    return full_bb;
  }

  const wchar_t *orig_str = str;
  int orig_len = len;

  max_w -= ellipsis_resv_w;
  if (!left_align)
    max_w = full_bb.width().x - max_w;

  Point2 pos = P2::ZERO;
  BBox2 b, word_box;
  while (*str && len)
  {
    const DagorFontBinDump::GlyphData &g = fctx.font->getGlyphU(*str);
    int dx2 = 0;
    pos.x += fctx.font->getDx2(dx2, fctx.monoW, g, str[1]);

    b = get_char_bboxS(fctx.font, g);
    inplace_max(b[1].x, b[0].x + fctx.monoW);
    b[0] += pos;
    b[1] += pos;

    BBox2 nbox(box);
    nbox += b;
    int new_w = nbox.width().x;
    bool cur_brk = break_sym && wcschr(break_sym, *str);
    if (new_w > max_w && (left_align || cur_brk || !break_sym))
    {
      if (!left_align)
        out_count = str - orig_str + 1;
      break;
    }

    box = nbox;
    pos.x += dx2 + fctx.spacing;

    bool prev_brk = break_sym && wcschr(break_sym, *(str - 1));
    ++str;
    len--;
    if (!break_sym || (left_align && cur_brk && !prev_brk) || (!left_align && !cur_brk && prev_brk))
    {
      out_count = str - orig_str;
      if (break_sym)
        word_box = box;
    }
  }

  if (!left_align)
  {
    out_start_idx = out_count;
    out_count = orig_len - out_count;
    return get_str_bbox_u(orig_str + out_start_idx, out_count, fctx);
  }
  if (break_sym)
    box = word_box;
  return box;
}

BBox2 get_str_bbox(const char *str, int len, const StdGuiFontContext &fctx)
{
  G_ASSERT(fctx.font);

  if (!str)
    return BBox2(0, 0, 0, 0);

  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  replace_tabs_with_zwspaces(tmpU16);
  return get_str_bbox_u(tmpU16, len, fctx);
}


bool get_font_context(StdGuiFontContext &ctx_out, int font_id, int font_spacing, int mono_w, int ht)
{
  ctx_out.spacing = font_spacing;
  ctx_out.monoW = mono_w;
  if (font_id >= 0 && font_id < rfont.size())
  {
    ctx_out.font = &rfont[font_id];
    ctx_out.font->fetch();
    ctx_out.fontHt = ctx_out.font->isFullyDynamicFont() ? ht : 0;
    return true;
  }
  ctx_out.font = NULL;
  return false;
}

// get font name; return empty string, if failed
const char *get_font_name(int font_id)
{
  if (font_id < 0 || font_id >= rfont.size())
    return NULL;

  return rfont[font_id].name;
}

bool is_font_dynamic(int font_id)
{
  DagorFontBinDump *font = get_font(font_id);
  return font && font->isResDependent();
}

#define RETURN_FONT_MX(MX, MX_LEGACY)                       \
  if (fctx.font)                                            \
  {                                                         \
    if (const auto *mx = fctx.font->getFontMx(fctx.fontHt)) \
      return mx->MX;                                        \
    return fctx.font->MX_LEGACY;                            \
  }                                                         \
  return 0

#define RETURN_FONT_MX1(MX) RETURN_FONT_MX(MX, MX)

Point2 get_font_cell_size(const StdGuiFontContext &fctx)
{
  if (fctx.font)
  {
    if (const auto *mx = fctx.font->getFontMx(fctx.fontHt))
      return Point2(mx->spaceWd, mx->ascent);
    const DagorFontBinDump::GlyphData &g = fctx.font->getGlyphU(' ');
    return Point2(g.dx, fctx.font->ascent);
  }
  return Point2(1, 1);
}

int get_font_caps_ht(const StdGuiFontContext &fctx) { RETURN_FONT_MX(capsHt, getCapsHt()); }
real get_font_line_spacing(const StdGuiFontContext &fctx) { RETURN_FONT_MX1(lineSpacing); }
int get_font_ascent(const StdGuiFontContext &fctx) { RETURN_FONT_MX1(ascent); }
int get_font_descent(const StdGuiFontContext &fctx) { RETURN_FONT_MX1(descent); }

#undef RETURN_FONT_MX1
#undef RETURN_FONT_MX

dag::ConstSpan<FontEntry> get_fonts_list()
{
  G_STATIC_ASSERT(sizeof(FontEntry) == sizeof(FastStrMap::Entry));
  G_STATIC_ASSERT(sizeof(FontEntry::fontName) == sizeof(FastStrMap::Entry::name));
  G_STATIC_ASSERT(sizeof(FontEntry::fontId) == sizeof(FastStrMap::Entry::id));
  G_STATIC_ASSERT(offsetof(FontEntry, fontName) == offsetof(FastStrMap::Entry, name));
  G_STATIC_ASSERT(offsetof(FontEntry, fontId) == offsetof(FastStrMap::Entry, id));
  return dag::ConstSpan<FontEntry>((const FontEntry *)font_names.getMapRaw().data(), font_names.getMapRaw().size());
}

int get_font_id(const char *font_name) { return font_names.getStrId(font_name); }

DagorFontBinDump *get_font(int font_id)
{
  if ((uint32_t)font_id >= (uint32_t)rfont.size())
    return NULL;

  rfont[font_id].fetch();
  return &rfont[font_id];
}

//************************************************************************
//* renderer
//************************************************************************

void LayerParams::reset()
{
  texInLinear = false;
  texId = texId2 = BAD_TEXTUREID;
  fontTexId = BAD_TEXTUREID;
  alphaBlend = NO_BLEND;
  maskTexId = BAD_TEXTUREID;
  texSampler = d3d::INVALID_SAMPLER_HANDLE;
  tex2Sampler = d3d::INVALID_SAMPLER_HANDLE;
  fontTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  maskTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  maskTransform0 = Point3(1, 0, 0);
  maskTransform1 = Point3(0, 1, 0);

  ff.reset();

  fontFx = false;
  colorM1 = 1;
  memcpy(colorM, TMatrix4::IDENT.m, sizeof(colorM));
}

bool LayerParams::cmpEq(const LayerParams &other) const
{
  if (fontFx != other.fontFx)
    return false;
  if ((maskTexId != other.maskTexId || maskTexSampler != other.maskTexSampler) ||
      (maskTexId != BAD_TEXTUREID && (maskTransform0 != other.maskTransform0 || maskTransform1 != other.maskTransform1)))
    return false;

  if (colorM1 != other.colorM1 || (!colorM1 && memcmp(colorM, other.colorM, sizeof(colorM)) != 0))
    return false;

  if (!fontFx)
  {
    return (texId == other.texId && texId2 == other.texId2) && (fontTexId == other.fontTexId) && (alphaBlend == other.alphaBlend) &&
           (texSampler == other.texSampler) && (tex2Sampler == other.tex2Sampler) && (fontTexSampler == other.fontTexSampler) &&
           (texInLinear == other.texInLinear);
  }
  return fontTexId == other.fontTexId && fontTexSampler == other.fontTexSampler && memcmp(&ff, &other.ff, sizeof(ff)) == 0;
}

#if LOG_CACHE
void LayerParams::dump() const
{
  debug("lp ff:{%d %08x %d (%d,%d)} ab:%d, ti:%d/%d fti:%d fx:%d", ff.type, ff.col, ff.factor_x32, ff.ofsX, ff.ofsY, alphaBlend, texId,
    texId2, fontTexId, int(fontFx));
};

void FontFxAttr::dump() const
{
  debug("fa fi:%d tex2:%d (%d %d) %d (%d, %d)", fontTex, tex2, tex2su_x32, tex2sv_x32, tex2bv_ofs, tex2su, tex2sv);
};
#endif

void FontFxAttr::reset()
{
  fontTex = BAD_TEXTUREID;
  fontTexSampler = d3d::INVALID_SAMPLER_HANDLE;

  tex2 = BAD_TEXTUREID;
  tex2Sampler = d3d::INVALID_SAMPLER_HANDLE;
  tex2su_x32 = 32;
  tex2sv_x32 = 32;
  tex2bv_ofs = 0;
  tex2su = 0.0f;
  tex2sv = 0.0f;

  writeToZ = 0;
  testDepth = 0;
}

GuiShader::GuiShader() : inited(false)
{
  material = NULL;
  element = NULL;
}

bool GuiShader::init(const char *shader_name, bool do_fatal)
{
  G_ASSERT(!inited);

  Ptr<ShaderMaterial> shMat = new_shader_material_by_name_optional(shader_name);

  if (shMat == NULL)
  {
    if (do_fatal)
      DAG_FATAL("StdGuiRender - shader '%s' not found!", shader_name);
    return false;
  }

  material = shMat;
  element = shMat->make_elem();

  link();

  inited = true;
  return true;
}

void GuiShader::close()
{
  if (!inited)
    return;

  inited = false;
  element = NULL;
  material = NULL;
}
static ShaderVariableInfo maskMatrixLine0VarId("mask_matrix_line_0", true), maskMatrixLine1VarId("mask_matrix_line_1", true),
  viewportRectId("viewportRect"), textureVarId("tex"), textureSamplerVarId("tex_samplerstate"), textureSdrVarId("texSdr", true),
  textureSdrSamplerVarId("texSdr_samplerstate", true), alphaVarId("transparent"), enableTextureId("hasTexture"),
  linearSourceVarId("linearSource", true),

  fontFxTypeId("fontFxType"), fontFxOfsId("fontFxOfs"), fontFxColId("fontFxColor"), fontFxScaleId("fontFxScale"),

  fontTex2Id("fontTex2"), fontTex2SamplerId("fontTex2_samplerstate"), fontTex2ofsId("fontTex2ofs"),
  fontTex2rotCCSmSId("fontTex2rotCCSmS"),

  useColorMatrixId("useColorMatrix", true), colorMatrix0VarId("colorMatrix0", true), colorMatrix1VarId("colorMatrix1", true),
  colorMatrix2VarId("colorMatrix2", true), colorMatrix3VarId("colorMatrix3", true);

static ShaderVariableInfo guiTextDepthVarId("gui_text_depth", true), writeToZVarId("gui_write_to_z", true),
  testDepthVarId("gui_test_depth", true), maskTexVarId("mask_tex", true), maskTexSamplerVarId("mask_tex_samplerstate", true);
static int gui_user_const_ps_no = -1;
static int gui_user_const_vs_no = -1;

static const CompiledShaderChannelId stdgui_channels[] = {{SCTYPE_SHORT2, SCUSAGE_POS, 0, 0}, {SCTYPE_E3DCOLOR, SCUSAGE_VCOL, 0, 0},
  {SCTYPE_SHORT2, SCUSAGE_TC, 0, 0}, {SCTYPE_SHORT2, SCUSAGE_TC, 1, 0}};

void StdGuiShader::channels(const CompiledShaderChannelId *&channels, int &num_channels)
{
  channels = stdgui_channels;
  num_channels = countof(stdgui_channels);
}

void StdGuiShader::ExtStateLocal::setFontTex2Ofs(GuiVertexTransform &xform, float su, float sv, float x0, float y0)
{
  G_STATIC_ASSERT(sizeof(*this) <= sizeof(ExtState));

  fontTex2rotCCSmS = Color4(1, 1, 0, 0);
  if (bool(fontTex2rotCCSmSId)) // -V1051
  {
    fontTex2ofs = Color4(su, sv, -xform.transformComponent(x0, y0, 0), -xform.transformComponent(x0, y0, 1));
  }
  else
    fontTex2ofs = Color4(su, sv, -xform.transformComponent(x0, y0, 0) * su, -xform.transformComponent(x0, y0, 1) * sv);
}

void StdGuiShader::link()
{
  int gui_user_const_ps_noVarId = get_shader_variable_id("gui_user_const_ps_no", true);
  if (VariableMap::isGlobVariablePresent(gui_user_const_ps_noVarId))
    gui_user_const_ps_no = ShaderGlobal::get_int_fast(gui_user_const_ps_noVarId);
  else
    gui_user_const_ps_no = -1;

  int gui_user_const_vs_noVarId = get_shader_variable_id("gui_user_const_vs_no", true);
  if (VariableMap::isGlobVariablePresent(gui_user_const_vs_noVarId))
    gui_user_const_vs_no = ShaderGlobal::get_int_fast(gui_user_const_vs_noVarId);
  else
    gui_user_const_vs_no = -1;
}
void StdGuiShader::cleanup()
{
  if (!material)
    return;
  ShaderGlobal::set_texture(textureVarId, BAD_TEXTUREID);
  ShaderGlobal::set_sampler(textureSamplerVarId, d3d::INVALID_SAMPLER_HANDLE);
  ShaderGlobal::set_texture(textureSdrVarId, BAD_TEXTUREID);
  ShaderGlobal::set_sampler(textureSdrSamplerVarId, d3d::INVALID_SAMPLER_HANDLE);
  ShaderGlobal::set_texture(fontTex2Id, BAD_TEXTUREID);
  ShaderGlobal::set_sampler(fontTex2SamplerId, d3d::INVALID_SAMPLER_HANDLE);
}

void StdGuiShader::setStates(const float viewport[4], const GuiState &guiState, const ExtState *extState, bool viewport_changed,
  bool /*guistate_changed*/, bool /*extstate_changed*/, int targetW, int targetH)
{
  TIME_PROFILE_UNIQUE_DEV;
  G_ASSERT(element);

  const FontFxAttr &fontAttr = guiState.fontAttr;
  const LayerParams &params = guiState.params;
  const ExtStateLocal &est = *(const ExtStateLocal *)extState;

  if (viewport_changed) // fixme: it should be if targetSize has changed, i.e. never happen
  {
    G_UNUSED(targetW);
    G_UNUSED(targetH);
    // switched off because of tactical map clipping
    /*ShaderGlobal::set_color4(viewportRectId,
           Color4( 2.0/targetW/GUI_POS_SCALE,
                  (-2.0*1.0f)/targetH/GUI_POS_SCALE,
                  -1.0, 1.0f));*/
    int w = viewport[2];
    int h = viewport[3];
    ShaderGlobal::set_color4(viewportRectId,
      Color4(w > 0. ? 2.0 / w / GUI_POS_SCALE : 0, h > 0. ? -(1.0f * 2.0) / h / GUI_POS_SCALE : 0,
        w > 0. ? -2.0 * viewport[0] / w - 1.0 : 0, h > 0. ? 1.0f * (2.0 * viewport[1] / h + 1.0) : 0));
  }

  if (fontAttr.getTex2() != BAD_TEXTUREID)
  {
    if (bool(fontTex2rotCCSmSId))
      ShaderGlobal::set_color4(fontTex2rotCCSmSId, est.fontTex2rotCCSmS);
    ShaderGlobal::set_color4(fontTex2ofsId, est.fontTex2ofs);
  }

  // if (extstate_changed)
  if (gui_user_const_ps_no >= 0)
    d3d::set_ps_const(gui_user_const_ps_no, &est.user[0].r, sizeof(est.user) / sizeof(Color4));

  if (gui_user_const_vs_no >= 0)
    d3d::set_vs_const(gui_user_const_vs_no, &est.user[0].r, sizeof(est.user) / sizeof(Color4));

  ShaderGlobal::set_texture(fontTex2Id, fontAttr.getTex2());
  ShaderGlobal::set_sampler(fontTex2SamplerId, fontAttr.getTex2Sampler());

  ShaderGlobal::set_int(alphaVarId, (int)params.alphaBlend);

  if (params.fontFx)
  {
    if (lastFontFxTexture != params.getFontTexId() || !lastFontFxTW)
    {
      TextureInfo ti;
      if (Texture *tex = (Texture *)acquire_managed_tex(params.getFontTexId()))
      {
        tex->getinfo(ti);
        release_managed_tex(lastFontFxTexture = params.getFontTexId());
      }
      lastFontFxTW = ti.w;
      lastFontFxTH = ti.h;
    }
#if LOG_DRAWS
    guiState.params.dump();
    guiState.fontAttr.dump();
#endif
    ShaderGlobal::set_int(fontFxTypeId, params.ff.type);
    ShaderGlobal::set_real(fontFxScaleId, params.ff.factor_x32 / 32.0);
    ShaderGlobal::set_color4(fontFxColId, color4(params.ff.col));
    ShaderGlobal::set_color4(fontFxOfsId, Color4(params.ff.ofsX, params.ff.ofsY, 1.0 / lastFontFxTW, 1.0 / lastFontFxTH));
  }
  else
    ShaderGlobal::set_int(fontFxTypeId, FFT_NONE);

  if (params.getFontTexId() != BAD_TEXTUREID)
  {
    ShaderGlobal::set_int(enableTextureId, 2);
    ShaderGlobal::set_texture(textureVarId, params.getFontTexId());
    ShaderGlobal::set_sampler(textureSamplerVarId, params.getFontTexSampler());
    ShaderGlobal::set_int(linearSourceVarId, (int)params.texInLinear);
  }
  else if (params.getTexId() != BAD_TEXTUREID)
  {
    ShaderGlobal::set_int(enableTextureId, params.getTexId2() == BAD_TEXTUREID ? 1 : 4); // 4 - two textures
    ShaderGlobal::set_texture(textureVarId, params.getTexId());
    ShaderGlobal::set_sampler(textureSamplerVarId, params.getTexSampler());
    ShaderGlobal::set_texture(textureSdrVarId, params.getTexId2());
    ShaderGlobal::set_sampler(textureSdrSamplerVarId, params.getTex2Sampler());
    ShaderGlobal::set_int(linearSourceVarId, (int)params.texInLinear);
  }
  else
  {
    ShaderGlobal::set_int(enableTextureId, 0);
    ShaderGlobal::set_int(linearSourceVarId, 0);
  }

  if (params.colorM1)
    ShaderGlobal::set_int(useColorMatrixId, 0);
  else
  {
    ShaderGlobal::set_int(useColorMatrixId, 1);
    ShaderGlobal::set_color4(colorMatrix0VarId, Color4(&params.colorM[0]));
    ShaderGlobal::set_color4(colorMatrix1VarId, Color4(&params.colorM[4]));
    ShaderGlobal::set_color4(colorMatrix2VarId, Color4(&params.colorM[8]));
    ShaderGlobal::set_color4(colorMatrix3VarId, Color4(&params.colorM[12]));
  }

  ShaderGlobal::set_color4(guiTextDepthVarId, 1.0f, 1.0f, 0.0f, 0.0f);
  ShaderGlobal::set_int(writeToZVarId, fontAttr.writeToZ);
  ShaderGlobal::set_int(testDepthVarId, fontAttr.testDepth);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_OBJECT, false);

  if (bool(maskTexVarId))
  {
    ShaderGlobal::set_texture(maskTexVarId, params.getMaskTexId());
    ShaderGlobal::set_sampler(maskTexSamplerVarId, params.getMaskTexSampler());
    if (params.getMaskTexId() != BAD_TEXTUREID)
    {
      ShaderGlobal::set_color4(maskMatrixLine0VarId, P3D(params.maskTransform0), 0);
      ShaderGlobal::set_color4(maskMatrixLine1VarId, P3D(params.maskTransform1), 0);
    }
  }
  TIME_PROFILE_UNIQUE_EVENT_DEV("GuiSetStates");
  element->setStates(0, true);
}

void init_dynamic_buffers(int quad_num, int extra_indices)
{
  max_dynbuf_quad_num = quad_num;
  max_extra_indices = extra_indices;
}

// init rendering - must be called after shaders initilization
void init_render()
{
  debug("init_render");

  stdgui_default_shader.init("gui_default");

  StdGuiRender::GuiContext &ctx = stdgui_context;
  new (&ctx, _NEW_INPLACE) GuiContext();
  bool res = ctx.createBuffer(0, &stdgui_default_shader, max_dynbuf_quad_num, max_extra_indices, "stdgui.buf");
  if (!res)
    DAG_FATAL("Can't init stdgui");

  ctx.setTarget();
  debug("StdGuiRender::init_render: phys res %dx%d == log res", ctx.screenWidth, ctx.screenHeight);

  int w, h;
  d3d::get_screen_size(w, h);
  g_screen_size = Point2(w, h);
}

// close renderer
void close_render()
{
  stdgui_context.close();
  stdgui_default_shader.close();
  stdgui_context.~GuiContext();
  memset(stdgui_context_buf, 0, sizeof(stdgui_context_buf));
}

struct RecorderCallback
{
  SmallTab<GuiVertex> &qv;
  SmallTab<uint16_t> &texQCnt;
  SmallTab<d3d::SamplerHandle> &texSamplers;
  int missingGlyphs = 0;
  bool checkVis = false;
  RecorderCallback(SmallTab<GuiVertex> &_qv, SmallTab<uint16_t> &_tex_qcnt, SmallTab<d3d::SamplerHandle> &smp, bool cv) :
    qv(_qv), texQCnt(_tex_qcnt), texSamplers(smp), checkVis(cv)
  {
    qv.resize(0);
    texQCnt.resize(0);
    smp.resize(0);
  }

  void *qAlloc(int num)
  {
    int idx = qv.size();
    qv.resize(idx + 4 * num);
    G_ASSERT(texQCnt.size() > 1);
    texQCnt.back()++;
    return &qv[idx];
  }
  void setTex(TEXTUREID tex_id, d3d::SamplerHandle smp)
  {
    if (texQCnt.size() < 2 || texQCnt[texQCnt.size() - 2] != tex_id.index())
    {
      G_ASSERTF(tex_id != BAD_TEXTUREID && tex_id.index() < 0x10000, "tex_idx=0x%x", tex_id);
      texQCnt.push_back(tex_id.index());
      texQCnt.push_back(0);
      texSamplers.push_back(smp);
    }
  }
};

GuiContext::GuiContext() : viewportList(midmem)
{
  renderer = new BufferedRenderer();
  recCb = NULL;
  setExtStateLocation(0, &extStateStorage);

  screenWidth = 0;
  screenHeight = 0;

  prevTextTextureId = BAD_TEXTUREID;
  halfFontTexelX = halfFontTexelY = 0.5 / 1024.f;
  isCurrentFontTexturePow2 = true;

  viewX = 0;
  viewY = 0;
  viewW = 0;
  viewH = 0;
  viewN = 0;
  viewF = 0;

  screenSize = Point2(0, 0);
  screenScale = P2::ONE;
  screenScaleRcp = P2::ONE;

  qCacheUsed = 0;
  qCachePushed = 0;
  qCacheTotal = 0;
  qCacheStride = 1;

  currentChunk = -1;

  defaultState();

  rollState = ROLL_ALL_STATES;

  isInRender = false;
}

GuiContext::~GuiContext() { close(); }

void GuiContext::close()
{
  isInRender = false;
  setBuffer(-1);
  currentShader = NULL;
  del_it(renderer);
}

void GuiContext::defaultState()
{
  vertexTransform.resetViewTm();
  calcInverseVertexTransform();
  guiState.reset();
  extState().reset();

  qCacheUsed = 0;
  qCachePushed = 0;
  rollState = 0;

  drawRawLayer = false;
  currentShader = NULL;
  currentBufferId = -1;
  currentFontId = -1;
  curRenderFont = StdGuiFontContext();
  curQuadMask = 0x00010001;
  currentTextureAlpha = false;
  currentTextureFont = false;

  currentPos = Point2(0, 0);
  currentColor = E3DCOLOR(255, 255, 255, 255);
}

void GuiContext::resetFrame()
{
#if LOG_DRAWS | LOG_CACHE
  debug("STDG:resetFrame");
#endif
  G_ASSERT(!isInRender);
  renderer->resetBuffers();

  defaultState();

  rollState = ROLL_ALL_STATES;
}

bool GuiContext::createBuffer(int id, GuiShader *shader, int num_quads, int extra_indices, const char *name)
{
  if (shader == NULL)
    shader = &stdgui_default_shader;

  static const int64_t maxQuads = (1LL << (sizeof(IndexType) * 8)) / 4;
  if (num_quads > maxQuads)
  {
    renderer->defaultShaders[id] = NULL;
    debug("[E] GuiContext::createBuffer() too much quads %d / %d", num_quads, maxQuads);
    return false;
  }

  int num_chans;
  const CompiledShaderChannelId *chans;
  shader->channels(chans, num_chans);

  bool res = renderer->createBuffer(id, chans, num_chans, num_quads, extra_indices, name);
  renderer->defaultShaders[id] = res ? shader : NULL;
  return res;
}

void GuiContext::setExtStateLocation(int buffer_id, ExtState *state) { renderer->extStatePtrs[buffer_id] = state; }

void GuiContext::setRenderCallback(RenderCallBack cb, uintptr_t data)
{
  renderer->callback = cb;
  renderer->callbackData = data;
}

void GuiContext::execCommand(int command, Point2 pos, Point2 size)
{
  flushData();
  renderer->execCommand(command, pos, size);
  flushData();
}

void GuiContext::execCommand(int command, Point2 pos, Point2 size, RenderCallBack cb, uintptr_t data)
{
  flushData();
  renderer->execCommand(command, pos, size, cb, data);
  flushData();
}

void GuiContext::execCommand(int command)
{
  Point2 zero = Point2(0, 0);
  execCommand(command, zero, zero);
}

uintptr_t GuiContext::execCommandImmediate(int command, Point2 pos, Point2 size)
{
  return renderer->execCommandImmediate(command, pos, size);
}

void GuiContext::setBuffer(int buffer_id, bool force)
{
  if (currentBufferId == buffer_id && !force)
    return;

  // TODO: ignore last states
  flushData();

  reset_draw_str_attr();
  reset_draw_str_texture();

  if (buffer_id < 0)
  {
    currentBufferId = -1;
    currentShader = NULL;
    return;
  }

  currentBufferId = buffer_id;
  currentShader = renderer->currentShaders[buffer_id];
  if (currentShader == NULL)
    currentShader = renderer->defaultShaders[buffer_id];
  G_ASSERT(currentShader);

  // guiState.params.reset();
  guiState.reset();
  currentTextureAlpha = false;
  currentTextureFont = false;
  set_color(currentColor);

  rollState |= ROLL_GUI_STATE | ROLL_EXT_STATE;

  uint32_t stride = renderer->buffers[buffer_id].stride;
  qCacheStride = stride;
  qCacheTotal = uint8_t(sizeof(qCacheBuffer) / (stride * 4));

  renderer->setBufAndShader(currentBufferId, currentShader);
}

void GuiContext::setShaderToBuffer(int buffer_id, GuiShader *shader)
{
  G_ASSERT(buffer_id >= 0);

  if (!shader)
    shader = renderer->defaultShaders[buffer_id];
  G_ASSERT(shader != NULL);

  if (renderer->currentShaders[buffer_id] != shader)
  {
    renderer->validateShader(buffer_id, shader);
    renderer->currentShaders[buffer_id] = shader;
    // update buffer if active
    if (currentBufferId == buffer_id)
      setBuffer(buffer_id, true);
  }
}

void *GuiContext::qCacheAlloc(int num)
{
  if (recCb)
    return recCb->qAlloc(num);
  G_ASSERT(num <= qCacheCapacity());
  if (rollState || (qCacheUsed + num) > qCacheCapacity())
    qCacheFlush(false);
  size_t offset = qCacheUsed * qCacheStride;
  qCacheUsed += num;
  return (void *)&qCacheBuffer[offset * 4];
}

// flush, qcache, layers, and update states
void GuiContext::qCacheFlush(bool force)
{
  TIME_PROFILE_UNIQUE_DEV;
  if (!currentViewPort.isZero() && qCacheUsed > 0)
  {
    // delayed sorted draw
    if (!drawRawLayer && currentBufferId == 0)
      renderer->quadCache.addQuads((const GuiVertex *)qCacheBuffer, qCacheUsed);
    else // just copy data (quadCache is flushed already)
    {
#if LOG_DRAWS
      debug("STDG:qCcopyQuads %d + %d", qCachePushed, qCacheUsed);
#endif
      if (renderer->copyQuads(qCacheBuffer, qCacheUsed))
        qCachePushed += qCacheUsed;
    }
  }

  if (rollState | uint8_t(force))
  {
    if (qCachePushed > 0)
    {
#if LOG_DRAWS
      debug("STDG:qCdrawQuads %d", qCachePushed);
#endif
      renderer->drawQuads(qCachePushed);
      qCachePushed = 0;
    }

    if (force && currentBufferId == 0)
      renderer->quadCache.flush();

    rollStateBlock();
  }

  qCacheUsed = 0;
}

//************************************************************************
//* viewports
//************************************************************************

void GuiContext::applyViewport(GuiViewPort &vp)
{
  if (vp.isNull)
  {
    preTransformViewport.setempty();
    return;
  }

  // DEBUG_CTX("%p.applyViewport(%.0f,%.0f, %.0fx%.0f)", this, vp.leftTop.x, vp.leftTop.y, vp.getWidth(), vp.getHeight());

  flushData();

  viewX = vp.leftTop.x;
  viewY = vp.leftTop.y;
  viewW = vp.getWidth();
  viewH = vp.getHeight();
  viewN = 0.0f;
  viewF = 1.0f;

  renderer->pushViewportRect(vp.leftTop.x, vp.leftTop.y, vp.getWidth(), vp.getHeight());
  vp.applied = true;

  calcPreTransformViewport(vp.leftTop, vp.rightBottom);
}

// set viewport (add it to stack)
void GuiContext::set_viewport(const GuiViewPort &rt)
{
#if LOG_DRAWS
  debug("viewport+ %d %d", rt.getWidth(), rt.getHeight());
#endif
  if (currentViewPort.applied && viewportList.size())
    viewportList.back().applied = true;
  viewportList.push_back(rt);

  GuiViewPort &newViewPort = viewportList.back();
  currentViewPort.clipView(newViewPort);

  newViewPort.leftTop.x = floorf(newViewPort.leftTop.x * screenScale.x) * screenScaleRcp.x;
  newViewPort.leftTop.y = floorf(newViewPort.leftTop.y * screenScale.y) * screenScaleRcp.y;
  newViewPort.rightBottom.x = floorf(newViewPort.rightBottom.x * screenScale.x) * screenScaleRcp.x;
  newViewPort.rightBottom.y = floorf(newViewPort.rightBottom.y * screenScale.y) * screenScaleRcp.y;
  newViewPort.updateNull();
  newViewPort.applied = false;

  if (currentViewPort != newViewPort)
  {
    if (qCacheUsed)
    {
#if LOG_CACHE
      debug("QC:will flush %d quads on viewport changed!", qCacheUsed);
#endif
      qCacheFlush(false);
    }
    currentViewPort = newViewPort;
    currentViewPort.applied = false;
    applyViewport(currentViewPort);
  }
}

// reset to previous viewport (remove it from stack). return false, if no items to remove
bool GuiContext::restore_viewport()
{
#if LOG_DRAWS
  debug("viewport-");
#endif
  if (viewportList.size() <= 1)
    return false;
  viewportList.pop_back();

  if (currentViewPort.applied)
    for (int i = viewportList.size() - 1; i >= 0; i--)
      if (viewportList[i].applied)
      {
        applyViewport(viewportList[i]);
        break;
      }

  GuiViewPort &newViewPort = viewportList.back();
  if (currentViewPort != newViewPort)
  {
    currentViewPort = newViewPort;
    calcPreTransformViewport(currentViewPort.leftTop, currentViewPort.rightBottom);
  }

  return true;
}

// get current viewport
const GuiViewPort &GuiContext::get_viewport() { return currentViewPort; }

//************************************************************************
//* low-level rendering
//************************************************************************
// setup render parameters. call it before any GUI rendering

void GuiContext::setTarget()
{
  d3d::GpuAutoLock acquire;

  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);

  // int screen size
  screenWidth = viewW;
  screenHeight = viewH;

  // setup viewport
  deviceViewPort.leftTop.x = real(viewX);
  deviceViewPort.leftTop.y = real(viewY);
  deviceViewPort.rightBottom.x = real(viewX + viewW);
  deviceViewPort.rightBottom.y = real(viewY + viewH);
  deviceViewPort.updateNull();

  // get screen size
  screenSize.x = deviceViewPort.getWidth();
  screenSize.y = deviceViewPort.getHeight();

  // calculate scale coefficient
  screenScale = P2::ONE;
  screenScaleRcp = P2::ONE;
}

void GuiContext::setTarget(int screen_width, int screen_height, int left, int top)
{
  screenWidth = screen_width;
  screenHeight = screen_height;

  viewX = left;
  viewY = top;
  viewW = screen_width;
  viewH = screen_height;
  viewN = 0.;
  viewF = 1.;

  // setup viewport
  deviceViewPort.leftTop.x = real(viewX);
  deviceViewPort.leftTop.y = real(viewY);
  deviceViewPort.rightBottom.x = real(viewX + viewW);
  deviceViewPort.rightBottom.y = real(viewY + viewH);
  deviceViewPort.updateNull();

  // get screen size
  screenSize.x = screenWidth;
  screenSize.y = screenHeight;

  // calculate scale coefficient
  // TODO: pass viewport as params?
  screenScale.x = float_nonzero(screenSize.x) ? (deviceViewPort.getWidth() / screenSize.x) : 1.0f;
  screenScale.y = float_nonzero(screenSize.y) ? (deviceViewPort.getHeight() / screenSize.y) : 1.0f;
  screenScaleRcp.x = float_nonzero(screenScale.x) ? 1.0f / screenScale.x : 0;
  screenScaleRcp.y = float_nonzero(screenScale.y) ? 1.0f / screenScale.y : 0;
}

int GuiContext::beginChunk()
{
  G_ASSERT(isInRender == false);

#if LOG_DRAWS
  debug("beginChunk");
#endif

  if (viewportList.size() != 0)
  {
    DAG_FATAL("you must call StdGuiRender::end_render() before new StdGuiRender::start_render()!");
  }

  //    debug("%.4f %.4f - %.4f %.4f",
  //      deviceViewPort.getWidth(), deviceViewPort.getHeight(),
  //      screenScale.x, screenScale.y);

  renderer->screenOrig = deviceViewPort.leftTop;
  renderer->screenScale = screenScale;

  // set current viewport as fullscreen
  currentViewPort.leftTop = P2::ZERO;
  currentViewPort.rightBottom = screenSize;
  currentViewPort.updateNull();
  currentViewPort.applied = true;

  // current viewport is on top of list
  viewportList.push_back(currentViewPort);

  int chunk = renderer->beginChunk();

  G_ASSERT((qCachePushed + qCacheUsed) == 0); // flush is not called?

  qCachePushed = 0; // needed to avoid flush
  qCacheUsed = 0;   // needed to avoid flush
  rollState = 0;    // for applyViewport
  applyViewport(currentViewPort);

  defaultState();

  setBuffer(0);
  set_font(0);

  renderer->quadCache.guiState = guiState;
  renderer->pushGuiState(&renderer->quadCache.guiState);

  rollState = ROLL_ALL_STATES;
  isInRender = true;
  return chunk;
}

void GuiContext::endChunk()
{
#if LOG_DRAWS
  debug("endChunk");
#endif
  G_ASSERTF(!drawRawLayer, "end_raw_layer is not called");

  if (viewportList.size() != 1)
  {
    DAG_FATAL("set_viewport/restore_viewport mismatch! left %d viewports in the stack!", viewportList.size() - 1);
  }
  viewportList.clear();

  setBuffer(-1); // calls flushData internally
  isInRender = false;

  renderer->endChunk();
}

static void renderInscriptionsChunk()
{
  if (!DagorFontBinDump::inscr_blur_inited())
    return;
  StdGuiRender::GuiContext &ctxBlur = DagorFontBinDump::inscr_ctx();
  ctxBlur.endChunk();

  Driver3dRenderTarget prev_rt;
  d3d::get_render_target(prev_rt);
  int vl, vt, vw, vh;
  float vzn, vzf;
  d3d::getview(vl, vt, vw, vh, vzn, vzf);

  d3d::set_render_target(DagorFontBinDump::inscr_atlas.texBlurred.getTex2D(), 0);
  ctxBlur.renderChunk(ctxBlur.currentChunk);
  ctxBlur.currentChunk = -1;

  d3d::set_render_target(prev_rt);
  d3d::setview(vl, vt, vw, vh, vzn, vzf);
}
void GuiContext::renderChunk(int chunk_id)
{
  if (!DagorFontBinDump::is_inscr_ctx(this) && DagorFontBinDump::inscr_inited() && DagorFontBinDump::inscr_ctx().currentChunk != -1)
    renderInscriptionsChunk();
  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);
  int targetW, targetH;
  d3d::get_target_size(targetW, targetH);
  d3d::setview(0, 0, targetW, targetH, 0, 1);

  int objectBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT);

  renderer->renderChunk(chunk_id, targetW, targetH);

  ShaderGlobal::setBlock(objectBlock, ShaderGlobal::LAYER_OBJECT);

  d3d::setview(viewX, viewY, viewW, viewH, viewN, viewF);
}

// return true, if start_render already has been called
bool GuiContext::isRenderStarted() const { return isInRender; }

void GuiContext::beginChunkImm()
{
  G_ASSERT(currentChunk == -1);
#if LOG_DRAWS
  debug("beginChunkImm");
#endif
  // TODO: set less states
  currentChunk = beginChunk();
}

void GuiContext::endChunkImm()
{
#if LOG_DRAWS
  debug("endChunkImm");
#endif
  G_ASSERT(currentChunk != -1);
  endChunk();

  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);
  int targetW, targetH;
  d3d::get_target_size(targetW, targetH);
  d3d::setview(0, 0, targetW, targetH, 0, 1);

  int objectBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT);
  renderer->renderChunk(currentChunk, targetW, targetH);
  ShaderGlobal::setBlock(objectBlock, ShaderGlobal::LAYER_OBJECT);

  d3d::setview(viewX, viewY, viewW, viewH, viewN, viewF);

  currentChunk = -1;
}

void GuiContext::start_render()
{
  acquire();

  G_ASSERT(currentChunk == -1);

  currentChunk = beginChunk();
}

// render and restore parameters. call it after all GUI rendering
void GuiContext::end_render()
{
  /// DEBUG_CTX("end_render");

  G_ASSERT(currentChunk != -1);

  endChunk();

  renderChunk(currentChunk);
  currentChunk = -1;

  if (!update_internals_per_act_is_used && DagorFontBinDump::reqCharGenChanged)
    update_dyn_fonts();
  release();
}

void GuiContext::start_raw_layer()
{
  flushData();
  drawRawLayer = true;
}

void GuiContext::end_raw_layer() { drawRawLayer = false; }

// get current logical resolution
Point2 GuiContext::screen_size()
{
  G_ASSERT(isInRender);
  return screenSize;
}
real GuiContext::screen_width()
{
  G_ASSERT(isInRender);
  return screenSize.x;
}
real GuiContext::screen_height()
{
  G_ASSERT(isInRender);
  return screenSize.y;
}

// get real screen size
real GuiContext::real_screen_width()
{
  G_ASSERT(isInRender);
  return deviceViewPort.getWidth();
}
real GuiContext::real_screen_height()
{
  G_ASSERT(isInRender);
  return deviceViewPort.getHeight();
}

// return logical screen-to-real scale coefficient
Point2 GuiContext::logic2real()
{
  G_ASSERT(isInRender);
  return screenScale;
}
// return real-to-logical screen scale coefficient
Point2 GuiContext::real2logic()
{
  G_ASSERT(isInRender);
  return screenScaleRcp;
}

void GuiContext::setZMode(bool write_to_z, int test_depth)
{
  G_ASSERTF(bool(writeToZVarId), "%s 'gui_write_to_z' shader variable should exist", __FUNCTION__);
  G_ASSERTF(bool(testDepthVarId), "%s 'gui_test_depth' shader variable should exist", __FUNCTION__);

  uint8_t write_z = write_to_z ? 1 : 0;
  if (guiState.fontAttr.writeToZ != write_z || guiState.fontAttr.testDepth != test_depth)
  {
    guiState.fontAttr.writeToZ = write_z;
    guiState.fontAttr.testDepth = test_depth;
    rollState |= ROLL_GUI_STATE;
  }
}

BlendMode GuiContext::get_alpha_blend() { return guiState.params.alphaBlend; }

void GuiContext::set_alpha_blend(BlendMode blend_mode)
{
  if (guiState.params.alphaBlend != blend_mode)
  {
    guiState.params.alphaBlend = blend_mode;
    rollState |= ROLL_GUI_STATE;
  }
}

// set current color
void GuiContext::set_color(E3DCOLOR color)
{
  currentColor = color;
  GuiContext::set_ablend(currentColor.a != 255 || currentTextureAlpha);
}

void GuiContext::set_textures(TEXTUREID tex_id, d3d::SamplerHandle smp_id, TEXTUREID tex_id2, d3d::SamplerHandle smp_id2, bool font_l8,
  bool tex_in_linear)
{
  if (recCb)
    return recCb->setTex(tex_id, smp_id);
  curQuadMask = font_l8 ? 0x00000001 : (tex_id == BAD_TEXTUREID ? 0x00010001 : 0x00010000);

  bool update = false;
  if (font_l8)
  {
    if (tex_id != guiState.params.getFontTexId())
    {
      currentTextureAlpha = true;
      guiState.params.setFontTexId(tex_id, smp_id);
      guiState.params.setTexId2(tex_id2, smp_id2);
      guiState.params.alphaBlend = PREMULTIPLIED;
      guiState.params.texInLinear = tex_in_linear;
      update = true;
    }
  }
  else
  {
    if (tex_id != guiState.params.getTexId() || tex_id2 != guiState.params.getTexId2())
    {
      currentTextureAlpha = true;
      guiState.params.setTexId(tex_id, smp_id);
      guiState.params.setTexId2(tex_id2, smp_id2);
      guiState.params.alphaBlend = PREMULTIPLIED;
      guiState.params.texInLinear = tex_in_linear;
      update = true;
    }
  }

  if (currentTextureFont != font_l8)
    update = true;
  currentTextureFont = font_l8;

  bool newFontFx = font_l8 && guiState.params.ff.type != FFT_NONE;
  if (guiState.params.fontFx != newFontFx)
  {
    guiState.params.fontFx = newFontFx;
    update = true;
  }

  if (font_l8)
  {
    if (guiState.params.getTexId() != BAD_TEXTUREID)
    {
      guiState.params.setTexId(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE);
      guiState.params.setTexId2(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE);
      update = true;
    }
  }
  else
  {
    if (guiState.params.getFontTexId() != BAD_TEXTUREID)
    {
      guiState.params.setFontTexId(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE);
      update = true;
    }
  }

  rollState |= update ? ROLL_GUI_STATE : 0;
}

void GuiContext::set_mask_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, Point3 transform0, Point3 transform1)
{
  if (tex_id != guiState.params.getMaskTexId() || transform0 != guiState.params.maskTransform0 ||
      transform1 != guiState.params.maskTransform1 || smp_id != guiState.params.getMaskTexSampler())
  {
    guiState.params.maskTransform0 = transform0;
    guiState.params.maskTransform1 = transform1;
    guiState.params.setMaskTexId(tex_id, smp_id);
    rollState |= ROLL_GUI_STATE;
  }
}


void GuiContext::rollStateBlock()
{
#if LOG_CACHE
  debug("STDG:rollStateBlock st=%x cb=%d", rollState, currentBufferId);
  guiState.params.dump();
  guiState.fontAttr.dump();
#endif
  renderer->quadCache.guiState = guiState;

  // flushed by rendercache if cache enabled (bufferId > 0)
  if (rollState & ROLL_GUI_STATE)
    renderer->pushGuiState(&renderer->quadCache.guiState);

  if ((rollState & ROLL_EXT_STATE) && (currentBufferId >= 0))
    renderer->pushExtState(renderer->extStatePtrs[currentBufferId]);

  rollState = 0;
}

// draw vertices
void GuiContext::draw_quads(const void *verts, int num_quads)
{
  flushData();
#if LOG_DRAWS
  debug("STDG:draw_quads %x, %d", verts, num_quads);
#endif

  if (!currentViewPort.isZero() && num_quads > 0)
  {
    if (!drawRawLayer && currentBufferId == 0)
      renderer->quadCache.addQuads((const GuiVertex *)verts, num_quads);
    else //(render cache is flushed already)
    {
      if (renderer->copyQuads(verts, num_quads))
        renderer->drawQuads(num_quads);
    }
  }
}

void GuiContext::draw_faces(const void *verts, int num_verts, const IndexType *indices, int num_faces)
{
  if (!drawRawLayer)
  {
    logerr("draw_faces requires raw_layer clause");
    return;
  }
#if LOG_DRAWS
  debug("STDG:draw_faces v:%x, %d i:%x %d", verts, num_verts, indices, num_faces);
#endif

  if (!currentViewPort.isZero() && num_verts > 0)
  {
    flushData();
    if (renderer->copyFaces(verts, num_verts, indices, num_faces * 3))
      renderer->drawIndexed(num_faces);
  }
}

void GuiContext::draw_prims(int d3d_prim, int num_prims, const void *verts, const IndexType *indices)
{
  if (!drawRawLayer)
  {
    logerr("draw_prim requires raw_layer clause");
    return;
  }

  if (currentViewPort.isZero() || num_prims <= 0)
    return;

  flushData();

  (void)indices;
  switch (d3d_prim)
  {
    case PRIM_LINESTRIP:

      G_ASSERT(indices == NULL);
      if (renderer->copyFaces(verts, num_prims + 1, NULL, 0))
        renderer->drawPrims(DrawElem::DRAW_LINESTRIP, num_prims);
      break;

    default: return;
  };
}

bool GuiContext::vpBoxIsVisible(Point2 p0, Point2 p1, Point2 p2, Point2 p3) const
{
  return vpBoxIsVisible(min4(p0.x, p1.x, p2.x, p3.x), min4(p0.y, p1.y, p2.y, p3.y), max4(p0.x, p1.x, p2.x, p3.x),
    max4(p0.y, p1.y, p2.y, p3.y));
}


// vertex order:
// 0 3
// 1 2
inline void assign_color(GuiVertex v[4], E3DCOLOR color, uint32_t curMask)
{
  v[0].color = v[1].color = v[2].color = v[3].color = color;
  v[0].tc1 = v[1].tc1 = v[2].tc1 = v[3].tc1 = curMask;
}

// draw two poligons, using current color & texture (if present)
void GuiContext::render_quad(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc0, Point2 tc3, Point2 tc2, Point2 tc1)
{
  if (!vpBoxIsVisible(p0, p3, p2, p1))
    return;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(1);
  assign_color(qv, currentColor, curQuadMask);

#if LOG_DRAWS
//    debug("STDG:quad %d, %d - %d, %d", p0.x, p0.y, p2.x, p2.y);
#endif

#define MAKEDATA(index)                                \
  {                                                    \
    GuiVertex &v = qv[index];                          \
    v.setPos(vertexTransform, p##index + correctCord); \
    v.setTc0(tc##index);                               \
  }

  MAKEDATA(0);
  MAKEDATA(1);
  MAKEDATA(2);
  MAKEDATA(3);
#undef MAKEDATA
}

// draw two poligons, using colors & texture (if present)
void GuiContext::render_quad_color(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc0, Point2 tc3, Point2 tc2, Point2 tc1,
  E3DCOLOR c0, E3DCOLOR c3, E3DCOLOR c2, E3DCOLOR c1)
{
  if (!vpBoxIsVisible(p0, p3, p2, p1))
    return;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(1);

#if LOG_DRAWS
//    debug("STDG:quad %d, %d - %d, %d", p0.x, p0.y, p2.x, p2.y);
#endif

#define MAKEDATA(index)                                \
  {                                                    \
    GuiVertex &v = qv[index];                          \
    v.setPos(vertexTransform, p##index + correctCord); \
    v.setTc0(tc##index);                               \
    v.color = c##index;                                \
    v.tc1 = curQuadMask;                               \
  }

  MAKEDATA(0);
  MAKEDATA(1);
  MAKEDATA(2);
  MAKEDATA(3);
#undef MAKEDATA
}

void GuiContext::render_quad_t(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc_lt, Point2 tc_rb)
{
  if (!vpBoxIsVisible(p0, p3, p2, p1))
    return;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(1);
  assign_color(qv, currentColor, curQuadMask);

#define MAKEDATA(index, tc)                            \
  {                                                    \
    GuiVertex &v = qv[index];                          \
    v.setPos(vertexTransform, p##index + correctCord); \
    v.setTc0(tc);                                      \
  }


  MAKEDATA(0, tc_lt);
  MAKEDATA(1, Point2(tc_rb.x, tc_lt.y));
  MAKEDATA(2, tc_rb);
  MAKEDATA(3, Point2(tc_lt.x, tc_rb.y));

#undef MAKEDATA
}

//************************************************************************
//* render primitives
//************************************************************************
// draw frame, using current color
void GuiContext::render_frame(real left, real top, real right, real bottom, real thickness)
{
  if (!float_nonzero(thickness))
    return;
  render_box(left, top, right, top + thickness);
  render_box(left, bottom - thickness, right, bottom);
  render_box(left, top + thickness, left + thickness, bottom - thickness);
  render_box(right - thickness, top + thickness, right, bottom - thickness);
}

// draw rectangle, using current color
void GuiContext::render_box(real left, real top, real right, real bottom)
{
  alignValues(left, right);
  alignValues(top, bottom);

  if (!vpBoxIsVisible(left, top, right, bottom))
    return;

  reset_textures();
  GuiVertex *v = qCacheAllocT<GuiVertex>(1);
  assign_color(v, currentColor, curQuadMask);

  v[0].setPos(vertexTransform, left, top);
  v[1].setPos(vertexTransform, right, top);
  v[2].setPos(vertexTransform, right, bottom);
  v[3].setPos(vertexTransform, left, bottom);
  v[0].zeroTc0();
  v[1].zeroTc0();
  v[2].zeroTc0();
  v[3].zeroTc0();
}

// draw rectangle box, using current color & texture (if present)
void GuiContext::render_rect(real left, real top, real right, real bottom, Point2 left_top_tc, Point2 dx_tc, Point2 dy_tc)
{
  // if no texture, draw only color box & exit
  if (guiState.params.getTexId() == BAD_TEXTUREID)
  {
    render_box(left, top, right, bottom);
    return;
  }

  Point2 lt = Point2(left, top) + correctCord;
  Point2 rb = Point2(right, bottom) + correctCord;
  Point2 ltuv = Point2(0, 0);
  Point2 rbuv = Point2(1, 1);

  alignValues(lt.x, rb.x, ltuv.x, rbuv.x);
  alignValues(lt.y, rb.y, ltuv.y, rbuv.y);

  if (!vpBoxIsVisible(lt, rb))
    return;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(1);
  assign_color(qv, currentColor, curQuadMask);

#define MAKEDATA(index, p1, p2)                                                                                                \
  {                                                                                                                            \
    GuiVertex &v = qv[index];                                                                                                  \
    v.setPos(vertexTransform, p1.x, p2.y);                                                                                     \
    v.setTc0(p1##uv.x *dx_tc.x + p2##uv.y * dy_tc.x + left_top_tc.x, p1##uv.x * dx_tc.y + p2##uv.y * dy_tc.y + left_top_tc.y); \
  }

  FILL_VERTICES;

#undef MAKEDATA
}

// draw rectangle box, using current color & texture (if present)
void GuiContext::render_rect_t(real left, real top, real right, real bottom, Point2 left_top_tc, Point2 right_bottom_tc)
{
  // if no texture, draw only color box & exit
  if (guiState.params.getTexId() == BAD_TEXTUREID)
  {
    render_box(left, top, right, bottom);
    return;
  }

  Point2 lt = Point2(left, top) + correctCord;
  Point2 rb = Point2(right, bottom) + correctCord;
  Point2 ltuv = left_top_tc;
  Point2 rbuv = right_bottom_tc;

  alignValues(lt.x, rb.x, ltuv.x, rbuv.x);
  alignValues(lt.y, rb.y, ltuv.y, rbuv.y);

  if (!vpBoxIsVisible(lt, rb))
    return;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(1);
  assign_color(qv, currentColor, curQuadMask);

#define MAKEDATA(index, p1, p2)            \
  {                                        \
    GuiVertex &v = qv[index];              \
    v.setPos(vertexTransform, p1.x, p2.y); \
    v.setTc0(p1##uv.x, p2##uv.y);          \
  }

  FILL_VERTICES;

#undef MAKEDATA
}


void GuiContext::render_imgui_list()
{
  G_STATIC_ASSERT(sizeof(dag_imgui_drawlist::ImDrawIdx) == sizeof(StdGuiRender::IndexType));
  G_STATIC_ASSERT(sizeof(dag_imgui_drawlist::ImDrawVert) >= sizeof(GuiVertex));
  GuiVertex *dst = (GuiVertex *)(char *)&imDrawList.VtxBuffer[0];
  GuiVertex v;
  v.zeroTc1();
  for (int i = 0, ei = imDrawList.VtxBuffer.size(); i < ei; ++i, ++dst)
  {
    auto srcV = imDrawList.VtxBuffer[i];
    // debug("%d: %@ %@", i, Point2(srcV.pos), E3DCOLOR(srcV.col));
    v.color = srcV.col;
    v.setPos(vertexTransform, srcV.pos);
    v.setTc0(srcV.uv);
    *dst = v;
  }
  start_raw_layer();
  draw_faces((GuiVertex *)(char *)&imDrawList.VtxBuffer[0], imDrawList.VtxBuffer.size(), &imDrawList.IdxBuffer[0],
    imDrawList.IdxBuffer.size() / 3);
  end_raw_layer();
  imDrawList.Clear();
}

void GuiContext::resetViewTm()
{
  vertexTransform.resetViewTm();
  calcInverseVertexTransform();
  calcPreTransformViewport();
  extState().resetRotation();
}

void GuiContext::setViewTm(const float m[2][3])
{
  vertexTransform.setViewTm(m);
  calcInverseVertexTransform();
  calcPreTransformViewport();
  extState().resetRotation();
}

void GuiContext::setViewTm(Point2 ax, Point2 ay, Point2 o, bool add)
{
  if (add)
    vertexTransform.addViewTm(ax, ay, o);
  else
    vertexTransform.setViewTm(ax, ay, o);
  calcInverseVertexTransform();
  calcPreTransformViewport();
}

void GuiContext::setRotViewTm(float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add)
{
  vertexTransform.setRotViewTm(extState().fontTex2rotCCSmS, x0, y0, rot_ang_rad, skew_ang_rad, add);
  calcInverseVertexTransform();
  calcPreTransformViewport();
}

void GuiContext::getViewTm(float dest_vtm[2][3], bool pure_trans) const { vertexTransform.getViewTm(dest_vtm, pure_trans); }

void GuiContext::calcInverseVertexTransform() { GuiVertexTransform::inverseViewTm(vertexTransformInverse.vtm, vertexTransform.vtm); }

void GuiContext::calcPreTransformViewport(Point2 lt, Point2 rb)
{
  float k = GUI_POS_SCALE;
  lt *= k;
  rb *= k;
  preTransformViewport.setempty();
  preTransformViewport +=
    Point2(vertexTransformInverse.transformComponent(lt.x, lt.y, 0), vertexTransformInverse.transformComponent(lt.x, lt.y, 1));
  preTransformViewport +=
    Point2(vertexTransformInverse.transformComponent(lt.x, rb.y, 0), vertexTransformInverse.transformComponent(lt.x, rb.y, 1));
  preTransformViewport +=
    Point2(vertexTransformInverse.transformComponent(rb.x, lt.y, 0), vertexTransformInverse.transformComponent(rb.x, lt.y, 1));
  preTransformViewport +=
    Point2(vertexTransformInverse.transformComponent(rb.x, rb.y, 0), vertexTransformInverse.transformComponent(rb.x, rb.y, 1));
}

void GuiContext::calcPreTransformViewport()
{
  if (currentViewPort.isNull)
    preTransformViewport.setempty();
  else
    calcPreTransformViewport(currentViewPort.leftTop, currentViewPort.rightBottom);
}


void GuiContext::render_rounded_box(Point2 lt, Point2 rb, E3DCOLOR col, E3DCOLOR border, Point4 rounding, float thickness)
{
  if (!vpBoxIsVisible(lt, rb))
    return;
  reset_textures();
  const uint32_t aaMask =
    get_alpha_blend() == NO_BLEND ? 0 : (get_alpha_blend() == NONPREMULTIPLIED ? dag_imgui_drawlist::DAG_IM_COL32_A_MASK : 0xFFFFFFFF);
  if ((border.u || aaMask == 0) && thickness > 0.f) // completely invisible border
  {
    imDrawList.AddRectFilledBordered(lt, rb, border, col, rounding, thickness, aaMask);
  }
  else
  {
    imDrawList.AddRectFilled(lt, rb, col, rounding, aaMask);
  }
  render_imgui_list();
}


void GuiContext::render_rounded_frame(Point2 lt, Point2 rb, E3DCOLOR col, Point4 rounding, float thickness)
{
  if (thickness <= 0.f)
    return;
  if (!vpBoxIsVisible(lt, rb))
    return;
  reset_textures();
  const uint32_t aaMask =
    get_alpha_blend() == NO_BLEND ? 0 : (get_alpha_blend() == NONPREMULTIPLIED ? dag_imgui_drawlist::DAG_IM_COL32_A_MASK : 0xFFFFFFFF);
  if ((col.u || aaMask == 0)) // completely invisible border
  {
    imDrawList.AddRect(lt, rb, col, rounding, thickness, aaMask);
    render_imgui_list();
  }
}


void GuiContext::render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 tc_rightbottom, E3DCOLOR col, Point4 rounding)
{
  if (!vpBoxIsVisible(lt, rb))
    return;
  const uint32_t aaMask =
    get_alpha_blend() == NO_BLEND ? 0 : (get_alpha_blend() == NONPREMULTIPLIED ? dag_imgui_drawlist::DAG_IM_COL32_A_MASK : 0xFFFFFFFF);
  // actually texId doesn't matter here anyway
  imDrawList.AddImageRounded(guiState.params.getTexId(), lt, rb, tc_lefttop, tc_rightbottom, col, rounding, aaMask);
  render_imgui_list();
}

void GuiContext::render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 dx_tc, Point2 dy_tc, E3DCOLOR col,
  Point4 rounding)
{
  if (!vpBoxIsVisible(lt, rb))
    return;
  const uint32_t aaMask =
    get_alpha_blend() == NO_BLEND ? 0 : (get_alpha_blend() == NONPREMULTIPLIED ? dag_imgui_drawlist::DAG_IM_COL32_A_MASK : 0xFFFFFFFF);
  // actually texId doesn't matter here anyway
  imDrawList.AddImageRounded(guiState.params.getTexId(), lt, rb, tc_lefttop, dx_tc, dy_tc, col, rounding, aaMask);
  render_imgui_list();
}


// draw line
void GuiContext::draw_line(real left, real top, real right, real bottom, real thickness)
{
  if (!float_nonzero(thickness))
    return;

  thickness /= 2;

  if (!float_nonzero(left - right))
  {
    // vertical
    render_box(left - thickness, top, left + thickness, bottom);
    return;
  }
  else if (!float_nonzero(top - bottom))
  {
    // horizontal
    render_box(left, top - thickness, right, top + thickness);
    return;
  }

  reset_textures();

  Point2 ofs = Point2(-(bottom - top), right - left);
  ofs.normalize();
  ofs *= thickness;

  render_quad(Point2(left - ofs.x, top - ofs.y), Point2(right - ofs.x, bottom - ofs.y), Point2(right + ofs.x, bottom + ofs.y),
    Point2(left + ofs.x, top + ofs.y));
}

// draw rectangle with frame, using optimization tricks
void GuiContext::solid_frame(real left, real top, real right, real bottom, real thickness, E3DCOLOR background, E3DCOLOR frame)
{
  if (left + thickness < right - thickness)
  {
    set_color(background);
    render_box(left + thickness, top + thickness, right - thickness, bottom - thickness);
  }

  if (thickness > 0)
  {
    set_color(frame);
    render_frame(left, top, right, bottom, thickness);
  }
}


//************************************************************************
//* Anti-aliased primitives
//************************************************************************

#define MAKEDATA(quad_num, vindex, vcolor, coord) \
  {                                               \
    GuiVertex &v = qv[quad_num * 4 + vindex];     \
    v.color = vcolor;                             \
    v.setPos(vertexTransform, coord);             \
    v.zeroTc0();                                  \
  }

#define THIN_LINE_THRESHOLD      1.8f
#define MAX_CACHED_ENDING_SINCOS 28
float cachedEndingRadius = -9999.0f;
StaticTab<Point2, MAX_CACHED_ENDING_SINCOS> cachedEndingSinCos;

void GuiContext::render_line_ending_aa(E3DCOLOR color, const Point2 pos, const Point2 fwd, const Point2 left, float radius)
{
  reset_textures();

  if (radius < THIN_LINE_THRESHOLD)
  {
    GuiVertex *qv = qCacheAllocT<GuiVertex>(3);

    Point2 d0 = pos + fwd + (fwd + left) * radius;
    Point2 d1 = pos + fwd + (fwd - left) * radius;
    Point2 d2 = d0 + left;
    Point2 d3 = d1 - left;

    MAKEDATA(0, 0, E3DCOLOR(0), d1);
    MAKEDATA(0, 1, E3DCOLOR(0), d0);
    MAKEDATA(0, 2, color, pos + left * radius);
    MAKEDATA(0, 3, color, pos - left * radius);

    MAKEDATA(1, 0, E3DCOLOR(0), d0);
    MAKEDATA(1, 1, E3DCOLOR(0), d2);
    MAKEDATA(1, 2, E3DCOLOR(0), pos + left * (radius + 1));
    MAKEDATA(1, 3, color, pos + left * radius);

    MAKEDATA(2, 0, E3DCOLOR(0), d1);
    MAKEDATA(2, 1, E3DCOLOR(0), d3);
    MAKEDATA(2, 2, E3DCOLOR(0), pos - left * (radius + 1));
    MAKEDATA(2, 3, color, pos - left * radius);

    return;
  }

  bool scFromCache = fabsf(radius - cachedEndingRadius) < 0.01f;
  int count = scFromCache ? cachedEndingSinCos.size() - 1 : (4 + (int(cvt(radius, 2, 40, 0, MAX_CACHED_ENDING_SINCOS - 7)) & 0xfffe));

  if (!scFromCache)
  {
    cachedEndingRadius = radius;
    cachedEndingSinCos.resize(count + 1);

    for (int i = 0; i <= count; i++)
    {
      Point2 sc;
      sincos((float(i) / count) * PI, sc.y, sc.x);
      cachedEndingSinCos[i] = sc;
    }
  }

  for (int i = 0; i < count; i += 2)
  {
    GuiVertex *qv = qCacheAllocT<GuiVertex>(3);

    Point2 d0;
    Point2 d1;
    Point2 d2;
    d0 = cachedEndingSinCos[i];
    d1 = cachedEndingSinCos[i + 1];
    d2 = cachedEndingSinCos[i + 2];

    MAKEDATA(0, 0, color, pos);
    MAKEDATA(0, 1, color, pos + (d2.y * fwd + d2.x * left) * radius);
    MAKEDATA(0, 2, color, pos + (d1.y * fwd + d1.x * left) * radius);
    MAKEDATA(0, 3, color, pos + (d0.y * fwd + d0.x * left) * radius);

    MAKEDATA(1, 0, E3DCOLOR(0), pos + (d1.y * fwd + d1.x * left) * (radius + 1));
    MAKEDATA(1, 1, color, pos + (d1.y * fwd + d1.x * left) * radius);
    MAKEDATA(1, 2, color, pos + (d0.y * fwd + d0.x * left) * radius);
    MAKEDATA(1, 3, E3DCOLOR(0), pos + (d0.y * fwd + d0.x * left) * (radius + 1));

    MAKEDATA(2, 0, E3DCOLOR(0), pos + (d2.y * fwd + d2.x * left) * (radius + 1));
    MAKEDATA(2, 1, color, pos + (d2.y * fwd + d2.x * left) * radius);
    MAKEDATA(2, 2, color, pos + (d1.y * fwd + d1.x * left) * radius);
    MAKEDATA(2, 3, E3DCOLOR(0), pos + (d1.y * fwd + d1.x * left) * (radius + 1));
  }
}

void GuiContext::render_line_aa(const Point2 *points, int points_count, bool is_closed, float line_width, const Point2 line_indent,
  E3DCOLOR color)
{
  if (!color)
    return;

  BBox2 bbox;
  bbox.setempty();
  for (int i = 0; i < points_count; i++)
    bbox += points[i];

  if (!vpBoxIsVisible(bbox[0] - Point2(line_width, line_width), bbox[1] + Point2(line_width, line_width)))
  {
    return;
  }

  reset_textures();

  float halfW = max((line_width - 1) * 0.5f, 0.f);
  Point2 from;
  Point2 dir = Point2(1, 0);
  Point2 left = Point2(0, -1);
  Point2 to = points[0];
  Point2 initialPos = to;

  int idx = 1;
  int length = points_count;
  int lastIndex = is_closed ? points_count + 1 : points_count;

  Point2 innerFrom;
  Point2 innerTo = to;
  bool useIndents = (line_indent.x != 0.f || line_indent.y != 0.f);
  bool renderLastEnding = !is_closed;
  bool thinLine = halfW < THIN_LINE_THRESHOLD;

  do
  {
    from = to;
    int closedIdx = (idx >= length) ? 0 : idx;
    to = points[closedIdx];

    innerFrom = from;
    innerTo = to;
    dir = normalizeDef(innerTo - innerFrom, Point2(1, 0));

    if (useIndents)
    {
      renderLastEnding = true;
      if ((innerFrom - innerTo).length() < line_indent.x + line_indent.y)
      {
        idx++;
        renderLastEnding = false;
        continue;
      }
      else
      {
        innerFrom += line_indent.x * dir;
        innerTo -= line_indent.y * dir;
      }
    }

    left = Point2(-dir.y, dir.x);

    GuiVertex *qv = qCacheAllocT<GuiVertex>(4);

    MAKEDATA(0, 0, E3DCOLOR(0), innerTo + left * (halfW + 1));
    MAKEDATA(0, 1, color, innerTo + left * halfW);
    MAKEDATA(0, 2, color, innerFrom + left * halfW);
    MAKEDATA(0, 3, E3DCOLOR(0), innerFrom + left * (halfW + 1));

    MAKEDATA(1, 0, color, innerTo - left * halfW);
    MAKEDATA(1, 1, color, innerTo);
    MAKEDATA(1, 2, color, innerFrom);
    MAKEDATA(1, 3, color, innerFrom - left * halfW);

    MAKEDATA(2, 0, color, innerTo);
    MAKEDATA(2, 1, color, innerTo + left * halfW);
    MAKEDATA(2, 2, color, innerFrom + left * halfW);
    MAKEDATA(2, 3, color, innerFrom);

    MAKEDATA(3, 0, color, innerTo - left * halfW);
    MAKEDATA(3, 1, E3DCOLOR(0), innerTo - left * (halfW + 1));
    MAKEDATA(3, 2, E3DCOLOR(0), innerFrom - left * (halfW + 1));
    MAKEDATA(3, 3, color, innerFrom - left * halfW);

    render_line_ending_aa(color, innerFrom, -dir, -left, halfW);

    if (thinLine || useIndents)
      render_line_ending_aa(color, innerTo, dir, left, halfW);

    idx++;
  } while (idx < lastIndex);

  if (!useIndents && !thinLine)
    if (renderLastEnding && ((innerTo - initialPos).lengthSq() > 1e-6f || points_count == 2))
      render_line_ending_aa(color, innerTo, dir, left, halfW);
}

void GuiContext::render_dashed_line(const Point2 p0, const Point2 p1, const float dash, const float space, const float line_width,
  E3DCOLOR color)
{
  const float invLength = safeinv((p0 - p1).length());

  const float x1 = p0.x;
  const float y1 = p0.y;
  const float kx = p1.x - p0.x;
  const float ky = p1.y - p0.y;
  float t = 0.f;

  const float tDash = dash * invLength;
  const float tSpace = space * invLength;

  Tab<Point2> dashPoints(framemem_ptr());
  dashPoints.resize(2);
  while (t <= 1.f)
  {
    dashPoints[0].x = x1 + kx * t;
    dashPoints[0].y = y1 + ky * t;
    t += tDash;
    if (t > 1.f)
      t = 1.f;
    dashPoints[1].x = x1 + kx * t;
    dashPoints[1].y = y1 + ky * t;
    t += tSpace;

    render_line_aa(dashPoints, /*is_closed*/ false, line_width, ZERO<Point2>(), color);

    if (t < 1e-4f) // max = 10000 dashes
      return;
  }
}

void GuiContext::render_poly(dag::ConstSpan<Point2> points, E3DCOLOR fill_color)
{
  if (!fill_color || points.size() < 3)
    return;

  BBox2 bbox;
  bbox.setempty();
  for (int i = 0; i < points.size(); i++)
    bbox += points[i];

  if (!vpBoxIsVisible(bbox[0] - Point2(1, 1), bbox[1] + Point2(1, 1)))
    return;


  Tab<int> indices(framemem_ptr());
  indices.reserve(points.size() * 3);

  triangulate_poly(points, indices);

  if (indices.size() <= 2)
    return;

  reset_textures();

  int vertices = indices.size();
  int faces = vertices / 3;
  int drawnVertices = 0;
  while (faces > 0)
  {
    int facesToDraw = min(faces, qCacheCapacity());
    int verticesToDraw = facesToDraw * 3;
    GuiVertex *qv = qCacheAllocT<GuiVertex>(facesToDraw);
    int quad = 0;
    for (int i = 0; i < verticesToDraw; i += 3, quad++)
    {
      MAKEDATA(quad, 0, fill_color, points[indices[drawnVertices + i]]);
      MAKEDATA(quad, 1, fill_color, points[indices[drawnVertices + i + 2]]);
      MAKEDATA(quad, 2, fill_color, points[indices[drawnVertices + i + 1]]);
      MAKEDATA(quad, 3, fill_color, points[indices[drawnVertices + i]]);
    }

    faces -= facesToDraw;
    drawnVertices += verticesToDraw;
  }
}


void GuiContext::render_inverse_poly(dag::ConstSpan<Point2> points_ccw, E3DCOLOR fill_color, Point2 left_top, Point2 right_bottom)
{
  int count = points_ccw.size();
  if (!fill_color || count < 3)
    return;

  BBox2 bbox;
  bbox.setempty();
  for (int i = 0; i < count; i++)
    bbox += points_ccw[i];

  reset_textures();

  if (!vpBoxIsVisible(bbox[0] - Point2(1, 1), bbox[1] + Point2(1, 1)))
  {
    GuiVertex *qv = qCacheAllocT<GuiVertex>(1);
    MAKEDATA(0, 0, fill_color, left_top);
    MAKEDATA(0, 1, fill_color, Point2(left_top.x, right_bottom.y));
    MAKEDATA(0, 2, fill_color, right_bottom);
    MAKEDATA(0, 3, fill_color, Point2(right_bottom.x, left_top.y));
    return;
  }

  TmpVec<int, 64> next(count);
  next[count - 1] = 0;
  for (int i = 0; i < count - 1; i++)
    next[i] = i + 1;

  int baseIdx = 0;
  int rightIdx = 0;
  float minX = points_ccw[0].x;
  float maxX = points_ccw[0].x;
  for (int i = 1; i < count; i++)
  {
    if (points_ccw[i].x < minX)
    {
      minX = points_ccw[i].x;
      baseIdx = i;
    }

    if (points_ccw[i].x > maxX)
    {
      maxX = points_ccw[i].x;
      rightIdx = i;
    }
  }


  TmpVec<int, 64> indices; // concave parts

  bool changed = true;

  while (changed)
  {
    changed = false;
    for (int i = baseIdx;;)
    {
      for (int j = 0; j < count - 2; j++)
      {
        int p0 = i;
        int p1 = next[p0];
        int p2 = next[p1];

        if (p2 == p0 || p1 == p0)
          break;

        Point2 d1 = points_ccw[p2] - points_ccw[p0];
        Point2 d2 = points_ccw[p1] - points_ccw[p0];

        if (d1.x * d2.y - d1.y * d2.x > 0.0f)
          break;

        if (next[i] == baseIdx)
          baseIdx = p2;

        next[i] = p2;
        changed = true;

        indices.push_back(p0);
        indices.push_back(p1);
        indices.push_back(p2);
      }

      i = next[i];
      if (i == baseIdx)
        break;
    }
  }


  int vertices = indices.size();
  int faces = vertices / 3;
  int drawnVertices = 0;
  while (faces > 0)
  {
    int facesToDraw = min(faces, qCacheCapacity());
    int verticesToDraw = facesToDraw * 3;
    GuiVertex *qv = qCacheAllocT<GuiVertex>(facesToDraw);
    int quad = 0;
    for (int i = 0; i < verticesToDraw; i += 3, quad++)
    {
      MAKEDATA(quad, 0, fill_color, points_ccw[indices[drawnVertices + i]]);
      MAKEDATA(quad, 1, fill_color, points_ccw[indices[drawnVertices + i + 2]]);
      MAKEDATA(quad, 2, fill_color, points_ccw[indices[drawnVertices + i + 1]]);
      MAKEDATA(quad, 3, fill_color, points_ccw[indices[drawnVertices + i]]);
    }

    faces -= facesToDraw;
    drawnVertices += verticesToDraw;
  }


  indices.clear();
  bool down = false;
  for (int i = baseIdx;;)
  {
    if (down)
    {
      indices.push_back(i);
      indices.push_back(next[i]);
    }
    else
    {
      indices.push_back(-i);
      indices.push_back(-next[i]);
    }

    i = next[i];
    if (i == baseIdx)
      break;

    if (i == rightIdx)
      down = !down;
  }


  vertices = indices.size();
  faces = vertices / 2;
  drawnVertices = 0;
  while (faces > 0)
  {
    int facesToDraw = min(faces, qCacheCapacity());
    int verticesToDraw = facesToDraw * 2;
    GuiVertex *qv = qCacheAllocT<GuiVertex>(facesToDraw);
    int quad = 0;
    for (int i = 0; i < verticesToDraw; i += 2, quad++)
    {
      if (indices[drawnVertices + i + 1] < 0 || indices[drawnVertices + i] < 0)
      {
        MAKEDATA(quad, 0, fill_color, points_ccw[-indices[drawnVertices + i + 1]]);
        MAKEDATA(quad, 1, fill_color, points_ccw[-indices[drawnVertices + i]]);
        MAKEDATA(quad, 2, fill_color, Point2(points_ccw[-indices[drawnVertices + i]].x, right_bottom.y));
        MAKEDATA(quad, 3, fill_color, Point2(points_ccw[-indices[drawnVertices + i + 1]].x, right_bottom.y));
      }
      else
      {
        G_UNUSED(left_top);
        MAKEDATA(quad, 0, fill_color, Point2(points_ccw[indices[drawnVertices + i]].x, left_top.y));
        MAKEDATA(quad, 1, fill_color, Point2(points_ccw[indices[drawnVertices + i + 1]].x, left_top.y));
        MAKEDATA(quad, 2, fill_color, points_ccw[indices[drawnVertices + i + 1]]);
        MAKEDATA(quad, 3, fill_color, points_ccw[indices[drawnVertices + i]]);
      }
    }

    faces -= facesToDraw;
    drawnVertices += verticesToDraw;
  }


  {
    GuiVertex *qv = qCacheAllocT<GuiVertex>(4);
    MAKEDATA(0, 0, fill_color, left_top);
    MAKEDATA(0, 1, fill_color, Point2(left_top.x, 0));
    MAKEDATA(0, 2, fill_color, points_ccw[baseIdx]);
    MAKEDATA(0, 3, fill_color, Point2(points_ccw[baseIdx].x, left_top.y));

    MAKEDATA(1, 0, fill_color, Point2(left_top.x, 0));
    MAKEDATA(1, 1, fill_color, Point2(left_top.x, right_bottom.y));
    MAKEDATA(1, 2, fill_color, Point2(points_ccw[baseIdx].x, right_bottom.y));
    MAKEDATA(1, 3, fill_color, points_ccw[baseIdx]);

    MAKEDATA(2, 0, fill_color, right_bottom);
    MAKEDATA(2, 1, fill_color, Point2(points_ccw[rightIdx].x, right_bottom.y));
    MAKEDATA(2, 2, fill_color, points_ccw[rightIdx]);
    MAKEDATA(2, 3, fill_color, Point2(right_bottom.x, 0));

    MAKEDATA(3, 0, fill_color, Point2(right_bottom.x, 0));
    MAKEDATA(3, 1, fill_color, points_ccw[rightIdx]);
    MAKEDATA(3, 2, fill_color, Point2(points_ccw[rightIdx].x, left_top.y));
    MAKEDATA(3, 3, fill_color, Point2(right_bottom.x, left_top.y));
  }
}


Tab<Tab<Point2>> cachedEllipseSinCos; // [points_count][point_index]

void GuiContext::render_ellipse_aa(Point2 pos, Point2 radius, float line_width, E3DCOLOR color, E3DCOLOR mid_color,
  E3DCOLOR fill_color)
{
  if (!color && !fill_color && !mid_color)
    return;

  if (line_width < 1.f)
    line_width = 1;

  if (!vpBoxIsVisible(pos - radius - Point2(line_width, line_width), pos + radius + Point2(line_width, line_width)))
  {
    return;
  }

  radius -= Point2(0.5f, 0.5f);
  float maxR = max(radius.x, radius.y);
  int count = maxR < 5 ? 8 : 12 + (int(cvt(maxR + line_width * 0.5f, 4, 110, 0, 96)) & 0xfffc);
  int checkVisibilityAfter = (fill_color || count < 24) ? 9999 : 0;

  if (cachedEllipseSinCos.size() <= count)
    cachedEllipseSinCos.resize(count + 1);

  Tab<Point2> &points = cachedEllipseSinCos[count];

  if (points.empty())
  {
    points.resize(count + 1);
    for (int i = 0; i <= count; i++)
    {
      Point2 sc;
      sincos(i * 2 * PI / count, sc.x, sc.y);
      points[i] = sc;
    }
  }

  reset_textures();

  if (color == fill_color && color == mid_color)
  {
    radius += Point2(line_width * 0.5f, line_width * 0.5f);

    for (int i = 0; i < count; i += 2)
    {
      GuiVertex *qv = qCacheAllocT<GuiVertex>(3);

      Point2 d0 = points[i];
      Point2 d1 = points[i + 1];
      Point2 d2 = points[i + 2];

      MAKEDATA(0, 0, color, pos);
      MAKEDATA(0, 1, color, pos + Point2(d2.x * radius.x, d2.y * radius.y));
      MAKEDATA(0, 2, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(0, 3, color, pos + Point2(d0.x * radius.x, d0.y * radius.y));

      MAKEDATA(1, 0, E3DCOLOR(0), pos + Point2(d1.x * (radius.x + 1), d1.y * (radius.y + 1)));
      MAKEDATA(1, 1, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(1, 2, color, pos + Point2(d0.x * radius.x, d0.y * radius.y));
      MAKEDATA(1, 3, E3DCOLOR(0), pos + Point2(d0.x * (radius.x + 1), d0.y * (radius.y + 1)));

      MAKEDATA(2, 0, E3DCOLOR(0), pos + Point2(d2.x * (radius.x + 1), d2.y * (radius.y + 1)));
      MAKEDATA(2, 1, color, pos + Point2(d2.x * radius.x, d2.y * radius.y));
      MAKEDATA(2, 2, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(2, 3, E3DCOLOR(0), pos + Point2(d1.x * (radius.x + 1), d1.y * (radius.y + 1)));
    }
  }
  else // color != fill_color
  {
    Point2 outerR = radius + Point2(line_width, line_width) * 0.5f;
    Point2 innerR = radius - Point2(line_width, line_width) * 0.5f;
    bool visible = true;

    for (int i = 0; i < count; i += 2)
    {
      Point2 d0 = points[i];
      Point2 d1 = points[i + 1];
      Point2 d2 = points[i + 2];

      checkVisibilityAfter--;
      if (checkVisibilityAfter <= 0)
      {
        Point2 p[4];
        p[0] = pos + Point2(d0.x * (innerR.x - 2), d0.y * (innerR.y - 2));
        p[1] = pos + Point2(d0.x * (outerR.x + 2), d0.y * (outerR.y + 2));
        p[2] = pos + Point2(d2.x * (innerR.x - 2), d2.y * (innerR.y - 2));
        p[3] = pos + Point2(d2.x * (outerR.x + 2), d2.y * (outerR.y + 2));
        visible = vpBoxIsVisible(p[0], p[1], p[2], p[3]);
        if (visible)
          checkVisibilityAfter = 16;
      }

      if (!visible)
        continue;

      GuiVertex *qv = qCacheAllocT<GuiVertex>(fill_color ? 7 : 6);


      MAKEDATA(0, 0, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(0, 1, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
      MAKEDATA(0, 2, fill_color, pos + Point2(d0.x * innerR.x, d0.y * innerR.y));
      MAKEDATA(0, 3, mid_color, pos + Point2(d0.x * (innerR.x + 1), d0.y * (innerR.y + 1)));

      MAKEDATA(1, 0, mid_color, pos + Point2(d2.x * (innerR.x + 1), d2.y * (innerR.y + 1)));
      MAKEDATA(1, 1, fill_color, pos + Point2(d2.x * innerR.x, d2.y * innerR.y));
      MAKEDATA(1, 2, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
      MAKEDATA(1, 3, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));

      MAKEDATA(2, 0, color, pos + Point2(d1.x * (outerR.x), d1.y * (outerR.y)));
      MAKEDATA(2, 1, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(2, 2, mid_color, pos + Point2(d0.x * (innerR.x + 1), d0.y * (innerR.y + 1)));
      MAKEDATA(2, 3, color, pos + Point2(d0.x * (outerR.x), d0.y * (outerR.y)));

      MAKEDATA(3, 0, color, pos + Point2(d2.x * (outerR.x), d2.y * (outerR.y)));
      MAKEDATA(3, 1, mid_color, pos + Point2(d2.x * (innerR.x + 1), d2.y * (innerR.y + 1)));
      MAKEDATA(3, 2, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(3, 3, color, pos + Point2(d1.x * (outerR.x), d1.y * (outerR.y)));

      MAKEDATA(4, 0, E3DCOLOR(0), pos + Point2(d1.x * (outerR.x + 1), d1.y * (outerR.y + 1)));
      MAKEDATA(4, 1, color, pos + Point2(d1.x * outerR.x, d1.y * outerR.y));
      MAKEDATA(4, 2, color, pos + Point2(d0.x * outerR.x, d0.y * outerR.y));
      MAKEDATA(4, 3, E3DCOLOR(0), pos + Point2(d0.x * (outerR.x + 1), d0.y * (outerR.y + 1)));

      MAKEDATA(5, 0, E3DCOLOR(0), pos + Point2(d2.x * (outerR.x + 1), d2.y * (outerR.y + 1)));
      MAKEDATA(5, 1, color, pos + Point2(d2.x * outerR.x, d2.y * outerR.y));
      MAKEDATA(5, 2, color, pos + Point2(d1.x * outerR.x, d1.y * outerR.y));
      MAKEDATA(5, 3, E3DCOLOR(0), pos + Point2(d1.x * (outerR.x + 1), d1.y * (outerR.y + 1)));

      if (fill_color)
      {
        MAKEDATA(6, 0, fill_color, pos);
        MAKEDATA(6, 1, fill_color, pos + Point2(d2.x * innerR.x, d2.y * innerR.y));
        MAKEDATA(6, 2, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
        MAKEDATA(6, 3, fill_color, pos + Point2(d0.x * innerR.x, d0.y * innerR.y));
      }
    }
  }
}

void GuiContext::render_ellipse_aa(Point2 pos, Point2 radius, float line_width, E3DCOLOR color, E3DCOLOR fill_color)
{
  render_ellipse_aa(pos, radius, line_width, color, color, fill_color);
}

void GuiContext::render_sector_aa(Point2 pos, Point2 radius, Point2 angles, float line_width, E3DCOLOR color, E3DCOLOR mid_color,
  E3DCOLOR fill_color)
{
  if (!color && !fill_color && !mid_color)
    return;

  if (line_width < 1.f)
    line_width = 1;

  if (!vpBoxIsVisible(pos - radius - Point2(line_width, line_width), pos + radius + Point2(line_width, line_width)))
  {
    return;
  }

  float bias = PI * 4096;
  angles = Point2(fmodf((-angles.y + bias + PI * 0.5), PI * 2), fmodf((-angles.x + bias + PI * 0.5), PI * 2));
  if (angles.x == angles.y)
    return;

  if (angles.y < angles.x)
    angles.y += PI * 2;


  radius -= Point2(0.5f, 0.5f);
  float maxR = max(radius.x, radius.y);
  int count = 12 + (int(cvt((maxR + line_width * 0.5f) * (angles.y - angles.x) / (2 * PI), 4, 110, 0, 96)) & 0xfffc);

  float astep = (angles.y - angles.x) / count;
  float angle = angles.x;
  //        sincos(i * 2 * PI / count, sc.x, sc.y);

  reset_textures();

  if (color == fill_color && color == mid_color)
  {
    radius += Point2(line_width * 0.5f, line_width * 0.5f);

    for (int ci = 0; ci < count; ci += 2, angle += astep * 2)
    {
      GuiVertex *qv = qCacheAllocT<GuiVertex>(3);

      Point2 d0;
      Point2 d1;
      Point2 d2;
      sincos(angle, d0.x, d0.y);
      sincos(angle + astep, d1.x, d1.y);
      sincos((ci + 2 >= count) ? angles.y : angle + astep * 2, d2.x, d2.y);

      MAKEDATA(0, 0, color, pos);
      MAKEDATA(0, 1, color, pos + Point2(d2.x * radius.x, d2.y * radius.y));
      MAKEDATA(0, 2, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(0, 3, color, pos + Point2(d0.x * radius.x, d0.y * radius.y));

      MAKEDATA(1, 0, E3DCOLOR(0), pos + Point2(d1.x * (radius.x + 1), d1.y * (radius.y + 1)));
      MAKEDATA(1, 1, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(1, 2, color, pos + Point2(d0.x * radius.x, d0.y * radius.y));
      MAKEDATA(1, 3, E3DCOLOR(0), pos + Point2(d0.x * (radius.x + 1), d0.y * (radius.y + 1)));

      MAKEDATA(2, 0, E3DCOLOR(0), pos + Point2(d2.x * (radius.x + 1), d2.y * (radius.y + 1)));
      MAKEDATA(2, 1, color, pos + Point2(d2.x * radius.x, d2.y * radius.y));
      MAKEDATA(2, 2, color, pos + Point2(d1.x * radius.x, d1.y * radius.y));
      MAKEDATA(2, 3, E3DCOLOR(0), pos + Point2(d1.x * (radius.x + 1), d1.y * (radius.y + 1)));
    }
  }
  else // color != fill_color
  {
    Point2 outerR = radius + Point2(line_width, line_width) * 0.5f;
    Point2 innerR = radius - Point2(line_width, line_width) * 0.5f;

    for (int ci = 0; ci < count; ci += 2, angle += astep * 2)
    {
      Point2 d0;
      Point2 d1;
      Point2 d2;
      sincos(angle, d0.x, d0.y);
      sincos(angle + astep, d1.x, d1.y);
      sincos((ci + 2 >= count) ? angles.y : angle + astep * 2, d2.x, d2.y);

      GuiVertex *qv = qCacheAllocT<GuiVertex>(fill_color ? 7 : 6);

      MAKEDATA(0, 0, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(0, 1, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
      MAKEDATA(0, 2, fill_color, pos + Point2(d0.x * innerR.x, d0.y * innerR.y));
      MAKEDATA(0, 3, mid_color, pos + Point2(d0.x * (innerR.x + 1), d0.y * (innerR.y + 1)));

      MAKEDATA(1, 0, mid_color, pos + Point2(d2.x * (innerR.x + 1), d2.y * (innerR.y + 1)));
      MAKEDATA(1, 1, fill_color, pos + Point2(d2.x * innerR.x, d2.y * innerR.y));
      MAKEDATA(1, 2, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
      MAKEDATA(1, 3, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));

      MAKEDATA(2, 0, color, pos + Point2(d1.x * (outerR.x), d1.y * (outerR.y)));
      MAKEDATA(2, 1, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(2, 2, mid_color, pos + Point2(d0.x * (innerR.x + 1), d0.y * (innerR.y + 1)));
      MAKEDATA(2, 3, color, pos + Point2(d0.x * (outerR.x), d0.y * (outerR.y)));

      MAKEDATA(3, 0, color, pos + Point2(d2.x * (outerR.x), d2.y * (outerR.y)));
      MAKEDATA(3, 1, mid_color, pos + Point2(d2.x * (innerR.x + 1), d2.y * (innerR.y + 1)));
      MAKEDATA(3, 2, mid_color, pos + Point2(d1.x * (innerR.x + 1), d1.y * (innerR.y + 1)));
      MAKEDATA(3, 3, color, pos + Point2(d1.x * (outerR.x), d1.y * (outerR.y)));

      MAKEDATA(4, 0, E3DCOLOR(0), pos + Point2(d1.x * (outerR.x + 1), d1.y * (outerR.y + 1)));
      MAKEDATA(4, 1, color, pos + Point2(d1.x * outerR.x, d1.y * outerR.y));
      MAKEDATA(4, 2, color, pos + Point2(d0.x * outerR.x, d0.y * outerR.y));
      MAKEDATA(4, 3, E3DCOLOR(0), pos + Point2(d0.x * (outerR.x + 1), d0.y * (outerR.y + 1)));

      MAKEDATA(5, 0, E3DCOLOR(0), pos + Point2(d2.x * (outerR.x + 1), d2.y * (outerR.y + 1)));
      MAKEDATA(5, 1, color, pos + Point2(d2.x * outerR.x, d2.y * outerR.y));
      MAKEDATA(5, 2, color, pos + Point2(d1.x * outerR.x, d1.y * outerR.y));
      MAKEDATA(5, 3, E3DCOLOR(0), pos + Point2(d1.x * (outerR.x + 1), d1.y * (outerR.y + 1)));

      if (fill_color)
      {
        MAKEDATA(6, 0, fill_color, pos);
        MAKEDATA(6, 1, fill_color, pos + Point2(d2.x * innerR.x, d2.y * innerR.y));
        MAKEDATA(6, 2, fill_color, pos + Point2(d1.x * innerR.x, d1.y * innerR.y));
        MAKEDATA(6, 3, fill_color, pos + Point2(d0.x * innerR.x, d0.y * innerR.y));
      }
    }
  }
}

void GuiContext::render_sector_aa(Point2 pos, Point2 radius, Point2 angles, float line_width, E3DCOLOR color, E3DCOLOR fill_color)
{
  render_sector_aa(pos, radius, angles, line_width, color, color, fill_color);
}

void GuiContext::render_rectangle_aa(Point2 lt, Point2 rb, float line_width, E3DCOLOR color, E3DCOLOR mid_color, E3DCOLOR fill_color)
{
  if (!color && !fill_color && !mid_color)
    return;

  reset_textures();

  if (line_width < 1.f)
    line_width = 1.f;

  // 0-----------------1
  // | 2-------------3 |
  // | | 4---------5 | |
  // | | | 6-----7 | | |
  // | | | |     | | | |
  // | | | |     | | | |
  // | | | 8-----9 | | |
  // | |10---------11| |
  // |12-------------13|
  // 14-----------------15
  //
  // 0 2 3 1  2 4 5 3  4 6 7 5  1 3 13 15  3 5 11 13  5 7 9 11  15 13 12 14  13 11 10 12  11 9 8 10  14 12 2 0  12 10 4 2  10 8 6 4
  // 6 8 9 7


  Point2 halfW = Point2(line_width, line_width) * 0.5;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(fill_color ? 13 : 12);

  lt += Point2(0.5f, 0.5f);
  rb -= Point2(0.5f, 0.5f);

  carray<Point2, 16> p;
  p[0] = lt - halfW - Point2(1, 1);
  p[1] = Point2(rb.x, lt.y) + Point2(halfW.x, -halfW.x) + Point2(1, -1);
  p[2] = lt - halfW;
  p[3] = Point2(rb.x, lt.y) + Point2(halfW.x, -halfW.x);
  p[4] = lt + halfW - Point2(1, 1);
  p[5] = Point2(rb.x, lt.y) - Point2(halfW.x, -halfW.x) + Point2(1, -1);
  p[6] = lt + halfW;
  p[7] = Point2(rb.x, lt.y) - Point2(halfW.x, -halfW.x);
  p[8] = Point2(lt.x, rb.y) - Point2(-halfW.x, halfW.x);
  p[9] = rb - halfW;
  p[10] = Point2(lt.x, rb.y) - Point2(-halfW.x, halfW.x) + Point2(-1, 1);
  p[11] = rb - halfW + Point2(1, 1);
  p[12] = Point2(lt.x, rb.y) + Point2(-halfW.x, halfW.x);
  p[13] = rb + halfW;
  p[14] = Point2(lt.x, rb.y) + Point2(-halfW.x, halfW.x) + Point2(-1, 1);
  p[15] = rb + halfW + Point2(1, 1);

  MAKEDATA(0, 0, E3DCOLOR(0), p[0]);
  MAKEDATA(0, 1, color, p[2]);
  MAKEDATA(0, 2, color, p[3]);
  MAKEDATA(0, 3, E3DCOLOR(0), p[1]);

  MAKEDATA(1, 0, color, p[2]);
  MAKEDATA(1, 1, mid_color, p[4]);
  MAKEDATA(1, 2, mid_color, p[5]);
  MAKEDATA(1, 3, color, p[3]);

  MAKEDATA(2, 0, mid_color, p[4]);
  MAKEDATA(2, 1, fill_color, p[6]);
  MAKEDATA(2, 2, fill_color, p[7]);
  MAKEDATA(2, 3, mid_color, p[5]);

  MAKEDATA(3, 0, E3DCOLOR(0), p[1]);
  MAKEDATA(3, 1, color, p[3]);
  MAKEDATA(3, 2, color, p[13]);
  MAKEDATA(3, 3, E3DCOLOR(0), p[15]);

  MAKEDATA(4, 0, color, p[3]);
  MAKEDATA(4, 1, mid_color, p[5]);
  MAKEDATA(4, 2, mid_color, p[11]);
  MAKEDATA(4, 3, color, p[13]);

  MAKEDATA(5, 0, mid_color, p[5]);
  MAKEDATA(5, 1, fill_color, p[7]);
  MAKEDATA(5, 2, fill_color, p[9]);
  MAKEDATA(5, 3, mid_color, p[11]);

  MAKEDATA(6, 0, E3DCOLOR(0), p[15]);
  MAKEDATA(6, 1, color, p[13]);
  MAKEDATA(6, 2, color, p[12]);
  MAKEDATA(6, 3, E3DCOLOR(0), p[14]);

  MAKEDATA(7, 0, color, p[13]);
  MAKEDATA(7, 1, mid_color, p[11]);
  MAKEDATA(7, 2, mid_color, p[10]);
  MAKEDATA(7, 3, color, p[12]);

  MAKEDATA(8, 0, mid_color, p[11]);
  MAKEDATA(8, 1, fill_color, p[9]);
  MAKEDATA(8, 2, fill_color, p[8]);
  MAKEDATA(8, 3, mid_color, p[10]);

  MAKEDATA(9, 0, E3DCOLOR(0), p[14]);
  MAKEDATA(9, 1, color, p[12]);
  MAKEDATA(9, 2, color, p[2]);
  MAKEDATA(9, 3, E3DCOLOR(0), p[0]);

  MAKEDATA(10, 0, color, p[12]);
  MAKEDATA(10, 1, mid_color, p[10]);
  MAKEDATA(10, 2, mid_color, p[4]);
  MAKEDATA(10, 3, color, p[2]);

  MAKEDATA(11, 0, mid_color, p[10]);
  MAKEDATA(11, 1, fill_color, p[8]);
  MAKEDATA(11, 2, fill_color, p[6]);
  MAKEDATA(11, 3, mid_color, p[4]);

  if (fill_color)
  {
    MAKEDATA(12, 0, fill_color, p[6]);
    MAKEDATA(12, 1, fill_color, p[8]);
    MAKEDATA(12, 2, fill_color, p[9]);
    MAKEDATA(12, 3, fill_color, p[7]);
  }
}

void StdGuiRender::GuiContext::render_line_gradient_out(Point2 from, Point2 to, E3DCOLOR center_col, float center_width,
  float outer_width, E3DCOLOR outer_col)
{
  reset_textures();

  // 0 --------- 3
  // | \       / |
  // |   1 - 2   |
  // |   |   |   |
  // |   |   |   |
  // |   |   |   |
  // |   |   |   |
  // |   |   |   |
  // |   5 - 6   |
  // | /       \ |
  // 4 ----------7

  Point2 y = to - from;
  y.normalize();

  Point2 x = Point2(y.y, -y.x);
  x.normalize();

  carray<Point2, 16> p;
  p[0] = to - x * (center_width + outer_width) + y * outer_width;
  p[1] = to - x * (center_width);
  p[2] = to + x * (center_width);
  p[3] = to + x * (center_width + outer_width) + y * outer_width;
  p[4] = from - x * (center_width + outer_width) - y * outer_width;
  p[5] = from - x * (center_width);
  p[6] = from + x * (center_width);
  p[7] = from + x * (center_width + outer_width) - y * outer_width;

  GuiVertex *qv = qCacheAllocT<GuiVertex>(5);

  MAKEDATA(0, 0, outer_col, p[0]);
  MAKEDATA(0, 1, center_col, p[1]);
  MAKEDATA(0, 2, center_col, p[5]);
  MAKEDATA(0, 3, outer_col, p[4]);

  MAKEDATA(1, 0, center_col, p[1]);
  MAKEDATA(1, 1, center_col, p[2]);
  MAKEDATA(1, 2, center_col, p[6]);
  MAKEDATA(1, 3, center_col, p[5]);

  MAKEDATA(2, 0, center_col, p[2]);
  MAKEDATA(2, 1, outer_col, p[3]);
  MAKEDATA(2, 2, outer_col, p[7]);
  MAKEDATA(2, 3, center_col, p[6]);

  MAKEDATA(3, 0, outer_col, p[0]);
  MAKEDATA(3, 1, outer_col, p[3]);
  MAKEDATA(3, 2, center_col, p[2]);
  MAKEDATA(3, 3, center_col, p[1]);

  MAKEDATA(4, 0, center_col, p[5]);
  MAKEDATA(4, 1, center_col, p[6]);
  MAKEDATA(4, 2, outer_col, p[7]);
  MAKEDATA(4, 3, outer_col, p[4]);
}

void GuiContext::render_rectangle_aa(Point2 lt, Point2 rb, float line_width, E3DCOLOR color, E3DCOLOR fill_color)
{
  render_rectangle_aa(lt, rb, line_width, color, color, fill_color);
}

void GuiContext::render_smooth_round_rect(Point2 lt, Point2 rb, float corner_inner_radius, float corner_outer_radius, E3DCOLOR color)
{
  E3DCOLOR transparent(0);

  float innerR = max(corner_inner_radius, 0.0f);
  float outerR = max(corner_outer_radius, innerR);
  int cornerSegments = clamp(int(outerR * 0.5f) + 2, 4, 32);

  Tab<GuiVertex> vertices(framemem_ptr());
  vertices.resize(22 + (cornerSegments - 1) * 8);
  Tab<int> indices(framemem_ptr());
  indices.resize((9 + 3 * 4 * (cornerSegments + 1)) * 3);

  int curIndex = 0;
  int curVertex = 0;


#define SET_VERTEX(index, pos, c)   \
  {                                 \
    GuiVertex &v = vertices[index]; \
    v.color = c;                    \
    v.setPos(vertexTransform, pos); \
    v.zeroTc0();                    \
  }

#define SET_FACE(i0, i1, i2)    \
  {                             \
    indices[curIndex] = i0;     \
    indices[curIndex + 1] = i1; \
    indices[curIndex + 2] = i2; \
    curIndex += 3;              \
  }

#define SET_QUAD(i0, i1, i2, i3) \
  {                              \
    SET_FACE(i0, i2, i1);        \
    SET_FACE(i2, i3, i1);        \
  }

  //       8 9
  //       6 7
  // 20 18 0 1 14 16
  // 21 19 2 3 15 17
  //      10 11
  //      12 13


  SET_VERTEX(0, lt, color);
  SET_VERTEX(1, Point2(rb.x, lt.y), color);
  SET_VERTEX(2, Point2(lt.x, rb.y), color);
  SET_VERTEX(3, rb, color);
  SET_VERTEX(6, Point2(lt.x, lt.y - innerR), color);
  SET_VERTEX(7, Point2(rb.x, lt.y - innerR), color);
  SET_VERTEX(8, Point2(lt.x, lt.y - outerR), transparent);
  SET_VERTEX(9, Point2(rb.x, lt.y - outerR), transparent);
  SET_VERTEX(10, Point2(lt.x, rb.y + innerR), color);
  SET_VERTEX(11, Point2(rb.x, rb.y + innerR), color);
  SET_VERTEX(12, Point2(lt.x, rb.y + outerR), transparent);
  SET_VERTEX(13, Point2(rb.x, rb.y + outerR), transparent);
  SET_VERTEX(14, Point2(rb.x + innerR, lt.y), color);
  SET_VERTEX(15, Point2(rb.x + innerR, rb.y), color);
  SET_VERTEX(16, Point2(rb.x + outerR, lt.y), transparent);
  SET_VERTEX(17, Point2(rb.x + outerR, rb.y), transparent);
  SET_VERTEX(18, Point2(lt.x - innerR, lt.y), color);
  SET_VERTEX(19, Point2(lt.x - innerR, rb.y), color);
  SET_VERTEX(20, Point2(lt.x - outerR, lt.y), transparent);
  SET_VERTEX(21, Point2(lt.x - outerR, rb.y), transparent);

  SET_QUAD(8, 9, 6, 7);
  SET_QUAD(6, 7, 0, 1);
  SET_QUAD(0, 1, 2, 3);
  SET_QUAD(2, 3, 10, 11);
  SET_QUAD(10, 11, 12, 13);
  SET_QUAD(18, 0, 19, 2);
  SET_QUAD(20, 18, 21, 19);
  SET_QUAD(1, 14, 3, 15);
  SET_QUAD(14, 16, 15, 17);


  int vCnt = 0;
  curVertex = 22;

  for (int i = 1; i < cornerSegments; i++)
  {
    float s, c;
    sincos(i * PI * 0.5 / cornerSegments, s, c);
    int vi = curVertex + (i - 1) * 8;
    SET_VERTEX(vi + 0, Point2(-c, -s) * innerR + lt, color);
    SET_VERTEX(vi + 1, Point2(-c, -s) * outerR + lt, transparent);
    SET_VERTEX(vi + 2, Point2(c, -s) * innerR + Point2(rb.x, lt.y), color);
    SET_VERTEX(vi + 3, Point2(c, -s) * outerR + Point2(rb.x, lt.y), transparent);
    SET_VERTEX(vi + 4, Point2(c, s) * innerR + rb, color);
    SET_VERTEX(vi + 5, Point2(c, s) * outerR + rb, transparent);
    SET_VERTEX(vi + 6, Point2(-c, s) * innerR + Point2(lt.x, rb.y), color);
    SET_VERTEX(vi + 7, Point2(-c, s) * outerR + Point2(lt.x, rb.y), transparent);
    vCnt += 8;

    if (i == 1)
    {
      SET_FACE(18, 0, vi);
      SET_QUAD(vi + 1, vi, 20, 18);
      SET_FACE(1, 14, vi + 2);
      SET_QUAD(vi + 2, vi + 3, 14, 16);
      SET_FACE(3, 15, vi + 4);
      SET_QUAD(vi + 5, vi + 4, 17, 15);
      SET_FACE(2, 19, vi + 6);
      SET_QUAD(vi + 7, vi + 6, 21, 19);
    }
    else
    {
      SET_FACE(vi - 8, 0, vi);
      SET_QUAD(vi + 1, vi, vi - 8 + 1, vi - 8);
      SET_FACE(vi - 8 + 2, 1, vi + 2);
      SET_QUAD(vi + 1 + 2, vi + 2, vi - 8 + 1 + 2, vi - 8 + 2);
      SET_FACE(vi - 8 + 4, 3, vi + 4);
      SET_QUAD(vi + 1 + 4, vi + 4, vi - 8 + 1 + 4, vi - 8 + 4);
      SET_FACE(vi - 8 + 6, 2, vi + 6);
      SET_QUAD(vi + 1 + 6, vi + 6, vi - 8 + 1 + 6, vi - 8 + 6);
    }

    if (i == cornerSegments - 1)
    {
      SET_FACE(vi, 0, 6);
      SET_QUAD(8, 6, vi + 1, vi);
      SET_FACE(vi + 2, 1, 7);
      SET_QUAD(9, 7, vi + 1 + 2, vi + 2);
      SET_FACE(vi + 4, 3, 11);
      SET_QUAD(13, 11, vi + 1 + 4, vi + 4);
      SET_FACE(vi + 6, 2, 10);
      SET_QUAD(12, 10, vi + 1 + 6, vi + 6);
    }
  }

  start_raw_layer();
  draw_faces(vertices.data(), curVertex + vCnt, indices.data(), indices.size() / 3);
  end_raw_layer();

#undef SET_VERTEX
#undef SET_FACE
#undef SET_QUAD
}

#undef MAKEDATA

void GuiContext::set_picquad_color_matrix(const TMatrix4 *cm)
{
  if (cm == &TMatrix4::IDENT)
    cm = nullptr;

  if (!cm && guiState.params.colorM1)
    return;
  if (!cm)
  {
    memcpy(guiState.params.colorM, TMatrix4::IDENT.m, sizeof(guiState.params.colorM));
    guiState.params.colorM1 = 1;
  }
  else if (!guiState.params.colorM1 && memcmp(guiState.params.colorM, cm->m, sizeof(guiState.params.colorM)) == 0)
    return;
  else
  {
    memcpy(guiState.params.colorM, cm->m, sizeof(guiState.params.colorM));
    guiState.params.colorM1 = 0;
  }
  rollState |= ROLL_GUI_STATE;
}
void GuiContext::set_picquad_color_matrix_saturate(float factor)
{
  static const TMatrix4 toYYYA(0.299, 0.587, 0.114, 0, 0.299, 0.587, 0.114, 0, 0.299, 0.587, 0.114, 0, 0, 0, 0, 1);
  if (factor <= 0)
    set_picquad_color_matrix(&toYYYA);
  else if (factor == 1.0f)
    set_picquad_color_matrix(nullptr);
  else
  {
    TMatrix4 m(0.299 + 0.70099545 * factor, 0.587 - 0.5870010517 * factor, 0.114 - 0.1139943983 * factor, 0,
      0.299 - 0.2990041455 * factor, 0.587 + 0.413001793 * factor, 0.114 - 0.114001594 * factor, 0, 0.299 - 0.2989843443 * factor,
      0.587 - 0.5869952946 * factor, 0.114 + 0.88599996 * factor, 0, 0, 0, 0, 1);
    set_picquad_color_matrix(&m);
  }
}
void GuiContext::set_picquad_color_matrix_sepia(float factor)
{
  static const TMatrix4 toYYYA(0.393, 0.769, 0.189, 0, 0.349, 0.686, 0.168, 0, 0.272, 0.534, 0.131, 0, 0, 0, 0, 1);
  if (factor >= 1)
    set_picquad_color_matrix(&toYYYA);
  else if (factor <= 0)
    set_picquad_color_matrix(nullptr);
  else
  {
    TMatrix4 m(lerp(1.0, 0.393, factor), lerp(0.0, 0.769, factor), lerp(0.0, 0.189, factor), 0, lerp(0.0, 0.349, factor),
      lerp(1.0, 0.686, factor), lerp(0.0, 0.168, factor), 0, lerp(0.0, 0.272, factor), lerp(0.0, 0.534, factor),
      lerp(1.0, 0.131, factor), 0, 0, 0, 0, 1);
    set_picquad_color_matrix(&m);
  }
}

//************************************************************************
//* text stuff
//************************************************************************
// set curent text out position
void GuiContext::goto_xy(Point2 pos) { currentPos = pos; }

// get curent text out position
Point2 GuiContext::get_text_pos() { return currentPos; }

// set font spacing
void GuiContext::set_spacing(int spacing) { curRenderFont.spacing = spacing; }
void GuiContext::set_mono_width(int mw) { curRenderFont.monoW = mw; }
#undef GET_GLYPH_CHECK_FONT_ID

// get font spacing
int GuiContext::get_spacing() { return curRenderFont.spacing; }

// set current font
int GuiContext::set_font(int font_id, int font_spacing, int mono_w)
{
  int prevFontId = currentFontId;
  if (get_font_context(curRenderFont, font_id, font_spacing, mono_w))
    currentFontId = font_id;
  else
    currentFontId = -1;
  return prevFontId;
}
int GuiContext::set_font_ht(int font_ht)
{
  int prev_ht = curRenderFont.fontHt;
  curRenderFont.fontHt = font_ht;
  if (curRenderFont.font && !curRenderFont.font->isFullyDynamicFont())
    logerr("%s(%d) used for non-fulldynamic font", __FUNCTION__, font_ht);
  return prev_ht;
}

void GuiContext::set_draw_str_attr(FontFxType type, int ofs_x, int ofs_y, E3DCOLOR col, int factor_x32)
{
  LayerParams::FontFx &fx = guiState.params.ff;
  if (type != fx.type || ofs_x != fx.ofsX || ofs_y != fx.ofsY || col != fx.col || factor_x32 != fx.factor_x32)
  {
    guiState.fontAttr.setFontTex(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE);

    fx.type = type;
    fx.ofsX = ofs_x;
    fx.ofsY = ofs_y;
    fx.col = col;
    fx.factor_x32 = factor_x32;

    rollState |= ROLL_GUI_STATE;
  }
}

void GuiContext::set_draw_str_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, int su_x32, int sv_x32, int bv_ofs)
{
  auto &fontAttr = guiState.fontAttr;
  if (tex_id != fontAttr.getTex2() || bv_ofs != fontAttr.tex2bv_ofs || su_x32 != fontAttr.tex2su_x32 || sv_x32 != fontAttr.tex2sv_x32)
  {
    if (tex_id != fontAttr.getTex2() || tex_id != BAD_TEXTUREID)
      flushData();

    fontAttr.setTex2(tex_id, smp_id);
    fontAttr.tex2bv_ofs = bv_ofs;
    fontAttr.tex2su_x32 = su_x32;
    fontAttr.tex2sv_x32 = sv_x32;

    if (tex_id != BAD_TEXTUREID)
    {
      Texture *t = (Texture *)acquire_managed_tex(tex_id);
      if (t)
      {
        TextureInfo ti;
        t->getinfo(ti);
        fontAttr.tex2su = su_x32 ? (32.0 / su_x32) / 4.0 / ti.w : float(ti.h) / float(ti.w);
        fontAttr.tex2sv = sv_x32 ? (32.0 / sv_x32) / 4.0 / ti.h : 1.0 / 4.0;
      }
      else
        fontAttr.tex2su = fontAttr.tex2sv = 0;
      release_managed_tex(tex_id);
    }
    rollState |= ROLL_GUI_STATE;
  }
}

void GuiContext::start_font_str(float s)
{
  auto &fontAttr = guiState.fontAttr;
  if (fontAttr.getTex2() != BAD_TEXTUREID)
  {
    flushData();

    const auto *mx = curRenderFont.font->getFontMx(curRenderFont.fontHt);
    unsigned ascent = mx ? mx->ascent : curRenderFont.font->ascent;
    unsigned descent = mx ? mx->descent : curRenderFont.font->descent;
    if (fontAttr.tex2su_x32 && fontAttr.tex2sv_x32)
    {
      extState().setFontTex2Ofs(vertexTransform, fontAttr.tex2su, fontAttr.tex2sv, currentPos.x,
        currentPos.y - (fontAttr.tex2bv_ofs + ascent) * s);
    }
    else
    {
      float ts = 1.0 / s / 4.0 / (ascent + descent);
      extState().setFontTex2Ofs(vertexTransform, fontAttr.tex2su_x32 ? fontAttr.tex2su : fontAttr.tex2su * ts,
        fontAttr.tex2sv_x32 ? fontAttr.tex2sv : ts, currentPos.x, currentPos.y - (fontAttr.tex2bv_ofs + ascent) * s);
    }
    rollState |= ROLL_EXT_STATE;
  }
}
void GuiContext::set_user_state(int state, const float *v, int cnt)
{
  const int storageSize = countof(extState().user) - state;
  cnt = min(cnt, (int)(storageSize * sizeof(float)));
  if (cnt < 0)
    return;
  if (memcmp(&extState().user[0].r, v, cnt) == 0)
    return;
  flushData();
  memcpy(&extState().user[0].r, v, cnt);
  rollState |= ROLL_EXT_STATE;
}

static void update_font_fx_tex(GuiContext &ctx, TEXTUREID font_tex_id, d3d::SamplerHandle font_smp_id)
{
  if (!ctx.getRenderCallback() && ctx.guiState.params.ff.type != FFT_NONE && ctx.guiState.fontAttr.getFontTex() != font_tex_id)
  {
    ctx.guiState.fontAttr.setFontTex(font_tex_id, font_smp_id);
    ctx.setRollState(GuiContext::ROLL_GUI_STATE);
  }
}

#define ROUND_COORD(X) GUI_POS_SCALE_INV *GuiVertex::fast_floori(GUI_POS_SCALE *((X) + 0.05))

static inline void update_font_halftexel(GuiContext &ctx, TEXTUREID tex_id)
{
  ctx.prevTextTextureId = tex_id;
  TextureInfo tinfo;
  if (BaseTexture *tex = acquire_managed_tex(tex_id))
    tex->getinfo(tinfo, 0);
  release_managed_tex(tex_id);
  ctx.halfFontTexelX = 0.5f / (tinfo.w ? tinfo.w : 1);
  ctx.halfFontTexelY = 0.5f / (tinfo.h ? tinfo.h : 1);
  ctx.isCurrentFontTexturePow2 = is_pow2(tinfo.w) && is_pow2(tinfo.h);
}

static inline void enqueue_glyph(GuiContext &ctx, TEXTUREID tex_id, d3d::SamplerHandle smp_id, TEXTUREID tex_id2,
  d3d::SamplerHandle smp_id2, bool halftexel_expansion, float g_u0, float g_v0, float g_u1, float g_v1, int rbgw, int rbgh, Point2 lt,
  Point2 rb)
{
  ctx.set_textures(tex_id, smp_id, tex_id2, smp_id2, true);

  if (halftexel_expansion)
  {
    g_u0 -= ctx.halfFontTexelX;
    g_v0 -= ctx.halfFontTexelY;
    g_u1 += ctx.halfFontTexelX;
    g_v1 += ctx.halfFontTexelY;
  }


  const Point2 ltuv(g_u0, g_v0);
  const Point2 rbuv(g_u1, g_v1);
  int ltgw = -rbgw;
  int ltgh = -rbgh;

  GuiVertex *qv = ctx.qCacheAllocT<GuiVertex>(1);
  assign_color(qv, ctx.currentColor, ctx.curQuadMask);

#define MAKEDATA(index, p1, p2)                     \
  {                                                 \
    GuiVertex &v = qv[index];                       \
    v.setPos(ctx.getVertexTransform(), p1.x, p2.y); \
    v.setTc0(p1##uv.x, p2##uv.y);                   \
  }

  FILL_VERTICES;

#undef MAKEDATA

  if (ctx.guiState.params.ff.type != FFT_NONE)
  {
    qv[0].tc1u = ltgw;
    qv[0].tc1v = ltgh;
    qv[1].tc1u = rbgw;
    qv[1].tc1v = ltgh;
    qv[2].tc1u = rbgw;
    qv[2].tc1v = rbgh;
    qv[3].tc1u = ltgw;
    qv[3].tc1v = rbgh;
  }
  // renderQuad.render(use_buffer);
}
static inline void enqueue_glyph(GuiContext &ctx, TEXTUREID tex_id, d3d::SamplerHandle smp_id, bool halftexel_expansion, float g_u0,
  float g_v0, float g_u1, float g_v1, int rbgw, int rbgh, Point2 lt, Point2 rb)
{
  return enqueue_glyph(ctx, tex_id, smp_id, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, halftexel_expansion, g_u0, g_v0, g_u1, g_v1,
    rbgw, rbgh, lt, rb);
}

// render single character using current font with scale
static void draw_char_internal(GuiContext &ctx, const DagorFontBinDump::GlyphData &g, wchar_t ch, real scale, wchar_t next_ch = 0)
{
  StdGuiFontContext &curRenderFont = ctx.curRenderFont;

  if (g.texIdx >= curRenderFont.font->tex.size())
  {
    ctx.currentPos.x +=
      ((curRenderFont.monoW ? curRenderFont.monoW : curRenderFont.font->getDx(g, next_ch)) + curRenderFont.spacing) * scale;
    if (g.isDynGrpValid() && ch)
    {
      curRenderFont.font->reqCharGen(ch);
      if (ctx.getRenderCallback())
        ctx.getRenderCallback()->missingGlyphs++;
    }
    return;
  }

  int dx2 = 0;
  ctx.currentPos.x += curRenderFont.font->getDx2(dx2, curRenderFont.monoW, g, next_ch) * scale;

  TEXTUREID fontTexId = curRenderFont.font->tex[g.texIdx].texId;

  if (ctx.prevTextTextureId != fontTexId)
    update_font_halftexel(ctx, fontTexId);

  bool halftexelExpansion = ctx.isCurrentFontTexturePow2;

  Point2 lt = curRenderFont.font->getGlyphLt(g);
  if (halftexelExpansion)
    lt -= Point2(0.5, 0.5);
  lt.x = ROUND_COORD(lt.x * scale + ctx.currentPos.x);
  lt.y = (lt.y * scale + ctx.currentPos.y);
  Point2 rb(lt.x + (g.x1 - g.x0 + (halftexelExpansion ? 1 : 0)) * scale, lt.y + (g.y1 - g.y0 + (halftexelExpansion ? 1 : 0)) * scale);

  ctx.currentPos.x += (dx2 + curRenderFont.spacing) * scale;

  if ((!ctx.getRenderCallback() || ctx.getRenderCallback()->checkVis) && !ctx.vpBoxIsVisible(lt, rb))
    return;

  d3d::SamplerHandle fontSmpId = d3d::request_sampler({});

  update_font_fx_tex(ctx, fontTexId, fontSmpId);
  G_ASSERT(curRenderFont.font->isClassicFontTex());
  enqueue_glyph(ctx, fontTexId, fontSmpId, halftexelExpansion, g.u0, g.v0, g.u1, g.v1, g.x1 - g.x0, g.y1 - g.y0, lt, rb);
}

static void draw_char_internal_hb(GuiContext &ctx, const hb_glyph_position_t &glyph_pos, hb_codepoint_t cp, real scale, float uv_scale,
  int &sum_x_advance, int &sum_y_advance, int adv_sp, DagorFontBinDump *f, int fgidx)
{
  const DagorFontBinDump::GlyphPlace *gp = f->getDynGlyph(fgidx, cp);
  int mono_w = ctx.curRenderFont.monoW ? int(floorf(ctx.curRenderFont.monoW * scale * 64)) : 0;

  if (gp && gp->texIdx < DagorFontBinDump::dynFontTex.size())
  {
    TEXTUREID fontTexId = DagorFontBinDump::dynFontTex[gp->texIdx].texId;
    d3d::SamplerHandle fontSmpId = d3d::request_sampler({});

    if (ctx.prevTextTextureId != fontTexId)
      update_font_halftexel(ctx, fontTexId);

    G_ASSERT(ctx.isCurrentFontTexturePow2);
    // using half texel expansion
    int gw = gp->u1 - gp->u0, gh = gp->v1 - gp->v0;
    int gx_offset = (sum_x_advance + glyph_pos.x_offset + (mono_w ? (mono_w - gw * 64) / 2 : 0) + 32) / 64 + gp->ox;
    int gy_offset = (sum_y_advance + glyph_pos.y_offset + 32) / 64 + gp->oy;

    Point2 lt(ROUND_COORD((gx_offset - 0.5f) * scale + ctx.currentPos.x), (gy_offset - 0.5f) * scale + ctx.currentPos.y);
    Point2 rb(lt.x + (gw + 1) * scale, lt.y + (gh + 1) * scale);

    if ((!ctx.getRenderCallback() || ctx.getRenderCallback()->checkVis) && !ctx.vpBoxIsVisible(lt, rb))
      goto ret;

    update_font_fx_tex(ctx, fontTexId, fontSmpId);
    enqueue_glyph(ctx, fontTexId, fontSmpId, true, gp->u0 * uv_scale, gp->v0 * uv_scale, gp->u1 * uv_scale, gp->v1 * uv_scale, gw, gh,
      lt, rb);
  }
  else if (!gp || gp->texIdx == gp->TEXIDX_NOTPLACED)
  {
    f->reqCharGen(cp, f->getHtForFontGlyphIdx(fgidx));
    if (ctx.getRenderCallback())
      ctx.getRenderCallback()->missingGlyphs++;
  }

ret:
  sum_x_advance += mono_w ? mono_w : glyph_pos.x_advance + adv_sp;
  sum_y_advance += glyph_pos.y_advance;
}

// render single character using current font
void GuiContext::draw_char_u(wchar_t wc)
{
  if (curRenderFont.font->isFullyDynamicFont())
    return draw_str_scaled_u(1.0f, &wc, 1);
  start_font_str(1.0);
  draw_char_internal(*this, curRenderFont.font->getGlyphU(wc), wc, 1.0f);
}

// render text string using current font with scale
void GuiContext::draw_str_scaled_u(real scale, const wchar_t *str, int len)
{
#if LOG_DRAWS
  debug("STDG:draw_str_scaled_u");
#endif
  G_ASSERT(curRenderFont.font);
  if (!str)
    return;

  if ((!recCb || recCb->checkVis) && !vpBoxIsVisibleRBCheck(currentPos - Point2(0, curRenderFont.font->ascent)))
    return;

  start_font_str(scale);
  if (curRenderFont.font->isFullyDynamicFont())
  {
    int fgidx = -1;
    float uv_scale = 1.0f / DagorFontBinDump::dynFontTexSize;
    int adv_x = 0, adv_y = 0, adv_sp = curRenderFont.spacing * 64;
    int font_ht = curRenderFont.fontHt;
    if (!font_ht)
      font_ht = curRenderFont.font->getFontHt();
    if (len < 0)
      len = wcslen(str);

    DagorFontBinDump::ScopeHBuf buf;
    for (int eff_font_ht = 0, seglen = len; len > 0; str += seglen, len -= seglen)
    {
      DagorFontBinDump *f = curRenderFont.font->dynfont_get_next_segment(str, len, eff_font_ht = font_ht, seglen);
      f->dynfont_prepare_str(buf, eff_font_ht, str, seglen, &fgidx);
      if (fgidx < 0)
        break;
      unsigned glyph_count = 0;
      hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
      hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
      for (int i = 0; i < glyph_count; ++i, glyph_pos++, glyph_info++)
        draw_char_internal_hb(*this, *glyph_pos, glyph_info->codepoint, scale, uv_scale, adv_x, adv_y, adv_sp, f, fgidx);
    }
    currentPos.x += float((adv_x + 32) / 64) * scale;
    currentPos.y += float((adv_y + 32) / 64) * scale;
    return;
  }

  // legacy font case
  while (*str && len)
  {
    if (*str >= 0x20)
      draw_char_internal(*this, curRenderFont.font->getGlyphU(*str), *str, scale, str[1]);
    ++str;
    len--;
  }
}

void GuiContext::draw_str_scaled(real scale, const char *str, int len)
{
#if LOG_DRAWS
  debug("STDG:draw_str_scaled '%s'", str);
#endif
  G_ASSERT(curRenderFont.font);
  if (!str)
    return;

  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  return draw_str_scaled_u(scale, tmpU16, len);
}

bool GuiContext::draw_str_scaled_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt,
  SmallTab<d3d::SamplerHandle> &out_smp, unsigned dsb_flags, float scale, const char *str, int len)
{
  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  return draw_str_scaled_u_buf(out_qv, out_tex_qcnt, out_smp, dsb_flags, scale, tmpU16);
}
bool GuiContext::draw_str_scaled_u_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt,
  SmallTab<d3d::SamplerHandle> &out_smp, unsigned dsb_flags, float scale, const wchar_t *str, int len)
{
  if (!str || !len || !*str)
  {
    out_qv.resize(0);
    out_tex_qcnt.resize(1);
    out_tex_qcnt[0] = dyn_font_atlas_reset_generation;
    return true;
  }

  RecorderCallback cb(out_qv, out_tex_qcnt, out_smp, dsb_flags & DSBFLAG_checkVis);
  GuiVertex v0 = {0};
  char prev_m[sizeof(vertexTransform.vtm)];
  Point2 prev_curPos = currentPos;
  if (!(dsb_flags & DSBFLAG_abs))
  {
    memcpy(prev_m, vertexTransform.vtm, sizeof(prev_m));
    vertexTransform.resetViewTm();
    if (cb.checkVis)
      v0.setPos(vertexTransform, currentPos.x, currentPos.y);
    else
      currentPos.x = currentPos.y = 0;
  }

  out_tex_qcnt.push_back(dyn_font_atlas_reset_generation);
  recCb = &cb;
  draw_str_scaled_u(scale, str, len);
  recCb = NULL;

  if (!(dsb_flags & DSBFLAG_abs) && (v0.px | v0.py))
    for (int j = 0; j < out_qv.size(); j++)
      out_qv[j].px -= v0.px, out_qv[j].py -= v0.py;
  if (!(dsb_flags & DSBFLAG_abs))
  {
    memcpy(vertexTransform.vtm, prev_m, sizeof(prev_m));
    if (!cb.checkVis)
      currentPos += prev_curPos;
  }

  return cb.missingGlyphs == 0;
}
void GuiContext::render_str_buf(dag::ConstSpan<GuiVertex> buf_qv, dag::ConstSpan<uint16_t> tex_qcnt,
  dag::ConstSpan<d3d::SamplerHandle> samplers, unsigned dsb_flags)
{
  if (tex_qcnt.size() < 3 || tex_qcnt[0] != dyn_font_atlas_reset_generation)
    return;
  G_ASSERT(tex_qcnt.size() % 2 == 1);

  IBBox2 view_bb, quad_bb;
  if (dsb_flags & DSBFLAG_abs)
  {
    // use final viewport
    view_bb[0].set(viewX * int(GUI_POS_SCALE), viewY * int(GUI_POS_SCALE));
    view_bb[1].set((viewX + viewW) * int(GUI_POS_SCALE), (viewY + viewH) * int(GUI_POS_SCALE));
  }
  else
  {
    if (preTransformViewport.isempty())
      return;
    // use pre-transform viewport (shifted back to compensate currentPos)
    view_bb[0].set_xy((preTransformViewport[0] - currentPos) * GUI_POS_SCALE);
    view_bb[1].set_xy((preTransformViewport[1] - currentPos) * GUI_POS_SCALE);
  }

  const GuiVertex *cur_qv = buf_qv.data();
  const d3d::SamplerHandle *cur_smp = samplers.data();
  for (const uint16_t *tq = &tex_qcnt[1], *tq_end = tex_qcnt.data() + tex_qcnt.size(); tq < tq_end; tq += 2, cur_smp++)
  {
    int full_qnum = tq[1];
    if (!full_qnum)
      continue;
    TEXTUREID font_tid = D3DRESID::fromIndex(tq[0]);
    d3d::SamplerHandle font_smp = *cur_smp;
    update_font_fx_tex(*this, font_tid, font_smp);
    set_textures(font_tid, font_smp, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, true);
    while (full_qnum > 0)
    {
      int qnum = full_qnum < qCacheCapacity() ? full_qnum : qCacheCapacity(), qunused = 0;
      GuiVertex *qv = qCacheAllocT<GuiVertex>(qnum);
      if ((dsb_flags & (DSBFLAG_abs | DSBFLAG_checkVis)) == DSBFLAG_abs)
        memcpy(qv, cur_qv, 4 * qnum * elem_size(buf_qv));
      else if (dsb_flags & DSBFLAG_checkVis)
      {
        for (const GuiVertex *sqv = cur_qv, *sqv_e = sqv + 4 * qnum; sqv < sqv_e; sqv += 4)
        {
          memcpy(qv, sqv, 4 * elem_size(buf_qv));
          if (dsb_flags & DSBFLAG_abs)
          {
            // quads in abs coords may be transformed, so we calc full bbox
            quad_bb[0].x = min(min((qv + 0)->px, (qv + 1)->px), min((qv + 2)->px, (qv + 3)->px));
            quad_bb[1].x = max(max((qv + 0)->px, (qv + 1)->px), max((qv + 2)->px, (qv + 3)->px));
            quad_bb[0].y = min(min((qv + 0)->py, (qv + 1)->py), min((qv + 2)->py, (qv + 3)->py));
            quad_bb[1].y = max(max((qv + 0)->py, (qv + 1)->py), max((qv + 2)->py, (qv + 3)->py));
          }
          else
          {
            // quads in rel coords are not transformed, so get point 0 and 2 as bbox extents
            quad_bb[0].set((qv + 0)->px, (qv + 0)->py);
            quad_bb[1].set((qv + 2)->px, (qv + 2)->py);
            for (GuiVertex *v = qv, *v_e = v + 4; v < v_e; v++)
              v->setPos(vertexTransform, v->px * GUI_POS_SCALE_INV + currentPos.x, v->py * GUI_POS_SCALE_INV + currentPos.y);
          }

          if (quad_bb & view_bb)
            qv += 4;
          else
            qunused++;
        }
        qv -= 4 * (qnum - qunused);
        qCacheReturnUnused(qunused);
      }
      else // DSBFLAG_rel && DSBFLAG_allQuads
      {
        memcpy(qv, cur_qv, 4 * qnum * elem_size(buf_qv));
        for (GuiVertex *qv_e = qv + 4 * qnum; qv < qv_e; qv++)
          qv->setPos(vertexTransform, qv->px * GUI_POS_SCALE_INV + currentPos.x, qv->py * GUI_POS_SCALE_INV + currentPos.y);
        qv -= 4 * qnum;
      }

      if (dsb_flags & DSBFLAG_curColor)
        for (GuiVertex *qv_e = qv + 4 * (qnum - qunused); qv < qv_e; qv++)
          qv->color = currentColor;

      cur_qv += qnum * 4;
      full_qnum -= qnum;
    }
    qCacheFlush(false);
  }
  G_ASSERTF(cur_qv <= buf_qv.data() + buf_qv.size(), "cur_qv-buf_qv.data()=%d buf_qv.size()=%d", cur_qv - buf_qv.data(),
    buf_qv.size());
}

// get string real bounding box
static BBox2 get_str_real_visible_bbox_u(const wchar_t *str, int len, const StdGuiFontContext &fctx, float &dx)
{
  G_ASSERT(fctx.font);

  dx = 0;
  if (!str || !len)
    return BBox2(0, 0, 0, 0);

  BBox2 box;
  if (fctx.monoW)
  {
    len = dag_wcsnlen(str, len);
    box[0].set(0, 0);
    box[1].set(len * fctx.monoW, 0);
  }
  if (fctx.font->isFullyDynamicFont())
  {
    if (fctx.monoW)
      dx = box[1].x, fctx.font->dynfont_update_bbox_y(fctx.fontHt, box, true);
    else
      dx = fctx.font->dynfont_compute_str_visbox_u(fctx.fontHt, str, len, 1.0f, fctx.spacing, box);
    return box;
  }

  Point2 pos = P2::ZERO;
  while (*str && len)
  {
    const DagorFontBinDump::GlyphData &g = fctx.font->getGlyphU(*str);
    int dx2 = 0;
    pos.x += fctx.font->getDx2(dx2, fctx.monoW, g, str[1]);

    box += pos + fctx.font->getGlyphLt(g);
    box += pos + fctx.font->getGlyphRb(g);

    pos.x += dx2 + fctx.spacing;

    ++str;
    len--;
  }
  dx = pos.x;
  return box;
}
static BBox2 get_str_real_visible_bbox(const char *str, int len, const StdGuiFontContext &fctx, float &dx)
{
  G_ASSERT(fctx.font);

  dx = 0;
  if (!str || !len)
    return BBox2(0, 0, 0, 0);

  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  return get_str_real_visible_bbox_u(tmpU16, len, fctx, dx);
}

static inline bool is_inscr_ready(const DagorFontBinDump::InscriptionData *g) { return g && g->dx < g->ST_restoring; }
static void render_inscription_u(int fontId, const DagorFontBinDump::InscriptionData *g, const wchar_t *str, int len, float scale = 1)
{
  GuiContext &ctxBlur = DagorFontBinDump::inscr_ctx();
  bool ownChunk = false;
  if (ctxBlur.currentChunk == -1 && DagorFontBinDump::inscr_blur_inited())
  {
    ownChunk = true;
    ctxBlur.currentChunk = ctxBlur.beginChunk();
    ctxBlur.set_texture(DagorFontBinDump::inscr_atlas.tex.first.getId(), DagorFontBinDump::inscr_atlas.tex.second);
  }

  if (!rfont[fontId].rasterize_str_u(*g, str, len, scale))
    const_cast<DagorFontBinDump::InscriptionData *>(g)->discard();
  const_cast<DagorFontBinDump::InscriptionData *>(g)->lruFrame = dagor_frame_no();
  if (DagorFontBinDump::inscr_blur_inited() && is_inscr_ready(g))
  {
    int m = DagorFontBinDump::inscr_atlas.getMarginLtOfs();
    float mdu = DagorFontBinDump::inscr_atlas.getMarginDu();
    float mdv = DagorFontBinDump::inscr_atlas.getMarginDv();
    enqueue_glyph(ctxBlur, DagorFontBinDump::inscr_atlas.tex.first.getId(), DagorFontBinDump::inscr_atlas.tex.second, false,
      g->u0 - mdu, g->v0 - mdv, g->u1 + mdu, g->v1 + mdv, g->w + 2 * m, g->h + 2 * m, Point2(g->x0 - m, g->y0 - m) / 4,
      Point2(g->x0 + g->w + m, g->y0 + g->h + m) / 4);
  }

  if (ownChunk)
  {
    ctxBlur.endChunk();
    ctxBlur.currentChunk = -1;
  }
}
static const DagorFontBinDump::InscriptionData *get_inscription(int fontId, const char *str, int len)
{
  if (!len || !*str)
    return NULL;
  if (len < 0)
    len = strlen(str);
  if (len > 256) // max inscription string len
    return NULL;
  unsigned hash = DagorFontBinDump::build_hash((const unsigned char *)str, (const unsigned char *)str + len, fontId, 1.0f);
  if (!hash)
    return NULL;
  if (const DagorFontBinDump::InscriptionData *g = DagorFontBinDump::find_inscription(hash))
  {
    if (is_inscr_ready(g))
      return g;
    CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
    replace_tabs_with_zwspaces(tmpU16);
    render_inscription_u(fontId, g, tmpU16, len);
    return g;
  }

  GuiContext &ctx = DagorFontBinDump::inscr_ctx();
  G_ASSERTF(fontId >= 0 && fontId < 255, "fontId=%d", fontId);
  ctx.set_font(fontId); // this call resets spacing and mono-width
  float inscr_dx = 0;
  BBox2 bb = get_str_real_visible_bbox(str, len, ctx.curRenderFont, inscr_dx);
  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  if (const DagorFontBinDump::InscriptionData *g = DagorFontBinDump::add_inscription(bb, hash, inscr_dx))
  {
    replace_tabs_with_zwspaces(tmpU16);
    render_inscription_u(fontId, g, tmpU16, len);
    return g;
  }
  else if (ctx.curRenderFont.font)
    ctx.curRenderFont.font->reportInscriptionFailedToAdd(bb, str, len, 1.0f);
  return NULL;
}

void GuiContext::draw_inscription_scaled(real scale, const char *str, int len)
{
#if LOG_DRAWS
  debug("STDG:draw_inscription_scaled '%s'", str);
#endif
  G_ASSERT(curRenderFont.font);
  if (!str)
    return;
  if (!DagorFontBinDump::inscr_inited())
    return draw_str_scaled(scale, str, len);

  if (len < 0)
    len = strlen(str);
  float spacing = (curRenderFont.font->getGlyphU(' ').dx + curRenderFont.spacing) * scale;
  while (len > 0)
  {
    while (len > 0 && strchr(" \n\r", *str))
    {
      if (*str == ' ')
        currentPos.x += spacing;
      str++;
      len--;
    }
    if (!len)
      return;

    int sublen = 0;
    while (sublen < len && !strchr(" \n\r", str[sublen]))
      sublen++;
    const DagorFontBinDump::InscriptionData *g = get_inscription(currentFontId, str, sublen);
    if (!g)
      break;
    if (!is_inscr_ready(g))
      break;

    Point2 lt(ROUND_COORD(currentPos.x + g->xBaseOfs * scale), currentPos.y + g->yBaseOfs * scale);
    Point2 rb(lt.x + g->w * scale, lt.y + g->h * scale);

    if (vpBoxIsVisible(lt, rb))
    {
      G_ASSERT(curRenderFont.font->isClassicFontTex());
      enqueue_glyph(*this, DagorFontBinDump::inscr_atlas.tex.first.getId(), DagorFontBinDump::inscr_atlas.tex.second,
        DagorFontBinDump::inscr_atlas.texBlurred.getId(), DagorFontBinDump::inscr_atlas.texBlurredSampler, false, g->u0, g->v0, g->u1,
        g->v1, g->w, g->h, lt, rb);
    }
    currentPos.x += g->dx * scale;
    str += sublen;
    len -= sublen;
  }
  if (len)
    draw_str_scaled(scale, str, len);
}

uint32_t GuiContext::create_inscription(int font_id, real scale, const char *str, int len)
{
  typedef DagorFontBinDump::InscriptionData inscr_data_t;
  if (!len || !*str)
    return 0;
  if (len < 0)
    len = strlen(str);
  if (len > 256) // max inscription string len
    return 0;
  int len0 = len;
  uint32_t hash = DagorFontBinDump::build_hash((const unsigned char *)str, (const unsigned char *)str + len, font_id, scale);
  if (const inscr_data_t *g = DagorFontBinDump::find_inscription(hash))
  {
    if (is_inscr_ready(g))
      return hash;
    CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
    replace_tabs_with_zwspaces(tmpU16);
    render_inscription_u(font_id, g, tmpU16, len, scale);
    return hash;
  }

  scale = floorf(rfont[font_id].fontHt * scale + 0.5f) / rfont[font_id].fontHt;
  GuiContext &ctx = DagorFontBinDump::inscr_ctx();
  G_ASSERTF(font_id >= 0 && font_id < 255, "fontId=%d", font_id);
  ctx.set_font(font_id); // this call resets spacing and mono-width
  CVT_TO_UTF16_ON_STACK(tmpU16, str, len);
  replace_tabs_with_zwspaces(tmpU16);
  BBox2 bb;
  float inscr_dx = rfont[font_id].compute_str_width_u(tmpU16, len, scale, bb);
  if (const inscr_data_t *g = DagorFontBinDump::add_inscription(bb, hash, inscr_dx))
  {
    render_inscription_u(font_id, g, tmpU16, len, scale);
    return hash;
  }
  else
    rfont[font_id].reportInscriptionFailedToAdd(bb, str, len0, scale);
  return 0;
}
void GuiContext::draw_inscription(uint32_t inscr_handle, float scale)
{
  typedef DagorFontBinDump::InscriptionData inscr_data_t;
  if (const inscr_data_t *g = DagorFontBinDump::find_inscription(inscr_handle))
  {
    if (!is_inscr_ready(g))
      return;
    int expand_pix = DagorFontBinDump::inscr_atlas.getMarginLtOfs() / 2;
    int width = g->w + expand_pix;
    int height = g->h + expand_pix;
    Point2 lt(ROUND_COORD(currentPos.x + g->xBaseOfs * scale), currentPos.y + g->yBaseOfs * scale);
    Point2 rb(lt.x + width * scale, lt.y + height * scale);

    G_ASSERT(curRenderFont.font->isClassicFontTex());
    enqueue_glyph(*this, DagorFontBinDump::inscr_atlas.tex.first.getId(), DagorFontBinDump::inscr_atlas.tex.second,
      DagorFontBinDump::inscr_atlas.texBlurred.getId(), DagorFontBinDump::inscr_atlas.texBlurredSampler, false, g->u0, g->v0,
      g->u1 + expand_pix * DagorFontBinDump::inscr_atlas.getInvW(), g->v1 + expand_pix * DagorFontBinDump::inscr_atlas.getInvH(),
      width, height, lt, rb);
    currentPos.x += g->dx * scale;
  }
}
BBox2 GuiContext::get_inscription_size(uint32_t inscr_handle)
{
  typedef DagorFontBinDump::InscriptionData inscr_data_t;
  BBox2 bb;
  if (const inscr_data_t *g = DagorFontBinDump::find_inscription(inscr_handle))
    if (is_inscr_ready(g))
    {
      bb[0].set(min(int(g->xBaseOfs), 0), g->yBaseOfs);
      bb[1].set(max(int(g->xBaseOfs + g->w + 1), int(g->dx)), g->yBaseOfs + g->h + 1);
      return bb;
    }
  bb[0] = bb[1] = Point2(0, 0);
  return bb;
}
void GuiContext::touch_inscription(uint32_t inscr_handle)
{
  typedef DagorFontBinDump::InscriptionData inscr_data_t;
  if (inscr_data_t *g = const_cast<inscr_data_t *>(DagorFontBinDump::find_inscription(inscr_handle)))
    if (g->lruFrame != dagor_frame_no())
      g->lruFrame = dagor_frame_no();
}
void GuiContext::purge_inscription(uint32_t inscr_handle)
{
  typedef DagorFontBinDump::InscriptionData inscr_data_t;
  if (inscr_data_t *g = const_cast<inscr_data_t *>(DagorFontBinDump::find_inscription(inscr_handle)))
    g->lruFrame = 0;
}
bool GuiContext::is_inscription_ready(uint32_t inscr_handle)
{
  return is_inscr_ready(DagorFontBinDump::find_inscription(inscr_handle));
}

// render text string using current font (printf form)
void GuiContext::draw_strv(float scale, const char *fmt, const DagorSafeArg *arg, int anum)
{
  char buf[2048];
  DagorSafeArg::print_fmt(buf, sizeof(buf), fmt, arg, anum);
  draw_str_scaled(scale, buf);
}

void GuiContext::draw_inscription_v(float scale, const char *fmt, const DagorSafeArg *arg, int anum)
{
  char buf[2048];
  DagorSafeArg::print_fmt(buf, sizeof(buf), fmt, arg, anum);
  draw_inscription_scaled(scale, buf);
}

// render text string using current font with format: 'hh':'mm':'ss'.'msms'
void GuiContext::draw_timestr(unsigned int ms)
{
  const unsigned int msOut = ms % 1000;
  ms /= 1000;
  const unsigned int ssOut = ms % 60;
  ms /= 60;
  const unsigned int mmOut = ms % 60;
  const unsigned int hhOut = ms / 60;

  draw_strf("%02d:%02d:%02d.%04d", hhOut, mmOut, ssOut, msOut);
}

//************************************************************************
//* legacy wrappers
//************************************************************************

int reset_per_frame_dynamic_buffer_pos()
{
  stdgui_context.resetFrame();
  if (DagorFontBinDump::inscr_inited())
    DagorFontBinDump::inscr_ctx().resetFrame();
  return 0;
}

// unsupported
void set_shader(ShaderMaterial * /* mat */) { G_ASSERT(0); }
void set_shader(const char * /* name */) { G_ASSERT(0); }

// set new shader for buffer
void set_shader(GuiShader *shader) { stdgui_context.setShader(shader); }
void reset_shader() { stdgui_context.setShader(NULL); }

GuiShader *get_shader() { return stdgui_context.getShader(); }

void set_user_state(int state, const float *v, int cnt) // currently no more than 8 floats. to be changed
{
  stdgui_context.set_user_state(state, v, cnt);
}
void set_color(E3DCOLOR color) { stdgui_context.set_color(color); }
BlendMode get_alpha_blend() { return stdgui_context.get_alpha_blend(); }
void set_alpha_blend(BlendMode blend_mode) { stdgui_context.set_alpha_blend(blend_mode); }
void set_textures(const TEXTUREID tex, d3d::SamplerHandle smp_id, const TEXTUREID tex2, d3d::SamplerHandle smp_id2)
{
  stdgui_context.set_textures(tex, smp_id, tex2, smp_id2);
}
void set_mask_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, Point3 transform0, Point3 transform1)
{
  stdgui_context.set_mask_texture(tex_id, smp_id, transform0, transform1);
}
void set_viewport(const GuiViewPort &rt) { stdgui_context.set_viewport(rt); }
void apply_viewport(GuiViewPort &rt) { stdgui_context.applyViewport(rt); }
bool restore_viewport() { return stdgui_context.restore_viewport(); }
const GuiViewPort &get_viewport() { return stdgui_context.get_viewport(); }
void start_render()
{
  stdgui_context.setTarget();
  stdgui_context.start_render();
}
void start_render(int screen_width, int screen_height)
{
  stdgui_context.setTarget(screen_width, screen_height);
  stdgui_context.start_render();
}
void continue_render() { stdgui_context.start_render(); }
void end_render() { stdgui_context.end_render(); }
bool is_render_started() { return stdgui_context.isRenderStarted(); }
/*  const Point2& screen_size()
  {
    return stdgui_context.screen_size();
  }*/

// get virtual screen size (first fullscreen, then VR 2d background)
Point2 screen_size() { return g_screen_size; }
real screen_width() { return g_screen_size.x; }
real screen_height() { return g_screen_size.y; }
// some tools attempts to read screen size at startup without start_render()
real real_screen_width()
{
  // return stdgui_context.real_screen_width();
  return g_screen_size.x;
}
real real_screen_height()
{
  // return stdgui_context.real_screen_height();
  return g_screen_size.y;
}

Point2 logic2real() { return stdgui_context.logic2real(); }
Point2 real2logic() { return stdgui_context.real2logic(); }
void start_raw_layer() { stdgui_context.start_raw_layer(); }
void end_raw_layer() { stdgui_context.end_raw_layer(); }

void draw_quads(GuiVertex *verts, int num_quads) { stdgui_context.draw_quads(verts, num_quads); }
void draw_faces(GuiVertex *verts, int num_verts, const IndexType *indices, int num_faces)
{
  stdgui_context.draw_faces(verts, num_verts, indices, num_faces);
}
void flush_data() { stdgui_context.flushData(); }
void render_quad(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc0, Point2 tc1, Point2 tc2, Point2 tc3)
{
  stdgui_context.render_quad(p0, p1, p2, p3, tc0, tc1, tc2, tc3);
}
void render_quad_color(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc0, Point2 tc3, Point2 tc2, Point2 tc1, E3DCOLOR c0,
  E3DCOLOR c3, E3DCOLOR c2, E3DCOLOR c1)
{
  stdgui_context.render_quad_color(p0, p3, p2, p1, tc0, tc3, tc2, tc1, c0, c3, c2, c1);
}
void render_quad_t(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc_lefttop, Point2 tc_rightbottom)
{
  stdgui_context.render_quad_t(p0, p1, p2, p3, tc_lefttop, tc_rightbottom);
}

void render_frame(real left, real top, real right, real bottom, real thickness)
{
  stdgui_context.render_frame(left, top, right, bottom, thickness);
}

void render_box(real left, real top, real right, real bottom) { stdgui_context.render_box(left, top, right, bottom); }

void solid_frame(real left, real top, real right, real bottom, real thickness, E3DCOLOR background, E3DCOLOR frame)
{
  stdgui_context.solid_frame(left, top, right, bottom, thickness, background, frame);
}

void render_rect(real left, real top, real right, real bottom, Point2 left_top_tc, Point2 dx_tc, Point2 dy_tc)
{
  stdgui_context.render_rect(left, top, right, bottom, left_top_tc, dx_tc, dy_tc);
}

void render_rect_t(real left, real top, real right, real bottom, Point2 left_top_tc, Point2 right_bottom_tc)
{
  stdgui_context.render_rect_t(left, top, right, bottom, left_top_tc, right_bottom_tc);
}
void render_rounded_box(Point2 lt, Point2 rb, E3DCOLOR col, E3DCOLOR border, Point4 rounding, float thickness)
{
  stdgui_context.render_rounded_box(lt, rb, col, border, rounding, thickness);
}
void render_rounded_frame(Point2 lt, Point2 rb, E3DCOLOR col, Point4 rounding, float thickness)
{
  stdgui_context.render_rounded_frame(lt, rb, col, rounding, thickness);
}
void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 tc_rightbottom, E3DCOLOR col, Point4 rounding)
{
  stdgui_context.render_rounded_image(lt, rb, tc_lefttop, tc_rightbottom, col, rounding);
}

void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 dx_tc, Point2 dy_tc, E3DCOLOR col, Point4 rounding)
{
  stdgui_context.render_rounded_image(lt, rb, tc_lefttop, dx_tc, dy_tc, col, rounding);
}

void draw_line(real left, real top, real right, real bottom, real thickness)
{
  stdgui_context.draw_line(left, top, right, bottom, thickness);
}

void goto_xy(Point2 pos) { stdgui_context.goto_xy(pos); }

Point2 get_text_pos() { return stdgui_context.get_text_pos(); }

void set_spacing(int spacing) { stdgui_context.set_spacing(spacing); }
void set_mono_width(int mw) { stdgui_context.set_mono_width(mw); }

int get_spacing() { return stdgui_context.get_spacing(); }
int set_font(int font_id, int font_spacing, int mono_w) { return stdgui_context.set_font(font_id, font_spacing, mono_w); }
int set_font_ht(int font_ht) { return stdgui_context.set_font_ht(font_ht); }
int get_initial_font_ht(int font_id)
{
  if (DagorFontBinDump *f = get_font(font_id))
    return f->getInitialFontHt();
  return 0;
}
int get_def_font_ht(int font_id)
{
  if (DagorFontBinDump *f = get_font(font_id))
    return f->getFontHt();
  return 0;
}
bool set_def_font_ht(int font_id, int ht)
{
  if (DagorFontBinDump *f = get_font(font_id))
    return f->setFontHt(ht);
  return false;
}

void prefetch_font_glyphs(const wchar_t *chars, int count, const StdGuiFontContext &fctx)
{
  if (count == -1)
    count = wcslen(chars);
  if (fctx.font->isFullyDynamicFont())
  {
    int font_ht = fctx.fontHt, fgidx = 0;
    if (!font_ht)
      font_ht = fctx.font->getHtForFontGlyphIdx(0);
    else
      fgidx = fctx.font->getFontGlyphIdxForHt(font_ht);

    for (; count > 0; chars++, count--)
    {
      const DagorFontBinDump::GlyphPlace *gp = fctx.font->getDynGlyph(fgidx, *chars);
      if (!gp || gp->texIdx == gp->TEXIDX_NOTPLACED)
        fctx.font->reqCharGen(*chars, font_ht);
    }
  }
  else
    for (; count > 0; chars++, count--)
    {
      const DagorFontBinDump::GlyphData &g = fctx.font->getGlyphU(*chars);
      if (g.texIdx >= fctx.font->tex.size() && g.isDynGrpValid() && *chars)
        fctx.font->reqCharGen(*chars);
    }
}

void set_picquad_color_matrix(const TMatrix4 *cm) { stdgui_context.set_picquad_color_matrix(cm); }
void set_picquad_color_matrix_saturate(float factor) { stdgui_context.set_picquad_color_matrix_saturate(factor); }
void set_picquad_color_matrix_sepia(float factor) { stdgui_context.set_picquad_color_matrix_sepia(factor); }

void set_draw_str_attr(FontFxType t, int ofs_x, int ofs_y, E3DCOLOR col, int factor_x32)
{
  stdgui_context.set_draw_str_attr(t, ofs_x, ofs_y, col, factor_x32);
}
void set_draw_str_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, int su_x32, int sv_x32, int bv_ofs)
{
  stdgui_context.set_draw_str_texture(tex_id, smp_id, su_x32, sv_x32, bv_ofs);
}
void draw_char_u(wchar_t ch) { stdgui_context.draw_char_u(ch); }
void draw_str_scaled(real scale, const char *str, int len) { stdgui_context.draw_str_scaled(scale, str, len); }
void draw_str_scaled_u(real scale, const wchar_t *str, int len) { stdgui_context.draw_str_scaled_u(scale, str, len); }
void draw_inscription_scaled(real scale, const char *str, int len) { stdgui_context.draw_inscription_scaled(scale, str, len); }
void draw_inscription(uint32_t inscr_handle, float over_scale) { stdgui_context.draw_inscription(inscr_handle, over_scale); }
bool draw_str_scaled_u_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
  unsigned dsb_flags, real scale, const wchar_t *str, int len)
{
  return stdgui_context.draw_str_scaled_u_buf(out_qv, out_tex_qcnt, out_smp, dsb_flags, scale, str, len);
}
bool draw_str_scaled_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
  unsigned dsb_flags, real scale, const char *str, int len)
{
  return stdgui_context.draw_str_scaled_buf(out_qv, out_tex_qcnt, out_smp, dsb_flags, scale, str, len);
}
void render_str_buf(dag::ConstSpan<GuiVertex> qv, dag::ConstSpan<uint16_t> tex_qcnt, dag::ConstSpan<d3d::SamplerHandle> smp,
  unsigned dsb_flags)
{
  stdgui_context.render_str_buf(qv, tex_qcnt, smp, dsb_flags);
}
//***********************************************************************

void draw_strv(float scale, const char *fmt, const DagorSafeArg *arg, int anum) { stdgui_context.draw_strv(scale, fmt, arg, anum); }
void draw_inscription_v(float scale, const char *fmt, const DagorSafeArg *arg, int anum)
{
  stdgui_context.draw_inscription_v(scale, fmt, arg, anum);
}
void draw_timestr(unsigned int ms) { stdgui_context.draw_timestr(ms); }
void set_textures(TEXTUREID tex_id, d3d::SamplerHandle smp_id, TEXTUREID tex_id2, d3d::SamplerHandle smp_id2, bool font_l8)
{
  stdgui_context.set_textures(tex_id, smp_id, tex_id2, smp_id2, font_l8);
}
}; // namespace StdGuiRender

//***********************************************************************

void GuiVertex::resetViewTm() { StdGuiRender::stdgui_context.resetViewTm(); }
void GuiVertex::setViewTm(const float m[2][3]) { StdGuiRender::stdgui_context.setViewTm(m); }
void GuiVertex::setViewTm(Point2 ax, Point2 ay, Point2 o, bool add) { StdGuiRender::stdgui_context.setViewTm(ax, ay, o, add); }

void GuiVertex::setRotViewTm(float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add)
{
  StdGuiRender::stdgui_context.setRotViewTm(x0, y0, rot_ang_rad, skew_ang_rad, add);
}
void GuiVertex::getViewTm(float dest_vtm[2][3], bool pure_trans) { StdGuiRender::stdgui_context.getViewTm(dest_vtm, pure_trans); }

// make left-top <= right-bottom
void GuiViewPort::normalize() { alignPoints(leftTop, rightBottom); }

// dump contens to debug
void GuiViewPort::dump(const char *msg) const
{
  if (msg)
  {
    debug("%s: (%.4f %.4f)-(%.4f %.4f)", msg, leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
  }
  else
  {
    debug("view(%.4f %.4f)-(%.4f %.4f)", leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
  }
}

// check visiblility for points (viewport left-top must be <= right-bottom)
bool GuiViewPort::isVisible(Point2 p0, Point2 p1, Point2 p2, Point2 p3) const
{
  return isVisible(min4(p0.x, p1.x, p2.x, p3.x), min4(p0.y, p1.y, p2.y, p3.y), max4(p0.x, p1.x, p2.x, p3.x),
    max4(p0.y, p1.y, p2.y, p3.y));
}

// clip other viewport by current viewport
void GuiViewPort::clipView(GuiViewPort &view_to_clip) const
{
  view_to_clip.normalize();

  // check visibility
  if (isZero() || leftTop.x > view_to_clip.rightBottom.x || leftTop.y > view_to_clip.rightBottom.y ||
      rightBottom.x < view_to_clip.leftTop.x || rightBottom.y < view_to_clip.leftTop.y)
  {
    view_to_clip.leftTop = view_to_clip.rightBottom = StdGuiRender::P2::ZERO;
    view_to_clip.isNull = true;
    return;
  }

  // clip
  view_to_clip.leftTop.x = max(leftTop.x, view_to_clip.leftTop.x);
  view_to_clip.leftTop.y = max(leftTop.y, view_to_clip.leftTop.y);
  view_to_clip.rightBottom.x = min(rightBottom.x, view_to_clip.rightBottom.x);
  view_to_clip.rightBottom.y = min(rightBottom.y, view_to_clip.rightBottom.y);
  view_to_clip.updateNull();
}


static inline void mul3_inplace(float dm[2][3], float sm00, float sm01, float sm02, float sm10, float sm11, float sm12)
{
  float m[2][3];
  m[0][0] = dm[0][0] * sm00 + dm[0][1] * sm10;
  m[0][1] = dm[0][0] * sm01 + dm[0][1] * sm11;
  m[0][2] = dm[0][0] * sm02 + dm[0][1] * sm12 + dm[0][2];
  m[1][0] = dm[1][0] * sm00 + dm[1][1] * sm10;
  m[1][1] = dm[1][0] * sm01 + dm[1][1] * sm11;
  m[1][2] = dm[1][0] * sm02 + dm[1][1] * sm12 + dm[1][2];
  memcpy(dm, m, sizeof(m));
}

void GuiVertexTransform::resetViewTm()
{
  static float init_vtm[2][3] = {{GUI_POS_SCALE, 0.f, 0.5f}, {0.f, GUI_POS_SCALE, 0.5f}};
  memcpy(vtm, init_vtm, sizeof(vtm));
  // expect fontTex2rotCCSmS.set(1, 1, 0, 0);
}

void GuiVertexTransform::setViewTm(Point2 ax, Point2 ay, Point2 o)
{
  vtm[0][0] = ax.x * GUI_POS_SCALE;
  vtm[0][1] = ay.x * GUI_POS_SCALE;
  vtm[0][2] = o.x + 0.5f;
  vtm[1][0] = ax.y * GUI_POS_SCALE;
  vtm[1][1] = ay.y * GUI_POS_SCALE;
  vtm[1][2] = o.y + 0.5f;
}

void GuiVertexTransform::setViewTm(const float m[2][3])
{
  memcpy(vtm, m, sizeof(vtm));
  // expect fontTex2rotCCSmS.set(1, 1, 0, 0);
}

void GuiVertexTransform::addViewTm(Point2 ax, Point2 ay, Point2 o) { mul3_inplace(vtm, ax.x, ay.x, o.x, ax.y, ay.y, o.y); }

void GuiVertexTransform::setRotViewTm(Color4 &fontTex2rotCCSmS, float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add)
{
  float cc1, ss1;
  float cc2, ss2;
  sincos(rot_ang_rad, ss1, cc1);
  sincos(rot_ang_rad + skew_ang_rad, ss2, cc2);

  float idet = safediv(1.0f, cc1 * cc2 + ss1 * ss2);
  fontTex2rotCCSmS.set(cc2 * idet, cc1 * idet, ss2 * idet, -ss1 * idet);

  if (!add)
  {
    vtm[0][0] = cc1 * GUI_POS_SCALE;
    vtm[0][1] = -ss2 * GUI_POS_SCALE;
    vtm[1][0] = ss1 * GUI_POS_SCALE;
    vtm[1][1] = cc2 * GUI_POS_SCALE;

    vtm[0][2] = x0 * GUI_POS_SCALE - (x0 * vtm[0][0] + y0 * vtm[0][1]) + 0.5f;
    vtm[1][2] = y0 * GUI_POS_SCALE - (x0 * vtm[1][0] + y0 * vtm[1][1]) + 0.5f;
  }
  else
  {
    float m[2][3];
    m[0][0] = cc1;
    m[0][1] = -ss2;
    m[1][0] = ss1;
    m[1][1] = cc2;

    m[0][2] = x0 - (x0 * m[0][0] + y0 * m[0][1]);
    m[1][2] = y0 - (x0 * m[1][0] + y0 * m[1][1]);

    mul3_inplace(vtm, m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2]);
  }
}

void GuiVertexTransform::getViewTm(float dest_vtm[2][3], bool pure_trans) const
{
  memcpy(dest_vtm, vtm, sizeof(vtm));
  if (pure_trans)
  {
    dest_vtm[0][0] *= GUI_POS_SCALE_INV;
    dest_vtm[0][1] *= GUI_POS_SCALE_INV;
    dest_vtm[0][2] = (dest_vtm[0][2] - 0.5f) * GUI_POS_SCALE_INV;
    dest_vtm[1][0] *= GUI_POS_SCALE_INV;
    dest_vtm[1][1] *= GUI_POS_SCALE_INV;
    dest_vtm[1][2] = (dest_vtm[1][2] - 0.5f) * GUI_POS_SCALE_INV;
  }
}

void GuiVertexTransform::inverseViewTm(float dm[2][3], const float sm[2][3])
{
  float idet = safediv(1.0f, sm[0][0] * sm[1][1] - sm[0][1] * sm[1][0]);

  dm[0][0] = sm[1][1] * idet;
  dm[0][1] = -sm[0][1] * idet;
  dm[0][2] = (sm[0][1] * sm[1][2] - sm[1][1] * sm[0][2]) * idet;
  dm[1][0] = -sm[1][0] * idet;
  dm[1][1] = sm[0][0] * idet;
  dm[1][2] = (-sm[0][0] * sm[1][2] + sm[1][0] * sm[0][2]) * idet;
}

bool (*dgs_gui_may_rasterize_glyphs_cb)() = NULL;
bool (*dgs_gui_may_load_ttf_for_rasterization_cb)() = NULL;
