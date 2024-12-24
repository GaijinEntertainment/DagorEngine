// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texMgrData.h"
#include <generic/dag_fixedVectorMap.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_rwLock.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/fileTexFactory.h>
#include <math/dag_adjpow2.h>

using namespace texmgr_internal;

// #define RMGR_TRACE debug  // trace stages of texture reading
#ifndef RMGR_TRACE
#define RMGR_TRACE(...)          ((void)0)
#define RMGR_DUMP_TEX_STATE(IDX) ((void)0)
#else
#define RMGR_DUMP_TEX_STATE(IDX) RMGR.dumpTexState(IDX)
#endif

TEXTUREID add_managed_texture(const char *name, TextureFactory *factory)
{
  if (!name || !name[0])
    return BAD_TEXTUREID;

  acquire_texmgr_lock();
  int idx = find_texture_rec(name, true, factory);
  if (idx >= 0)
  {
    TEX_REC_LOCK();
    if (!RMGR.getFactory(idx) && !RMGR.getD3dRes(idx))
      RMGR.setFactory(idx, factory ? factory : default_tex_mgr_factory);
    else if (factory && RMGR.getFactory(idx) != factory && !disable_add_warning)
      LOGERR_CTX("%s(%s), idx=0x%x: factory already set (%p, %p)", __FUNCTION__, name, idx, RMGR.getFactory(idx), factory);
    if (RMGR.isScheduledForRemoval(idx))
    {
      TEX_REC_UNLOCK();
      release_texmgr_lock();
      DEV_FATAL("adding texture %s that wasnt properly removed", name);
      return BAD_TEXTUREID;
    }
    TEX_REC_UNLOCK();
    mark_texture_rec(idx);
  }

  release_texmgr_lock();
  return RMGR.toId(idx);
}

D3DRESID register_managed_res(const char *name, D3dResource *res)
{
  acquire_texmgr_lock();
  int idx = find_texture_rec(name, true, nullptr);
  TEXTUREID tid = RMGR.toId(idx);
  if (idx >= 0)
  {
    TEX_REC_LOCK();
    if (!RMGR.getFactory(idx) && !RMGR.getD3dRes(idx))
    {
      RMGR.setD3dRes(idx, res);
      apply_mip_bias_rules(RMGR.baseTexture(idx), RMGR.getName(idx));
    }
    else
      LOGERR_CTX("add_managed_texture_basetex: <%s> texture id already used; refCount=%d forRemove=%d name=%s", name,
        RMGR.getRefCount(idx), RMGR.isScheduledForRemoval(idx), RMGR.getName(idx));
    if (RMGR.isScheduledForRemoval(idx))
    {
      TEX_REC_UNLOCK();
      release_texmgr_lock();
      DEV_FATAL("adding texture %s that wasnt properly removed", name);
      return BAD_TEXTUREID;
    }

    RMGR.incRefCount(idx);
    TEX_REC_UNLOCK();
  }

  release_texmgr_lock();
  if (hook_on_get_texture_id && idx >= 0)
    hook_on_get_texture_id(tid);
  return tid;
}
TEXTUREID register_managed_tex(const char *name, BaseTexture *texture) { return register_managed_res(name, texture); }

bool is_managed_res_factory_set(D3DRESID rid)
{
  int idx = RMGR.toIndex(rid);
  if (idx < 0)
    return false;
  return RMGR.isAlive(idx) && RMGR.getFactory(idx) != nullptr;
}

const char *get_managed_res_name(D3DRESID rid)
{
  int idx = RMGR.toIndex(rid);
  if (idx < 0)
    return NULL;
  return RMGR.isAlive(idx) ? RMGR.getName(idx) : nullptr;
}

D3DRESID get_managed_res_id(const char *name)
{
  if (!name || !name[0])
    return BAD_TEXTUREID;
  acquire_texmgr_lock();
  int idx = find_texture_rec(name, auto_add_tex_on_get_id, nullptr);
  D3DRESID rid = RMGR.toId(idx);
  if (auto_add_tex_on_get_id && idx >= 0)
  {
    TEX_REC_LOCK();
    if (!RMGR.getFactory(idx) && !RMGR.getD3dRes(idx))
      RMGR.setFactory(idx, default_tex_mgr_factory);
    TEX_REC_UNLOCK();
  }
  if (idx >= 0)
    mark_texture_rec(idx);
  release_texmgr_lock();
  if (hook_on_get_texture_id && idx >= 0)
    hook_on_get_texture_id(rid);
  return rid;
}

