//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <GuillotineBinPack.h>
#include <SkylineBinPack.h>
#include <EASTL/hash_map.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_query.h>
#include <3d/dag_textureIDHolder.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

template <bool persistent>
class DynamicAtlasTexTemplate
{
public:
  typedef eastl::hash_map<unsigned, int> HashToIdx;
  struct ItemData
  {
    enum
    {
      ST_restoring = 0x7FFEu,
      ST_discarded = 0x7FFFu
    };
    float u0, v0, u1, v1;
    uint16_t w, h, x0, y0;
    int16_t xBaseOfs, yBaseOfs;
    int16_t dx;
    uint16_t allocW;
    uint32_t lruFrame, hash;

    bool valid() const { return hash != 0 && (!persistent || dx < ST_restoring); }
    void discard() { dx = ST_discarded; }
    bool scheduleRestoring()
    {
      if (dx == ST_discarded)
      {
        dx = ST_restoring;
        return true;
      }
      return false;
    }
  };

  void init(const IPoint2 &sz, int resv, int _margin, const char *tex_name, int tex_fmt = TEXFMT_R8 | TEXCF_UPDATE_DESTINATION)
  {
    texSz = sz;
    if (_margin >= 0)
      margin = _margin, marginLtOfs = 0;
    else
      margin = -_margin * 2, marginLtOfs = -_margin;
    if (texSz.x && texSz.y)
    {
      invW = 1.0f / texSz.x;
      invH = 1.0f / texSz.y;
      binPack.Init(texSz.x, texSz.y);
      itemData.reserve(resv);
      int pure_fmt = tex_fmt & TEXFMT_MASK;
      if (pure_fmt == TEXFMT_DXT1 || pure_fmt == TEXFMT_DXT5 || pure_fmt == TEXFMT_BC7)
      {
        cornerResv = 4;
        marginLtOfs = 0;
        if (margin != 0 && margin != 4)
        {
          logerr("bad margin=%d for block compression, changing to 0", margin);
          margin = 0;
        }
      }
      else
        cornerResv = 2;

      if (!persistent)
        itemUu.reserve(resv);
      else
        binPack.Insert(cornerResv, cornerResv, true, binPack.RectBestShortSideFit, binPack.SplitMinimizeArea, false);

      if (tex_name)
      {
        tex.first.set(d3d::create_tex(NULL, texSz.x, texSz.y, tex_fmt, 1, tex_name), String(0, "$%s", tex_name));
        if (tex.first.getTex2D())
        {
          tex.first.getTex2D()->disableSampler();
          d3d::SamplerInfo smpInfo;
          smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
          tex.second = d3d::request_sampler(smpInfo);
          if (tex_fmt & TEXCF_RTARGET)
            d3d::resource_barrier({tex.first.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
        }
        else
          logerr("failed to allocate dynamic atlas texture, name='%s'", tex_name);
      }
    }
  }
  void term()
  {
    if (isInited())
    {
      tex.first.close();
      tex.second = d3d::INVALID_SAMPLER_HANDLE;
      hash2idx.clear();
      clear_and_shrink(itemData);
      clear_and_shrink(itemUu);
      binPack.Init(0, 0);
    }
  }

  bool isInited() { return tex.first.getId() != BAD_TEXTUREID; }
  const IPoint2 &getSz() const { return texSz; }
  int getMargin() const { return margin; }
  int getMarginLtOfs() const { return marginLtOfs; }
  int getCornerResvSz() const { return cornerResv; }
  float getMarginDu() const { return margin * invW; }
  float getMarginDv() const { return margin * invH; }
  float getInvW() const { return invW; }
  float getInvH() const { return invH; }
  int getItemsCount() const { return itemData.size(); }
  int getItemsUsed() const { return itemData.size() - itemUu.size(); }

  const ItemData *findItem(unsigned hash, bool update_lru = true)
  {
    HashToIdx::iterator it = hash2idx.find(hash);
    if (it == hash2idx.end())
      return NULL;
    int idx = it->second;
    if (update_lru && itemData[idx].lruFrame != dagor_frame_no())
      itemData[idx].lruFrame = dagor_frame_no();
    return &itemData[idx];
  }
  const ItemData *findItemIndexed(int idx, bool update_lru = true)
  {
    if (idx < 0 || idx >= itemData.size())
      return NULL;
    if (itemData[idx].hash == 0)
      return NULL;
    if (update_lru && itemData[idx].lruFrame != dagor_frame_no())
      itemData[idx].lruFrame = dagor_frame_no();
    return &itemData[idx];
  }

  const ItemData *addItem(int bmin_x, int bmin_y, int bmax_x, int bmax_y, unsigned hash, int idx = -1, bool allow_discard = true)
  {
    int need_w = (bmax_x - bmin_x) + margin, need_h = (bmax_y - bmin_y) + margin;
    if (cornerResv == 4 && ((need_w % 4) || (need_h % 4)))
      return NULL;
    rbp::Rect r = binPack.Insert(need_w, need_h, true, binPack.RectBestShortSideFit, binPack.SplitMinimizeArea, false);

    if (r.width <= 0 || r.height <= 0)
    {
      if (!persistent && !allow_discard)
        return NULL;

      rbp::Rect r_old;
      for (int i = 0; i < itemData.size(); i++)
        if (i != idx && itemData[i].valid() && itemData[i].lruFrame + 3 < dagor_frame_no())
        {
          discardItem(i, r_old);
          if (r_old.width >= need_w && r_old.height >= need_h)
            break;
        }

      r = binPack.Insert(need_w, need_h, true, binPack.RectBestShortSideFit, binPack.SplitMinimizeArea, false);
      if ((r.width <= 0 || r.height <= 0) && persistent) // try to defragment
        reArrangeAndAlloc(r, need_w, need_h);
      if (r.width <= 0 || r.height <= 0)
        return NULL;
    }

    if (idx != -1)
    {
      if (idx < 0 || idx >= itemData.size())
        return NULL;
      if (itemData[idx].valid() && (itemData[idx].hash || itemData[idx].w || itemData[idx].h))
        return NULL;
    }
    else if (itemUu.size())
    {
      idx = itemUu.back();
      itemUu.pop_back();
    }
    else
      idx = append_items(itemData, 1);

    hash2idx.insert(hash).first->second = idx;
    ItemData &g = itemData[idx];

    g.x0 = r.x + marginLtOfs;
    g.y0 = r.y + marginLtOfs;
    G_ASSERTF(cornerResv != 4 || (!(g.x0 % 4) && !(g.y0 % 4)), "rect=%d,%d, %dx%d", r.x, r.y, r.width, r.height);
    g.w = g.allocW = r.width - margin;
    g.h = r.height - margin;
    g.xBaseOfs = bmin_x;
    g.yBaseOfs = bmin_y;
    initTc(g, hash);
    return &g;
  }
  int getItemIdx(const ItemData *d) const
  {
    if (!d || d < itemData.data())
      return -1;
    int idx = d - itemData.data();
    return idx < itemData.size() ? idx : -1;
  }

  void discardItem(int i, rbp::Rect &r_old)
  {
    if (!itemData[i].valid())
      return;
    if (clear_discarded_cb)
      clear_discarded_cb(copy_left_top_margin_cb_arg, tex.first.getTex2D(), itemData[i], margin);
    r_old.x = itemData[i].x0 - marginLtOfs;
    r_old.y = itemData[i].y0 - marginLtOfs;
    r_old.width = itemData[i].allocW + margin;
    r_old.height = itemData[i].h + margin;
    binPack.freeUsedRect(r_old, true);
    if (!persistent)
    {
      hash2idx.erase(hash2idx.find(itemData[i].hash));
      itemData[i].hash = 0;
      itemUu.push_back(i);
    }
    else
      itemData[i].discard();
  }
  void discardAllItems()
  {
    if (!persistent)
    {
      itemData.clear();
      hash2idx.clear();
      clear_and_shrink(itemUu);
    }
    else
      for (int i = 0; i < itemData.size(); i++)
        itemData[i].discard();
    binPack.Init(texSz.x, texSz.y);
    binPack.Insert(cornerResv, cornerResv, true, binPack.RectBestShortSideFit, binPack.SplitMinimizeArea, false);
  }

  const ItemData *addDirectItemOnly(unsigned hash, int x0, int y0, int w, int h, bool sched_for_load = false)
  {
    int idx = -1;
    if (itemUu.size())
    {
      idx = itemUu.back();
      itemUu.pop_back();
    }
    else
      idx = append_items(itemData, 1);

    hash2idx.insert(hash).first->second = idx;
    ItemData &g = itemData[idx];

    g.x0 = x0;
    g.y0 = y0;
    g.w = w;
    g.h = h;

    g.allocW = 0;
    g.xBaseOfs = 0;
    g.yBaseOfs = 0;
    initTc(g, hash);
    if (persistent && sched_for_load)
    {
      g.discard();
      g.scheduleRestoring();
    }
    return &g;
  }

public:
  eastl::pair<TextureIDHolder, d3d::SamplerHandle> tex;
  void (*copy_left_top_margin_cb)(void *cb_arg, Texture *tex, const ItemData &d, int m) = NULL;
  void (*clear_discarded_cb)(void *cb_arg, Texture *tex, const ItemData &d, int m) = NULL;
  void *copy_left_top_margin_cb_arg = NULL;

protected:
  HashToIdx hash2idx;
  Tab<ItemData> itemData;
  Tab<int> itemUu;
  IPoint2 texSz = IPoint2(1, 1);
  float invW = 1, invH = 1;
  rbp::GuillotineBinPack binPack;
  int margin = 1;
  int marginLtOfs = 0;
  int cornerResv = 2;

  void initTc(ItemData &g, unsigned hash, bool update_lru = true)
  {
    g.u0 = g.x0 * invW;
    g.v0 = g.y0 * invH;
    g.u1 = (g.x0 + g.w) * invW;
    g.v1 = (g.y0 + g.h) * invH;
    if (update_lru)
      g.lruFrame = dagor_frame_no();
    g.hash = hash;
    g.dx = 0;
  }
  void reArrangeAndAlloc(rbp::Rect &r, int need_w, int need_h)
  {
    if (!persistent)
      return;

    unsigned used_area = need_w * need_h, rect_num = 1;
    for (int i = 0; i < itemData.size(); i++)
      if (itemData[i].valid() && itemData[i].lruFrame + 3 >= dagor_frame_no())
        used_area += (itemData[i].w + margin) * (itemData[i].h + margin), rect_num++;
    if (used_area > texSz.x * texSz.y * 9 / 10)
    {
      logwarn("%s: %d items will use %d%%, rearrange is meaningless", get_managed_texture_name(tex.first.getId()), rect_num,
        used_area * 100 / (texSz.x * texSz.y));
      return;
    }

    TextureInfo ti;
    Texture *t = tex.first.getTex2D();
    const char *name = get_managed_texture_name(tex.first.getId());
    if (!t->getinfo(ti))
      return;
    Texture *tmp_tex = d3d::create_tex(NULL, texSz.x, texSz.y, ti.cflg, 1, name);
    if (!tmp_tex)
    {
      logerr("%s: failed to alloc temp texture %dx%d to rearrange items", name, texSz.x, texSz.y);
      return;
    }

    eastl::vector<rbp::RectSizeRequest> src_rects;
    src_rects.reserve(rect_num);
    {
      rbp::RectSizeRequest req;
      req.width = need_w;
      req.height = need_h;
      req.id = -1;
      src_rects.push_back(req);
    }
    for (int i = 0; i < itemData.size(); i++)
    {
      ItemData &g = itemData[i];
      if (!g.valid())
        continue;
      g.discard();
      if (g.lruFrame + 3 < dagor_frame_no())
        continue;

      rbp::RectSizeRequest req;
      req.width = g.w + margin;
      req.height = g.h + margin;
      req.id = i;
      src_rects.push_back(req);
    }

    logwarn("%s: rearrange atlas (%d items will use %d%%)", name, src_rects.size(), used_area * 100 / (texSz.x * texSz.y));

    binPack.Init(texSz.x, texSz.y);
    binPack.Insert(cornerResv, cornerResv, true, binPack.RectBestShortSideFit, binPack.SplitMinimizeArea, false);
    bool upd_ok = tmp_tex->updateSubRegion(t, 0, 0, 0, 0, cornerResv, cornerResv, 1, 0, 0, 0, 0);
#if DAGOR_DBGLEVEL > 0
    if (!upd_ok)
      logerr("%s: failed to update corner quad %dx%d (tex %dx%d cflg=0x%X)", name, cornerResv, cornerResv, texSz.x, texSz.y, ti.cflg);
#endif

    eastl::vector<rbp::RectResult> new_rects;
    new_rects.reserve(src_rects.size());
    binPack.Insert(src_rects, new_rects, true, binPack.RectBestAreaFit, binPack.SplitMinimizeArea, false);
    for (int i = 0; i < new_rects.size(); i++)
    {
      if (new_rects[i].id == -1)
      {
        r = new_rects[i];
        continue;
      }
      ItemData &g = itemData[new_rects[i].id];
      upd_ok = tmp_tex->updateSubRegion(t, 0, g.x0 - marginLtOfs, g.y0 - marginLtOfs, 0, g.w + margin, g.h + margin, 1, 0,
        new_rects[i].x, new_rects[i].y, 0);
#if DAGOR_DBGLEVEL > 0
      if (!upd_ok)
        logerr("%s: failed to copy image[%d] (%d,%d) %dx%d to new place (%d,%d) (tex %dx%d cflg=0x%X, marginLtOfs=%d margin=%d)", name,
          i, g.x0 - marginLtOfs, g.y0 - marginLtOfs, g.w + margin, g.h + margin, new_rects[i].x, new_rects[i].y, texSz.x, texSz.y,
          ti.cflg, marginLtOfs, margin);
#endif
      g.x0 = new_rects[i].x + marginLtOfs;
      g.y0 = new_rects[i].y + marginLtOfs;
      g.w = g.allocW = new_rects[i].width - margin;
      g.h = new_rects[i].height - margin;
      initTc(g, g.hash, false);
      if (copy_left_top_margin_cb && !marginLtOfs && margin)
        copy_left_top_margin_cb(copy_left_top_margin_cb_arg, tmp_tex, g, margin);
    }
    logwarn("-> allocated %d rects, new_rect=%d,%d %dx%d", new_rects.size(), r.x, r.y, r.width, r.height);

    upd_ok = t->updateSubRegion(tmp_tex, 0, 0, 0, 0, texSz.x, texSz.y, 1, 0, 0, 0, 0);
#if DAGOR_DBGLEVEL > 0
    if (!upd_ok)
      logerr("%s: failed to swap/copy image (tex %dx%d cflg=0x%X, marginLtOfs=%d margin=%d)", name, texSz.x, texSz.y, ti.cflg,
        marginLtOfs, margin);
#endif
    if (ti.cflg & TEXCF_RTARGET)
      d3d::resource_barrier({t, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    del_d3dres(tmp_tex);
    G_UNUSED(upd_ok);
  }
};

typedef DynamicAtlasTexTemplate<false> DynamicAtlasTex;


class DynamicAtlasTexUpdater
{
public:
  static constexpr int MAX_FRAME_QUEUE = 3;
  struct SysTexRec
  {
    rbp::SkylineBinPack pack;
    unsigned frameNo = 0;
    Texture *tex = NULL;
  };
  struct CopyRec
  {
    uint16_t src_idx, x0, y0, w, h, dest_idx, dest_x0, dest_y0;
  };
  static constexpr int MAX_GEN_SYS_TEX_CNT = 3;

  void init(const IPoint2 &sz, int tex_fmt = TEXFMT_R8)
  {
    sysTexSz = sz;
    texFmt = tex_fmt;
    if (texFmt == TEXFMT_R8)
      bpp = 1;
    else if (texFmt == TEXFMT_A8R8G8B8)
      bpp = 4;
    else
      bpp = 1; //== unrecognized but safe
  }
  void term()
  {
    for (int i = 0; i < sysTexCount; i++)
    {
      del_d3dres(sysTex[i].tex);
      sysTex[i].pack.Init(1, 1, false);
    }
    sysTexCount = 0;
    curSysTex = -1;
  }
  void afterDeviceReset() { term(); }

  void alloc()
  {
    G_ASSERT(sysTexCount < MAX_GEN_SYS_TEX_CNT);
    curSysTex = sysTexCount++;
    SysTexRec &r = sysTex[curSysTex];
    r.tex = (Texture *)d3d::create_tex(NULL, sysTexSz.x, sysTexSz.y,
      texFmt | TEXCF_SYSMEM | TEXCF_DYNAMIC | TEXCF_READABLE | TEXCF_LINEAR_LAYOUT, 1, "upd-sysTex");
    r.pack.Init(sysTexSz.x, sysTexSz.y, false);
    r.frameNo = dagor_frame_no();
  }
  uint8_t *getDataPtr(uint8_t *dest, int dest_stride, int x0, int y0) { return dest + y0 * dest_stride + x0 * bpp; }
  void clearData(int w, int h, uint8_t *dest, int dest_stride, int x0, int y0, char clear_with = 0x0)
  {
    for (dest += y0 * dest_stride + x0 * bpp; h > 0; dest += dest_stride, h--)
      memset(dest, clear_with, w * bpp);
  }
  void copyData(const uint8_t *src_data, int w, int h, uint8_t *dest, int dest_stride, int x0, int y0)
  {
    if (y0 < 0)
    {
      if (y0 + h <= 0)
        return;
      src_data += w * (-y0);
      h += y0;
      y0 = 0;
    }
    for (dest += y0 * dest_stride + x0 * bpp; h > 0; dest += dest_stride - w * bpp, h--)
      for (int cnt = w * bpp; cnt > 0; cnt--, dest++, src_data++)
        *dest |= *src_data;
  }
  Texture *add(int w, int h, CopyRec &out_rec)
  {
    if (curSysTex < 0)
      alloc();

    rbp::Rect r;
    SysTexRec *rec = &sysTex[curSysTex];
    for (int attempts = sysTexCount; attempts > 0; attempts--)
    {
      r = rec->pack.Insert(w, h, rbp::SkylineBinPack::LevelMinWasteFit, false);
      if (r.width > 0 && r.height > 0)
        return fillOutRec(out_rec, r, rec);

      curSysTex = (curSysTex + 1) % sysTexCount;
      rec = &sysTex[curSysTex];
      if (rec->frameNo + MAX_FRAME_QUEUE <= dagor_frame_no())
      {
        rec->pack.Init(sysTexSz.x, sysTexSz.y, false);

        void *ptr;
        int stride;
        rec->tex->lockimg(&ptr, stride, 0, TEXLOCK_DEFAULT);
        rec->tex->unlockimg();
      }
    }

    if (sysTexCount < MAX_GEN_SYS_TEX_CNT)
    {
      alloc();
      rec = &sysTex[curSysTex];
      r = rec->pack.Insert(w, h, rbp::SkylineBinPack::LevelMinWasteFit, false);
      if (r.width > 0 && r.height > 0)
        return fillOutRec(out_rec, r, rec);
    }
    return NULL;
  }

  const IPoint2 &getSz() const { return sysTexSz; }

protected:
  SysTexRec sysTex[MAX_GEN_SYS_TEX_CNT];
  int curSysTex = -1, sysTexCount = 0;
  IPoint2 sysTexSz = IPoint2(1, 1);
  int texFmt = TEXFMT_R8;
  int bpp = 1;

  Texture *fillOutRec(CopyRec &out_rec, const rbp::Rect &r, SysTexRec *rec)
  {
    out_rec.src_idx = curSysTex;
    out_rec.x0 = r.x;
    out_rec.y0 = r.y;
    out_rec.w = r.width;
    out_rec.h = r.height;
    rec->frameNo = dagor_frame_no();
    return rec->tex;
  }
};

class DynamicAtlasFencedTexUpdater
{
public:
  static constexpr int MAX_FRAME_QUEUE = 3;

  struct CopyRec
  {
    uint16_t src_idx, x0, y0, w, h, dest_idx, dest_x0, dest_y0;
  };

  struct UnlockedTexInfo
  {
    Texture *tex;
    D3dEventQuery *writeCompeletionFence;
  };

  struct FencedGPUResource
  {
    Texture *tex = 0;
    D3dEventQuery *writeCompeletionFence = nullptr;
    uint8_t *texturePtr = nullptr;
    int stride = 0;

    bool lock()
    {
      G_ASSERT(!texturePtr);

      if (!d3d::get_event_query_status(writeCompeletionFence, false))
        return false;

      if (!tex->lockimg((void **)&texturePtr, stride, 0, TEXLOCK_WRITE | TEXLOCK_RAWDATA))
      {
        logerr("DynamicAtlasFencedTexUpdater can't lock texture");
        return false;
      }

      return true;
    }

    void unlock()
    {
      G_ASSERT(texturePtr);

      tex->unlockimg();
      texturePtr = nullptr;
    }

    void init(const IPoint2 &sz, int tex_fmt = TEXFMT_R8)
    {
      tex = d3d::create_tex(NULL, sz.x, sz.y, tex_fmt | TEXCF_SYSMEM | TEXCF_DYNAMIC | TEXCF_READABLE | TEXCF_LINEAR_LAYOUT, 1,
        "upd-sysTex");
      writeCompeletionFence = d3d::create_event_query();
    }

    void term()
    {
      if (texturePtr)
        unlock();

      del_d3dres(tex);
      tex = nullptr;

      d3d::release_event_query(writeCompeletionFence);
    }
  };

  void init(const IPoint2 &sz, int tex_fmt = TEXFMT_R8)
  {
    sysTexSz = sz;
    texFmt = tex_fmt;
    if (texFmt == TEXFMT_R8)
      bpp = 1;
    else if (texFmt == TEXFMT_A8R8G8B8)
      bpp = 4;
    else
      bpp = 1; //== unrecognized but safe

    append_items(sysTextures, MAX_FRAME_QUEUE);
    for (int i = 0; i < MAX_FRAME_QUEUE; ++i)
      sysTextures[i].init(sz, tex_fmt);
  }

  void term()
  {
    pack.Init(1, 1, false);
    for (int i = 0; i < MAX_FRAME_QUEUE; ++i)
      sysTextures[i].term();
    sysTextures.clear();
  }

  void afterDeviceReset() { term(); }

  // sync it with atomic here or sync with extern means when tryLock succeeds
  bool isLocked() { return currentLockedTex >= 0; }

  // this shoud be called from main thread
  UnlockedTexInfo unlockCurrent()
  {
    G_ASSERT(isLocked());

    auto &currentTex = sysTextures[currentLockedTex];
    currentLockedTex = -1;

    currentTex.unlock();
    return {currentTex.tex, currentTex.writeCompeletionFence};
  }

  // this shoud be called from main thread
  bool tryLock()
  {
    if (isLocked())
      return false;

    if (!sysTextures[nextTexture].lock())
      return false;

    currentLockedTex = nextTexture;
    nextTexture = (nextTexture + 1) % MAX_FRAME_QUEUE;

    pack.Init(sysTexSz.x, sysTexSz.y, false);
    return true;
  }

  uint8_t *getDataPtr(int x0, int y0)
  {
    return sysTextures[currentLockedTex].texturePtr + y0 * sysTextures[currentLockedTex].stride + x0 * bpp;
  }

  void fillData(const uint8_t *src_data, int w, int h, int x0, int y0)
  {
    if (y0 < 0)
    {
      if (y0 + h <= 0)
        return;
      src_data += w * (-y0);
      h += y0;
      y0 = 0;
    }

    auto stride = sysTextures[currentLockedTex].stride;
    for (uint8_t *texturePtr = getDataPtr(x0, y0); h > 0; texturePtr += stride, h--, src_data += w * bpp)
      memcpy(texturePtr, src_data, w * bpp);
  }

  // try lock until locked on main thread
  // then push data with add until it is full or no data left ON external thread
  // signal main thread that we can unlock it
  // unlock on main thread
  // copy rects to target texture

  bool add(int w, int h, CopyRec &out_rec)
  {
    rbp::Rect r = pack.Insert(w, h, rbp::SkylineBinPack::LevelMinWasteFit, false);
    if (r.width > 0 && r.height > 0)
    {
      fillOutRec(out_rec, r);
      return true;
    }

    return false;
  }

  const IPoint2 &getSz() const { return sysTexSz; }

protected:
  StaticTab<FencedGPUResource, MAX_FRAME_QUEUE> sysTextures;
  int currentLockedTex = -1;
  int nextTexture = 0;

  IPoint2 sysTexSz = IPoint2(1, 1);
  int texFmt = TEXFMT_R8;
  int bpp = 1;

  rbp::SkylineBinPack pack;

  void fillOutRec(CopyRec &out_rec, const rbp::Rect &r)
  {
    out_rec.x0 = r.x;
    out_rec.y0 = r.y;
    out_rec.w = r.width;
    out_rec.h = r.height;
  }
};
