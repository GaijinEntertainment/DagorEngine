// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_stdGuiRender.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <math/dag_Point2.h>
#include <util/dag_bfind.h>
#include <util/dag_simpleString.h>
#include <util/dag_hash.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <3d/dag_dynAtlas.h>
#include <ioSys/dag_dataBlock.h>
#include <hb.h>
#include <drv/3d/dag_renderTarget.h>
#if defined(__GNUC__)
#include <wchar.h>
#endif


// forward declarations for external classes
class BaseTexture;
typedef BaseTexture Texture;
class IGenLoad;
class FullFileLoadCB;
class String;
class FastIntList;
struct DagorFontBinDump;
struct DagorFreeTypeFontRec;
DAG_DECLARE_RELOCATABLE(DagorFreeTypeFontRec);

typedef Tab<DagorFontBinDump> FontArray;


struct DagorFontBinDump
{
  struct GlyphPlace // 96 bit
  {
    enum
    {
      TEXIDX_UNUSED = 0xFF,
      TEXIDX_NOTPLACED = 0xFE,
      TEXIDX_INVISIBLE = 0xFD
    };
    uint32_t u0 : 16, v0 : 16;
    uint32_t u1 : 16, v1 : 16;
    uint32_t texIdx : 8;
    int32_t ox : 12, oy : 12;
  };
  struct Unicode256Place
  {
    GlyphPlace gp[256];
    Unicode256Place()
    {
      memset(this, 0, sizeof(*this));
      for (GlyphPlace &g : gp)
        g.texIdx = GlyphPlace::TEXIDX_UNUSED;
    }
  };
  struct FontData
  {
    struct FontMetrics
    {
      int16_t capsHt = 0, ascent = 0, descent = 0, lineSpacing = 0, spaceWd = 0;
    };

    // parallel arrays (glyphs, ht, metrics) for all font heights
    Tab<Tab<Unicode256Place *>> fontGlyphs;
    Tab<int16_t> fontHt;
    Tab<FontMetrics> fontMx;

    int ttfNid;
    uint16_t addFontRange[2];   //< start/end index for wcRangesPair and wcRangeFontIdx (/2)
    uint64_t addFontMask;       //< 64 bitmask for 1024-char subranges (pre-check before iterating ranges)
    int16_t pixHt;              //< default height for font (in pixels)
    uint16_t initialPixHt : 15; //< (invariant) initial height for font (in pixels), read from .dynFont.blk
    uint16_t forceAutoHint : 1;

    // parallel arrays, wcRangesPair.count == wcRangeFontIdx.count*2 == wcRangeFontScl.count*2
    static Tab<uint16_t> wcRangePair, wcRangeFontIdx;
    static Tab<float> wcRangeFontScl;
    enum
    {
      WCRFI_FONT_MASK = 0x00FF,
      WCRFI_ADDIDX_SHR = 8
    };

    void loadBaseFontData(const DataBlock &desc);
    inline uint16_t *getEffFontIdxPtr(wchar_t c) const;
  };

  struct GlyphData
  {
    float u0, v0, u1, v1;
    uint8_t x0, y0, x1, y1;
    int16_t dx;
    uint8_t texIdx;
    uint8_t dynGrpIdx : 7;
    uint8_t hasPk : 1;

    bool isDynGrpValid() const { return ((dynGrpIdx + 1) & 0x7F) > 1; }
  };
  struct UnicodeTable
  {
    PatchablePtr<uint16_t> glyphIdx;
    uint32_t startIdx, count;
  };
  struct Tex
  {
    PatchablePtr<Texture> tex;
    TEXTUREID texId;
    uint32_t _resv;
  };
  struct DynGroupDesc
  {
    static constexpr int FLG_FORCE_AUTOHINT = 1;
    int ttfNid;
    short pixHt;
    unsigned char flags;
    signed char vaScale;
  };