bool check_managed_texture_loaded(TEXTUREID tid, bool fq_loaded)
{
  if (texmgr_internal::dbg_texq_only_stubs)
    return true;
  int idx = RMGR.toIndex(tid);
  if (idx < 0)
    return false;
  TEX_REC_AUTO_LOCK();
  if (!RMGR.getD3dRes(idx))
    return false;

  return RMGR.isTexLoaded(idx, fq_loaded);
}
bool check_all_managed_textures_loaded(dag::ConstSpan<TEXTUREID> id_list, bool fq_loaded, bool ignore_unref_tex)
{
  if (texmgr_internal::dbg_texq_only_stubs)
    return true;
  TEX_REC_AUTO_LOCK();
  for (int i = 0; i < id_list.size(); i++)
  {
    int idx = RMGR.toIndex(id_list[i]);
    if (idx < 0)
    {
      if (ignore_unref_tex)
        continue;
      return false;
    }
    if (ignore_unref_tex && RMGR.getRefCount(idx) == 0)
      continue;
    if (!RMGR.getD3dRes(idx))
      return false;

    if (!RMGR.isTexLoaded(idx, fq_loaded))
      return false;
  }
  return true;
}

void discard_unused_managed_textures()
{
  for (unsigned i = 0, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) == 0)
      discard_unused_managed_texture(RMGR.toId(i));
}

D3DRESID iterate_all_managed_d3dres(D3DRESID after_tid, int min_ref_count)
{
  for (unsigned i = after_tid == BAD_D3DRESID ? 0 : after_tid.index() + 1, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= min_ref_count)
      return RMGR.toId(i);
  return BAD_D3DRESID;
}

TEXTUREID iterate_all_managed_textures(TEXTUREID after_tid, int min_ref_count)
{
  for (unsigned i = after_tid == BAD_D3DRESID ? 0 : after_tid.index() + 1, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= min_ref_count)
      if (!RMGR.getD3dRes(i) || RMGR.baseTexture(i))
        return RMGR.toId(i);
  return BAD_TEXTUREID;
}

TEXTUREID get_max_managed_texture_id() { return RMGR.toId(RMGR.getAccurateIndexCount() - 1); }

// internal helpers
int texmgr_internal::find_texture_rec(const char *name, bool auto_add, TextureFactory *f)
{
  if (!name || !name[0])
    return -1;

  TextureMetaData tmd;
  String stor(framemem_ptr());
  char *nm = (char *)tmd.decode(name, &stor);
  if (nm != stor.c_str())
  {
    stor = nm;
    nm = stor.c_str();
  }
  if (strncmp(nm, "b64://", sizeof("b64://") - 1) != 0)
  {
    dd_simplify_fname_c(nm);
    dd_strlwr(nm);
  }

  int idx = managed_tex_map_by_name.getStrId(nm);
  if (idx >= 0)
    return idx;

  if (!auto_add)
    return -1;

  idx = RMGR.allocEntry();
  if (idx == -1)
    return -1;
  TEX_REC_LOCK();
  RMGR.initAllocatedRec(idx, name, f);
  TEX_REC_UNLOCK();

  const char *stored_name = nullptr;
  managed_tex_map_by_name.addStrId(nm, idx, stored_name);
  G_ASSERT(stored_name);
  if (DAGOR_UNLIKELY(idx >= managed_tex_map_by_idx.size()))
  {
    G_ASSERTF(!mt_enabled || idx < managed_tex_map_by_idx.capacity(),
      "mt_enabled=%d idx=%d managed_tex_map_by_id.count=%d (.capacity=%d)", mt_enabled, idx, managed_tex_map_by_idx.size(),
      managed_tex_map_by_idx.capacity());
    int b = managed_tex_map_by_idx.size();
    managed_tex_map_by_idx.resize(idx + 1);
    mem_set_0(make_span(managed_tex_map_by_idx).subspan(b));
  }
  managed_tex_map_by_idx[idx] = stored_name;
  RMGR.texSamplers[idx] = d3d::request_sampler(get_sampler_info(tmd));

  // debug("[TEXMGR] added texture (%s) as %d", name, idx);

  return idx;
}

