// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "font.h"
#include <gui/dag_fonts.h>
#include <util/dag_string.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderVar.h>
#include <image/dag_texPixel.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastIntList.h>
#include <util/dag_baseDef.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_adjpow2.h>
#include <startup/dag_globalSettings.h>
#include <memory/dag_framemem.h>
#include <GuillotineBinPack.h>
#include <SkylineBinPack.h>
#include <EASTL/hash_map.h>
#include <hb-ft.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

static int quotaPerFrameUsecOverride = 100000;

void set_font_raster_quota_per_frame(int quota_usec)
{
  G_ASSERTF(quota_usec >= 0, "Quota must be non-negative");
  quotaPerFrameUsecOverride = quota_usec;
}

static FastNameMapEx ttfNames;
static FT_Library ft_lib;
static SimpleString ttf_path_prefix;
OSReadWriteLock DagorFontBinDump::fontRwLock;

DagorFontBinDump::InscriptionsAtlas DagorFontBinDump::inscr_atlas;
DynamicAtlasTexUpdater DagorFontBinDump::gen_sys_tex;

Tab<uint16_t> DagorFontBinDump::FontData::wcRangePair, DagorFontBinDump::FontData::wcRangeFontIdx;
Tab<float> DagorFontBinDump::FontData::wcRangeFontScl;
Tab<DagorFontBinDump> DagorFontBinDump::add_font(inimem_ptr());

struct DagorFreeTypeFontRec
{
  FT_Face face = nullptr;
  int16_t curHt = -1;
  uint16_t ftFlags = 0;
  SmallTab<FT_Byte, MidmemAlloc> data;
  hb_font_t *font = nullptr;
  static hb_language_t hb_lang;
  static hb_buffer_t *main_hb_buf;

public:
  ~DagorFreeTypeFontRec() { closeTTF(); }

  bool openTTF(const char *fn, bool init_harfbuz = false)
  {
    closeTTF();

    int64_t reft = profile_ref_ticks();
    G_UNUSED(reft);
    file_ptr_t fp = df_open(fn, DF_READ | DF_IGNORE_MISSING);
    if (!fp)
    {
      logerr("failed to load font: %s", fn);
      curHt = -2;
      return false;
    }
    clear_and_resize(data, df_length(fp));
    df_read(fp, data.data(), data_size(data));
    df_close(fp);

    if (FT_New_Memory_Face(ft_lib, data.data(), data_size(data), 0, &face) != 0)
    {
      logerr("failed to open font: %s", fn);
      curHt = -2;
      return false;
    }
    curHt = 0;
    debug("loaded font file <%s> for %.2f ms", fn, profile_time_usec(reft) / 1000.f);
    if (init_harfbuz)
      font = hb_ft_font_create_referenced(face);
    return true;
  }
  void closeTTF()
  {
    if (curHt < 0)
      return;
    if (font)
      hb_font_destroy(font);
    FT_Done_Face(face);
    font = nullptr;
    face = nullptr;
    curHt = -1;
    ftFlags = 0;
    clear_and_shrink(data);
  }
  bool resizeFont(int ht, bool force_auto_hint)
  {
    ftFlags = force_auto_hint ? FT_LOAD_FORCE_AUTOHINT : 0;
    if (ht == curHt)
    {
      if (!font)
        return true;

      if (hb_ft_font_get_load_flags(font) == ftFlags)
        return true;
      hb_ft_font_set_load_flags(font, ftFlags);
      hb_ft_font_changed(font);
      return true;
    }

    if (FT_Set_Pixel_Sizes(face, 0, ht) != 0)
      return false;

    curHt = ht;
    if (!font)
      return true;
    hb_ft_font_set_load_flags(font, ftFlags);
    hb_ft_font_changed(font);
    return true;
  }

  void fillFontMetrics(DagorFontBinDump::FontData::FontMetrics &mx)
  {
    const FT_Size_Metrics &metrics = face->size->metrics;
    mx.ascent = metrics.ascender / 64;
    mx.descent = -metrics.descender / 64;
    mx.lineSpacing = metrics.height / 64;

    mx.capsHt = mx.ascent - mx.descent;
    mx.spaceWd = mx.capsHt / 2;
    if (FT_Load_Glyph(face, getWcharGlyphIdx('A'), ftFlags) == 0)
      mx.capsHt = face->glyph->metrics.height / 64;
    if (FT_Load_Glyph(face, getWcharGlyphIdx(' '), ftFlags) == 0)
      mx.spaceWd = face->glyph->advance.x / 64;
  }

  int getWcharGlyphIdx(int ch) { return curHt >= 0 ? FT_Get_Char_Index(face, ch) : 0; }
  FT_GlyphSlot renderGlyph(int index)
  {
    if (curHt <= 0)
      return NULL;

    if (FT_Load_Glyph(face, index, ftFlags | FT_LOAD_RENDER) != 0)
      return NULL;

    if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) // convert to 8-bit (if not already)
    {
      FT_Bitmap bm8;
      FT_Bitmap_Init(&bm8);
      FT_Bitmap_Convert(ft_lib, &face->glyph->bitmap, &bm8, 1);
      if (face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        for (uint8_t *p = bm8.buffer, *pe = p + bm8.rows * bm8.width; p < pe; p++)
          if (*p)
            *p = 0xFF;
      FT_Bitmap_Done(ft_lib, &face->glyph->bitmap);
      face->glyph->bitmap = bm8;
    }
    return face->glyph;
  }

  static inline hb_buffer_t *acquire_hb_buf() DAG_TS_ACQUIRE_SHARED(DagorFontBinDump::fontRwLock)
  {
    DagorFontBinDump::fontRwLock.lockRead();
    if (is_main_thread())
      return main_hb_buf;
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, DagorFreeTypeFontRec::hb_lang);
    return buf;
  }
  static inline void release_hb_buf(hb_buffer_t *buf) DAG_TS_RELEASE_SHARED(DagorFontBinDump::fontRwLock)
  {
    if (buf != main_hb_buf)
      hb_buffer_destroy(buf);
    DagorFontBinDump::fontRwLock.unlockRead();
  }
};
hb_language_t DagorFreeTypeFontRec::hb_lang = HB_LANGUAGE_INVALID;
hb_buffer_t *DagorFreeTypeFontRec::main_hb_buf = nullptr;

Tab<DagorFontBinDump::DynamicFontAtlas> DagorFontBinDump::dynFontTex;
int DagorFontBinDump::dynFontTexSize = 1;
static Tab<DagorFreeTypeFontRec> ttfRec;
bool DagorFontBinDump::reqCharGenChanged = false, DagorFontBinDump::reqCharGenReset = false;