  PatchableTab<const GlyphData> glyph;
  PatchableTab<const UnicodeTable> uTable;
  PatchableTab<Tex> tex;
  int32_t fontHt, ascent, descent, lineSpacing;
  int32_t xBase, yBase;
  uint16_t asciiGlyphIdx[256];
  PatchablePtr<const char> name;

private:
  PATCHABLE_DATA64(void *, selfData);
  uint32_t classicFont;
  uint32_t _resv;
  PatchableTab<uint16_t> glyphPairKernIdx; // parallel to glyph, contains base index offset to pairKernData
  PatchableTab<uint32_t> pairKernData;     // add_dx:wchar_code 16bit pairs (paired kerning data)

  // runtime data (not referenced from dump)
  FontData dfont;
  SmallTab<DynGroupDesc, MidmemAlloc> dynGrp;
  PatchableTab<Tex> texOrig;
  FastIntList *wcharToBuild;
  FastIntList *ttfNidToBuild;
  bool texOwner;
  bool dynamic;
  int fontCapsHt;

  SimpleString fontName, fileName;
  int fileOfs, fileVer;
  Tab<Tex> texCopy;

public:
  inline void init()
  {
    memset(this, 0, offsetof(DagorFontBinDump, dfont));
    dfont.ttfNid = -1;
    dfont.addFontRange[0] = dfont.addFontRange[1] = 0;
    dfont.addFontMask = 0;
    dfont.pixHt = 0;
    dfont.initialPixHt = 0;
    dfont.forceAutoHint = 0;
    wcharToBuild = ttfNidToBuild = nullptr;
    texOwner = dynamic = false;
    fontCapsHt = 0;
    fileOfs = fileVer = 0;
  }
  void clear();

  static void loadDynFonts(FontArray &fonts, const DataBlock &desc, int scr_height = -1);
  static void loadFonts(FontArray &fonts, const char *fn_prefix, int scr_height = -1);
  static void loadFontsStream(IGenLoad &crd, FontArray &fonts, const char *fname_prefix, bool dynamic_font);

  void discard();
  void doFetch();
  inline void fetch() { selfData || isFullyDynamicFont() ? (void)0 : doFetch(); }

  bool isFullyDynamicFont() const { return dfont.ttfNid >= 0; }
  int getInitialFontHt() const { return isFullyDynamicFont() ? dfont.initialPixHt : fontHt; }
  int getFontHt() const { return isFullyDynamicFont() ? dfont.pixHt : fontHt; }
  bool setFontHt(int ht);
  bool isResDependent() const { return dynamic; }
  inline int getCapsHt() const { return fontCapsHt; }

  int getFontGlyphIdxForHt(unsigned ht) const { return find_value_idx(dfont.fontHt, ht); }
  int getHtForFontGlyphIdx(unsigned fdidx) const { return dfont.fontHt[fdidx]; }
  const GlyphPlace *getDynGlyph(unsigned fgidx, hb_codepoint_t cp) const
  {
    hb_codepoint_t page = cp >> 8;
    if (fgidx < dfont.fontGlyphs.size())
      if (page < dfont.fontGlyphs[fgidx].size() && dfont.fontGlyphs[fgidx][page])
        return &dfont.fontGlyphs[fgidx][page]->gp[cp & 0xFF];
    return nullptr;
  }
  const FontData::FontMetrics *getFontMx(unsigned ht)
  {
    if (!isFullyDynamicFont())
      return nullptr;
    ScopedLockReadTemplate<decltype(fontRwLock)> rl(fontRwLock);
    if (!ht || ht == dfont.fontHt[0])
      return &dfont.fontMx[0];
    int idx = getFontGlyphIdxForHt(ht);
    if (idx >= 0)
      return &dfont.fontMx[idx];
    return &dfont.fontMx[appendFontHt(ht, /*ttf*/ nullptr, /*read_locked*/ true)];
  }
  int appendFontHt(unsigned ht, DagorFreeTypeFontRec *ttf = nullptr, bool read_locked = false);

  const GlyphData &getGlyphU(uint16_t unicode_c)
  {
    if (unicode_c == 0x200B) // older fonts (not fully dynamic) fail to process U+200B and render <missing> instead
      unicode_c = '\t';
    int g = unicode_c >> 8;
    if (g >= uTable.size() || !uTable[g].glyphIdx)
      return glyph[0];
    int l = int(unicode_c & 0xFF) - uTable[g].startIdx;
    if (l >= 0 && l < uTable[g].count)
      return glyph[uTable[g].glyphIdx[l]];
    return glyph[0];
  }