void TextureFactory::onTexFactoryDeleted(TextureFactory *f)
{
  if (!RMGR.getAccurateIndexCount() || !managed_tex_map_by_name.strCount())
    return;

  acquire_texmgr_lock();
  bool d3d_already_destroyed = !d3d::is_inited();
  for (unsigned i = 0, ie = RMGR.getAccurateIndexCount(); i < ie; i++)
  {
    if (RMGR.getFactory(i) != f || !RMGR.getD3dRes(i))
      continue;

    TEXTUREID tid = RMGR.toId(i);
    if (d3d_already_destroyed)
      RMGR.initAllocatedRec(i, nullptr, f);
    else if (RMGR.getRefCount(i) == 0)
    {
      f->releaseTexture(RMGR.baseTexture(i), tid);
      RMGR.initAllocatedRec(i, nullptr, f);
    }
    texmgr_internal::evict_managed_tex_and_id(tid);
  }
  release_texmgr_lock();
}

void dump_texture_usage(bool dump_used_only)
{
#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
  debug("======= texture dump start ========");
  int used = 0;
  int fail = 0;
  int free = 0;
  TEX_REC_AUTO_LOCK();
  for (unsigned i = 0, ie = RMGR.getAccurateIndexCount(); i < ie; i++)
  {
    const char *status = "USED";
    bool isUsed = false;
    int rc = RMGR.getRefCount(i);
    if (rc < 0)
    {
      if (rc != RMGR.INVALID_REFCOUNT || RMGR.getFactory(i))
      {
        status = "FAIL";
        fail++;
        isUsed = true;
      }
      else
      {
        status = "FREE";
        free++;
      }
    }
    else
    {
      used++;
      isUsed = true;
    }
    if (isUsed || !dump_used_only)
      debug("#%03d: [%s] Ref=%03d, Ptr=%p, Factory=%p, Name=%s", i, status, rc, RMGR.getD3dRes(i), RMGR.getFactory(i),
        RMGR.getName(i));
  }
  debug("=======");
  debug("Total: \n  Used: %d\n  Free: %d\n  Errors: %d", used, free, fail);
  debug("=======  texture dump end  ========");
#else
  (void)dump_used_only;
#endif
}

void load_anisotropy_from_settings()
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  ::dgs_tex_anisotropy = graphicsBlk->getInt("anisotropy", 1);
}

void add_anisotropy_exception(TEXTUREID id)
{
  TEX_REC_AUTO_LOCK();
  int idx = RMGR.toIndex(id);
  if (idx >= 0)
    anisotropy_exceptions.add((TEXTUREID)idx);
}

void reset_anisotropy(const char *tex_name_filter)
{
  String stor;
  bool isFilterValid = tex_name_filter != nullptr && strlen(tex_name_filter) > 0;
  TEX_REC_AUTO_LOCK();
  for (unsigned i = 0, ie = RMGR.getAccurateIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= 0) // It is not a free id.
    {
      const char *name = RMGR.getName(i);
      BaseTexture *bt = RMGR.baseTexture(i);
      if (!bt)
        continue;
      if (isFilterValid && strstr(name, tex_name_filter) == nullptr)
        continue;
      if (anisotropy_exceptions.has((TEXTUREID)i))
        continue;
      TextureMetaData tmd;
      tmd.decode(name, &stor);
      if (bt->isSamplerEnabled())
        bt->setAnisotropy(tmd.calcAnisotropy(::dgs_tex_anisotropy));
    }
}

static void (*lod_bias_change_cb)() = nullptr;

void set_add_lod_bias_cb(void (*cb)())
{
  G_ASSERT(lod_bias_change_cb == nullptr);
  lod_bias_change_cb = cb;
}

void set_add_lod_bias(float add, const char *required_part)
{
  TEX_REC_AUTO_LOCK();

  add_mip_bias_rule(required_part, add);
  for (unsigned i = 0, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= 0)
      apply_mip_bias_rules(RMGR.baseTexture(i), RMGR.getName(i));
  if (!lod_bias_change_cb)
    logerr("lod_bias_change_cb is not set! Samplers for materials are incorrect.");
  else
    lod_bias_change_cb();
}

void set_add_lod_bias_batch(dag::Span<const LODBiasRule> rules)
{
  for (auto &rule : rules)
    add_mip_bias_rule(rule.substring, rule.bias);

  for (unsigned i = 0, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= 0)
      apply_mip_bias_rules(RMGR.baseTexture(i), RMGR.getName(i));

  if (!lod_bias_change_cb)
    logerr("lod_bias_change_cb is not set! Samplers for materials are incorrect.");
  else
    lod_bias_change_cb();
}

// textag marks
struct TexTagMarkCtx
{
  unsigned curMask;
  unsigned scopeMaskStackDepth;
  unsigned scopeMaskStack[TEXTAG__COUNT];
};