void DagorFontBinDump::reqCharGen(wchar_t ch, unsigned font_ht)
{
  if (!dynGrp.size() && !isFullyDynamicFont())
    return;
  if (!wcharToBuild)
    wcharToBuild = new FastIntList;
  if (wcharToBuild->addInt((font_ht << 16) | ch))
    DagorFontBinDump::reqCharGenChanged = true;
}
bool DagorFontBinDump::updateGen(int64_t reft, bool allow_raster_glyphs, bool allow_load_ttfs)
{
  if (!wcharToBuild || wcharToBuild->size() == 0)
    return false;
  if (!allow_raster_glyphs)
  {
    wcharToBuild->reset(true);
    return false;
  }

  if (ttfNidToBuild && allow_load_ttfs)
    for (int i = ttfNidToBuild->getList().size() - 1; i >= 0; i--)
    {
      int ttfNid = ttfNidToBuild->getList()[i];
      if (ttfNid < ttfRec.size())
      {
        DagorFreeTypeFontRec &frec = ttfRec[ttfNid];
        if (frec.curHt < 0)
        {
          String fn(0, "%s/%s", ttf_path_prefix, ttfNames.getName(ttfNid));
          debug("load pending font: %d %s", ttfNid, ttfNames.getName(ttfNid));
          if (!frec.openTTF(fn))
            G_ASSERTF(0, "failed to load font '%s'", fn);
        }
      }
      ttfNidToBuild->delInt(ttfNid);
      if (profile_usec_passed(reft, quotaPerFrameUsecOverride))
      {
        logwarn("font load takes too long, %d msec, interrupt", profile_time_usec(reft) / 1000);
        break;
      }
    }

  if (allow_load_ttfs)
    debug("font %p requires %d chars", this, wcharToBuild->size());
  dag::ConstSpan<int> list = wcharToBuild->getList();
  Texture *locked_sys_tex = NULL;
  uint8_t *dest = NULL;
  int dest_stride = 0;
  Tab<DynamicAtlasTexUpdater::CopyRec> copy_rects(framemem_ptr());
  bool useRUB = d3d::get_driver_code().is(d3d::vulkan);
  if (!useRUB)
    copy_rects.reserve(list.size());

  bool added_glyph = false;
  {
    ScopedLockReadTemplate<decltype(fontRwLock)> rl(isFullyDynamicFont() ? &fontRwLock : nullptr);
    for (int i = 0; i < list.size(); i++)
    {
      GlyphPlace *gd = nullptr;
      GlyphData *g = nullptr;
      int ttfNid = -1, pixHt = list[i] >> 16;
      bool force_auto_hint = true;
      int wd = 0, ht = 0;
      unsigned gidx_to_gen = list[i] & 0xffff;

      if (isFullyDynamicFont())
      {
        G_ASSERT(pixHt);
        gd = const_cast<GlyphPlace *>(getDynGlyph(getFontGlyphIdxForHt(pixHt), gidx_to_gen));
        if (!gd)
          continue;
        ttfNid = dfont.ttfNid;
        force_auto_hint = dfont.forceAutoHint;
        wd = gd->u1 - gd->u0;
        ht = gd->v1 - gd->v0;
      }
      else
      {
        // for legacy fonts gidx_to_gen is unicode char here
        g = const_cast<GlyphData *>(&getGlyphU(gidx_to_gen));
        G_ASSERT(g->isDynGrpValid());
        DynGroupDesc &dgd = dynGrp[g->dynGrpIdx - 1];
        ttfNid = dgd.ttfNid;
        pixHt = dgd.pixHt;
        force_auto_hint = (dgd.flags & dgd.FLG_FORCE_AUTOHINT) != 0;
        if (dgd.ttfNid >= ttfRec.size())
          ttfRec.resize(dgd.ttfNid + 1);
        wd = g->x1 - g->x0;
        ht = g->y1 - g->y0;
      }
      if (!(wd && ht))
      {
        const wchar_t utf16symb[2] = {(wchar_t)gidx_to_gen, 0};
        logwarn("font: empty glyph %s 0x%04X(%s) %s %d", name, gidx_to_gen, utf16symb, ttfNames.getName(ttfNid), ttfNid);
        continue;
      }

      if (wd >= gen_sys_tex.getSz().x || ht >= gen_sys_tex.getSz().y)
      {
        logerr("%s (ht %d): glyph is too big (%dx%d) to fit in gen_sys_tex (%dx%d)", name, pixHt, wd, ht, gen_sys_tex.getSz().x,
          gen_sys_tex.getSz().y);
        continue;
      }
      // debug("  %4X %s %d", wc_to_gen, ttfNames.getName(ttfNid), ttfNid);

      DagorFreeTypeFontRec &frec = ttfRec[ttfNid];
      if (frec.curHt < -1)
        continue;
      if (profile_usec_passed(reft, quotaPerFrameUsecOverride))
      {
        logwarn("rasterization takes too long, %d msec, interrupt", profile_time_usec(reft) / 1000);
        break;
      }
      if (frec.curHt < 0)
      {
        String fn(0, "%s/%s", ttf_path_prefix, ttfNames.getName(ttfNid));
        VirtualRomFsData *vp;
        if (!allow_load_ttfs && vromfs_get_file_data(fn, &vp).size() > (1 << 20))
        {
          if (!ttfNidToBuild)
            ttfNidToBuild = new FastIntList;
          if (ttfNidToBuild->addInt(ttfNid))
            debug("delayed ttf load: %s", fn);
          continue;
        }
        if (!frec.openTTF(fn))
        {
          if (g)
            g->dynGrpIdx = 0x7F;
          G_ASSERTF(0, "failed to load font '%s' (for %c+%04X)", fn, isFullyDynamicFont() ? 'G' : 'U', gidx_to_gen);
          continue;
        }
      }
      if (profile_usec_passed(reft, quotaPerFrameUsecOverride))
      {
        logwarn("rasterization takes too long, %d msec, interrupt", profile_time_usec(reft) / 1000);
        break;
      }
      if (!isFullyDynamicFont())
        gidx_to_gen = frec.getWcharGlyphIdx(gidx_to_gen);

      if (useRUB)
      {
        frec.resizeFont(pixHt, force_auto_hint);

        int dest_x0;
        int dest_y0;
        int tex_idx = tryPlaceGlyph(wd, ht, dest_x0, dest_y0);
        if (tex_idx < 0)
        {
          debug("  no area left, resetting cache");
          DagorFontBinDump::reqCharGenReset = true;
          return true;
        }

        const FT_GlyphSlot slot = frec.renderGlyph(gidx_to_gen);
        if (!slot)
          continue;

        DynamicFontAtlas &dtex = dynFontTex[tex_idx];
        dtex.prepareTex(tex_idx);
        d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex_region(dtex.tex, 0, 0, dest_x0, dest_y0, 0, wd, ht, 1);

        if (!rub)
          continue;

        // after we write tex_idx and related info to g/gd, it will be treated as valid and will not be re-rendered
        // so we can't skip glyph upload after that too
        if (gd)
        {
          gd->texIdx = tex_idx;
          dtex.placeGlyphD(*gd, wd, ht, dest_x0, dest_y0);
        }
        else if (g)
        {
          dtex.placeGlyph(*g, wd, ht, dest_x0, dest_y0);
          for (int j = texOrig.size(); j < tex.size(); j++)
            if (tex[j].texId == dtex.texId)
            {
              g->texIdx = j;
              break;
            }
            else if (tex[j].texId == BAD_TEXTUREID)
            {
              tex[j].texId = dtex.texId;
              tex[j].tex = dtex.tex;
              g->texIdx = j;
              break;
            }
          G_ASSERT(g->texIdx < tex.size());
        }

        dest = (uint8_t *)d3d::get_update_buffer_addr_for_write(rub);
        dest_stride = d3d::get_update_buffer_pitch(rub);

        gen_sys_tex.clearData(wd, ht, dest, dest_stride, 0, 0);
        gen_sys_tex.copyData(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, dest, dest_stride, 0, 0);

        d3d::update_texture_and_release_update_buffer(rub);
        added_glyph = true;
        continue;
      }

      DynamicAtlasTexUpdater::CopyRec &cr = copy_rects.push_back();
      bool erase_rects_but_last = false;
      Texture *sys_tex = gen_sys_tex.add(wd, ht, cr);
      if (!sys_tex)
      {
        copy_rects.pop_back();
        continue;
      }

      frec.resizeFont(pixHt, force_auto_hint);

      int dest_x0;
      int dest_y0;
      int tex_idx = tryPlaceGlyph(wd, ht, dest_x0, dest_y0);
      if (tex_idx < 0)
      {
        debug("  no area left, resetting cache");
        DagorFontBinDump::reqCharGenReset = true;
        if (locked_sys_tex)
          locked_sys_tex->unlockimg();
        copy_rects.pop_back();
        return true;
      }
      cr.dest_x0 = dest_x0;
      cr.dest_y0 = dest_y0;
      cr.dest_idx = tex_idx;

      if (locked_sys_tex != sys_tex)
      {
        if (locked_sys_tex)
        {
          locked_sys_tex->unlockimg();
          if (copy_rects.size() > 1)
          {
            // flush ready rects
            erase_rects_but_last = true;
            for (int i = 0; i < (int)copy_rects.size() - 1; i++)
              dynFontTex[copy_rects[i].dest_idx].tex->updateSubRegion(locked_sys_tex, 0, copy_rects[i].x0, copy_rects[i].y0, 0,
                copy_rects[i].w, copy_rects[i].h, 1, 0, copy_rects[i].dest_x0, copy_rects[i].dest_y0, 0);
          }
        }
        locked_sys_tex = sys_tex;
        if (!locked_sys_tex->lockimgEx(&dest, dest_stride) || !dest)
        {
          copy_rects.pop_back();
          locked_sys_tex = NULL;
          continue;
        }
      }
      const FT_GlyphSlot slot = frec.renderGlyph(gidx_to_gen);
      if (!slot)
        continue;

      gen_sys_tex.clearData(wd, ht, dest, dest_stride, cr.x0, cr.y0);
      gen_sys_tex.copyData(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, dest, dest_stride, cr.x0, cr.y0);

      DynamicFontAtlas &dtex = dynFontTex[tex_idx];
      dtex.prepareTex(tex_idx);
      if (gd)
      {
        gd->texIdx = tex_idx;
        cr.w = slot->bitmap.width;
        cr.h = slot->bitmap.rows;
        dtex.placeGlyphD(*gd, wd, ht, dest_x0, dest_y0);
      }
      else if (g)
      {
        dtex.placeGlyph(*g, wd, ht, dest_x0, dest_y0);
        for (int j = texOrig.size(); j < tex.size(); j++)
          if (tex[j].texId == dtex.texId)
          {
            g->texIdx = j;
            break;
          }
          else if (tex[j].texId == BAD_TEXTUREID)
          {
            tex[j].texId = dtex.texId;
            tex[j].tex = dtex.tex;
            g->texIdx = j;
            break;
          }
        G_ASSERT(g->texIdx < tex.size());
      }

      if (erase_rects_but_last)
        erase_items(copy_rects, 0, copy_rects.size() - 1);
      added_glyph = true;
    }
  } // end of read lock scope
  wcharToBuild->reset(true);
  if (locked_sys_tex)
  {
    locked_sys_tex->unlockimg();
    // flush pending rects
    for (int i = 0; i < copy_rects.size(); i++)
      dynFontTex[copy_rects[i].dest_idx].tex->updateSubRegion(locked_sys_tex, 0, copy_rects[i].x0, copy_rects[i].y0, 0,
        copy_rects[i].w, copy_rects[i].h, 1, 0, copy_rects[i].dest_x0, copy_rects[i].dest_y0, 0);
  }

  return added_glyph;
}
void DagorFontBinDump::resetGen()
{
  for (int i = 0; i < glyph.size(); i++)
    if (glyph[i].isDynGrpValid() && glyph[i].texIdx >= texOrig.size())
      const_cast<GlyphData &>(glyph[i]).texIdx = 0xFF;
  for (int i = texOrig.size(); i < tex.size(); i++)
  {
    tex[i].texId = BAD_TEXTUREID;
    tex[i].tex = NULL;
  }
  if (isFullyDynamicFont())
    for (auto &u256p : dfont.fontGlyphs)
      clear_all_ptr_items(u256p);
}
int DagorFontBinDump::compute_str_width_u(const wchar_t *str, int len, float scale, BBox2 &out_bb)
{
  if (isFullyDynamicFont())
    return dynfont_compute_str_visbox_u(dfont.pixHt * scale, str, len, 1.0f, 0, out_bb);

  static const int spacing = 0, mono_w = 0;
  DynGroupDesc *dgd = NULL;
  DagorFreeTypeFontRec *frec = NULL;
  int cx = 0;

  out_bb.setempty();
  for (; *str && len; str++, len--)
  {
    if (*str < 0x20)
      continue;

    const DagorFontBinDump::GlyphData &gd = getGlyphU(*str);
    if (!gd.isDynGrpValid())
    {
      int dx2 = 0;
      inplace_min(out_bb[0].y, getGlyphLt(gd).y * scale);
      inplace_max(out_bb[1].y, getGlyphRb(gd).y * scale);
      inplace_min(out_bb[0].x, cx + getGlyphLt(gd).x * scale);
      inplace_max(out_bb[1].x, cx + getGlyphRb(gd).x * scale);
      cx += getDx2(dx2, mono_w, gd, str[1]) * scale;
      cx += (dx2 + spacing) * scale;
      continue;
    }

    if (dgd != &dynGrp[gd.dynGrpIdx - 1])
    {
      dgd = &dynGrp[gd.dynGrpIdx - 1];
      if (dgd->ttfNid >= ttfRec.size())
        ttfRec.resize(dgd->ttfNid + 1);
      frec = &ttfRec[dgd->ttfNid];
      if (frec->curHt < -1)
        frec = NULL;
      else
      {
        if (frec->curHt < 0)
        {
          String fn(0, "%s/%s", ttf_path_prefix, ttfNames.getName(dgd->ttfNid));
          if (!frec->openTTF(fn))
          {
            const_cast<GlyphData &>(gd).dynGrpIdx = 0x7F;
            G_ASSERTF(0, "failed to load font '%s' (for U+%04X)", fn, *str);
            frec = NULL;
          }
        }
        if (frec)
          frec->resizeFont(floorf(dgd->pixHt * scale), dgd->flags & dgd->FLG_FORCE_AUTOHINT);
      }
    }

    if (frec)
    {
      FT_UInt g0idx = frec->getWcharGlyphIdx(str[0]);
      FT_UInt g1idx = frec->getWcharGlyphIdx(str[1]);
      const FT_GlyphSlot slot = frec->renderGlyph(g0idx);
      FT_Vector pk;
      if (mono_w || FT_Get_Kerning(frec->face, g0idx, g1idx, FT_KERNING_DEFAULT, &pk) != 0)
        pk.x = 0;
      if (slot)
      {
        inplace_min(out_bb[0].y, -slot->bitmap_top);
        inplace_max(out_bb[1].y, -slot->bitmap_top + int(slot->bitmap.rows));
        inplace_min(out_bb[0].x, cx + slot->bitmap_left);
        inplace_max(out_bb[1].x, cx + slot->bitmap_left + int(slot->bitmap.width));
      }
      int glyph_dx = ((slot ? slot->advance.x : 0) + pk.x) >> 6;
      int next_cx = cx + (mono_w ? mono_w * scale : glyph_dx);
      cx = next_cx + spacing * scale;
    }
    else
      cx += ((mono_w ? mono_w : getDx(gd, str[1])) + spacing) * scale;
  }
  return cx;
}
bool DagorFontBinDump::rasterize_str_u(const DagorFontBinDump::InscriptionData &g, const wchar_t *str, int len, float scale)
{
  static const int spacing = 0, mono_w = 0;
  uint8_t *dest = NULL;
  int dest_stride;
  DynamicAtlasTexUpdater::CopyRec cr;
  int margin = inscr_atlas.getMargin();
  int margin_ofs = inscr_atlas.getMarginLtOfs();
  Texture *sys_tex = gen_sys_tex.add(g.w + margin, g.h + margin, cr);
  if (!sys_tex)
  {
    if (g.dx < g.ST_restoring)
      logwarn("rasterize_str_u: no temp storage for (%X)=%dx%d (max rect is %dx%d, font=<%s> ht=%d scaledHt=%.0f), f=%d", g.hash,
        g.w + margin, g.h + margin, gen_sys_tex.getSz().x, gen_sys_tex.getSz().y, name.get(), ascent, ascent * scale,
        dagor_frame_no());
    return false;
  }
  if (!sys_tex->lockimgEx(&dest, dest_stride) || !dest)
    return false;

  gen_sys_tex.clearData(cr.w, cr.h, dest, dest_stride, cr.x0, cr.y0);
  int cx = cr.x0 + margin_ofs - g.xBaseOfs, xbs = xBase * scale;
  int cy = cr.y0 + margin_ofs - g.yBaseOfs, ybs = yBase * scale;

  if (isFullyDynamicFont())
  {
    int pixHt = floorf(dfont.pixHt * scale);
    if (len < 0)
      len = wcslen(str);
    ScopeHBuf buf;
    int cursor_x = 0, cursor_y = 0, spacing_inc = (int)floorf(spacing * scale * 64);
    for (int eff_font_ht = 0, seglen = len; len > 0; str += seglen, len -= seglen)
    {
      DagorFontBinDump *f = dynfont_get_next_segment(str, len, eff_font_ht = pixHt, seglen);
      f->dynfont_prepare_str(buf, eff_font_ht, str, seglen, nullptr);

      unsigned glyph_count = 0;
      hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
      if (!glyph_count)
        continue;
      hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
      int ttfNid = f->dfont.ttfNid;
      ttfRec[ttfNid].resizeFont(pixHt, f->dfont.forceAutoHint);

      for (int i = 0; i < glyph_count; i++, glyph_info++, glyph_pos++)
      {
        hb_codepoint_t gidx = glyph_info->codepoint;
        if (const FT_GlyphSlot slot = ttfRec[ttfNid].renderGlyph(gidx))
          gen_sys_tex.copyData(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, dest, dest_stride,
            cx + (cursor_x + glyph_pos->x_offset + 32) / 64 + slot->bitmap_left,
            cy + (cursor_y + glyph_pos->y_offset + 32) / 64 - slot->bitmap_top);
        cursor_x += glyph_pos->x_advance + spacing_inc;
        cursor_y += glyph_pos->y_advance;
      }
    }

    cx += cursor_x / 64;
    cy += cursor_y / 64;
  }
  else
  {
    DynGroupDesc *dgd = NULL;
    DagorFreeTypeFontRec *frec = NULL;
    Texture *ftex = NULL;
    TextureInfo ftexinfo;

    for (; *str && len; str++, len--)
    {
      if (*str < 0x20)
        continue;

      const DagorFontBinDump::GlyphData &gd = getGlyphU(*str);
      if (!gd.isDynGrpValid())
      {
        int dx2 = 0;
        cx += getDx2(dx2, mono_w, gd, str[1]) * scale;
        if (gd.texIdx < tex.size())
        {
          if (ftex != tex[gd.texIdx].tex)
          {
            ftex = tex[gd.texIdx].tex;
            ftex->getinfo(ftexinfo, 0);
          }

          int dest_x = cx + xbs + gd.x0 * scale, dest_y = cy + ybs + gd.y0 * scale;
          if (dest_x >= 0 && dest_y >= 0)
            sys_tex->updateSubRegion(tex[gd.texIdx].tex, 0, int(gd.u0 * ftexinfo.w + 0.5), int(gd.v0 * ftexinfo.h + 0.5), 0,
              gd.x1 - gd.x0, gd.y1 - gd.y0, 1, 0, dest_x, dest_y, 0);
          else
            logerr("rasterize_str_u: failed to copy pre-rendered glyph (u%04X) to %d,%d (cpos=%d,%d)", *str, dest_x, dest_y, cx, cy);
        }
        cx += (dx2 + spacing) * scale;
        continue;
      }

      if (dgd != &dynGrp[gd.dynGrpIdx - 1])
      {
        dgd = &dynGrp[gd.dynGrpIdx - 1];
        if (dgd->ttfNid >= ttfRec.size())
          ttfRec.resize(dgd->ttfNid + 1);
        frec = &ttfRec[dgd->ttfNid];
        if (frec->curHt < -1)
          frec = NULL;
        else
        {
          if (frec->curHt < 0)
          {
            String fn(0, "%s/%s", ttf_path_prefix, ttfNames.getName(dgd->ttfNid));
            if (!frec->openTTF(fn))
            {
              const_cast<GlyphData &>(gd).dynGrpIdx = 0x7F;
              G_ASSERTF(0, "failed to load font '%s' (for U+%04X)", fn, *str);
              frec = NULL;
            }
          }
          if (frec)
            frec->resizeFont(floorf(dgd->pixHt * scale), dgd->flags & dgd->FLG_FORCE_AUTOHINT);
        }
      }

      if (frec)
      {
        FT_UInt g0idx = frec->getWcharGlyphIdx(str[0]);
        FT_UInt g1idx = frec->getWcharGlyphIdx(str[1]);
        const FT_GlyphSlot slot = frec->renderGlyph(g0idx);
        FT_Vector pk;
        if (mono_w || FT_Get_Kerning(frec->face, g0idx, g1idx, FT_KERNING_DEFAULT, &pk) != 0)
          pk.x = 0;
        int glyph_dx = (slot->advance.x + pk.x) >> 6;
        int next_cx = cx + (mono_w ? mono_w * scale : glyph_dx);
        if (mono_w)
          cx += (next_cx - cx - glyph_dx) / 2;
        gen_sys_tex.copyData(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, dest, dest_stride, cx + slot->bitmap_left,
          cy - slot->bitmap_top);
        cx = next_cx + spacing * scale;
      }
      else
        cx += ((mono_w ? mono_w : getDx(gd, str[1])) + spacing) * scale;
    }
  }
  cx -= (cr.x0 + margin_ofs);
  if (cx < g.w)
  {
    const_cast<InscriptionData &>(g).dx = cx;
    const_cast<InscriptionData &>(g).w = cx;
    const_cast<InscriptionData &>(g).u1 = (g.x0 + g.w) * inscr_atlas.getInvW();
  }
  else if (g.dx >= g.ST_restoring)
    const_cast<InscriptionData &>(g).dx = cx;
  // margins are only required when text box is prepared inaccurate (but they cost almost nothing)
  gen_sys_tex.clearData(cr.w, margin_ofs, dest, dest_stride, cr.x0, cr.y0);
  gen_sys_tex.clearData(cr.w, margin_ofs, dest, dest_stride, cr.x0, cr.y0 + cr.h - margin_ofs);
  gen_sys_tex.clearData(margin_ofs, cr.h, dest, dest_stride, cr.x0, cr.y0);
  gen_sys_tex.clearData(margin_ofs, cr.h, dest, dest_stride, cr.x0 + cr.w - margin_ofs, cr.y0);

  sys_tex->unlockimg();
  inscr_atlas.tex.first.getTex2D()->updateSubRegion(sys_tex, 0, cr.x0, cr.y0, 0, cr.w, cr.h, 1, 0, g.x0 - margin_ofs,
    g.y0 - margin_ofs, 0);
  return true;
}