  int getDx(const GlyphData &g, wchar_t next_c) const
  {
    if (!next_c || !g.hasPk)
      return g.dx;
    int g_idx = &g - glyph.data();
    const uint32_t *pk = pairKernData.data() + glyphPairKernIdx[g_idx];
    const uint32_t *pk_e = pairKernData.data() + glyphPairKernIdx[g_idx + 1];
    if (pk_e <= pk + 16)
    {
      for (; pk < pk_e; pk++)
        if (next_c == (*pk & 0xFFFF))
          return g.dx + int16_t(*pk >> 16);
    }
    else
    {
      int16_t add_dx = bfind_packed_uint16_x2(make_span_const(pk, pk_e - pk), next_c, 0x7FFF);
      if (add_dx != 0x7FFF)
        return g.dx + add_dx;
    }
    return g.dx;
  }
  int getDx2(int &out_dx2, int mono_w, const GlyphData &g, wchar_t next_c) const
  {
    if (!mono_w)
    {
      out_dx2 = getDx(g, next_c);
      return 0;
    }
    out_dx2 = (mono_w + g.dx) / 2;
    return mono_w - out_dx2;
  }

  void getGlyphBounds(const GlyphData &g, float &x0, float &y0, float &x1, float &y1) const
  {
    x0 = 0;
    y0 = -ascent;
    x1 = g.dx;
    y1 = 0;
  }
  void getGlyphVisBounds(const GlyphData &g, float &x0, float &y0, float &x1, float &y1) const
  {
    x0 = g.x0 + xBase;
    y0 = g.y0 + yBase;
    x1 = g.x1 + xBase;
    y1 = g.y1 + yBase;
  }

  Point2 getGlyphLt(const GlyphData &g) const { return Point2(g.x0 + xBase, g.y0 + yBase); };
  Point2 getGlyphRb(const GlyphData &g) const { return Point2(g.x1 + xBase, g.y1 + yBase); };
  bool isClassicFontTex() const { return classicFont; }

  void reqCharGen(wchar_t ch, unsigned font_ht = 0);
  bool updateGen(int64_t reft, bool allow_raster_glyphs, bool allow_load_ttfs);
  void resetGen();


  struct DynamicFontAtlas
  {
    Texture *tex;
    TEXTUREID texId;
    SmallTab<uint16_t, MidmemAlloc> hist;

    DynamicFontAtlas() : tex(NULL), texId(BAD_TEXTUREID) {}
    DynamicFontAtlas(const DynamicFontAtlas &) = delete;
    ~DynamicFontAtlas();

    int getHistMaxY(int x0, int dx, int y1)
    {
      int ymax = hist[x0];
      for (x0++, dx += x0; x0 < dx; x0++)
        if (hist[x0] > y1)
          return texSz();
        else if (hist[x0] > ymax)
          ymax = hist[x0];
      return ymax;
    }
    bool tryPlaceGlyph(int dx, int dy, int &x0, int &y0)
    {
      int tex_sz = texSz();
      int maxy = tex_sz - dy;
      x0 = 0, y0 = getHistMaxY(x0, dx + 1, maxy);
      for (int i = 1; i + dx < tex_sz; i++)
      {
        int y = getHistMaxY(i, i + dx + 1 < tex_sz ? dx + 1 : dx, maxy);
        if (y < y0)
        {
          x0 = i;
          y0 = y;
        }
      }
      return y0 < maxy && x0 < tex_sz - dx;
    }
    void occupyPlace(int dx, int dy, int x0, int y0)
    {
      int tex_sz = texSz();
      for (int i = x0 > 1 ? x0 - 1 : 0; i < x0 + dx + 1 && i < tex_sz; i++)
        if (hist[i] < y0 + dy + 1)
          hist[i] = y0 + dy + 1;
    }
    void placeGlyph(DagorFontBinDump::GlyphData &g, int dx, int dy, int x0, int y0)
    {
      occupyPlace(dx, dy, x0, y0);
      // code with divides was sensitive to codegen (rcp results were smaller)
      int tex_sz = texSz();
      float oneOverSz = 1.0f / tex_sz;
      float epsilon = oneOverSz / 32.;
      g.u0 = float(x0) * oneOverSz + epsilon;
      g.v0 = float(y0) * oneOverSz + epsilon;
      g.u1 = float(x0 + dx) * oneOverSz + epsilon;
      g.v1 = float(y0 + dy) * oneOverSz + epsilon;
      G_ASSERT(x0 == int(g.u0 * tex_sz) && y0 == int(g.v0 * tex_sz));
    }
    void placeGlyphD(DagorFontBinDump::GlyphPlace &g, int dx, int dy, int x0, int y0)
    {
      occupyPlace(dx, dy, x0, y0);
      g.u0 = x0;
      g.v0 = y0;
      g.u1 = x0 + dx;
      g.v1 = y0 + dy;
    }
    void prepareTex(int idx);
    int texSz() const { return hist.size(); }