static thread_local TexTagMarkCtx ttCtx;

void textag_mark_begin(int textag)
{
  G_ASSERT(textag >= 0 && textag < TEXTAG__COUNT);
  TexTagMarkCtx *ctx = &ttCtx;
  unsigned m = 1U << textag;
  G_ASSERTF(!(ctx->curMask & m), "textag=%d already begun, mask=%08X", textag, ctx->curMask);
  if (ctx->scopeMaskStackDepth < TEXTAG__COUNT)
  {
    ctx->scopeMaskStack[ctx->scopeMaskStackDepth] = ctx->curMask;
    ctx->scopeMaskStackDepth++;
    ctx->curMask |= m;
  }
  else
    G_ASSERTF(ctx->scopeMaskStackDepth < TEXTAG__COUNT, "too deep scopes (%d) for textag=%d", ctx->scopeMaskStackDepth, textag);
}
void textag_mark_end()
{
  TexTagMarkCtx *ctx = &ttCtx;
  if (ctx->scopeMaskStackDepth > 0)
  {
    ctx->curMask = ctx->scopeMaskStack[ctx->scopeMaskStackDepth - 1];
    ctx->scopeMaskStackDepth--;
  }
  else
    G_ASSERTF(ctx->scopeMaskStackDepth > 0, "too many textag_mark_end() calls");
}
void texmgr_internal::mark_texture_rec(int idx)
{
  TexTagMarkCtx *ctx = &ttCtx;
  int m = ctx->curMask;
  if (!m)
    return;
  G_ASSERTF(idx >= 0 && idx < RMGR.getAccurateIndexCount(), "idx=%d managed_tex_list=%d", idx, RMGR.getAccurateIndexCount());
  unsigned oldm = RMGR.getTagMask(idx);
  if ((oldm & m) == m)
    return;
  for (unsigned i = 0, tf = 1, mf = 0xFFFFFFFFU; i < TEXTAG__COUNT; i++, tf <<= 1, mf >>= 1)
  {
    if (!(m & mf))
      break;
    if (m & tf && !(oldm & tf))
      interlocked_increment(texmgr_internal::textagInfo[i].texCount);
  }
  RMGR.updTagMaskOr(idx, m);
  // debug("marktag %d <%s> with m=%X", idx, RMGR.getName(idx), m);
}
void texmgr_internal::D3dResMgrDataFinal::markUpdated(int idx, unsigned new_ld_lev)
{
  G_ASSERTF(idx >= 0 && idx < RMGR.getAccurateIndexCount(), "idx=%d managed_tex_list=%d", idx, RMGR.getAccurateIndexCount());

  bool ld_lev_increased = new_ld_lev > resQS[idx].getLdLev();
  if (uint8_t rd_lev = resQS[idx].getRdLev())
  {
    G_ASSERTF(rd_lev == new_ld_lev, "%s: rdLev=%d ldLev=%d new_ld_lev=%d", getName(idx), rd_lev, resQS[idx].getLdLev(), new_ld_lev);
    finishReading(idx);
  }
  else
  {
    resQS[idx].setCurQL(new_ld_lev < resQS[idx].getQLev() ? calcCurQL(idx, new_ld_lev) : resQS[idx].getMaxQL());
    resQS[idx].setLdLev(new_ld_lev);
  }

  unsigned tagm = RMGR.getTagMask(idx);
  if (!tagm || (!ld_lev_increased && RMGR.getRefCount(idx) >= 0))
    return;

  for (unsigned i = 0, tf = 1, mf = 0xFFFFFFFFU; i < TEXTAG__COUNT; i++, tf <<= 1, mf >>= 1)
  {
    if (!(tagm & mf))
      break;
    if (tagm & tf)
    {
      if (ld_lev_increased)
      {
        interlocked_increment(texmgr_internal::textagInfo[i].loadGeneration);
        // debug("markload %d <%s> tag=%d (-> %d)", idx, RMGR.getName(idx), i, new_ld_lev);
      }
      else if (RMGR.getRefCount(idx) < 0)
      {
        RMGR.updTagMaskAnd(idx, ~tf);
        interlocked_decrement(texmgr_internal::textagInfo[i].texCount);
      }
    }
  }
}