inline uint16_t *DagorFontBinDump::FontData::getEffFontIdxPtr(wchar_t c) const
{
  if ((addFontMask >> (c / 0x400)) & 1)
  {
    for (uint16_t *r = wcRangePair.data() + addFontRange[0], *r_e = wcRangePair.data() + addFontRange[1]; r < r_e; r += 2)
      if (c < r[0])
        break;
      else if (c <= r[1])
        return &wcRangeFontIdx[(r - wcRangePair.data()) / 2];
  }
  return nullptr;
}
static int resolve_ptr(uint16_t *p) { return p ? *p : -1; }

DagorFontBinDump *DagorFontBinDump::get_next_segment(const wchar_t *str, int len, int &inout_font_ht, int &out_seglen)
{
  uint16_t *eff_idx_ptr = dfont.getEffFontIdxPtr(*str);
  int eff_idx = resolve_ptr(eff_idx_ptr);
  out_seglen = 1;
  for (str++, len--; len > 0 && eff_idx == resolve_ptr(dfont.getEffFontIdxPtr(*str)); str++, len--)
    out_seglen++;

  if (!eff_idx_ptr)
    return this;

  int scl_idx = eff_idx_ptr - dfont.wcRangeFontIdx.data();
  inout_font_ht = int(floorf(inout_font_ht * FontData::wcRangeFontScl[scl_idx] + 0.5f));
  return &add_font[eff_idx & FontData::WCRFI_FONT_MASK];
}