    int calcUsage() const
    {
      int sum = 0;
      for (int i = 0; i < hist.size(); i++)
        sum += hist[i];
      return sum;
    }
    int calcUsagePercent() const { return calcUsage() * 100 / hist.size() / hist.size(); }
  };
  static Tab<DynamicFontAtlas> dynFontTex;
  static bool reqCharGenChanged, reqCharGenReset;
  static int dynFontTexSize;

  static void initDynFonts(int tex_cnt, int tex_sz, const char *path_prefix);
  static void termDynFonts();
  static void resetDynFontsCache();
  static int tryPlaceGlyph(int wd, int ht, int &x0, int &y0);
  static void dumpDynFontUsage();

  class InscriptionsAtlas : public DynamicAtlasTex
  {
  public:
    void init(const DataBlock &b);
    void term();

    inline StdGuiRender::GuiContext &ctx() { return *(StdGuiRender::GuiContext *)ctxBuf; }
    bool isCtx(void *p) { return p >= ctxBuf && p < ctxBuf + sizeof(ctxBuf); }
    inline bool isBlurInited() { return texBlurred.getId() != BAD_TEXTUREID; }

    const ItemData *addItem(int bmin_x, int bmin_y, int bmax_x, int bmax_y, unsigned hash, float dx)
    {
      const ItemData *d = DynamicAtlasTex::addItem(bmin_x, bmin_y, bmax_x, bmax_y, hash);
      if (d)
        const_cast<ItemData *>(d)->dx = dx;
      return d;
    }

  public:
    TextureIDHolder texBlurred;
    d3d::SamplerHandle texBlurredSampler;

  protected:
    alignas(StdGuiRender::GuiContext) char ctxBuf[sizeof(StdGuiRender::GuiContext)];
    StdGuiRender::StdGuiShader blurShader;
  };
  typedef InscriptionsAtlas::ItemData InscriptionData;

  static InscriptionsAtlas inscr_atlas;
  static DynamicAtlasTexUpdater gen_sys_tex;
  static OSReadWriteLock fontRwLock;     // non recursive
  static Tab<DagorFontBinDump> add_font; //< additional virtual fonts to be used from main fonts