bool texmgr_internal::D3dResMgrDataFinal::scheduleReading(int idx, TextureFactory *f)
{
  unsigned max_lev = resQS[idx].getMaxLev();
  if (resQS[idx].isReading())
    return false;
  if (resQS[idx].getLdLev() >= min<unsigned>(resQS[idx].getMaxReqLev(), max_lev) && !getBaseTexUsedCount(idx))
    return false;
  if (getRefCount(idx) == 0 && getBaseTexUsedCount(idx) == 0)
    return false;

  unsigned cur_ql = resQS[idx].getCurQL();
  if (texmgr_internal::enable_cur_ql_mismatch_assert)
  {
    G_ASSERTF(cur_ql == calcCurQL(idx, resQS[idx].getLdLev()), "idx=%d curQL=%d ldLev=%d getCurQL()=%d %s", idx, cur_ql,
      resQS[idx].getLdLev(), calcCurQL(idx, resQS[idx].getLdLev()), getName(idx));
  }
  if (cur_ql != TQL_stub && cur_ql + texDesc[idx].dim.bqGenmip > TQL_base && getBaseTexUsedCount(idx) &&
      !hasTexBaseData(idx)) // missing base data for loaded texture
    cur_ql = TexQL(TQL_base - texDesc[idx].dim.bqGenmip);
  if (getRefCount(idx) > 0 && !getTexMemSize4K(idx) && resQS[idx].getLdLev() > 1)
  {
    // since texture is missing we should mark ldLev=1 to force full data reload
    RMGR_TRACE("reset ldLev: %s (GPUmem=%dK) ld=%d ql=%d rc=%d btRc=%d", RMGR.getName(idx), getTexMemSize4K(idx) * 3,
      resQS[idx].getLdLev(), cur_ql, getRefCount(idx), getBaseTexUsedCount(idx));
    resQS[idx].setCurQL(TexQL(cur_ql = TQL_stub));
    resQS[idx].setLdLev(1);
    RMGR_DUMP_TEX_STATE(idx);
  }
  for (TexQL ql = cur_ql == TQL_stub ? TQL__FIRST : TexQL(cur_ql + 1); ql < TQL__COUNT; ql = TexQL(ql + 1))
    if (unsigned l = getLevDesc(idx, ql))
    {
      if (texDesc[idx].packRecIdx[ql].pack < 0)
        return false;
      if (getTexAddMemSizeNeeded4K(idx) && l > getLevDesc(idx, TQL_base) && !texmgr_internal::is_gpu_mem_enough_to_load_hq_tex() &&
          getTexImportance(idx) < 1)
        return false;

      if (!RMGR.startReading(idx, l))
        return false;
      RMGR_DUMP_TEX_STATE(idx);

      TEXTUREID bt_tid = RMGR.pairedBaseTexId[idx];
      if (bt_tid != BAD_TEXTUREID)
      {
        RMGR_TRACE("require basetex(%s)", RMGR.getName(bt_tid.index()));
        RMGR.incBaseTexRc(bt_tid);
        if (!hasTexBaseData(bt_tid.index()))
        {
          RMGR.updateResReqLev(bt_tid, bt_tid.index(), resQS[idx].getMaxReqLev());
          scheduleReading(bt_tid.index(), RMGR.getFactory(bt_tid.index()));
        }
      }

      f->scheduleTexLoading(RMGR.toId(idx), ql);
      return true;
    }
  return false;
}
unsigned texmgr_internal::D3dResMgrDataFinal::calcTexMemSize(int idx, int target_lev, const ddsx::Header &hdr)
{
  unsigned skip = (texDesc[idx].dim.maxLev - target_lev);
  unsigned w = max(texDesc[idx].dim.w >> skip, 1), h = max(texDesc[idx].dim.h >> skip, 1);
  unsigned d = (hdr.flags & hdr.FLG_VOLTEX) ? max(texDesc[idx].dim.d >> skip, 1) : 1;
  unsigned a = (hdr.flags & hdr.FLG_ARRTEX) ? texDesc[idx].dim.d : 1;
  unsigned l = texDesc[idx].dim.l - skip;
  if (hdr.flags & hdr.FLG_CUBTEX)
    a *= 6;

  uint32_t total = 0;
  while (l > 0)
  {
    total += hdr.calcSurfaceSz(w, h) * d;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;
    if (d > 1)
      d >>= 1;
    l--;
  };

  return total * a + (target_lev > 1 ? 4096 : 0);
}
TexLoadRes texmgr_internal::D3dResMgrDataFinal::readDdsxTex(TEXTUREID tid, const ddsx::Header &hdr, IGenLoad &crd, int quality_id,
  on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb)
{
  unsigned idx = tid.index();
  bool should_skip_reading = false;
  unsigned cur_ql = resQS[idx].getCurQL();
  unsigned max_lev = resQS[idx].getMaxLev();
  unsigned rd_lev = resQS[idx].getRdLev();
  unsigned min_lev = RMGR.getLevDesc(idx, TQL_thumb);
  if (!min_lev && resQS[idx].getMaxReqLev() < RMGR.getLevDesc(idx, TQL_base))
  {
    min_lev = max(2 + std::abs((int)get_log2i(texDesc[idx].dim.w) - (int)get_log2i(texDesc[idx].dim.h)), 5 /*approx thumb level*/);
    min_lev = max<unsigned>(min_lev, texDesc[idx].dim.maxLev - texDesc[idx].dim.l + 1);
    min_lev = min<unsigned>(min_lev, resQS[idx].getMaxLev());
  }
  if (rd_lev > max_lev)
    resQS[idx].setRdLev(rd_lev = max_lev);
  BaseTexture *t = getRefCount(idx) > 0 ? RMGR.baseTexture(idx) : nullptr;

  if (getRefCount(idx) == 0 && getBaseTexUsedCount(idx) == 0)
    should_skip_reading = true;
  else if (resQS[idx].getLdLev() >= min<unsigned>(resQS[idx].getMaxReqLev(), max_lev) && !getBaseTexUsedCount(idx))
    should_skip_reading = true;
  else if (t && getTexAddMemSizeNeeded4K(idx) && rd_lev > getLevDesc(idx, TQL_base) &&
           !texmgr_internal::is_gpu_mem_enough_to_load_hq_tex() && getTexImportance(idx) < 1)
  {
    if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && getBaseTexUsedCount(idx) > 0 && !hasTexBaseData(idx))
      t = nullptr;
    else
      should_skip_reading = true;
  }
  else if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && !texmgr_internal::is_sys_mem_enough_to_load_basedata())
    should_skip_reading = getBaseTexUsedCount(idx) < 1;

  if (t && (hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && !hasTexBaseData(idx) && rd_lev <= resQS[idx].getLdLev()) // fully loaded texture
                                                                                                              // with missing baseData
    t = nullptr;

  if (should_skip_reading)
  {
    RMGR_TRACE("%d: skip_reading(rd_lev=%d): %s", idx, rd_lev, getName(idx));
    resQS[idx].setRdLev(rd_lev = resQS[idx].getLdLev()); // skip reading, leave state as is
    return TexLoadRes::OK;
  }
  if (!(hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER)))
  {
    max_lev = max<unsigned>(min<unsigned>(max_lev, resQS[idx].getMaxReqLev()), min_lev);
    if (rd_lev > max_lev)
      resQS[idx].setRdLev(rd_lev = max_lev);
  }

  unsigned target_lev = (texmgr_internal::is_gpu_mem_enough_to_load_hq_tex() || texDesc[idx].dim.stubIdx < 0) ? max_lev : rd_lev;
  unsigned skip = (texDesc[idx].dim.maxLev - target_lev);
  unsigned w = max(texDesc[idx].dim.w >> skip, 1), h = max(texDesc[idx].dim.h >> skip, 1);
  unsigned d =
    max((hdr.flags & hdr.FLG_VOLTEX) ? texDesc[idx].dim.d >> skip : ((hdr.flags & hdr.FLG_CUBTEX) ? 6 : texDesc[idx].dim.d), 1);
  unsigned l = texDesc[idx].dim.l - skip;
  TEXTUREID base_tid = pairedBaseTexId[idx];
  TexLoadRes ret = TexLoadRes::ERR;

  if (incRefCount(idx) == 1)
    RMGR.decReadyForDiscardTex(idx);
  unsigned cur_sz = tql::sizeInKb(calcTexMemSize(idx, target_lev, hdr));
  unsigned full_sz = tql::sizeInKb(calcTexMemSize(idx, resQS[idx].getQLev(), hdr));
  if (t)
    RMGR.changeTexUsedMem(idx, cur_sz, full_sz);

  if (!t)
  {
    if ((hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY) && getBaseTexUsedCount(idx) > 0)
    {
      ret = d3d_load_genmip_sysmemcopy(tid, base_tid, hdr, crd, quality_id);
      resQS[idx].setRdLev(rd_lev = resQS[idx].getLdLev()); // since texture is missing we should mark ldLev=1 afterwards
    }
    else
      ret = TexLoadRes::OK;
  }
  else if (cur_ql == TQL_stub || !t->ressize())
  {
    if (BaseTexture *tmp_tex = t->makeTmpTexResCopy(w, h, d, l))
    {
      tmp_tex->texmiplevel(target_lev - rd_lev, l - 1);
      ret = d3d_load_ddsx_tex_contents(tmp_tex, tid, base_tid, hdr, crd, quality_id, target_lev - rd_lev, 0, on_tex_slice_loaded_cb);
      if (ret == TexLoadRes::OK)
        t->replaceTexResObject(tmp_tex);
      else
        del_d3dres(tmp_tex);
    }
    else if (!d3d::is_in_device_reset_now())
      logwarn("%p->makeTmpTexResCopy(%d, %d, %d, %d) returns null (%s), read skipped", t, w, h, d, l, getName(idx));
  }
  else
  {
    unsigned ld_lev = resQS[idx].getLdLev();
    if (tql::resizeTexture(t, w, h, d, l, ld_lev))
    {
      ret = rd_lev > ld_lev
              ? d3d_load_ddsx_tex_contents(t, tid, base_tid, hdr, crd, quality_id, target_lev - rd_lev, ld_lev, on_tex_slice_loaded_cb)
              : TexLoadRes::OK;
      if (ret == TexLoadRes::OK)
        t->texmiplevel(target_lev - rd_lev, l - 1);
    }
  }
  if (ret != TexLoadRes::OK)
    resQS[idx].setRdLev(rd_lev = resQS[idx].getLdLev()); // failed to read, leave state as is
  if (t)
  {
    unsigned res_sz = tql::sizeInKb(t->ressize());
    if (res_sz > cur_sz)
      RMGR.changeTexUsedMem(idx, res_sz, max(res_sz, full_sz));
  }
  if (decRefCount(idx) == 0)
    RMGR.incReadyForDiscardTex(idx);

  // RMGR.dumpMemStats();
  return ret;
}
void texmgr_internal::D3dResMgrDataFinal::finishReading(int idx)
{
  uint8_t rd_lev = resQS[idx].getRdLev();
  G_ASSERTF_RETURN(rd_lev != 0, , "rdLev=%d ldLev=%d", rd_lev, resQS[idx].getLdLev());
  TEXTUREID bt_tid = RMGR.pairedBaseTexId[idx];
  if (bt_tid != BAD_TEXTUREID)
  {
    RMGR_TRACE("release basetex(%s)", RMGR.getName(bt_tid.index()));
    RMGR.decBaseTexRc(bt_tid);
  }
  resQS[idx].setCurQL(rd_lev < resQS[idx].getQLev() ? calcCurQL(idx, rd_lev) : resQS[idx].getMaxQL());
  resQS[idx].setLdLev(rd_lev);
  if (texmgr_internal::texq_load_on_demand)
    resQS[idx].setMaxReqLev(rd_lev);
  if (int imp = interlocked_acquire_load(texImportance[idx]))
    interlocked_add(texImportance[idx], -imp + (resQS[idx].getQLev() == resQS[idx].getLdLev() ? 0 : 1));
  RMGR_DUMP_TEX_STATE(idx);
}
void texmgr_internal::D3dResMgrDataFinal::cancelReading(int idx)
{
  G_ASSERTF_RETURN(resQS[idx].isReading(), , "rdLev=%d ldLev=%d", resQS[idx].getRdLev(), resQS[idx].getLdLev());
  TEXTUREID bt_tid = RMGR.pairedBaseTexId[idx];
  if (bt_tid != BAD_TEXTUREID && RMGR.isValidID(bt_tid, nullptr))
  {
    RMGR_TRACE("release basetex(%s)", RMGR.getName(bt_tid.index()));
    RMGR.decBaseTexRc(bt_tid);
  }
  resQS[idx].setRdLev(0);
  resQS[idx].setMaxReqLev(resQS[idx].getLdLev());
  RMGR_DUMP_TEX_STATE(idx);
}