DagorFontBinDump::ScopeHBuf::ScopeHBuf() DAG_TS_ACQUIRE_SHARED(DagorFontBinDump::fontRwLock) :
  buf(DagorFreeTypeFontRec::acquire_hb_buf())
{}
DagorFontBinDump::ScopeHBuf::~ScopeHBuf() DAG_TS_RELEASE_SHARED(DagorFontBinDump::fontRwLock)
{
  DagorFreeTypeFontRec::release_hb_buf(buf);
}

void DagorFontBinDump::dynfont_prepare_str(DagorFontBinDump::ScopeHBuf &buf, int font_ht, const wchar_t *str, int len, int *out_fgidx)
  DAG_TS_REQUIRES_SHARED(DagorFontBinDump::fontRwLock)
{
  hb_buffer_set_length(buf, 0);
#if (WCHAR_MAX + 0) == 0xffff || (WCHAR_MAX + 0) == 0x7fff // sizeof(wchar_t) == 2
  G_STATIC_ASSERT(sizeof(wchar_t) == sizeof(uint16_t));
  hb_buffer_add_utf16(buf, (const uint16_t *)str, len, 0, -1);
#elif (WCHAR_MAX + 0) == 0xffffffff || (WCHAR_MAX + 0) == 0x7fffffff // sizeof(wchar_t) == 4
  G_STATIC_ASSERT(sizeof(wchar_t) == sizeof(uint32_t));
  hb_buffer_add_utf32(buf, (const uint32_t *)str, len, 0, -1);
#else
#error dynfont_prepare_str() unsupported wchar_t implementation!
#endif

  DagorFreeTypeFontRec &ttf = ttfRec[dfont.ttfNid];
  ttf.resizeFont(font_ht, dfont.forceAutoHint);
  {
    TIME_PROFILE_DEV(harfbuzz_shape);
    hb_shape(ttf.font, buf, NULL, 0);
  }
  if (!out_fgidx)
    return;

  int fgidx = getFontGlyphIdxForHt(font_ht);
  if (fgidx < 0)
    fgidx = appendFontHt(font_ht, &ttf, /*read_locked*/ true);
  *out_fgidx = fgidx;

  unsigned glyph_count = 0;
  const hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_codepoint_t max_cp = 0;
  unsigned new_glyphs = 0;
  Tab<Unicode256Place *> &u256p = dfont.fontGlyphs[fgidx];
  for (int i = 0; i < glyph_count; i++)
  {
    hb_codepoint_t gidx = (glyph_info + i)->codepoint;
    hb_codepoint_t page = gidx >> 8;
    if (page < u256p.size())
    {
      if (!u256p[page] || u256p[page]->gp[gidx & 0xFF].texIdx == GlyphPlace::TEXIDX_UNUSED)
        new_glyphs++;
    }
    else
    {
      if (max_cp < gidx)
        max_cp = gidx;
      new_glyphs++;
    }
  }
  if (!new_glyphs)
    return; //-V1020 The function exited without calling 'fontRwLock.unlockRead'

  // prepare basic glyph data
  DagorFontBinDump::fontRwLock.unlockRead();
  {
    ScopedLockWriteTemplate<decltype(fontRwLock)> wl(fontRwLock);

    if (((max_cp + 1) >> 8) >= u256p.size())
    {
      int st = u256p.size();
      int new_sz = ((max_cp + 1) >> 8) + 1;
      append_items(u256p, new_sz - st);
      mem_set_0(make_span(u256p).subspan(st, new_sz - st));
    }

    hb_glyph_extents_t extents = {0};
    for (int i = 0; i < glyph_count; i++, glyph_info++)
    {
      hb_codepoint_t gidx = glyph_info->codepoint;
      hb_codepoint_t page = gidx >> 8;

      if (!u256p[page])
        u256p[page] = new Unicode256Place;

      GlyphPlace &g = u256p[page]->gp[gidx & 0xFF];
      if (g.texIdx == GlyphPlace::TEXIDX_UNUSED)
      {
        if (hb_font_get_glyph_extents(ttf.font, gidx, &extents))
        {
          g.u1 = (extents.width + 63) / 64 + 1;
          g.v1 = (-extents.height + 63) / 64 + 1;
          g.ox = (extents.x_bearing + (extents.x_bearing >= 0 ? 32 : -32)) / 64;
          g.oy = -(extents.y_bearing + (extents.y_bearing >= 0 ? 32 : -32)) / 64;
          g.texIdx = (extents.width && extents.height) ? GlyphPlace::TEXIDX_NOTPLACED : GlyphPlace::TEXIDX_INVISIBLE;
          // debug("glyph %d (%c): u0=%d v0=%d u1=%d v1=%d ox=%d oy=%d texIdx=%d (font %p ht=%d) %dx%d o=%d,%d",
          //   gidx, str[i], g.u0, g.v0, g.u1, g.v1, g.ox, g.oy, g.texIdx, ttf.font, font_ht,
          //   extents.width, extents.height, extents.x_bearing, extents.y_bearing);
        }
        else
          g.texIdx = GlyphPlace::TEXIDX_INVISIBLE;

        new_glyphs--;
        if (!new_glyphs)
          break;
      }
    }
  }
  DagorFontBinDump::fontRwLock.lockRead();
}

float DagorFontBinDump::dynfont_compute_str_width_u(int font_ht, const wchar_t *str, int len, float scale, int spacing, BBox2 &out_bb)
{
  if (!font_ht)
    font_ht = dfont.pixHt;
  if (len < 0)
    len = wcslen(str);

  ScopeHBuf buf;
  int cursor_x = 0, cursor_y = 0;
  for (int eff_font_ht = 0, seglen = len; len > 0; str += seglen, len -= seglen)
  {
    DagorFontBinDump *f = dynfont_get_next_segment(str, len, eff_font_ht = font_ht, seglen);
    f->dynfont_prepare_str(buf, eff_font_ht, str, seglen, nullptr);

    unsigned glyph_count = 0;
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    if (glyph_count > 1)
      cursor_x += (glyph_count - 1) * spacing * 64;
    for (int i = 0; i < glyph_count; i++, glyph_pos++)
    {
      cursor_x += glyph_pos->x_advance;
      cursor_y += glyph_pos->y_advance;
    }
  }
  out_bb.lim[0].set(0, -floorf(ascent * scale * font_ht / dfont.pixHt + 0.5f));
  out_bb.lim[1].set(floorf(cursor_x * scale / 64.0f + 0.5f), floorf(cursor_y * scale / 64.0f + 0.5f));
  return out_bb.lim[1].x;
}
float DagorFontBinDump::dynfont_compute_str_visbox_u(int font_ht, const wchar_t *str, int len, float scale, int spacing, BBox2 &out_bb)
{
  if (!font_ht)
    font_ht = dfont.pixHt;
  if (len < 0)
    len = wcslen(str);

  ScopeHBuf buf;
  int cursor_x = 0, cursor_y = 0;
  for (int eff_font_ht = 0, seglen = len; len > 0; str += seglen, len -= seglen)
  {
    DagorFontBinDump *f = dynfont_get_next_segment(str, len, eff_font_ht = font_ht, seglen);
    f->dynfont_prepare_str(buf, eff_font_ht, str, seglen, nullptr);

    unsigned glyph_count = 0;
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    if (glyph_count > 1)
      cursor_x += (glyph_count - 1) * spacing * 64;
    for (int i = 0; i < glyph_count; i++, glyph_pos++)
    {
      cursor_x += glyph_pos->x_advance;
      cursor_y += glyph_pos->y_advance;
    }
  }
  out_bb.lim[0].set(0, -floorf(ascent * scale * font_ht / dfont.pixHt + 0.5f));
  out_bb.lim[1].set(floorf(cursor_x * scale / 64.0f + 0.5f), floorf(cursor_y * scale / 64.0f + 0.5f));
  inplace_max(out_bb.lim[1].y, descent * scale * font_ht / dfont.pixHt);
  return out_bb.lim[1].x;
}
BBox2 DagorFontBinDump::dynfont_compute_str_width_u_ex1(int font_ht, const wchar_t *str, int len, float scale, int spacing, int mono_w,
  float max_w, bool left_align, const wchar_t *break_sym, float ellipsis_resv_w, int &out_start_idx, int &out_count)
{
  ScopeHBuf buf;
  dynfont_prepare_str(buf, font_ht, str, len, nullptr);
  out_start_idx = out_count = 0;

  int mono_step = mono_w ? int(floorf(mono_w * scale * 64)) : 0;
  unsigned glyph_count = 0;
  hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
  int cursor_x = mono_w ? mono_step * glyph_count : (glyph_count > 1) ? (glyph_count - 1) * spacing * 64 : 0, cursor_y = 0;
  if (!mono_w)
    for (int i = 0; i < glyph_count; i++, glyph_pos++)
    {
      cursor_x += glyph_pos->x_advance;
      cursor_y += glyph_pos->y_advance;
    }

  int max_cpos = int(max_w * 64 / scale);
  if (cursor_x <= max_cpos)
    out_count = len;
  else
  {
    int full_pos = cursor_x;
    int break_idx = 0, break_pos = 0;
    max_cpos = int((max_w - ellipsis_resv_w / scale) * 64);
    if (!left_align)
      max_cpos = full_pos - max_cpos, break_idx = len, break_pos = full_pos;

    cursor_x = 0;
    glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    for (int i = 0; i < glyph_count; i++, glyph_pos++)
    {
      int npos = cursor_x + (mono_w ? mono_step : glyph_pos->x_advance + spacing * 64);
      if (npos > max_cpos)
      {
        if (!break_sym)
          break_idx = left_align ? i : i + 1, break_pos = left_align ? cursor_x : npos;
        if (left_align || !break_sym)
          break;
      }
      cursor_x = npos;
      if (break_sym && wcschr(break_sym, str[i]))
      {
        break_idx = i + 1, break_pos = cursor_x;
        if (!left_align && npos > max_cpos)
          break;
      }
    }

    if (left_align)
      out_count = break_idx, cursor_x = break_pos;
    else
      out_start_idx = break_idx, out_count = len - break_idx, cursor_x = full_pos - break_pos;
  }

  BBox2 out_bb;
  out_bb.lim[0].set(0, -floorf(ascent * scale * font_ht / dfont.pixHt + 0.5f));
  out_bb.lim[1].set(floorf(cursor_x * scale / 64.0f + 0.5f), floorf(cursor_y * scale / 64.0f + 0.5f));
  return out_bb;
}
BBox2 DagorFontBinDump::dynfont_compute_str_width_u_ex2(int font_ht, const wchar_t *str, int len, float scale, int spacing,
  float max_w, bool left_align, const wchar_t *break_sym, float ellipsis_resv_w, int &out_start_idx, int &out_count)
{
  BBox2 bb;
  dynfont_compute_str_width_u(font_ht, str, len, scale, spacing, bb);
  if (bb.width().x <= max_w)
  {
    out_start_idx = 0;
    out_count = len;
    return bb;
  }

  int l = 0, r = len;
  int c = (l + r) / 2, res = left_align ? 0 : len;
  BBox2 res_bb(bb[0], bb[0]);
  max_w -= ellipsis_resv_w;
  while (c > l)
  {
    if (break_sym)
    {
      if (left_align)
        while (c > l && !wcschr(break_sym, str[c]))
          c--;
      else
        while (c < r && !wcschr(break_sym, str[c]))
          c++;
      if (c == l || c == r)
        break;
    }
    dynfont_compute_str_width_u(font_ht, left_align ? str : str + c, left_align ? c : len - c, scale, spacing, bb);
    if (bb.width().x <= max_w)
      (left_align ? l : r) = c, res = c, res_bb = bb;
    else
      (left_align ? r : l) = c;

    c = (l + r) / 2;
  }
  out_start_idx = left_align ? 0 : res;
  out_count = left_align ? res : len - res;
  return res_bb;
}