  static inline StdGuiRender::GuiContext &inscr_ctx() { return inscr_atlas.ctx(); }
  static bool is_inscr_ctx(void *p) { return inscr_atlas.isCtx(p); }
  static inline bool inscr_blur_inited() { return inscr_atlas.isBlurInited(); }
  static inline bool inscr_inited() { return inscr_atlas.isInited(); }
  static void initInscriptions(const DataBlock &b)
  {
    inscr_atlas.init(b);

    int ref_hres = b.getInt("refHRes", 0);
    IPoint2 gen_sz = b.getIPoint2("genSysTexSz", inscr_atlas.isInited() ? IPoint2(inscr_atlas.getSz().x, 128) : IPoint2(256, 256));

    if (gen_sz.x && gen_sz.y && ref_hres)
    {
      static constexpr int SZ_ALIGN = 32;
      int scrw = 0, scrh = 0;
      d3d::get_screen_size(scrw, scrh);
      if (scrh > ref_hres)
      {
        gen_sz.x = (gen_sz.x * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
        gen_sz.y = (gen_sz.y * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
        if (gen_sz.y > 256)
          gen_sz.y = 256;
      }
      debug("gen_sys_tex: uses dynamic sz=%d,%d due to refHRes:i=%d and screen size %dx%d", gen_sz.x, gen_sz.y, ref_hres, scrw, scrh);
    }

    gen_sys_tex.init(gen_sz, TEXFMT_L8);
  }
  static void termInscriptions()
  {
    inscr_atlas.term();
    gen_sys_tex.term();
  }

  static const InscriptionData *find_inscription(unsigned hash) { return inscr_atlas.findItem(hash); }
  static const InscriptionData *add_inscription(const BBox2 &bb, unsigned hash, float dx)
  {
    int x0 = (int)floorf(bb[0].x), y0 = (int)floorf(bb[0].y);
    int x1 = (int)ceilf(bb[1].x), y1 = (int)ceilf(bb[1].y);
    if ((x1 - x0 + inscr_atlas.getMargin()) <= gen_sys_tex.getSz().x && (y1 - y0 + inscr_atlas.getMargin()) <= gen_sys_tex.getSz().y)
      return inscr_atlas.addItem(x0, y0, x1, y1, hash, dx);
    return nullptr;
  }
  void reportInscriptionFailedToAdd(const BBox2 &bb, const char *str, int len, float scale)
  {
    int x0 = (int)floorf(bb[0].x), y0 = (int)floorf(bb[0].y);
    int x1 = (int)ceilf(bb[1].x), y1 = (int)ceilf(bb[1].y);
    G_UNUSED(bb);
    G_UNUSED(str);
    G_UNUSED(len);
    G_UNUSED(scale);
    G_UNUSED(x0);
    G_UNUSED(y0);
    G_UNUSED(x1);
    G_UNUSED(y1);
    logerr("failed to allocate inscription=%dx%d in atlas=%dx%d (max upd_rect=%dx%d, font=<%s> ht=%d scaledHt=%.0f), str(%d)=%.*s",
      x1 - x0, y1 - y0, inscr_atlas.getSz().x, inscr_atlas.getSz().y, gen_sys_tex.getSz().x, gen_sys_tex.getSz().y, name.get(), fontHt,
      fontHt * scale, len, len < 32 ? len : 32, str);
  }

  static inline uint32_t build_hash(const unsigned char *p, const unsigned char *p_end, int font_id, float scale)
  {
    uint32_t c, result = fnv1_step<32>(font_id);
    result = fnv1_step<32>(unsigned(scale * 64), result);
    for (; p < p_end && (c = *p) != 0; p++)
      result = fnv1_step<32>(c, result);
    return result;
  }

  int compute_str_width_u(const wchar_t *str, int len, float scale, BBox2 &out_bb);
  bool rasterize_str_u(const InscriptionData &g, const wchar_t *str, int len, float scale);

  float dynfont_compute_str_width_u(int font_ht, const wchar_t *str, int len, float scale, int spacing, BBox2 &out_bb);
  float dynfont_compute_str_visbox_u(int font_ht, const wchar_t *str, int len, float scale, int spacing, BBox2 &out_bb);
  inline BBox2 dynfont_compute_str_width_u_ex(int font_ht, const wchar_t *str, int len, float scale, int spacing, int mono_w,
    float max_w, bool left_align, const wchar_t *break_sym, float ellipsis_resv_w, int &out_start_idx, int &out_count)
  {
    if (!font_ht)
      font_ht = dfont.pixHt;
    if (len < 0)
      len = wcslen(str);

    if (mono_w)
      return dynfont_compute_str_width_u_ex1(font_ht, str, len, scale, spacing, mono_w, max_w, left_align, break_sym, ellipsis_resv_w,
        out_start_idx, out_count);

    int eff_font_ht = font_ht, seglen = 0;
    DagorFontBinDump *f = dynfont_get_next_segment(str, len, eff_font_ht = font_ht, seglen);
    if (seglen == len)
      return f->dynfont_compute_str_width_u_ex1(eff_font_ht, str, len, scale, spacing, mono_w, max_w, left_align, break_sym,
        ellipsis_resv_w, out_start_idx, out_count);

    return dynfont_compute_str_width_u_ex2(font_ht, str, len, scale, spacing, max_w, left_align, break_sym, ellipsis_resv_w,
      out_start_idx, out_count);
  }
  void dynfont_update_bbox_y(int font_ht, BBox2 &inout_bb, bool vis_bb)
  {
    inout_bb.lim[0].y = -floorf(ascent * font_ht / float(dfont.pixHt) + 0.5f);
    inplace_max(inout_bb.lim[1].y, vis_bb ? descent * font_ht / float(dfont.pixHt) : 0);
  }

  inline DagorFontBinDump *dynfont_get_next_segment(const wchar_t *str, int len, int &inout_font_ht, int &out_seglen)
  {
    if (!dfont.addFontRange[1])
    {
      out_seglen = len;
      return this;
    }
    return get_next_segment(str, len, inout_font_ht, out_seglen);
  }
  struct ScopeHBuf
  {
    hb_buffer_t *buf;
    operator hb_buffer_t *() { return buf; }
    ScopeHBuf();
    ScopeHBuf(const ScopeHBuf &) = delete;
    ScopeHBuf &operator=(const ScopeHBuf &) = delete;
    ~ScopeHBuf();
  };
  void dynfont_prepare_str(ScopeHBuf &buf, int font_ht, const wchar_t *str, int len, int *out_fgidx);

  static void after_device_reset();

private:
  static bool tryOpenFontFile(FullFileLoadCB &reader, const char *fname_prefix, int screen_height, bool &dynamic_font);
  int loadDumpData(IGenLoad &crd);
  DagorFontBinDump *get_next_segment(const wchar_t *str, int len, int &inout_font_ht, int &out_seglen);
  BBox2 dynfont_compute_str_width_u_ex1(int font_ht, const wchar_t *str, int len, float scale, int spacing, int mono_w, float max_w,
    bool left_align, const wchar_t *break_sym, float ellipsis_resv_w, int &out_start_idx, int &out_count);
  BBox2 dynfont_compute_str_width_u_ex2(int font_ht, const wchar_t *str, int len, float scale, int spacing, float max_w,
    bool left_align, const wchar_t *break_sym, float ellipsis_resv_w, int &out_start_idx, int &out_count);
};
DAG_DECLARE_RELOCATABLE(DagorFontBinDump::DynamicFontAtlas);

#define STRBOXCACHE_DEBUG     0
#define CHECK_HASH_COLLISIONS (DAGOR_DBGLEVEL > 0)
namespace strboxcache
{
enum Hash2IdxStratum
{
  H2I_STR,
  H2I_CHAR,
  H2I__COUNT
};
static constexpr int HASH_BITS = 32;
typedef HashVal<HASH_BITS> hash_t;
typedef ska::flat_hash_map<hash_t, int, ska::power_of_two_std_hash<hash_t>> HashToIdx;

struct StrBboxCache
{
  HashToIdx hash2idx[H2I__COUNT];
  Tab<BBox2> bb;
};
extern StrBboxCache priCache, secCache;
static constexpr int DEF_CACHE_SIZE = 2048;

extern uint32_t resetCnt, strVisBoxCnt, resetFrameNo;
extern OSSpinlock cc;
#if STRBOXCACHE_DEBUG
extern uint64_t testCnt, hitCnt, secHitCnt;
extern uint64_t cacheTimeTicks, calcTimeTicks, calcCount;
#endif

static inline hash_t build_hash(const wchar_t *p, int len, uint32_t font_id, uint32_t font_ht, uint32_t font_sp)
{
  hash_t result = fnv1_step<HASH_BITS>(font_id);
  wchar_t c;
  result = fnv1_step<HASH_BITS>(font_ht, result);
  result = fnv1_step<HASH_BITS>(font_sp, result);
  for (const wchar_t *pe = p + len; p != pe && (c = *p) != 0; p++)
    result = fnv1_step<HASH_BITS>(c, result);
  return result;
}

#if CHECK_HASH_COLLISIONS
extern Tab<uint32_t> hcc_hash2; // hash collision detector
static inline uint32_t build_hash2_fnv1a(const wchar_t *p, int len, uint32_t font_id, uint32_t font_ht, uint32_t font_sp)
{
  uint32_t result = fnv1a_step<32>(font_id);
  wchar_t c;
  result = fnv1a_step<32>(font_ht, result);
  result = fnv1a_step<32>(font_sp, result);
  for (const wchar_t *pe = p + len; p != pe && (c = *p) != 0; p++)
    result = fnv1a_step<32>(c, result);
  return result;
}
#else
static inline uint32_t build_hash2_fnv1a(const wchar_t *, int, uint32_t, uint32_t, uint32_t) { return 0; }
#endif

static inline void add_bbox(hash_t hash, Hash2IdxStratum h2i, const BBox2 &bb, unsigned check_hash2)
{
  OSSpinlockScopedLock lock(cc);
  int idx = priCache.bb.size();
  priCache.bb.push_back(bb);
  priCache.hash2idx[h2i][hash] = idx;
#if CHECK_HASH_COLLISIONS
  hcc_hash2.push_back(check_hash2);
#endif
  G_UNUSED(check_hash2);
}

static inline const BBox2 *find_bbox(hash_t hash, Hash2IdxStratum h2i, unsigned check_hash2)
{
  OSSpinlockScopedLock lock(cc);
  HashToIdx::iterator it = priCache.hash2idx[h2i].find(hash);
#if STRBOXCACHE_DEBUG
  testCnt++;
#endif
  if (it != priCache.hash2idx[h2i].end())
  {
#if STRBOXCACHE_DEBUG
    hitCnt++;
#endif
#if CHECK_HASH_COLLISIONS
    G_ASSERTF(hcc_hash2[it->second] == check_hash2, "hash=0x%x check_hash2=0x%x != 0x%x\nchange HASH_BITS to 64", hash, check_hash2,
      hcc_hash2[it->second]);
#endif
    G_UNUSED(check_hash2);
    return &priCache.bb[it->second];
  }
  if (priCache.bb.size() < priCache.bb.capacity())
  {
    it = secCache.hash2idx[h2i].find(hash);
    if (it != secCache.hash2idx[h2i].end())
    {
#if STRBOXCACHE_DEBUG
      secHitCnt++;
#endif
      int idx = priCache.bb.size();
      priCache.bb.push_back(secCache.bb[it->second]);
#if CHECK_HASH_COLLISIONS
      hcc_hash2.push_back(check_hash2);
#endif
      return &priCache.bb[priCache.hash2idx[h2i][hash] = idx];
    }
    if (secCache.bb.size() && dagor_frame_no() > resetFrameNo + 16)
    {
#if STRBOXCACHE_DEBUG
      debug("strboxcache: clear secCache at frame=%d, secHit=%d", dagor_frame_no(), secHitCnt);
#endif
      clear_and_shrink(secCache.bb);
      for (auto &h : secCache.hash2idx)
        h.clear();
    }
  }
  else
  {
    secCache = eastl::move(priCache);
    priCache.bb.clear();
    priCache.bb.reserve(secCache.bb.capacity());
#if CHECK_HASH_COLLISIONS
    hcc_hash2.clear();
#endif
    for (auto &h : priCache.hash2idx)
      h.clear();
    resetCnt++;
    resetFrameNo = dagor_frame_no();
#if STRBOXCACHE_DEBUG
    debug("strboxcache: resetCnt=%d test=%d hit=%d secHit=%d at frame=%d", resetCnt, testCnt, hitCnt, secHitCnt, dagor_frame_no());
    debug("  secCache.bb=%d h2i=[%d, %d]", secCache.bb.size(), secCache.hash2idx[0].size(), secCache.hash2idx[1].size());
#endif
  }
  return nullptr;
}
} // namespace strboxcache