void textag_clear_tag(int textag)
{
  G_ASSERT(textag >= 0 && textag < TEXTAG__COUNT);
  acquire_texmgr_lock();
  int m = 1 << textag;
  for (unsigned i = 0, ie = RMGR.getAccurateIndexCount(); i < ie; i++)
    if (RMGR.getRefCount(i) >= 0 && (RMGR.getTagMask(i) & m))
    {
      RMGR.updTagMaskAnd(i, ~m);
      interlocked_decrement(texmgr_internal::textagInfo[textag].texCount);
    }
  release_texmgr_lock();
}

void mark_managed_textures_important(dag::ConstSpan<TEXTUREID> id, unsigned add_importance, int min_lev_for_dyn_decrease)
{
  for (TEXTUREID tid : id)
  {
    int idx = RMGR.toIndex(tid);
    if (idx >= 0)
    {
      if (interlocked_add(RMGR.texImportance[idx], add_importance) > (48 << 10))
        interlocked_release_store(RMGR.texImportance[idx], (48 << 10));
      int max_lev = RMGR.resQS[idx].getQLev();
      if (tql::dyn_qlev_decrease > 0 && max_lev >= min_lev_for_dyn_decrease)
      {
        max_lev--;
        if (RMGR.resQS[idx].getMaxLev() != max_lev)
          RMGR.changeTexMaxLev(idx, max_lev);
      }
      else if (RMGR.resQS[idx].getMaxLev() < max_lev)
        RMGR.changeTexMaxLev(idx, max_lev);
    }
  }
}