bool DagorFontBinDump::setFontHt(int ht)
{
  if (!isFullyDynamicFont())
    return false;
  dfont.pixHt = ht;
  G_ASSERT(dfont.fontHt.size() && dfont.fontHt.size() == dfont.fontGlyphs.size() && dfont.fontHt.size() == dfont.fontMx.size());

  int idx = getFontGlyphIdxForHt(ht);
  if (idx > 0)
  {
    clear_all_ptr_items(dfont.fontGlyphs[0]);
    dfont.fontGlyphs[0] = eastl::move(dfont.fontGlyphs[idx]);
    dfont.fontHt[0] = dfont.fontHt[idx];
    dfont.fontMx[0] = dfont.fontMx[idx];

    erase_items(dfont.fontGlyphs, idx, 1);
    erase_items(dfont.fontHt, idx, 1);
    erase_items(dfont.fontMx, idx, 1);
  }
  else if (idx < 0)
  {
    ttfRec[dfont.ttfNid].resizeFont(ht, dfont.forceAutoHint);

    clear_all_ptr_items(dfont.fontGlyphs[0]);
    dfont.fontHt[0] = ht;
    ttfRec[dfont.ttfNid].fillFontMetrics(dfont.fontMx[0]);
  }
  fontHt = dfont.fontHt[0];
  fontCapsHt = dfont.fontMx[0].capsHt;
  ascent = dfont.fontMx[0].ascent;
  descent = dfont.fontMx[0].descent;
  lineSpacing = dfont.fontMx[0].lineSpacing;
  return true;
}

// NOTE: conditional locking is too complicated for thread safety analysis.
int DagorFontBinDump::appendFontHt(unsigned ht, DagorFreeTypeFontRec *ttf, bool read_locked) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT(isFullyDynamicFont());
  G_ASSERT(dfont.fontHt.size() == dfont.fontGlyphs.size() && dfont.fontHt.size() == dfont.fontMx.size());
  G_ASSERTF(ht && ht < 16384, "ht=%d dfont.fontHt.size()=%d", ht, dfont.fontHt.size());

  if (!ttf)
  {
    ttf = &ttfRec[dfont.ttfNid];
    ttf->resizeFont(ht, dfont.forceAutoHint);
  }

  int idx;
  if (read_locked) // in order to lock for write we have to unlock for read first
    fontRwLock.unlockRead();

  {
    ScopedLockWriteTemplate<decltype(fontRwLock)> wl(fontRwLock);
    idx = append_items(dfont.fontGlyphs, 1);
    dfont.fontHt.push_back(ht);
    ttf->fillFontMetrics(dfont.fontMx.push_back());
  }

  if (read_locked) // re-lock
    fontRwLock.lockRead();

  return idx;
}

DagorFontBinDump::DynamicFontAtlas::~DynamicFontAtlas() { ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texId, tex); }
void DagorFontBinDump::DynamicFontAtlas::prepareTex(int idx)
{
  if (tex)
    return;
  tex = (Texture *)d3d::create_tex(NULL, hist.size(), hist.size(), TEXFMT_L8 | TEXCF_MAYBELOST | TEXCF_UPDATE_DESTINATION, 1,
    "dynFontAtlas");
  texId = register_managed_tex(String(0, "dynFontAtlas%d", idx), tex);
  if (d3d::is_stub_driver()) // we don't want doing useless concurrent lock along with gen_sys_tex
    return;

  unsigned texLockFlags = TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY;
  uint8_t *imgPtr;
  int stride;
  if (tex->lockimg((void **)&imgPtr, stride, 0, texLockFlags))
  {
    for (int y = 0; y < hist.size(); y++, imgPtr += stride)
      memset(imgPtr, 0, stride);
    tex->unlockimg();
  }
}
void DagorFontBinDump::initDynFonts(int tex_cnt, int tex_sz, const char *path_prefix)
{
  dynFontTexSize = tex_sz;
  dynFontTex.resize(tex_cnt);
  for (int i = 0; i < dynFontTex.size(); i++)
  {
    clear_and_resize(dynFontTex[i].hist, tex_sz);
    mem_set_0(dynFontTex[i].hist);
  }

  FT_Init_FreeType(&ft_lib);
  ttf_path_prefix = path_prefix;
  DagorFreeTypeFontRec::hb_lang = hb_language_from_string("ru", -1);
  DagorFreeTypeFontRec::main_hb_buf = hb_buffer_create();
  hb_buffer_set_direction(DagorFreeTypeFontRec::main_hb_buf, HB_DIRECTION_LTR);
  hb_buffer_set_script(DagorFreeTypeFontRec::main_hb_buf, HB_SCRIPT_LATIN);
  hb_buffer_set_language(DagorFreeTypeFontRec::main_hb_buf, DagorFreeTypeFontRec::hb_lang);
}
void DagorFontBinDump::termDynFonts()
{
  for (DagorFontBinDump &f : add_font)
    f.clear();
  clear_and_shrink(add_font);
  clear_and_shrink(dynFontTex);
  clear_and_shrink(ttfRec);
  ttfNames.clear();
  ttfNames.shrink_to_fit();
  hb_buffer_destroy(DagorFreeTypeFontRec::main_hb_buf);
  DagorFreeTypeFontRec::main_hb_buf = nullptr;
  FT_Done_FreeType(ft_lib);
  clear_and_shrink(FontData::wcRangePair);
  clear_and_shrink(FontData::wcRangeFontIdx);
  clear_and_shrink(FontData::wcRangeFontScl);
  debug("strboxcache: resetCnt=%d priCache.bb=%d secCache.bb=%d h2i_str=%d h2i_char=%d  (strVisBoxCnt=%d)", strboxcache::resetCnt,
    strboxcache::priCache.bb.size(), strboxcache::secCache.bb.size(), strboxcache::priCache.hash2idx[strboxcache::H2I_STR].size(),
    strboxcache::priCache.hash2idx[strboxcache::H2I_CHAR].size(), strboxcache::strVisBoxCnt);
#if STRBOXCACHE_DEBUG
  debug("  test=%d hit=%d secHit=%d  hitRate=%.2f%%", strboxcache::testCnt, strboxcache::hitCnt, strboxcache::secHitCnt,
    strboxcache::testCnt ? 100.0 * (strboxcache::hitCnt + strboxcache::secHitCnt) / strboxcache::testCnt : 0);
  debug("  cacheTime=%d usec (avg %.3f usec/test), calcTime=%d usec (avg %.1f usec/calc for calcCount=%d)",
    profile_usec_from_ticks_delta(strboxcache::cacheTimeTicks),
    strboxcache::testCnt ? profile_usec_from_ticks_delta(strboxcache::cacheTimeTicks) / double(strboxcache::testCnt) : 0,
    profile_usec_from_ticks_delta(strboxcache::calcTimeTicks),
    strboxcache::calcCount ? profile_usec_from_ticks_delta(strboxcache::calcTimeTicks) / double(strboxcache::calcCount) : 0,
    strboxcache::calcCount);
#endif
}

void DagorFontBinDump::InscriptionsAtlas::init(const DataBlock &b)
{
  G_STATIC_ASSERT((sizeof(StdGuiRender::GuiContext) % 16) == 0);
  IPoint2 sz = b.getIPoint2("inscriptionsTexSz", IPoint2(0, 0));
  int ref_hres = b.getInt("refHRes", 0);
  if (sz.x && sz.y && ref_hres)
  {
    static constexpr int SZ_ALIGN = 32;
    int scrw = 0, scrh = 0;
    d3d::get_screen_size(scrw, scrh);
    if (scrh > ref_hres)
    {
      if (is_pow_of2(sz.x) && is_pow_of2(sz.y))
      {
        float area0 = float(sz.x) * sz.y, scl = float(scrh) / ref_hres;
        sz.x = get_bigger_pow2(sz.x * scrh / ref_hres);
        sz.y = get_bigger_pow2(sz.y * scrh / ref_hres);
        if (float(sz.x) * sz.y >= area0 * 1.9f * scl * scl)
          sz.y /= 2;
      }
      else
      {
        sz.x = (sz.x * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
        sz.y = (sz.y * scrh / ref_hres + SZ_ALIGN - 1) / SZ_ALIGN * SZ_ALIGN;
      }

      debug("inscriptionsAtlas: uses dynamic sz=%d,%d due to refHRes:i=%d and screen size %dx%d", sz.x, sz.y, ref_hres, scrw, scrh);
    }
  }

  DynamicAtlasTex::init(sz, b.getInt("inscriptionsCountReserve", 1024), -2, "inscriptionsAtlas", TEXFMT_L8 | TEXCF_UPDATE_DESTINATION);

  if (!inscr_atlas.isInited())
    return;
  int qpf_max = b.getInt("inscriptionsQuadPerFrameMax", 4096);

  new (&ctx(), _NEW_INPLACE) StdGuiRender::GuiContext();
  if (blurShader.init("gui_blur_gui", false))
  {
    texBlurred.set(d3d::create_tex(NULL, texSz.x / 4, texSz.y / 4, TEXFMT_L8 | TEXCF_RTARGET, 1, "inscriptionsAtlasBlurred"),
      "$inscriptionsAtlasBlurred");
    texBlurredSampler = d3d::request_sampler({});
    ctx().createBuffer(0, &blurShader, qpf_max, 0, "inscr.buf.blur");
    ctx().setTarget(texSz.x / 4, texSz.y / 4);
  }
  else
  {
    texBlurred.close(); // or we can use gui_default instead
    texBlurredSampler = d3d::INVALID_SAMPLER_HANDLE;
    debug("no gui_blur_gui shader");
  }
  debug("gui: inited inscriptions support, cache_tex=%dx%d, reserve=%d, max.quad/frame=%d", texSz.x, texSz.y, itemUu.capacity(),
    qpf_max);
}
void DagorFontBinDump::InscriptionsAtlas::term()
{
  if (inscr_inited())
  {
    debug("gui: max %d inscriptions", itemData.size());
    ctx().close();
    ctx().~GuiContext();
    blurShader.close();
    texBlurred.close();
    texBlurredSampler = d3d::INVALID_SAMPLER_HANDLE;
  }
  DynamicAtlasTex::term();
}

void DagorFontBinDump::after_device_reset()
{
  DagorFontBinDump::inscr_atlas.discardAllItems();
  gen_sys_tex.afterDeviceReset();
}

void DagorFontBinDump::resetDynFontsCache()
{
  if (!dynFontTex.size())
    return;

  int tex_cnt = dynFontTex.size(), tex_sz = dynFontTex[0].texSz();
  ;
  dynFontTex.clear();
  ShaderGlobal::reset_textures(true);

  debug("reiniting dynFonts for %d tex %dx%d", tex_cnt, tex_sz, tex_sz);
  dynFontTex.resize(tex_cnt);
  for (int i = 0; i < dynFontTex.size(); i++)
  {
    clear_and_resize(dynFontTex[i].hist, tex_sz);
    mem_set_0(dynFontTex[i].hist);
  }
}

int DagorFontBinDump::tryPlaceGlyph(int wd, int ht, int &x0, int &y0)
{
  for (int i = 0; i < dynFontTex.size(); i++)
    if (dynFontTex[i].tryPlaceGlyph(wd, ht, x0, y0))
      return i;
  return -1;
}

void DagorFontBinDump::dumpDynFontUsage()
{
  for (int i = 0; i < dynFontTex.size(); i++)
    if (dynFontTex[i].tex)
      debug("dynFontAtlas[%d] %dx%d, %d%% used", i, dynFontTex[i].hist.size(), dynFontTex[i].hist.size(),
        dynFontTex[i].calcUsagePercent());
}

bool DagorFontBinDump::tryOpenFontFile(FullFileLoadCB &reader, const char *fname_prefix, int scr_h, bool &dynamic_font)
{
  String fname(0, "%s.%d.bin", fname_prefix, scr_h);
  const int mode = DF_READ | DF_IGNORE_MISSING;
  dynamic_font = true;
  if (reader.open(fname, mode))
    return true;
  else
  {
    static const int stdH[] = {480, 576, 600, 720, 768, 864, 900, 960, 1024, 1050, 1080, 1200, 1440, 1800, 2160};
    int n = 0;
    for (; n < countof(stdH) - 1; n++)
      if (stdH[n] >= scr_h)
        break;

    if (n >= countof(stdH))
      n = countof(stdH) - 1;
    else if (n > 0 && (scr_h - stdH[n - 1]) < (stdH[n] - scr_h))
      n = n - 1;

    if (stdH[n] != scr_h)
      fname.printf(0, "%s.%d.bin", fname_prefix, stdH[n]);

    if (reader.open(fname, mode))
      return true;
    else
      for (; n >= 0; n--)
      {
        fname.printf(0, "%s.%d.bin", fname_prefix, stdH[n]);
        if (reader.open(fname, mode))
          return true;
      }

    fname.printf(0, "%s.bin", fname_prefix);
    if (reader.open(fname, mode))
    {
      dynamic_font = false;
      return true;
    }
  }
  return false;
}
#if _TARGET_PC_MACOSX
static const char *sysFolders[] = {"/System/Library/Fonts", "/System/Library/Fonts/Supplemental"};
#elif _TARGET_PC_LINUX
static const char *sysFolders[] = {"/usr/share/fonts/truetype", "/usr/share/fonts"};
#endif

bool is_system_font_exists(const char *name)
{
#if _TARGET_PC_WIN
  String fn(name);
  char winFolder[MAX_PATH];
  if (::GetWindowsDirectory(winFolder, sizeof(winFolder)))
  {
    String windowsFonts(0, "%s/Fonts", winFolder);
    fn.replace("<system>", windowsFonts.c_str());
  }
  return dd_file_exists(fn.c_str());
#elif _TARGET_PC_MACOSX | _TARGET_PC_LINUX
  for (const char *sysFolder : sysFolders)
  {
    String fn(name);
    fn.replace("<system>", sysFolder);
    if (dd_file_exists(fn.c_str()))
      return true;
  }
#else
  G_UNREFERENCED(name);
#endif
  return false;
}

void DagorFontBinDump::FontData::loadBaseFontData(const DataBlock &desc)
{
  Tab<uint16_t> tmp_r, tmp_fi;
  Tab<float> tmp_fs;

  ttfNid = ttfNames.addNameId(desc.getStr("baseFont"));
  forceAutoHint = desc.getBool("forceAutoHint", true);
  addFontRange[0] = addFontRange[1] = 0;
  addFontMask = 0;

  for (int i = 0, add_idx = 0, nid_addFont = desc.getNameId("addFont"); i < desc.blockCount(); i++)
    if (desc.getBlock(i)->getBlockNameId() == nid_addFont)
    {
      const DataBlock &b2 = *desc.getBlock(i);
      const char *add_font_name = b2.getStr("addFont");
      if ((strncmp(add_font_name, "<system>", 8) == 0) && !is_system_font_exists(add_font_name))
      {
        bool found = false;
        for (int j = 0, nid_sysFont = b2.getNameId("sysFont"); j < b2.paramCount(); ++j)
        {
          if (b2.getParamType(j) == DataBlock::TYPE_STRING && b2.getParamNameId(j) == nid_sysFont)
          {
            const char *sysFontFile = b2.getStr(j);
            found = is_system_font_exists(sysFontFile);
            if (found)
            {
              add_font_name = sysFontFile;
              break;
            }
          }
        }
        if (!found)
          continue;
      }

      float add_font_ht_scale = b2.getReal("htScale", 1.0f);
      int add_font_idx = -1;

      for (auto &f : add_font)
        if (strcmp(f.name, add_font_name) == 0)
        {
          add_font_idx = &f - add_font.data();
          break;
        }
      if (add_font_idx < 0)
      {
        add_font_idx = add_font.size();
        DagorFontBinDump &bin = add_font.push_back();
        bin.init();
        bin.dynamic = true;
        bin.classicFont = 0xFF;

        bin.fontName = add_font_name;
        bin.name.setPtr(bin.fontName);
        bin.dfont.ttfNid = ttfNames.addNameId(add_font_name);
        bin.dfont.forceAutoHint = b2.getBool("forceAutoHint", true);
        bin.dfont.pixHt = 0;
        bin.dfont.initialPixHt = 0;
        G_ASSERTF((add_font_idx & WCRFI_FONT_MASK) == add_font_idx, "add_font_idx=%d", add_font_idx);
      }
      G_ASSERTF((add_idx << WCRFI_ADDIDX_SHR) < 0x10000, "add_idx=%d", add_idx);
      add_font_idx |= (add_idx << WCRFI_ADDIDX_SHR);

      tmp_r.reserve(b2.paramCount() * 2 - 2);
      tmp_fi.reserve(b2.paramCount() - 1);
      tmp_fs.reserve(b2.paramCount() - 1);
      for (int j = 0, nid_range = desc.getNameId("range"); j < b2.paramCount(); j++)
        if (b2.getParamNameId(j) == nid_range && b2.getParamType(j) == DataBlock::TYPE_IPOINT2)
        {
          int r1 = b2.getIPoint2(j).y, ins_at = -1;
          for (int k = 1; k < tmp_r.size(); k += 2)
            if (tmp_r[k] > r1)
            {
              ins_at = k - 1;
              break;
            }
          if (ins_at < 0)
          {
            tmp_r.push_back(b2.getIPoint2(j).x);
            tmp_r.push_back(r1);
            tmp_fi.push_back(add_font_idx);
            tmp_fs.push_back(add_font_ht_scale);
          }
          else
          {
            insert_item_at(tmp_r, ins_at, b2.getIPoint2(j).x);
            insert_item_at(tmp_r, ins_at + 1, r1);
            insert_item_at(tmp_fi, ins_at / 2, add_font_idx);
            insert_item_at(tmp_fs, ins_at / 2, add_font_ht_scale);
          }
        }
      add_idx++;
    }

  G_ASSERT(tmp_r.size() == tmp_fi.size() * 2);
  G_ASSERT(tmp_r.size() == tmp_fs.size() * 2);
  G_ASSERT(wcRangePair.size() == wcRangeFontIdx.size() * 2);
  G_ASSERT(wcRangePair.size() == wcRangeFontScl.size() * 2);
  for (int k = 0; k + tmp_r.size() <= wcRangePair.size(); k += 2)
    if (mem_eq(tmp_r, wcRangePair.data() + k) && mem_eq(tmp_fi, wcRangeFontIdx.data() + k / 2) &&
        mem_eq(tmp_fs, wcRangeFontScl.data() + k / 2))
    {
      addFontRange[0] = k;
      addFontRange[1] = k + tmp_r.size();
      break;
    }
  if (tmp_r.size() && !addFontRange[1])
  {
    addFontRange[0] = append_items(wcRangePair, tmp_r.size(), tmp_r.data());
    addFontRange[1] = wcRangePair.size();
    append_items(wcRangeFontIdx, tmp_fi.size(), tmp_fi.data());
    append_items(wcRangeFontScl, tmp_fs.size(), tmp_fs.data());
  }

  for (int i = addFontRange[0]; i < addFontRange[1]; i += 2)
  {
    static const uint64_t ff = ~uint64_t(0);
    addFontMask |= (ff << (wcRangePair[i + 0] / 0x400)) & (ff >> (63 - wcRangePair[i + 1] / 0x400));
    // debug("range[%2d]=%04X..%04X -> %16llx  fontIdx=%4X  scale=%.1f", i, wcRangePair[i+0], wcRangePair[i+1],
    //   (ff << (wcRangePair[i+0]/0x400)) & (ff >> (63-wcRangePair[i+1]/0x400)), wcRangeFontIdx[i/2], wcRangeFontScl[i/2]);
  }
}
void DagorFontBinDump::loadDynFonts(FontArray &fonts, const DataBlock &desc, int scr_ht)
{
  G_STATIC_ASSERT(sizeof(GlyphPlace) == 12);

  Tab<const DataBlock *> bfBlk;
  bfBlk.reserve(desc.blockCount());
  for (int i = 0, nid = desc.getNameId("baseFontData"); i < desc.blockCount(); i++)
    if (desc.getBlock(i)->getBlockNameId() == nid)
      bfBlk.push_back(desc.getBlock(i));

  Tab<int> hres;
  for (int i = 0; i < desc.blockCount(); i++)
    if (strncmp(desc.getBlock(i)->getBlockName(), "hRes.", 5) == 0)
    {
      hres.push_back(atoi(desc.getBlock(i)->getBlockName() + 5));
      G_ASSERTF(hres.size() < 2 || (hres[hres.size() - 2] < hres.back()), "block %s, prev hres=%d", desc.getBlock(i)->getBlockName(),
        hres.back());
    }
  int n_hres = hres.size() - 1;
  for (int i = 0; i < hres.size(); i++)
    if (hres[i] >= scr_ht)
    {
      n_hres = i;
      break;
    }
  if (n_hres > 0 && (scr_ht - hres[n_hres - 1]) < (hres[n_hres] - scr_ht))
    n_hres = n_hres - 1;
  const DataBlock *hresBlk = n_hres < 0 ? nullptr : desc.getBlockByName(String(0, "hRes.%d", hres[n_hres]));

  int prev_fonts_count = fonts.size();
  int prev_add_fonts_count = add_font.size();
  for (int i = 0, nid = desc.getNameId("font"); i < desc.blockCount(); i++)
    if (desc.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &b2 = *desc.getBlock(i);
      DagorFontBinDump &bin = fonts.push_back();
      bin.init();
      bin.dynamic = false;
      bin.classicFont = 0xFF;
      bin.fileName = desc.resolveFilename();

      bin.fontName = b2.getStr("name");
      bin.name.setPtr(bin.fontName);
      bin.dfont.pixHt = b2.getInt("ht", -1);
      if (bin.dfont.pixHt < 0 && hresBlk)
      {
        bin.dfont.pixHt = hresBlk->getBlockByNameEx(bin.fontName)->getInt("ht", -1);
        bin.dynamic = true;
      }
      if (bin.dfont.pixHt < 0 && b2.paramExists("htRel"))
      {
        if (b2.getBool("htEven", false))
          bin.dfont.pixHt = int(floorf(b2.getReal("htRel") * scr_ht / 100.0f / 2.0f + 0.5f)) * 2;
        else
          bin.dfont.pixHt = int(floorf(b2.getReal("htRel") * scr_ht / 100.0f + 0.5f));
        bin.dynamic = true;
      }
      bin.dfont.initialPixHt = (unsigned)bin.dfont.pixHt;

      bin.dfont.loadBaseFontData(*bfBlk[b2.getInt("baseFontData")]);
    }

  if (ttfRec.size() < ttfNames.nameCount())
    ttfRec.resize(ttfNames.nameCount());
  for (DagorFreeTypeFontRec &frec : ttfRec)
    if (frec.curHt < 0)
    {
      int ttfNid = &frec - ttfRec.data();
      String fn;
#if _TARGET_PC_WIN
      const char *fname = ttfNames.getName(ttfNid);
      if (strncmp(fname, "<system>", 8) == 0)
      {
        char winFolder[MAX_PATH];
        if (::GetWindowsDirectory(winFolder, sizeof(winFolder)))
        {
          fn.setStr(fname, strlen(fname));
          String windowsFonts(0, "%s/Fonts", winFolder);
          fn.replace("<system>", windowsFonts.c_str());
        }
        else
          continue;
      }
      else
#elif _TARGET_PC_MACOSX | _TARGET_PC_LINUX
      const char *fname = ttfNames.getName(ttfNid);
      if (strncmp(fname, "<system>", 8) == 0)
      {
        for (const char *sysFolder : sysFolders)
        {
          fn.setStr(fname, strlen(fname));
          fn.replace("<system>", sysFolder);
          if (dd_file_exists(fn.c_str()))
            break;
        }
      }
      else
#endif
        fn = String(0, "%s/%s", ttf_path_prefix, ttfNames.getName(ttfNid));

      debug("load font: %d %s", ttfNid, ttfNames.getName(ttfNid));
      if (!frec.openTTF(fn, true))
      {
        if (dag_on_assets_fatal_cb)
          dag_on_assets_fatal_cb(fn.c_str());
        G_ASSERTF(0, "failed to load font '%s'", fn);
      }
    }

  for (int i = prev_fonts_count; i < fonts.size(); i++)
  {
    DagorFontBinDump &bin = fonts[i];
    bin.appendFontHt(bin.dfont.pixHt);
    bin.fontHt = bin.dfont.fontHt[0];
    bin.fontCapsHt = bin.dfont.fontMx[0].capsHt;
    bin.ascent = bin.dfont.fontMx[0].ascent;
    bin.descent = bin.dfont.fontMx[0].descent;
    bin.lineSpacing = bin.dfont.fontMx[0].lineSpacing;
    // debug("%d: ht=%d ascent=%d descent=%d linesp=%d capsHt=%d spaceWd=%d baseFontData[%d].dynGrp.count=%d",
    //   bin.name.get(), bin.fontHt, bin.ascent, bin.descent, bin.lineSpacing, bin.fontCapsHt, bin.dfont.fontMx[0].spaceWd,
    //   bin.dfont.bfDataIdx, baseFontData[bin.dfont.bfDataIdx].dynGrp.size());
  }

  // prepare reference metrics (pixHt=32) for additional fonts
  for (int i = prev_add_fonts_count; i < add_font.size(); i++)
  {
    DagorFontBinDump &bin = add_font[i];
    bin.appendFontHt(bin.dfont.initialPixHt = bin.dfont.pixHt = 32);
    bin.fontHt = bin.dfont.fontHt[0];
    bin.fontCapsHt = bin.dfont.fontMx[0].capsHt;
    bin.ascent = bin.dfont.fontMx[0].ascent;
    bin.descent = bin.dfont.fontMx[0].descent;
    bin.lineSpacing = bin.dfont.fontMx[0].lineSpacing;

    // clear font height array
    bin.dfont.fontGlyphs.clear();
    bin.dfont.fontHt.clear();
    bin.dfont.fontMx.clear();
  }
  strboxcache::priCache.bb.reserve(strboxcache::DEF_CACHE_SIZE - strboxcache::priCache.bb.size());
  DEBUG_DUMP_VAR(strboxcache::priCache.bb.capacity());
#if CHECK_HASH_COLLISIONS
  strboxcache::hcc_hash2.reserve(strboxcache::DEF_CACHE_SIZE - strboxcache::hcc_hash2.size());
#endif
}
void DagorFontBinDump::loadFonts(FontArray &fonts, const char *fname_prefix, int scr_h)
{
  FullFileLoadCB crd(NULL, 0);
  bool dynamicFont = false;
  if (!tryOpenFontFile(crd, fname_prefix, scr_h, dynamicFont))
  {
    if (::dag_on_file_not_found)
    {
      debug("cannot find font file for: <%s>, h=%d", fname_prefix, scr_h);
      ::dag_on_file_not_found(fname_prefix);
    }
    else
      G_ASSERTF(0, "cannot find font file for: <%s>, h=%d", fname_prefix, scr_h);
    return;
  }
  G_ASSERT(crd.fileHandle != NULL);
  int fsize = df_length(crd.fileHandle);
  if (fsize < 8)
  {
    DAG_FATAL("bad font file size %d: <%s>, h=%d", fsize, fname_prefix, scr_h);
    return;
  }
  debug("load fonts from '%s'", crd.getTargetName());
  loadFontsStream(crd, fonts, fname_prefix, dynamicFont);
}

void DagorFontBinDump::loadFontsStream(IGenLoad &crd, FontArray &fonts, const char *fname_prefix, bool dynamic_font)
{
  DAGOR_TRY
  {
    int total_dump_sz = 0;
    int ver = crd.readInt();
    if (ver != _MAKE4C('DFB\5') && ver != _MAKE4C('DFB\6') && ver != _MAKE4C('DFz\7'))
      return;

    int first_index = fonts.size();
    int fonts_count = crd.readInt();
    int dynGrpCount = 0;
    if (ver == _MAKE4C('DFz\7'))
      dynGrpCount = crd.readInt();

    for (int font_index = 0; font_index < fonts_count; font_index++)
    {
      DagorFontBinDump &bin = fonts.push_back();
      bin.init();
      bin.dynamic = dynamic_font;
      bin.fileName = crd.getTargetName();
      bin.fileOfs = crd.tell();
      bin.fileVer = ver;

      total_dump_sz += bin.loadDumpData(crd);
      debug("  font #%02d  (pairKernData=%4d)  \"%s\" capsHt=%d", fonts.size() - 1, bin.pairKernData.size(), bin.name.get(),
        bin.fontCapsHt);
      G_ASSERTF(dynGrpCount < 127, "dynGrpCount=%d", dynGrpCount);
    }

    // load and create textures
    SmallTab<uint8_t, TmpmemAlloc> tmp;

    crd.beginBlock();
    fonts[first_index].texOwner = true;
    for (int i = 0; i < fonts[first_index].tex.size(); i++)
    {
      crd.beginBlock();
      Texture *tex;
      tex = (Texture *)d3d::create_ddsx_tex(crd, 0, 0, 0, fname_prefix);
      for (int font_index = first_index; font_index < fonts.size(); font_index++)
      {
        DagorFontBinDump &bin = fonts[font_index];
        bin.tex[i].tex = tex;
        d3d_err(bin.tex[i].tex);
      }

      String tex_name(0, "%s@%i", fname_prefix, i);
      if (get_managed_texture_id(tex_name) != BAD_TEXTUREID)
        tex_name.aprintf(0, "@%d", first_index);
      TEXTUREID texId = register_managed_tex(tex_name, fonts[first_index].tex[i].tex);

      for (int font_index = first_index; font_index < fonts.size(); font_index++)
      {
        DagorFontBinDump &bin = fonts[font_index];
        bin.tex[i].texId = texId;
      }
      crd.endBlock();
    }
    crd.endBlock();

    if (ver == _MAKE4C('DFB\6') || (ver == _MAKE4C('DFz\7') && dynGrpCount))
    {
      DagorFontBinDump &bin = fonts[first_index];
      String nm;

      crd.beginBlock();
      if (ver == _MAKE4C('DFB\6'))
        dynGrpCount = crd.readIntP<2>();
      G_ASSERTF(dynGrpCount < 127, "dynGrpCount=%d", dynGrpCount);
      clear_and_resize(bin.dynGrp, dynGrpCount);
      for (int i = 0; i < bin.dynGrp.size(); i++)
      {
        crd.readString(nm);
        bin.dynGrp[i].ttfNid = ttfNames.addNameId(nm);
        bin.dynGrp[i].pixHt = crd.readIntP<2>();
        bin.dynGrp[i].flags = crd.readIntP<1>();
        bin.dynGrp[i].vaScale = crd.readIntP<1>();
      }
      crd.endBlock();

      for (int i = first_index + 1; i < fonts.size(); i++)
        fonts[i].dynGrp = bin.dynGrp;

      for (int i = first_index; i < fonts.size(); i++)
      {
        int tnum = fonts[i].tex.size() + dynFontTex.size();
        fonts[i].tex.init(memalloc(sizeof(Tex) * tnum, midmem), tnum);
        mem_set_0(fonts[i].tex);
        mem_copy_to(fonts[i].texOrig, fonts[i].tex.data());
        for (int j = fonts[i].texOrig.size(); j < fonts[i].tex.size(); j++)
          fonts[i].tex[j].texId = BAD_TEXTUREID;
      }
    }
    debug("  loaded dynGrpCount=%d for %s (total dump(s) size %dK)", dynGrpCount, crd.getTargetName(), total_dump_sz >> 10);
    if (total_dump_sz > (128 << 10)) // discard only really large dumps
      for (int i = first_index; i < fonts.size(); i++)
        fonts[i].discard();
  }
  DAGOR_CATCH(const IGenLoad::LoadException &) { DAG_FATAL("Error reading fonts file '%s'", fname_prefix); }
}


void DagorFontBinDump::clear()
{
  del_it(wcharToBuild);
  del_it(ttfNidToBuild);
  clear_and_shrink(dynGrp);
  fontName = NULL;
  fileName = NULL;
  if (isFullyDynamicFont())
  {
    for (auto &u256p : dfont.fontGlyphs)
      clear_all_ptr_items(u256p);
    clear_and_shrink(dfont.fontGlyphs);
    clear_and_shrink(dfont.fontHt);
    clear_and_shrink(dfont.fontMx);
  }

  if (selfData)
  {
    if (texOwner)
      for (int i = 0; i < texOrig.size(); i++)
        if (texOrig[i].tex)
        {
          if (get_managed_texture_name(texOrig[i].texId))
          {
            debug("%p.del tex[%d]: %p, %d (%d)", this, i, texOrig[i].tex.get(), texOrig[i].texId,
              get_managed_texture_refcount(texOrig[i].texId));
            ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texOrig[i].texId, texOrig[i].tex);
          }
        }
    if (tex.data() != texOrig.data())
      memfree(tex.data(), midmem);
    inimem->free(selfData);
  }
  else if (texCopy.size() && texOwner)
    for (int i = 0; i < texCopy.size(); i++)
      if (texCopy[i].tex)
      {
        if (get_managed_texture_name(texCopy[i].texId))
        {
          debug("%p.del tex[%d]: %p, %d (%d)", this, i, texCopy[i].tex.get(), texCopy[i].texId,
            get_managed_texture_refcount(texCopy[i].texId));
          ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(texCopy[i].texId, texCopy[i].tex);
        }
      }
  clear_and_shrink(texCopy);
  init();
}