const TexTagInfo &textag_get_info(int textag) { return texmgr_internal::textagInfo[textag]; }

void textag_get_list(int textag, Tab<TEXTUREID> &out_list, bool skip_unused)
{
  G_ASSERT(textag >= 0 && textag < TEXTAG__COUNT);
  int cnt = interlocked_acquire_load(texmgr_internal::textagInfo[textag].texCount);
  out_list.clear();
  out_list.reserve(cnt);
  acquire_texmgr_lock();
  int m = 1 << textag;
  for (unsigned i = 0, ie = RMGR.getAccurateIndexCount(); i < ie; i++)
  {
    int rc = RMGR.getRefCount(i);
    if (rc < 0 || !(RMGR.getTagMask(i) & m))
      continue;
    if (skip_unused && !rc)
      continue;

    out_list.push_back(RMGR.toId(i));
    if (out_list.size() >= cnt)
      break;
  }
  release_texmgr_lock();
}

TextureMetaData get_texture_meta_data(TEXTUREID id)
{
  int idx = RMGR.toIndex(id);
  const char *name = RMGR.getName(idx);

  TextureMetaData tmd;
  tmd.decode(name);
  return tmd;
}

d3d::SamplerInfo get_sampler_info(const TextureMetaData &texture_meta_data, bool force_addr_from_tmd)
{
  uint32_t colorRGB = texture_meta_data.borderCol & 0xFFFFFF ? 0xFFFFFF : 0;
  uint32_t colorA = texture_meta_data.borderCol & 0xFF000000 ? 0xFF000000 : 0;
  d3d::SamplerInfo samplerInfo = {
    static_cast<d3d::MipMapMode>(texture_meta_data.d3dMipFilter()),
    static_cast<d3d::FilterMode>(texture_meta_data.d3dTexFilter()),
    d3d::AddressMode::Wrap,
    d3d::AddressMode::Wrap,
    d3d::AddressMode::Wrap,
    static_cast<d3d::BorderColor::Color>(colorA | colorRGB),
    static_cast<float>(texture_meta_data.calcAnisotropy(::dgs_tex_anisotropy)),
    texture_meta_data.lodBias / 1000.0f,
  };
  if (force_addr_from_tmd || (texture_meta_data.flags & texture_meta_data.FLG_OVERRIDE))
  {
    samplerInfo.address_mode_u = static_cast<d3d::AddressMode>(texture_meta_data.d3dTexAddr(texture_meta_data.addrU));
    samplerInfo.address_mode_v = static_cast<d3d::AddressMode>(texture_meta_data.d3dTexAddr(texture_meta_data.addrV));
    samplerInfo.address_mode_w = static_cast<d3d::AddressMode>(texture_meta_data.d3dTexAddr(texture_meta_data.addrW));
  }
  return samplerInfo;
}

d3d::SamplerHandle get_texture_separate_sampler(TEXTUREID id)
{
  int idx = RMGR.toIndex(id);
  if (DAGOR_UNLIKELY(idx < 0))
    return d3d::INVALID_SAMPLER_HANDLE;

  return RMGR.texSamplers[idx];
}

bool set_texture_separate_sampler(TEXTUREID id, const d3d::SamplerInfo &sampler_info)
{
  int idx = RMGR.toIndex(id);
  if (idx < 0)
    return false;

  RMGR.texSamplers[idx] = d3d::request_sampler(sampler_info);
  return true;
}