int DagorFontBinDump::loadDumpData(IGenLoad &crd)
{
  void *dump = NULL;
  int dumpsize = 0;

  fontCapsHt = 1;
  if (fileVer == _MAKE4C('DFz\7'))
  {
    crd.beginBlock();
    LzmaLoadCB zcrd(crd, crd.getBlockRest());
    dumpsize = zcrd.readInt();
    dump = inimem->alloc(dumpsize);
    if (zcrd.tryRead(dump, dumpsize) != dumpsize)
    {
      inimem->free(dump);
      crd.endBlock();
      return 0;
    }
    crd.endBlock();
  }
  else
  {
    dumpsize = crd.readInt();
    dump = inimem->alloc(dumpsize);
    if (crd.tryRead(dump, dumpsize) != dumpsize)
    {
      inimem->free(dump);
      return 0;
    }
  }

  // copy and patch root data
  memcpy(this, dump, offsetof(DagorFontBinDump, dfont));
  selfData = dump;
  glyph.patch(dump);
  tex.patch(dump);
  name.patch(dump);
  if (!_resv)
  {
    glyphPairKernIdx.init(NULL, 0);
    pairKernData.init(NULL, 0);
    for (int i = 0; i < glyph.size(); i++)
      const_cast<GlyphData &>(glyph[i]).hasPk = 0;
  }
  glyphPairKernIdx.patch(dump);
  pairKernData.patch(dump);

  // patch dynGrp for non-printable glyphs
  for (int i = 0; i < glyph.size(); i++)
    if (glyph[i].x0 == glyph[i].x1 && glyph[i].y0 == glyph[i].y1)
      const_cast<GlyphData &>(glyph[i]).dynGrpIdx = 0x7F;

  // patch unicode table
  uTable.patch(dump);
  for (int i = 0; i < uTable.size(); i++)
  {
    UnicodeTable &t = const_cast<UnicodeTable &>(uTable[i]);
    t.glyphIdx.patch(dump);

    uint16_t *idx = t.glyphIdx;
    for (int si = t.count; si > 0; idx++, si--)
      if (*idx == 0xFFFF)
        *idx = 0;
  }

  // fix extensions of space character
  GlyphData &g = const_cast<GlyphData &>(getGlyphU(' '));
  g.x0 = -xBase;
  g.x1 = g.x0 + g.dx;
  g.y0 = g.y1 = -yBase;

  const GlyphData &gA = getGlyphU('A');
  fontCapsHt = gA.y1 - gA.y0;
  if (fontCapsHt <= 0)
    fontCapsHt = fontHt;

  texOrig.init(tex.data(), tex.size());
  return dumpsize;
}

void DagorFontBinDump::discard()
{
  if (!selfData)
    return; // already discarded
  fontName = name;

  dag::set_allocator(texCopy, inimem);
  texCopy = tex;
  for (int j = texOrig.size(); j < texCopy.size(); j++)
    texCopy[j].texId = BAD_TEXTUREID, texCopy[j].tex = NULL;

  if (tex.data() != texOrig.data())
    memfree(tex.data(), midmem);
  texOrig.init(NULL, 0);
  inimem->free(selfData);

  int32_t s_fontHt = fontHt, s_ascent = ascent, s_descent = descent, s_lineSpacing = lineSpacing;
  memset(this, 0, offsetof(DagorFontBinDump, dfont));
  fontHt = s_fontHt, ascent = s_ascent, descent = s_descent, lineSpacing = s_lineSpacing;

  del_it(wcharToBuild);
  del_it(ttfNidToBuild);
  name.setPtr(fontName);

  debug("discarded font %s (%s,0x%08X)", fontName, fileName, fileOfs);
}

void DagorFontBinDump::doFetch()
{
  if (selfData || isFullyDynamicFont())
    return; // already fetched

  int64_t reft = profile_ref_ticks();
  G_UNUSED(reft);
  FullFileLoadCB crd(fileName);
  crd.seekto(fileOfs);
  int dump_sz = loadDumpData(crd);
  G_UNUSED(dump_sz);

  if (texCopy.size() == tex.size())
    mem_copy_to(texCopy, tex.data());
  else
  {
    tex.init(memalloc(data_size(texCopy), midmem), texCopy.size());
    mem_copy_from(tex, texCopy.data());
    mem_copy_from(texOrig, texCopy.data());
    for (int j = texOrig.size(); j < tex.size(); j++)
      tex[j].texId = BAD_TEXTUREID;
  }
  clear_and_shrink(texCopy);
  fontName = NULL;
  debug("fetched font %s (%s,0x%08X: memSz=%dK) for %.1f msec", name.get(), fileName, fileOfs, dump_sz >> 10,
    profile_time_usec(reft) / 1000.0f);
}

strboxcache::StrBboxCache strboxcache::priCache, strboxcache::secCache;
OSSpinlock strboxcache::cc;
uint32_t strboxcache::resetCnt = 0, strboxcache::strVisBoxCnt = 0, strboxcache::resetFrameNo = 0;
#if STRBOXCACHE_DEBUG
uint64_t strboxcache::testCnt = 0, strboxcache::hitCnt = 0, strboxcache::secHitCnt = 0;
uint64_t strboxcache::cacheTimeTicks = 0, strboxcache::calcTimeTicks = 0, strboxcache::calcCount = 0;
#endif
#if CHECK_HASH_COLLISIONS
Tab<uint32_t> strboxcache::hcc_hash2;
#endif
