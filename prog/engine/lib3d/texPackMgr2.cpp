// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texPackMgr2.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/ddsxTex.h>
#include <3d/ddsxTexMipOrder.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_fastSeqRead.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobsQueue.h>
#include <util/dag_roNameMap.h>
#include <util/dag_globDef.h>
#include <util/dag_simpleString.h>
#include <util/dag_texMetaData.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <3d/dag_texIdSet.h>
#include <3d/tql.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_perfTimer.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>

#if _TARGET_APPLE
#include <sys/resource.h>
#endif

// #define RMGR_TRACE debug  // trace resource manager's DDSx loads
#ifndef RMGR_TRACE
#define RMGR_TRACE(...) ((void)0)
#endif

struct DDSxLoadQueueRec
{
  BaseTexture *tex = NULL;
  TEXTUREID tid = BAD_TEXTUREID;
  SimpleString fn;
  int qId = -1;
  TEXTUREID btId = BAD_TEXTUREID;

  DDSxLoadQueueRec() {}
  DDSxLoadQueueRec(BaseTexture *t, TEXTUREID _tid, const char *_fn, int q_id, TEXTUREID bt_id) :
    tex(t), tid(_tid), fn(_fn), qId(q_id), btId(bt_id)
  {}
};

static cpujobs::JobQueue<DDSxLoadQueueRec, true> ddsxLoadQueue;
static struct TexPacksAreAllowedToLoad
{
#if _TARGET_C1 | _TARGET_C2 // Note: Add platforms as necessary















#else
  void waitUnlessMainThread() {}
  void toggle(bool) {}
#endif
} tex_packs_are_allowed_to_load;

static void process_arr_tex_load_queue();
static void term_arr_tex_load_queue();

#include "texMgrData.h"
using texmgr_internal::RMGR;

#include "ddsxDec.h"

struct DDSxArrayTextureFactory;
class DDSxTexturePack2;

struct TexPackRegRec
{
  int pathId;
  DDSxTexturePack2 *pack;
  int refCount;
};

static OAHashNameMap<true> packs_pathes;
static Tab<TexPackRegRec> tex_packs(inimem);
static SmartReadWriteLock tex_packs_lock;
struct TexPackALWr
{
  TexPackALWr() { tex_packs_lock.lockWrite(); }
  ~TexPackALWr() { tex_packs_lock.unlockWrite(); }
};
struct TexPackALRd
{
  TexPackALRd() { tex_packs_lock.lockRead(); }
  ~TexPackALRd() { tex_packs_lock.unlockRead(); }
};

static void simplify_path(const char *path, char *simplified)
{
  strncpy(simplified, path, DAGOR_MAX_PATH - 1);
  simplified[DAGOR_MAX_PATH - 1] = '\0';

  dd_simplify_fname_c(simplified);
}

static int get_path_id(const char *path)
{
  char simplified[DAGOR_MAX_PATH];
  simplify_path(path, simplified);
  return packs_pathes.getNameId(simplified);
}


#define debug(...) logmessage(_MAKE4C('LOAD'), __VA_ARGS__)

#if _TARGET_APPLE
static int setfplimit()
{
  struct rlimit rlp = {10240, RLIM_INFINITY};
  return setrlimit(RLIMIT_NOFILE, &rlp);
}
static int fplimitset = setfplimit(); // do it before first libc functions calls (http://stackoverflow.com/a/3214064)
#define ALWAYS_REOPEN_FILES (fplimitset != 0)
#elif _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
#define ALWAYS_REOPEN_FILES 1
#else
#define ALWAYS_REOPEN_FILES 0
#endif

static int ddsx_factory_delayed_load_entered_global = 0;
static int ddsx_factory_uses_dctx = 0;

DDSxDecodeCtx *DDSxDecodeCtx::dCtx = NULL;
static volatile int ddsx_loaded_tex_cnt[DXP_PRIO_COUNT] = {0};

uint32_t DDSxDecodeCtx::prioWorkerUsedMask[DXP_PRIO_COUNT] = {0};
static volatile int strmLoadMode = ddsx::MultiDecoders;

static int64_t sum_lq_mem_sz = 0, sum_lq_pack_sz = 0, sum_hq_mem_sz = 0, sum_hq_pack_sz = 0;
static int64_t sum_gm_mem_sz = 0, sum_gm_pack_sz = 0;
static int64_t sum_dt_mem_sz = 0, sum_dt_pack_sz = 0;
static int64_t sum_sm_mem_sz = 0, sum_sm_pack_sz = 0;
static int64_t sum_ot_mem_sz = 0, sum_ot_pack_sz = 0;
static int64_t sum_ns_mem_sz = 0, sum_ns_pack_sz = 0;
static int sum_lq_num = 0, sum_gm_num = 0, sum_dt_num = 0, sum_sm_num = 0, sum_ot_num = 0, sum_ns_num = 0;

static void gatherTexStatistics(const ddsx::Header &hdr, TexQL tql, int mul, bool not_split)
{
  // gather statistics
  if (hdr.flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX)
    sum_dt_num += mul, sum_dt_mem_sz += mul * int(hdr.memSz), sum_dt_pack_sz += mul * int(hdr.packedSz);
  else if (hdr.flags & (ddsx::Header::FLG_GENMIP_BOX | ddsx::Header::FLG_GENMIP_KAIZER))
    sum_gm_num += mul, sum_gm_mem_sz += mul * int(hdr.memSz), sum_gm_pack_sz += mul * int(hdr.packedSz);
  else if (hdr.levels == 1)
    sum_sm_num += mul, sum_sm_mem_sz += mul * int(hdr.memSz), sum_sm_pack_sz += mul * int(hdr.packedSz);
  else
    sum_ot_num += mul, sum_ot_mem_sz += mul * int(hdr.memSz), sum_ot_pack_sz += mul * int(hdr.packedSz);

  if (not_split)
    sum_ns_num += mul, sum_ns_mem_sz += mul * int(hdr.memSz), sum_ns_pack_sz += mul * int(hdr.packedSz);
  else if (tql == TQL_base)
    sum_lq_num += mul, sum_lq_mem_sz += mul * int(hdr.memSz), sum_lq_pack_sz += mul * int(hdr.packedSz);
  else if (tql == TQL_high || tql == TQL_uhq)
    sum_hq_mem_sz += mul * int(hdr.memSz), sum_hq_pack_sz += mul * int(hdr.packedSz);
}

static OSSpinlock forceQTexListSL;
static TextureIdSet forcedMqTexList, forcedLqTexList;
static int get_texq(TEXTUREID id, int tex_quality)
{
  if (forcedMqTexList.size() + forcedLqTexList.size() == 0)
    return tex_quality;
  OSSpinlockScopedLock autolock(forceQTexListSL);
  int q = tex_quality;
  if (tex_quality >= 2) // low
    q = tex_quality;
  else if (tex_quality == 1) // medium
    q = forcedLqTexList.has(id) ? 2 : tex_quality;
  else if (forcedLqTexList.has(id))
    q = 2;
  else if (forcedMqTexList.has(id))
    q = 1;
  return q;
}

static void reload_active_array_textures();


class DDSxTexturePack2
{
  struct Rec
  {
    TEXTUREID texId;
    int ofs;
    int packedDataSize;
  };

  class Factory : public TextureFactory
  {
  public:
    Factory(DDSxTexturePack2 &p) : pack(p)
    {
      clear_and_resize(texProps, pack.texRec.size());
      mem_set_0(texProps);
    }
    ~Factory() { onTexFactoryDeleted(this); }

    virtual BaseTexture *createTexture(TEXTUREID id);
    virtual void releaseTexture(BaseTexture *texture, TEXTUREID id);
    bool isPersistentTexName(const char *nm) override
    {
      if (intptr_t(nm) > intptr_t(&pack) && intptr_t(nm) < intptr_t(this))
        return true;
      if (nm)
      {
        int i = pack.findRec(nm);
        if (i >= 0 && pack.texNames.map[i] == nm)
          return true;
      }
      return false;
    }

    bool scheduleTexLoading(TEXTUREID tid, TexQL req_ql) override
    {
      int idx = RMGR.toIndex(tid);
      if (idx < 0 || req_ql >= TQL__COUNT)
        return false;

      auto *tex_desc_p = RMGR.texDesc[idx].packRecIdx;
      G_ASSERTF_RETURN(tex_desc_p[TQL_base].pack >= 0 && ::tex_packs[tex_desc_p[TQL_base].pack].pack->file == this, false,
        "tid=0x%x bq_pack=%d(%p) this=%p", tid, tex_desc_p[TQL_base].pack,
        tex_desc_p[TQL_base].pack < 0 ? nullptr : ::tex_packs[tex_desc_p[TQL_base].pack].pack->file, this);
      G_ASSERTF_RETURN(tex_desc_p[req_ql].pack >= 0, false, "tid=0x%x  ql=%d from=%d:%d", tid, req_ql, tex_desc_p[req_ql].pack,
        tex_desc_p[req_ql].rec);
      tex_packs[tex_desc_p[req_ql].pack].pack->file->toLoadAdd(tex_desc_p[req_ql].rec);
      return true;
    }

    inline void toLoadAdd(int rec_id);
    inline void toLoadDone(Tab<DDSxTexturePack2::Rec *> &toLoad, int toload_idx, int cnt = 1);
    static inline bool isTexToBeLoaded(TEXTUREID tid)
    {
      int rc = RMGR.getRefCount(tid);
      return rc >= 0 && rc + RMGR.getBaseTexRc(tid) > 0 && RMGR.resQS[tid.index()].isReading();
    }
    inline bool noPendingLoads()
    {
      OSSpinlockScopedLock autolock(toLoadPendSL);
      return toLoadPend.empty();
    }

    void reloadActiveTextures(int prio, int pack_idx);
    bool performDelayedLoad(int prio);

    struct RtProps
    {
      int8_t ldPrio;
      uint8_t initHqPartLev : 4, curQID : 4;
    };

    DDSxTexturePack2 &pack;
    Tab<DDSxTexturePack2::Rec *> toLoadPend;
    SmallTab<RtProps, MidmemAlloc> texProps;
    WinCritSec critSec;
    OSSpinlock toLoadPendSL;
    const char *packName;

    static unsigned get_maxq_ddsx_flags(unsigned idx)
    {
      TexQL max_ql = RMGR.calcMaxQL(idx);
      if (max_ql != TQL_stub)
      {
        auto &p_idx = RMGR.texDesc[idx].packRecIdx[max_ql];
        if (p_idx.pack != -1)
          return tex_packs[p_idx.pack].pack->texHdr[p_idx.rec].flags;
      }
      return 0;
    }
  };

  RoNameMap texNames;
  PatchableTab<ddsx::Header> texHdr;
  PatchableTab<Rec> texRec;

public:
  const char *getTextureNameById(int record_idx) const { return texNames.map[record_idx]; }

  struct File : public Factory
  {
    File(DDSxTexturePack2 &p, const char *real_name, int real_name_len, //
      const char *eff_real_name, unsigned eff_base_ofs, unsigned eff_size, bool eff_non_cached) :
      Factory(p)
    {
      static int global_chunk_size = dfa_chunk_size(eff_real_name); // implicitly assume that all packages are on same fs

      chunk = global_chunk_size;
      if (eff_real_name && eff_real_name != real_name)
      {
        handle = dfa_open_for_read(eff_real_name, eff_non_cached);
        baseOfs = eff_base_ofs;
        size = eff_size;
        G_ASSERTF(handle && baseOfs != 0 && size && chunk, "eff_real_name=%s (name=%s) baseOfs=0x%x size=0x%x chunk=0x%x",
          eff_real_name, real_name, baseOfs, size, chunk);
      }

      memcpy(&name[0], real_name, real_name_len + 1); //< This is not error, I know what I am doing
      packName = name;
    }
    ~File()
    {
      baseOfs = size = chunk = 0;
      closeHandle();
    }

    void *handle = nullptr;
    unsigned baseOfs = 0, size = 0, chunk = 0;
    char name[sizeof(void *)];

    void *getHandle()
    {
      if (!handle)
        handle = dfa_open_for_read(name, true);
      return handle;
    }
    int getSize()
    {
      if (!size)
      {
        void *h = getHandle();
        size = h ? dfa_file_length(h) : 0;
      }
      return size;
    }
    void closeHandle()
    {
      if (handle && !baseOfs)
      {
        dfa_close(handle);
        handle = NULL;
      }
    }
  } *file;

public:
  static DDSxTexturePack2 *make(const char *name, bool can_override_tex, int pack_idx)
  {
    String cache_fname(0, "%scache.bin", name);
    VromReadHandle dump_data = ::vromfs_get_file_data(cache_fname);
    FullFileLoadCB crd(dump_data.data() ? cache_fname.str() : name);
    if (!crd.fileHandle)
      return NULL;

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
    if (dump_data.data())
    {
      const DataBlock *settings = dgs_get_settings();
      if (!settings || settings->getBlockByNameEx("debug")->getBool("checkPacksConsistency", true))
      {
        FullFileLoadCB dxp_crd(name);
        if (!dxp_crd.fileHandle)
        {
          logerr("'%s' not found", name);
          return NULL;
        }
        SmallTab<char, TmpmemAlloc> dxp_hdr;
        clear_and_resize(dxp_hdr, dump_data.size());
        if (dxp_crd.tryRead(dxp_hdr.data(), dxp_hdr.size()) != dxp_hdr.size())
        {
          logerr("inconsistent cache for %s (short dxp)", name);
          crd.open(name, DF_READ);
          if (!crd.fileHandle)
            return NULL;
        }
        if (!mem_eq(dump_data, dxp_hdr.data()))
        {
          logerr("inconsistent cache for %s (dxp header differs)", name);
          crd.open(name, DF_READ);
          if (!crd.fileHandle)
            return NULL;
        }
      }
    }
#endif

    unsigned file_base_ofs = 0, file_size = 0;
    bool file_non_cached = true;
    const char *eff_real_name = FastSeqReader::resolve_real_name(name, file_base_ofs, file_size, file_non_cached);
    const char *real_name = file_base_ofs && file_size ? name : eff_real_name;
    if (!eff_real_name)
    {
      logerr("missing <%s> (while %s presents in hdr-cache vrom)", name, cache_fname);
      return NULL;
    }

    unsigned hdr[4];
    crd.read(hdr, sizeof(hdr));

    if (hdr[0] != _MAKE4C('DxP2') || (hdr[1] != 2 && hdr[1] != 3))
    {
      debug("bad DXP header 0x%x 0x%x 0x%x 0x%x at '%s'", hdr[0], hdr[1], hdr[2], hdr[3], real_name);
      return NULL;
    }

    int real_name_len = i_strlen(real_name);
    DDSxTexturePack2 *pack = nullptr;
    unsigned file_struct_ofs = 0;
    if (hdr[1] == 3)
    {
      file_struct_ofs = (hdr[3] + 15) & ~15;
      pack = (DDSxTexturePack2 *)memalloc(file_struct_ofs + sizeof(File) + real_name_len, midmem);
      crd.read(pack, hdr[3]);
      pack->texNames.patchData(pack);
      pack->texHdr.patch(pack);
      pack->texRec.patch(pack);
    }
    else // if (hdr[1] == 2)
    {
      struct DDSxTexturePack2v2
      {
        struct RecV2
        {
          PATCHABLE_DATA64(BaseTexture *, tex_unused);
          TEXTUREID texId;
          int ofs;
          int packedDataSize;
          int _resv;
        };
        RoNameMap texNames;
        PatchableTab<ddsx::Header> texHdr;
        PatchableTab<RecV2> texRec;
      };

      DDSxTexturePack2v2 *packV2 = (DDSxTexturePack2v2 *)memalloc(hdr[3], tmpmem);
      crd.read(packV2, hdr[3]);
      packV2->texRec.patch(packV2);
      G_ASSERTF(uintptr_t(packV2->texRec.cend()) == uintptr_t(packV2) + hdr[3], "dumpSz=%d texRec.begin.ofs=%d texRec.end.ofs=%d",
        hdr[3], uintptr_t(packV2->texRec.cbegin()) - uintptr_t(packV2), uintptr_t(packV2->texRec.cend()) - uintptr_t(packV2));
      hdr[3] = uintptr_t(packV2->texRec.cbegin()) - uintptr_t(packV2) + sizeof(Rec) * packV2->texRec.size();

      file_struct_ofs = (hdr[3] + 15) & ~15;
      pack = (DDSxTexturePack2 *)memalloc(file_struct_ofs + sizeof(File) + real_name_len, midmem);
      memcpy(pack, packV2, uintptr_t(packV2->texRec.cbegin()) - uintptr_t(packV2)); //-V780
      pack->texNames.patchData(pack);
      pack->texHdr.patch(pack);
      pack->texRec.rebase(pack, packV2);
      for (unsigned ti = 0; ti < pack->texRec.size(); ti++)
      {
        auto &dest = pack->texRec[ti];
        auto &src = packV2->texRec[ti];
        dest.texId = BAD_TEXTUREID;
        dest.ofs = src.ofs;
        dest.packedDataSize = src.packedDataSize;
      }
      memfree(packV2, tmpmem);
    }

    pack->file = new ((void *)(uintptr_t(pack) + file_struct_ofs), _NEW_INPLACE)
      File(*pack, real_name, real_name_len, eff_real_name, file_base_ofs, file_size, file_non_cached);
    pack->initTex(can_override_tex, pack_idx);
    return pack;
  }

  void initTex(bool can_override_tex, int pack_idx)
  {
    String bqName;
    TextureMetaData tmd;
    int tex_cnt = 0;

    G_ASSERTF_AND_DO(ddsx::hq_tex_priority >= 0 && ddsx::hq_tex_priority < DXP_PRIO_COUNT,
      ddsx::hq_tex_priority = clamp(ddsx::hq_tex_priority, 0, DXP_PRIO_COUNT - 1), "ddsx::hq_tex_priority=%d DXP_PRIO_COUNT=%d",
      ddsx::hq_tex_priority, DXP_PRIO_COUNT);

    struct SuffixAndTql
    {
      const char *suff;
      TexQL tql;
    };
    const SuffixAndTql suff_tql[] = {{"$hq*", TQL_high}, {"$tq*", TQL_thumb}, {"$uhq*", TQL_uhq}, {nullptr, TQL_base}};

    for (int i = 0; i < texRec.size(); i++)
      for (const auto &s : suff_tql)
        if (s.tql == TQL_base)
        {
          if (TEXTUREID bq_tid = get_managed_texture_id(texNames.map[i]))
          {
            unsigned idx = bq_tid.index();
            auto &tex_desc = RMGR.texDesc[idx];
            if (tex_desc.packRecIdx[TQL_base].pack == tex_desc.packRecIdx[TQL_thumb].pack &&
                tex_desc.packRecIdx[TQL_base].rec == tex_desc.packRecIdx[TQL_thumb].rec)
            {
              if (texHdr[i].w && texHdr[i].h)
              {
                // update BQ
                unsigned base_lev = max(get_log2i(max(max(texHdr[i].w, texHdr[i].h), texHdr[i].depth)), 1u);
                RMGR.setFactory(idx, file);
                texNames.map[i] = // redirect to proper thumbnail tex name with metadata encoded
                  ::tex_packs[tex_desc.packRecIdx[TQL_thumb].pack].pack->texNames.map[tex_desc.packRecIdx[TQL_thumb].rec];
                fillTexRecBaseQL(i, pack_idx, bq_tid, base_lev, true);
                tex_cnt++;
              }

              if ((texHdr[i].w == 0 && texHdr[i].h == 0) || // empty BQ, so treat original TQ as effective BQ
                  RMGR.getLevDesc(idx, TQL_thumb) >= RMGR.getLevDesc(idx, TQL_base) ||                          // new BQ <= TQ
                  RMGR.getLevDesc(idx, TQL_thumb) < RMGR.texDesc[idx].dim.maxLev + 1 - RMGR.texDesc[idx].dim.l) // mips gap
              {
                // reset TQ
                RMGR.setLevDesc(idx, TQL_thumb, 0);
                tex_desc.packRecIdx[TQL_thumb].pack = -2;
                tex_desc.packRecIdx[TQL_thumb].rec = 0xFFFFu;
              }
              break;
            }
          }

          if (updateTexRecBaseQL(i, pack_idx, can_override_tex))
          {
            tmd.decode(texNames.map[i], &bqName);
            RMGR.texDesc[texRec[i].texId.index()].dim.stubIdx =
              (tmd.stubTexIdx == 15) ? -1 : tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(texHdr[i]));
            tex_cnt++;
          }
          break;
        }
        else if (const char *nm_suffix = strstr(texNames.map[i], s.suff))
        {
          if (texHdr[i].levels || can_override_tex)
          {
            bqName.printf(0, "%.*s*", nm_suffix - texNames.map[i].get(), texNames.map[i]);
            TEXTUREID bq_tid = get_managed_texture_id(bqName);
            if (bq_tid == BAD_TEXTUREID && s.tql == TQL_thumb)
            {
              memmove((char *)nm_suffix, (char *)nm_suffix + 3, strlen(nm_suffix) + 1 - 3); // remove '$tq'
              if (updateTexRecBaseQL(i, pack_idx, can_override_tex))
              {
                texmgr_internal::TexMgrAutoLock autoLock;
                RMGR.texDesc[texRec[i].texId.index()].dim.stubIdx = tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(texHdr[i]));
                bq_tid = get_managed_texture_id(bqName);

                auto &tex_desc = RMGR.texDesc[bq_tid.index()];
                RMGR.setLevDesc(bq_tid.index(), s.tql, RMGR.getLevDesc(bq_tid.index(), TQL_base));
                tex_desc.packRecIdx[s.tql].pack = pack_idx;
                tex_desc.packRecIdx[s.tql].rec = i;
                tex_cnt++;
                break;
              }
            }

            if (updateTexRec(i, pack_idx, bq_tid, s.tql, can_override_tex))
              tex_cnt++;
          }
          break;
        }

    for (int i = 0; i + 1 < texNames.map.size(); i++)
      G_ASSERT(strcmp(texNames.map[i], texNames.map[i + 1]) < 0);

    if (!tex_cnt)
      file->packName = NULL;
  }
  void termTex()
  {
    for (int i = 0; i < texRec.size(); i++)
    {
      TEXTUREID tid = texRec[i].texId;
      if (!RMGR.isValidID(tid, nullptr))
        continue;
      int idx = tid.index();
      if (RMGR.resQS[idx].isReading())
        RMGR.cancelReading(idx);
      if (idx >= 0 && RMGR.getFactory(idx) == file)
        evict_managed_tex_id(tid);
      texRec[i].texId = BAD_TEXTUREID;
    }
  }
  void updatePairedBaseTex(int pack)
  {
    texmgr_internal::TexMgrAutoLock autoLock;
    for (int i = 0; i < texRec.size(); i++)
    {
      TEXTUREID tid = texRec[i].texId;
      if (!RMGR.isValidID(tid, nullptr))
        continue;
      int idx = tid.index();
      if (RMGR.texDesc[idx].packRecIdx[TQL_base].pack != pack || RMGR.texDesc[idx].packRecIdx[TQL_base].rec != i)
        continue;
      TEXTUREID bt_tid = RMGR.pairedBaseTexId[idx];
      bool tex_has_hq = file->texProps[i].initHqPartLev != 0;
      if (!bt_tid && !tex_has_hq && (texHdr[i].flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX))
      {
        // pre-setup pairedBaseTexId, only for BQ without HQ; NOTE: when both BQ+HQ are used BQ never needs basetex
        TextureMetaData tmd;
        String tmpStor;
        tmd.decode(RMGR.getName(idx), &tmpStor);
        if (!tmd.baseTexName.empty())
        {
          String bt_nm(64, "%s*", tmd.baseTexName.str());
          bt_tid = get_managed_texture_id(bt_nm);
          if (bt_tid == BAD_TEXTUREID)
            logerr("texMgr: failed to resolve basetex: %s (for %s)", bt_nm, RMGR.getName(idx));
          RMGR.pairedBaseTexId[idx] = bt_tid;
        }
      }

      if (bt_tid && tex_has_hq && !(Factory::get_maxq_ddsx_flags(bt_tid.index()) & ddsx::Header::FLG_HOLD_SYSMEM_COPY))
      {
        RMGR.pairedBaseTexId[idx] = BAD_TEXTUREID; // drop pairedBaseTexId
        if (Factory::get_maxq_ddsx_flags(idx) & ddsx::Header::FLG_NEED_PAIRED_BASETEX)
        {
          logwarn("<%s>(q%d) needs paired basetex while <%s>(q%d) doesn't hold sysmem copy; reset to TQL_base", RMGR.getName(idx),
            (int)RMGR.calcMaxQL(idx), RMGR.getName(bt_tid.index()), (int)RMGR.calcMaxQL(bt_tid.index()));

          TexQL max_ql = RMGR.calcMaxQL(idx);
          auto &mq_ref = RMGR.texDesc[idx].packRecIdx[max_ql];
          auto &resQS = RMGR.resQS[idx];
          auto *mq_pack = ::tex_packs[mq_ref.pack].pack;
          if (int hq_part = file->texProps[i].initHqPartLev)
          {
            auto &mq_hdr = mq_pack->texHdr[mq_ref.rec];
            texHdr[i].hqPartLevels = 0;
            texHdr[i].mQmip = mq_hdr.mQmip > hq_part ? (mq_hdr.mQmip - hq_part) : 0;
            texHdr[i].lQmip = mq_hdr.lQmip > hq_part ? (mq_hdr.lQmip - hq_part) : 0;
            texHdr[i].uQmip = mq_hdr.uQmip - mq_hdr.levels;
          }
          file->texProps[i].ldPrio = 0;

          auto &tex_desc = RMGR.texDesc[idx];
          int dlev = tex_desc.dim.maxLev - RMGR.getLevDesc(idx, RMGR.calcMaxQL(idx)) + file->texProps[i].initHqPartLev;
          tex_desc.dim.w = max(tex_desc.dim.w >> dlev, 1);
          tex_desc.dim.h = max(tex_desc.dim.h >> dlev, 1);
          tex_desc.dim.d = max(tex_desc.dim.d >> dlev, 1);
          tex_desc.dim.l -= dlev;
          tex_desc.dim.maxLev -= dlev;

          gatherTexStatistics(mq_pack->texHdr[mq_ref.rec], max_ql, -1, false);
          mq_pack->texRec[mq_ref.rec].texId = BAD_TEXTUREID;
          mq_ref.pack = -1;
          mq_ref.rec = 0xFFFFu;

          RMGR.setLevDesc(idx, max_ql, 0);
          resQS.setQLev(tex_desc.dim.maxLev);
          resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));
        }
      }
    }
  }
  void deleteFile() { file->~File(); }

  bool updateTexRecBaseQL(int i, int pack_idx, bool can_override_tex)
  {
    TEXTUREID tid = get_managed_texture_id(texNames.map[i]);
    if (tid != BAD_TEXTUREID)
    {
      if (!can_override_tex)
      {
        debug("%s texname conflict, skipping tex %s:%d", texNames.map[i], file->name, i);
        return false;
      }
      if (int rc = get_managed_texture_refcount(tid))
      {
        logerr("%s already created, rc=%d, skipping tex %s:%d", texNames.map[i], rc, file->name, i);
        return false;
      }
      if (RMGR.texDesc[tid.index()].packRecIdx[TQL_base].pack < 0)
      {
        logerr("%s is registered as non-DxP texture, skipping tex %s:%d", texNames.map[i], file->name, i);
        return false;
      }
    }

    unsigned base_lev = max(get_log2i(max(max(texHdr[i].w, texHdr[i].h), texHdr[i].depth)), 1u);
    if (tid != BAD_TEXTUREID)
    {
      int idx = tid.index();
      auto &tex_desc = RMGR.texDesc[idx];

      // re-check existing TQ records on BQ texture patch
      if (unsigned tq_lev = RMGR.getLevDesc(idx, TQL_thumb))
      {
        auto &tq_ref = tex_desc.packRecIdx[TQL_thumb];
        ::tex_packs[tq_ref.pack].pack->file->pack.texRec[tq_ref.rec].texId = BAD_TEXTUREID; // previous texId will be evicted!
        if (tq_lev >= base_lev)
        {
          // drop TQ since it is greater than new BQ
          RMGR.setLevDesc(idx, TQL_thumb, 0);
          tq_ref.pack = -1;
          tq_ref.rec = 0xFFFFu;
        }
      }

      // re-check existing HQ records on BQ texture patch
      for (TexQL tql_hq = TexQL(TQL_base + 1); tql_hq < TQL__COUNT; tql_hq = TexQL(tql_hq + 1))
        if (RMGR.getLevDesc(idx, tql_hq) > 0)
        {
          auto &hq_ref = tex_desc.packRecIdx[tql_hq];
          auto *hq_pack = ::tex_packs[hq_ref.pack].pack;
          auto &hq_hdr = hq_pack->texHdr[hq_ref.rec];

          hq_pack->texRec[hq_ref.rec].texId = BAD_TEXTUREID; // previous texId will be evicted!
          if (RMGR.getLevDesc(idx, TQL_base) != base_lev || hq_hdr.d3dFormat != texHdr[i].d3dFormat ||
              max(hq_hdr.w >> hq_hdr.levels, 1) != texHdr[i].w || max(hq_hdr.h >> hq_hdr.levels, 1) != texHdr[i].h)
          {
            if (RMGR.getLevDesc(idx, TQL_base) != base_lev)
              logwarn("skip HQ re-apply after BQ patch - BQ.lev changes %d->%d for %s", RMGR.getLevDesc(idx, TQL_base), base_lev,
                texNames.map[i]);
            else
              logwarn("skip HQ re-apply after BQ patch - non-matching base %dx%d lev=%d/%d fmt=%X for %s (%s) %dx%d lev=%d fmt=%X",
                texHdr[i].w, texHdr[i].h, texHdr[i].levels, texHdr[i].hqPartLevels, texHdr[i].d3dFormat,
                hq_pack->texNames.map[hq_ref.rec], hq_pack->file->name, hq_hdr.w, hq_hdr.h, hq_hdr.levels, hq_hdr.d3dFormat);
            RMGR.setLevDesc(idx, tql_hq, 0);
            gatherTexStatistics(hq_hdr, tql_hq, -1, false);
            hq_ref.pack = -1;
            hq_ref.rec = 0xFFFFu;
          }
          else
          {
            hq_hdr.mQmip = texHdr[i].mQmip;
            hq_hdr.lQmip = texHdr[i].lQmip;
            hq_hdr.uQmip = texHdr[i].uQmip + hq_hdr.levels;
            texHdr[i].hqPartLevels = 0;
            debug("patch: tex <%s> route to %s, reuse HQ from %s", texNames.map[i].get(), file->name, hq_pack->file->name);
          }
        }

      // remove older BQ record on patch
      auto &bq_ref = tex_desc.packRecIdx[TQL_base];
      DDSxTexturePack2 *prev_pack = ::tex_packs[bq_ref.pack].pack;
      prev_pack->texRec[bq_ref.rec].texId = BAD_TEXTUREID;

      debug("patch: tex <%s> route to %s", texNames.map[i].get(), file->name);
      evict_managed_tex_id(tid);
    }

    // register texture ID
    tid = texRec[i].texId = add_managed_texture(texNames.map[i], file);
    if (tid == BAD_TEXTUREID) // unlikely but better check
    {
      logerr("cannot add texId for %s (%s:%d)", texNames.map[i], file->name, i);
      return false;
    }
    fillTexRecBaseQL(i, pack_idx, tid, base_lev, can_override_tex);
    return true;
  }
  void fillTexRecBaseQL(int i, int pack_idx, TEXTUREID tid, unsigned base_lev, bool can_override_tex)
  {
    unsigned idx = tid.index();
    texRec[i].texId = tid;
    file->texProps[i].initHqPartLev = texHdr[i].hqPartLevels;
    if (dd_stricmp(RMGR.getName(idx), texNames.map[i]) != 0)
    {
      logwarn("texMgr: fix %s -> %s", RMGR.getName(idx), texNames.map[i]);
      RMGR.setName(idx, texNames.map[i], file);
    }
    if (texHdr[i].hqPartLevels && (texHdr[i].flags & (ddsx::Header::FLG_NEED_PAIRED_BASETEX | ddsx::Header::FLG_HOLD_SYSMEM_COPY)))
    {
      logwarn("texMgr: fix %s extra texHdr[%d].flags=0x%08X -> 0x%08X (%dx%d,L%d hqPartLevels=%d)", texNames.map[i], i,
        texHdr[i].flags, texHdr[i].flags & ~(ddsx::Header::FLG_NEED_PAIRED_BASETEX | ddsx::Header::FLG_HOLD_SYSMEM_COPY), texHdr[i].w,
        texHdr[i].h, texHdr[i].levels, texHdr[i].hqPartLevels);
      texHdr[i].flags &= ~(ddsx::Header::FLG_NEED_PAIRED_BASETEX | ddsx::Header::FLG_HOLD_SYSMEM_COPY);
    }

    auto &desc = RMGR.texDesc[idx];
    RMGR.setLevDesc(idx, TQL_base, base_lev);
    if (RMGR.calcMaxQL(idx) <= TQL_base)
    {
      // only when HQ was not reused after patching base earlier
      desc.dim.w = texHdr[i].w;
      desc.dim.h = texHdr[i].h;
      desc.dim.d = texHdr[i].depth;
      desc.dim.l = texHdr[i].levels;
      desc.dim.maxLev = base_lev;
    }
    desc.dim.bqGenmip =
      !texHdr[i].hqPartLevels && (texHdr[i].flags & (ddsx::Header::FLG_NEED_PAIRED_BASETEX | ddsx::Header::FLG_HOLD_SYSMEM_COPY)) != 0;

    auto &resQS = RMGR.resQS[idx];
    resQS.setQLev(desc.dim.maxLev);
    resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));
    desc.packRecIdx[TQL_base].pack = pack_idx;
    desc.packRecIdx[TQL_base].rec = i;

    if (can_override_tex) // update texId for all other parts (TQ, HQ, UHQ) after texture patch
      for (TexQL tql = TQL__FIRST; tql < TQL__COUNT; tql = TexQL(tql + 1))
        if (tql != TQL_base && RMGR.getLevDesc(idx, tql))
        {
          G_ASSERT_CONTINUE(desc.packRecIdx[tql].pack >= 0);
          ::tex_packs[desc.packRecIdx[tql].pack].pack->texRec[desc.packRecIdx[tql].rec].texId = tid;
        }
    gatherTexStatistics(texHdr[i], TQL_base, +1, true);
  }

  bool updateTexRec(int i, int pack_idx, TEXTUREID tid, TexQL tql, bool can_override_tex)
  {
    G_ASSERT_RETURN(tql != TQL_base && tql < TQL__COUNT, false);
    int idx = RMGR.toIndex(tid);
    if (idx < 0 || RMGR.texDesc[idx].packRecIdx[TQL_base].pack < 0)
    {
      if (tql == TQL_thumb)
      {
        logwarn("BQ.tex not registered before %s, TQ is not used", texNames.map[i]);
        return false;
      }
      if (texHdr[i].levels)
        debug("skip - no base found %s %p:%d", texNames.map[i], idx < 0 ? NULL : RMGR.getFactory(idx), idx);
      return false;
    }

    texmgr_internal::TexMgrAutoLock autoLock;
    auto &tex_desc = RMGR.texDesc[idx];
    auto &dq_ref = tex_desc.packRecIdx[tql];
    int dq_lev = get_log2i(max(max(texHdr[i].w, texHdr[i].h), texHdr[i].depth));

    // set TQ record
    if (tql == TQL_thumb)
    {
      bool bad_tq =
        !texHdr[i].levels || dq_lev < 2 || dq_lev >= RMGR.getLevDesc(idx, TQL_base) || dq_lev <= tex_desc.dim.maxLev - tex_desc.dim.l;
      if (dq_ref.pack >= 0 && bad_tq)
      {
        ::tex_packs[dq_ref.pack].pack->texRec[dq_ref.rec].texId = BAD_TEXTUREID;
        dq_ref.pack = -1;
        dq_ref.rec = 0xFFFFu;
        RMGR.setLevDesc(idx, tql, 0);
      }
      if (bad_tq)
        return false;

      RMGR.setLevDesc(idx, tql, dq_lev);
      if (tex_desc.packRecIdx[TQL_base].pack == tex_desc.packRecIdx[TQL_thumb].pack &&
          tex_desc.packRecIdx[TQL_base].rec == tex_desc.packRecIdx[TQL_thumb].rec)
      {
        tex_desc.packRecIdx[TQL_base].pack = pack_idx;
        tex_desc.packRecIdx[TQL_base].rec = i;
      }
      tex_desc.packRecIdx[tql].pack = pack_idx;
      tex_desc.packRecIdx[tql].rec = i;
      texRec[i].texId = tid; // mark tqTex with the same texId as baseTex
      return true;
    }

    // set HQ records
    auto &bq_ref = tex_desc.packRecIdx[TQL_base];
    auto *base_pack = ::tex_packs[bq_ref.pack].pack;
    auto &base_hdr = base_pack->texHdr[bq_ref.rec];
    auto &base_props = base_pack->file->texProps[bq_ref.rec];
    bool was_unsplit = RMGR.calcMaxQL(idx) == TQL_base;
    auto prev_dim = tex_desc.dim;

    if (dq_ref.pack >= 0)
    {
      auto *dq_pack = ::tex_packs[dq_ref.pack].pack;
      if (can_override_tex && tql >= TQL_high) //-V560
      {
        // restore base texHdr to apply HQ once again
        auto &dq_hdr = dq_pack->texHdr[dq_ref.rec];
        base_hdr.hqPartLevels = base_props.initHqPartLev;
        base_hdr.mQmip = dq_hdr.mQmip;
        base_hdr.lQmip = dq_hdr.lQmip;
        base_hdr.uQmip = dq_hdr.uQmip - dq_hdr.levels;

        RMGR.setLevDesc(idx, tql, 0);
        if (RMGR.calcMaxQL(idx) < tql)
        {
          int dlev = tex_desc.dim.maxLev - RMGR.getLevDesc(idx, RMGR.calcMaxQL(idx));
          tex_desc.dim.w = max(tex_desc.dim.w >> dlev, 1);
          tex_desc.dim.h = max(tex_desc.dim.h >> dlev, 1);
          tex_desc.dim.d = max(tex_desc.dim.d >> dlev, 1);
          tex_desc.dim.l -= dlev;
          tex_desc.dim.maxLev -= dlev;

          if (RMGR.calcMaxQL(idx) == TQL_base)
            G_ASSERTF(tex_desc.dim.w == base_hdr.w && tex_desc.dim.h == base_hdr.h && tex_desc.dim.d == max<int>(base_hdr.depth, 1) &&
                        dlev == dq_hdr.levels,
              "dim=%dx%dx%d,L%d base_hdr=%dx%dx%d,L%d dlev=%d dq_hdr.levels=%d", tex_desc.dim.w, tex_desc.dim.h, tex_desc.dim.d,
              tex_desc.dim.l, base_hdr.w, base_hdr.h, base_hdr.depth, base_hdr.levels, dlev, dq_hdr.levels);
        }
        auto &resQS = RMGR.resQS[idx];
        resQS.setQLev(tex_desc.dim.maxLev);
        resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));

        dq_pack->texRec[dq_ref.rec].texId = BAD_TEXTUREID;
        gatherTexStatistics(dq_hdr, tql, -1, was_unsplit);
        dq_ref.pack = -1;
        dq_ref.rec = 0xFFFFu;
        // debug("patch: tex <%s> route to %s", texNames.map[i].get(), file->name);
      }
      else if (!can_override_tex)
        logerr("tex name[%d] conflict: '%s' (%s and %s:%d)", i, texNames.map[i].get(), file->name, dq_pack->file->name, dq_ref.rec);
    }
    if (texHdr[i].levels == 0)
      return false;

    if (texHdr[i].d3dFormat != base_hdr.d3dFormat || max(texHdr[i].w >> texHdr[i].levels, 1) != base_hdr.w ||
        max(texHdr[i].h >> texHdr[i].levels, 1) != base_hdr.h)
    {
      logwarn("skip - non-matching base %dx%d lev=%d/%d fmt=%X for %s (%s) %dx%d lev=%d fmt=%X", base_hdr.w, base_hdr.h,
        base_hdr.levels, base_hdr.hqPartLevels, base_hdr.d3dFormat, texNames.map[i], file->name, texHdr[i].w, texHdr[i].h,
        texHdr[i].levels, texHdr[i].d3dFormat);
      return false;
    }
    else if (base_hdr.hqPartLevels != texHdr[i].levels)
    {
      if (tql != TQL_uhq && tex_desc.packRecIdx[TQL_thumb].pack != -2)
        logwarn("fixed - non-matching hqPartLevels (%d!=%d) for %s (%s)", base_hdr.hqPartLevels, texHdr[i].levels, texNames.map[i],
          file->name);
      base_hdr.hqPartLevels = texHdr[i].levels;
    }

    texHdr[i].mQmip = base_hdr.mQmip;
    texHdr[i].lQmip = base_hdr.lQmip;
    texHdr[i].uQmip = base_hdr.uQmip + texHdr[i].levels;
    base_hdr.hqPartLevels = 0;

    if (tql >= TQL_high)
      file->texProps[i].ldPrio = ddsx::hq_tex_priority;
    texRec[i].texId = tid; // mark hqTex with the same texId as baseTex to apply get_texq() properly

    RMGR.setLevDesc(idx, tql, dq_lev);
    if (RMGR.calcMaxQL(idx) <= tql)
    {
      int dlev = RMGR.getLevDesc(idx, RMGR.calcMaxQL(idx)) - tex_desc.dim.maxLev;
      tex_desc.dim.w = texHdr[i].w;
      tex_desc.dim.h = texHdr[i].h;
      tex_desc.dim.d = texHdr[i].depth;
      tex_desc.dim.l += dlev;
      tex_desc.dim.maxLev = RMGR.getLevDesc(idx, tql);
      G_ASSERTF(tex_desc.dim.l <= tex_desc.dim.maxLev + 1, "dim=%dx%dx%d,L%d maxLev=%d", tex_desc.dim.w, tex_desc.dim.h,
        tex_desc.dim.d, tex_desc.dim.l, tex_desc.dim.maxLev);
    }

    auto &resQS = RMGR.resQS[idx];
    resQS.setQLev(tex_desc.dim.maxLev);
    resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));
    tex_desc.packRecIdx[tql].pack = pack_idx;
    tex_desc.packRecIdx[tql].rec = i;

    if (tql == TQL_uhq) //== right now UHQ replaces HQ
    {
      int hq_pack = tex_desc.packRecIdx[TQL_high].pack;
      if (hq_pack != -1)
        gatherTexStatistics(::tex_packs[hq_pack].pack->texHdr[tex_desc.packRecIdx[TQL_high].rec], TQL_high, -1, false);
      tex_desc.packRecIdx[TQL_high].pack = -1;
      tex_desc.packRecIdx[TQL_high].rec = 0xFFFFu;
      RMGR.setLevDesc(idx, TQL_high, 0);
    }

    // pre-setup pairedBaseTexId to be available before createTexture() call
    if (texHdr[i].flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX)
    {
      const char *name = get_managed_texture_name(tid);
      RMGR.setupQLev(idx, get_texq(tid, ::dgs_tex_quality), texHdr[i]);
      if (RMGR.resQS[idx].getQLev() > RMGR.getLevDesc(idx, TQL_base))
      {
        TextureMetaData tmd;
        String tmpStor;
        tmd.decode(name, &tmpStor);
        if (!tmd.baseTexName.empty())
        {
          String bt_nm(64, "%s*", tmd.baseTexName.str());
          TEXTUREID bt_tid = get_managed_texture_id(bt_nm);
          if (bt_tid == BAD_TEXTUREID)
            logerr("texMgr: failed to resolve basetex: %s (for %s)", bt_nm, name);
          RMGR.pairedBaseTexId[idx] = bt_tid;
        }
      }
    }

    // update statistics
    if (was_unsplit)
    {
      gatherTexStatistics(base_hdr, TQL_base, -1, true);
      gatherTexStatistics(base_hdr, TQL_base, +1, false);
    }
    gatherTexStatistics(texHdr[i], tql, +1, false);
    if (prev_dim.l >= tex_desc.dim.l || prev_dim.maxLev >= tex_desc.dim.maxLev)
      if (BaseTexture *bt = RMGR.baseTexture(idx))
      {
        RMGR.downgradeTexQuality(idx, *bt, max<int>(RMGR.getLevDesc(idx, TQL_thumb), 1));
        logwarn("  %s already created, discard tex to apply changes", RMGR.getName(idx));
      }
    return true;
  }

  int findRec(const char *name) { return texNames.getNameId(name); }
  bool isRecValid(int rec) const { return (rec >= 0 && rec < texRec.size()) ? texRec[rec].texId != BAD_TEXTUREID : false; }

  friend bool ddsx::read_ddsx_contents(const char *tex_name, Tab<char> &out_data, ddsx::DDSxDataPublicHdr &out_desc);
  friend int ddsx::read_ddsx_header(const char *tex_name, ddsx::DDSxDataPublicHdr &out_desc, bool full_quality);
  friend struct DDSxArrayTextureFactory;
  friend bool is_managed_texture_incomplete(TEXTUREID tid);
};

static FastSeqReader *fastSeqCrdP[DXP_PRIO_COUNT] = {NULL};
static SmallTab<FastSeqReader::Range, InimemAlloc> rangesBufP[DXP_PRIO_COUNT];
static volatile int processingTexData[DXP_PRIO_COUNT] = {0};
static volatile int processingTexDataScheduled[DXP_PRIO_COUNT] = {0};
static volatile int pendingTexCount[DXP_PRIO_COUNT] = {0};

inline void DDSxTexturePack2::Factory::toLoadAdd(int rec_id)
{
  if (texmgr_internal::dbg_texq_only_stubs)
    return;
  OSSpinlockScopedLock autolock(toLoadPendSL);
  toLoadPend.push_back(&pack.texRec[rec_id]);
  interlocked_increment(pendingTexCount[texProps[rec_id].ldPrio]);
}
inline void DDSxTexturePack2::Factory::toLoadDone(Tab<DDSxTexturePack2::Rec *> &toLoad, int toload_idx, int cnt)
{
  for (int i = toload_idx; i < toload_idx + cnt; i++)
    interlocked_decrement(pendingTexCount[texProps[toLoad[i] - pack.texRec.data()].ldPrio]);
  erase_items(toLoad, toload_idx, cnt);
}
int texmgr_internal::get_pending_tex_prio(int prio)
{
  G_ASSERT(prio >= 0 && prio < DXP_PRIO_COUNT);
  return interlocked_relaxed_load(pendingTexCount[prio]);
}
bool texmgr_internal::should_start_load_prio(int prio)
{
  G_ASSERT(prio >= 0 && prio < DXP_PRIO_COUNT);
  return (interlocked_relaxed_load(pendingTexCount[prio]) || (prio == 0 && ddsxLoadQueue.size())) &&
         !interlocked_relaxed_load(processingTexData[prio]) && !interlocked_relaxed_load(processingTexDataScheduled[prio]);
}

bool isDiffuseTextureSuitableForMipColorization(const char *name)
{
  static bool on = dgs_get_settings()->getBlockByNameEx("debug")->getBool("debugColorizeMipmapsForDiffuseTextures", false);
  return on && name && (strstr(name, "tex_d") || strstr(name, "_c*"));
}

static void patchHeader(ddsx::Header &hdr, const char *name)
{
  G_UNUSED(hdr);
  G_UNUSED(name);
#if DAGOR_DBGLEVEL > 0
  // force uncompressed texture for mip vizualisation
  bool is2d = !(hdr.flags & (ddsx::Header::FLG_CUBTEX | ddsx::Header::FLG_VOLTEX | ddsx::Header::FLG_ARRTEX));
  if (isDiffuseTextureSuitableForMipColorization(name) && is2d) // D3DFMT_A8R8G8B8 = 21,
    hdr.d3dFormat = 21;
#endif
}

BaseTexture *DDSxTexturePack2::Factory::createTexture(TEXTUREID tid)
{
  const char *name = get_managed_texture_name(tid);
  G_ASSERT(name && *name);
  int rec_id = pack.texNames.getNameId(name);
  RMGR_TRACE("create <%s>", name);
  if (rec_id == -1)
    DAG_FATAL("not found %s", name);

  int idx = RMGR.toIndex(tid);
  TextureMetaData tmd;
  String stor;
  tmd.decode(name, &stor);

  ddsx::Header hdr = pack.texHdr[rec_id];
  hdr.w = RMGR.texDesc[idx].dim.w;
  hdr.h = RMGR.texDesc[idx].dim.h;
  hdr.depth = RMGR.texDesc[idx].dim.d;
  hdr.levels = RMGR.texDesc[idx].dim.l;
  if (hdr.hqPartLevels) // this means than BQ texture is scanned but HQ is missing (not scanned)
  {
    hdr.mQmip = hdr.mQmip > hdr.hqPartLevels ? (hdr.mQmip - hdr.hqPartLevels) : 0;
    hdr.lQmip = hdr.lQmip > hdr.hqPartLevels ? (hdr.lQmip - hdr.hqPartLevels) : 0;
    hdr.hqPartLevels = 0;
  }
  else if (tql::lq_not_more_than_split_bq_difftex)
  {
    TexQL max_ql = RMGR.calcMaxQL(idx);
    if (max_ql > TQL_base && max_ql != TQL_stub) // copy important flags from HQ part to be used later to setup hdr.lQmip
    {
      auto &p_idx = RMGR.texDesc[idx].packRecIdx[max_ql];
      hdr.flags |= tex_packs[p_idx.pack].pack->texHdr[p_idx.rec].flags & (hdr.FLG_HOLD_SYSMEM_COPY | hdr.FLG_NEED_PAIRED_BASETEX);
    }
  }
  unsigned skip_lev_to_bq = RMGR.texDesc[idx].dim.maxLev - RMGR.getLevDesc(idx, TQL_base);
  if (hdr.lQmip < skip_lev_to_bq)
    if (tql::lq_not_more_than_split_bq ||
        (tql::lq_not_more_than_split_bq_difftex && (hdr.flags & (hdr.FLG_NEED_PAIRED_BASETEX | hdr.FLG_HOLD_SYSMEM_COPY))))
      hdr.lQmip = skip_lev_to_bq;
  hdr.flags &= ~(hdr.FLG_HQ_PART | hdr.FLG_HOLD_SYSMEM_COPY | hdr.FLG_NEED_PAIRED_BASETEX);

  texProps[rec_id].curQID = get_texq(tid, ::dgs_tex_quality);
  RMGR.setupQLev(idx, texProps[rec_id].curQID, hdr);
  RMGR_TRACE("allocate tex %dx%dx%d,L%d  skip_lev=%d (q[%d]=%d qmip={%d,%d,%d}) %s", hdr.w, hdr.h, hdr.depth, hdr.levels,
    RMGR.texDesc[idx].dim.maxLev - RMGR.resQS[idx].getQLev(), texProps[rec_id].curQID, hdr.getSkipLevelsFromQ(texProps[rec_id].curQID),
    hdr.mQmip, hdr.lQmip, hdr.uQmip, name);

  TEXTUREID bt_tid = BAD_TEXTUREID;
  if (!tmd.baseTexName.empty() && RMGR.mayNeedBaseTex(idx, RMGR.resQS[idx].getQLev()))
  {
    String bt_nm(64, "%s*", tmd.baseTexName.str());
    bt_tid = get_managed_texture_id(bt_nm);
    if (bt_tid == BAD_TEXTUREID)
      logerr("texMgr: failed to resolve basetex: %s (for %s)", bt_nm, name);
  }

  patchHeader(hdr, name);
  BaseTexture *t = d3d::alloc_ddsx_tex(hdr, TEXCF_RGB | TEXCF_ABEST | TEXCF_LOADONCE, texProps[rec_id].curQID, 0, name,
    RMGR.texDesc[idx].dim.stubIdx);
  G_ASSERTF_RETURN(t, nullptr, "d3d::alloc_ddsx_tex(%s) failed with error '%s'", name, d3d::get_last_error());

  // force the same addrU,addrV for all DDSx headers (may be used during texture part load)
  for (const auto &idx : RMGR.texDesc[idx].packRecIdx)
    if (idx.pack >= 0 && idx.rec != 0xFFFFu)
      tex_packs[idx.pack].pack->texHdr[idx.rec].setAddr(tmd.d3dTexAddr(tmd.addrU), tmd.d3dTexAddr(tmd.addrV));

  RMGR.initResQS(idx, (texmgr_internal::texq_load_on_demand && RMGR.texDesc[idx].dim.stubIdx >= 0) ? TQL_stub : TQL_high);
  bool tex_has_hq = texProps[rec_id].initHqPartLev != 0;
  if (bt_tid && tex_has_hq && !(get_maxq_ddsx_flags(bt_tid.index()) & ddsx::Header::FLG_HOLD_SYSMEM_COPY))
  {
    if (RMGR.pairedBaseTexId[idx])
      logwarn("pairedBaseTexId(%s)=%s -> %s: dont use BT due to bt.flags=0x%x", RMGR.getName(idx),
        RMGR.getName(RMGR.pairedBaseTexId[idx].index()), RMGR.getName(bt_tid.index()), get_maxq_ddsx_flags(bt_tid.index()));
    bt_tid = BAD_TEXTUREID;
  }
  RMGR.pairedBaseTexId[idx] = bt_tid;
  t->setTID(tid);

  if (RMGR.resQS[idx].getMaxReqLev() > 1)
    RMGR.scheduleReading(idx, this);
  return t;
}
void DDSxTexturePack2::Factory::releaseTexture(BaseTexture *texture, TEXTUREID id)
{
  if (!critSec.tryLock())
  {
    bool intr = is_main_thread() ? ddsx::interrupt_texq_loading() : false;

    static const int ATTEMPTS_USEC = is_main_thread() ? 10000 : 200000;
    for (int64_t reft = profile_ref_ticks();;)
      if (critSec.tryLock())
        break;
      else if (!profile_usec_passed(reft, ATTEMPTS_USEC))
        sleep_msec(1);
      else
      {
        TEX_REC_LOCK();
        int idx = RMGR.toIndex(id);
        bool downgr = false;
        BaseTexture *bt = RMGR.baseTexture(idx);
        if (!RMGR.resQS[idx].isReading() && bt && !RMGR.getRefCount(idx))
        {
          if (RMGR.getTexMemSize4K(idx) > 8)
            downgr = RMGR.downgradeTexQuality(idx, *bt, max<int>(RMGR.getLevDesc(idx, TQL_thumb), 1));
          if (!RMGR.getBaseTexUsedCount(idx))
            RMGR.replaceTexBaseData(idx, nullptr);
        }
        TEX_REC_UNLOCK();
        logwarn("[0x%x].%s t=%p can't be released right now%s", id, RMGR.getName(idx), texture, downgr ? ", discarded" : "");
        ddsx::restore_texq_loading(intr);
        return;
      }
    ddsx::restore_texq_loading(intr);
  }

  const char *name = get_managed_texture_name(id);
  int rec_id = pack.texNames.getNameId(name);
  RMGR_TRACE("release %s", name);
  if (rec_id == -1)
    DAG_FATAL("not found %s", name);

  {
    TEX_REC_LOCK();
    int idx = RMGR.toIndex(id);
    if (!RMGR.baseTexture(idx) || RMGR.getRefCount(idx) ||
        (RMGR.resQS[idx].isReading() && texmgr_internal::texq_load_on_demand && !texmgr_internal::dbg_texq_only_stubs))
    {
      if (RMGR.baseTexture(idx))
        logwarn("[0x%x].%s t=%p can't be released right now: rdLev=%d refcount=%d", id, RMGR.getName(idx), texture,
          RMGR.resQS[idx].getRdLev(), RMGR.getRefCount(idx));
      TEX_REC_UNLOCK();
      critSec.unlock();
      return;
    }
    RMGR.setD3dRes(idx, nullptr);
    if (RMGR.resQS[idx].isReading())
      RMGR.cancelReading(idx);
    RMGR.markUpdated(idx, 0);
    TEX_REC_UNLOCK();
  }

  critSec.unlock();
  del_d3dres(texture);
}

void DDSxTexturePack2::Factory::reloadActiveTextures(int prio, int pack_idx)
{
  critSec.lock();
  int ptc_start[DXP_PRIO_COUNT];
  memcpy(ptc_start, (void *)pendingTexCount, sizeof(ptc_start));

  bool has_tex = false;
  for (int i = 0; i < pack.texNames.nameCount(); i++)
  {
    if (!RMGR.getD3dRes(pack.texRec[i].texId))
      continue;
    TEXTUREID tid = pack.texRec[i].texId;
    int rc = RMGR.getRefCount(tid);
    if (rc > 0)
    {
      if (prio >= 0 && prio != texProps[i].ldPrio)
        continue;
      int idx = tid.index();

      // update QLev (depends on dgs_tex_quality)
      if (RMGR.texDesc[idx].packRecIdx[TQL_base].pack == pack_idx && RMGR.texDesc[idx].packRecIdx[TQL_base].rec == i)
      {
        ddsx::Header hdr = pack.texHdr[i];
        if (hdr.hqPartLevels) // this means than BQ texture is scanned but HQ is missing (not scanned)
        {
          hdr.mQmip = hdr.mQmip > hdr.hqPartLevels ? (hdr.mQmip - hdr.hqPartLevels) : 0;
          hdr.lQmip = hdr.lQmip > hdr.hqPartLevels ? (hdr.lQmip - hdr.hqPartLevels) : 0;
          hdr.hqPartLevels = 0;
        }
        unsigned prev_qlev = RMGR.resQS[idx].getQLev();
        RMGR.setupQLev(idx, texProps[i].curQID = get_texq(tid, ::dgs_tex_quality), hdr);

        if (prev_qlev != RMGR.resQS[idx].getQLev() && RMGR.texDesc[idx].dim.stubIdx < 0)
          if (BaseTexture *t = RMGR.baseTexture(idx))
          {
            // we are loosing current tex content, so we must load it from scratch
            // to do so, reset loaded level
            RMGR.markUpdated(idx, 0);

            hdr.w = RMGR.texDesc[idx].dim.w;
            hdr.h = RMGR.texDesc[idx].dim.h;
            hdr.depth = RMGR.texDesc[idx].dim.d;
            hdr.levels = RMGR.texDesc[idx].dim.l;
            BaseTexture *new_t =
              d3d::alloc_ddsx_tex(hdr, TEXCF_RGB | TEXCF_ABEST | TEXCF_LOADONCE, texProps[i].curQID, 0, RMGR.getName(idx), -1);
            TextureInfo ti;
            new_t->getinfo(ti, 0);

            t->replaceTexResObject(new_t);
            debug("%*srecreate stub-less tex %dx%dx%d,L%d  %s", 9, "", ti.w, ti.h, ti.d, ti.mipLevels, pack.texNames.map[i]);
          }
      }

      if (!RMGR.scheduleReading(idx, RMGR.getFactory(idx)))
        continue;
      if (!has_tex)
      {
        has_tex = true;
        debug("reload %s:", packName);
      }
      debug("  [%4d] (0x%08x, %s) ref=%d qlev=%d", i, tid, pack.texNames.map[i], rc, RMGR.resQS[idx].getQLev());
    }
    else
      discard_unused_managed_texture(tid);
  }
  for (int i = 0; i < DXP_PRIO_COUNT; i++)
    if (interlocked_acquire_load(pendingTexCount[i]) != ptc_start[i])
      debug("should reload prio=%d", i);
  critSec.unlock();
}

bool DDSxTexturePack2::Factory::performDelayedLoad(int prio)
{
  if (noPendingLoads())
    return false;
  if (texmgr_internal::dbg_texq_only_stubs)
    return false;
  bool ddsx_dec_single_thread = (prio & 0x100) ? true : false;
  int dbg_sleep = (prio & 0x200) ? texmgr_internal::dbg_texq_load_sleep_ms : 0;
  bool do_fast = (prio & 0x400) ? (ddsx::get_streaming_mode() == ddsx::MultiDecoders) : false;
  prio &= 0xFF;
  G_ASSERT(prio >= 0 && prio < DXP_PRIO_COUNT);

  // debug_dump_stack("load");
  static int entrance_cnt[DXP_PRIO_COUNT] = {0};

  int64_t reft = profile_ref_ticks();
  for (bool passed_100ms = false; interlocked_increment(entrance_cnt[prio]) != 1;)
  {
    interlocked_decrement(entrance_cnt[prio]);
    sleep_msec(10);
    if (profile_usec_passed(reft, 100000000))
    {
      G_ASSERTF(false, "tex_pack2_perform_delayed_data_loading() is VERY busy");
      return false;
    }
    if (noPendingLoads())
      return false;
    if (!passed_100ms && profile_usec_passed(reft, 100000))
    {
      logwarn("simultaneous call to performDelayedLoad() stalled thread at least for %d msec", profile_time_usec(reft) / 1000);
      passed_100ms = true;
    }
  }

  Tab<DDSxTexturePack2::Rec *> toLoad;
  {
    OSSpinlockScopedLock autolock(toLoadPendSL);
    toLoad = toLoadPend;
    toLoadPend.clear();
  }

  WinAutoLock lock(critSec);
  const bool may_use_dctx = toLoad.size() > 1 && ((prio == 0 && !ddsx_dec_single_thread) || do_fast) && DDSxDecodeCtx::dCtx;

  interlocked_increment(ddsx_factory_delayed_load_entered_global);
  if (may_use_dctx)
    interlocked_increment(ddsx_factory_uses_dctx);

  reft = profile_ref_ticks();

  fast_sort(toLoad, [](const auto &a, const auto &b) { return a->ofs < b->ofs; });

  DDSxTexturePack2::Rec *lastrec = NULL;
  for (int i = toLoad.size() - 1; i >= 0; i--)
  {
    if (!isTexToBeLoaded(toLoad[i]->texId) || toLoad[i] == lastrec)
    {
      if (toLoad[i] != lastrec && RMGR.isValidID(toLoad[i]->texId, nullptr) && RMGR.resQS[toLoad[i]->texId.index()].isReading())
        RMGR.cancelReading(toLoad[i]->texId.index());
      int j = i - 1;
      while (j >= 0)
        if (!isTexToBeLoaded(toLoad[j]->texId) || toLoad[j] == lastrec)
        {
          if (toLoad[j] != lastrec && RMGR.isValidID(toLoad[j]->texId, nullptr) && RMGR.resQS[toLoad[j]->texId.index()].isReading())
            RMGR.cancelReading(toLoad[j]->texId.index());
          j--;
        }
        else
          break;

      RMGR_TRACE("remove queued items: %d..%d", j + 1, i);
      toLoadDone(toLoad, j + 1, i - j);
      i = j + 1;
      continue;
    }
    else
    {
      lastrec = toLoad[i];
      int idx = toLoad[i]->texId.index();
      RMGR_TRACE("toLoad[%d]=0x%x -> %d, tex=%p, rdLev=%d rc=%d bt.rc=%d %s", i, toLoad[i]->texId, idx, RMGR.baseTexture(idx),
        RMGR.resQS[idx].getRdLev(), RMGR.getRefCount(idx), RMGR.getBaseTexUsedCount(idx), RMGR.getName(idx));
      if (RMGR.getRefCount(idx) == 0 && !RMGR.getD3dRes(idx) && !RMGR.mayNeedBaseTex(idx, RMGR.resQS[idx].getRdLev()))
      {
        RMGR.finishReading(idx); // BQ part of split tex doesn't have hdr.FLG_HOLD_SYSMEM_COPY, so read is no-op
        toLoadDone(toLoad, i);
      }
    }
  }

  Tab<DDSxTexturePack2::Rec *> localLoad(toLoad);
  for (int i = localLoad.size() - 1; i >= 0; i--)
  {
    DDSxTexturePack2::Rec &p = *localLoad[i];
    int rec_id = &p - pack.texRec.data();

    if (texProps[rec_id].ldPrio != prio)
      erase_items(localLoad, i, 1);
    else
      toLoadDone(toLoad, i); // remove from the global list - it will be loaded now
  }

  // check readiness of paired base textures
  for (int i = 0; i < localLoad.size(); i++)
  {
    TEXTUREID tid = localLoad[i]->texId;
    TEXTUREID bt_tid = RMGR.pairedBaseTexId[tid.index()];
    if (bt_tid != BAD_TEXTUREID)
    {
      G_ASSERTF(RMGR.getBaseTexRc(bt_tid) > 0, "getBaseTexRc(0x%x)=%d", bt_tid, RMGR.getBaseTexRc(bt_tid));
      int rec_id = localLoad[i] - pack.texRec.data();
      unsigned need_bt = pack.texHdr[rec_id].flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX;
      int idx = tid.index();
      if (!need_bt || !RMGR.mayNeedBaseTex(idx, RMGR.resQS[idx].getRdLev()))
        continue; // BQ part of split tex doesn't need paired basetex - it contains full mipchain

      if (!RMGR.hasTexBaseData(bt_tid))
      {
        bool found = false;
        for (int j = 0; j < i; j++)
          if (localLoad[j]->texId == bt_tid)
          {
            found = true;
            break;
          }

        if (!found)
        {
          // repost derived texture for later loading
#if DAGOR_DBGLEVEL > 0
          logwarn("repost <%s> for later loading (paired basetex <%s*> not ready yet)", pack.texNames.map[rec_id],
            RMGR.getName(bt_tid.index()));
#endif
          RMGR.resQS[bt_tid.index()].setMaxReqLev(RMGR.getLevDesc(bt_tid.index(), TQL_high));
          if (RMGR.scheduleReading(bt_tid.index(), RMGR.getFactory(bt_tid.index())) || RMGR.resQS[bt_tid.index()].isReading())
          {
            toLoad.push_back(localLoad[i]);
            interlocked_increment(pendingTexCount[prio]);
          }
          else
          {
            debug("can't schedule %s loading (for %s)", RMGR.getName(bt_tid.index()), pack.texNames.map[rec_id]);
            RMGR.dumpTexState(bt_tid.index());
            RMGR.cancelReading(idx);
          }
          erase_items(localLoad, i, 1);
          i--;
          G_ASSERT(prio == ddsx::hq_tex_priority || !(pack.texHdr[rec_id].flags & ddsx::Header::FLG_HQ_PART));
        }
      }
    }
  }

  if (toLoad.size())
  {
    {
      OSSpinlockScopedLock autolock(toLoadPendSL);
      append_items(toLoadPend, toLoad.size(), toLoad.data());
    }
    clear_and_shrink(toLoad);
  }
  if (!localLoad.size())
  {
    interlocked_decrement(ddsx_factory_delayed_load_entered_global);
    interlocked_decrement(entrance_cnt[prio]);
    if (may_use_dctx)
      interlocked_decrement(ddsx_factory_uses_dctx);
    return false;
  }

  if (!fastSeqCrdP[prio])
    fastSeqCrdP[prio] = new FastSeqReader;
  FastSeqReader *fastSeqCrd = fastSeqCrdP[prio];
  SmallTab<FastSeqReader::Range, InimemAlloc> &rangesBuf = rangesBufP[prio];

  FATAL_CONTEXT_AUTO_SCOPE(pack.file->name);
  if (void *handle = pack.file->getHandle())
    fastSeqCrd->assignFile(handle, pack.file->baseOfs, pack.file->getSize(), pack.file->packName, pack.file->chunk, 32);
  else
    DAG_FATAL("Can't open TexPack '%s'", pack.file->name);

  if (rangesBuf.size() < localLoad.size())
    clear_and_resize(rangesBuf, localLoad.size());

  int num_ranges = 1;
  rangesBuf[0].start = localLoad[0]->ofs;
  rangesBuf[0].end = rangesBuf[0].start + localLoad[0]->packedDataSize;
  for (int i = 1; i < localLoad.size(); i++)
  {
    int st = localLoad[i]->ofs;
    if (st < rangesBuf[num_ranges - 1].end + (32 << 10))
      rangesBuf[num_ranges - 1].end = st + localLoad[i]->packedDataSize;
    else
    {
      rangesBuf[num_ranges].start = st;
      rangesBuf[num_ranges].end = st + localLoad[i]->packedDataSize;
      num_ranges++;
    }
  }
  fastSeqCrd->setRangesOfInterest(make_span(rangesBuf).first(num_ranges));

  int data_sz = 0, mem_data_sz = 0;
  int last_recid = -1;
  for (int i = 0; i < localLoad.size(); i++)
  {
    if (!interlocked_acquire_load(processingTexData[prio]))
    {
      {
        OSSpinlockScopedLock autolock(toLoadPendSL);
        append_items(toLoadPend, localLoad.size() - i, localLoad.data() + i);
      }
      interlocked_add(pendingTexCount[prio], localLoad.size() - i);
      debug("performDelayedLoad(%d) interrupted, pending=%d", prio, interlocked_relaxed_load(pendingTexCount[prio]));
      break;
    }
    DDSxTexturePack2::Rec &p = *localLoad[i];

    int rec_id = &p - pack.texRec.data();
    if (!RMGR.getRefCount(p.texId.index()) && !RMGR.getBaseTexUsedCount(p.texId.index()))
    {
      logwarn("skip loading tex %s at 0x%x, all references released", pack.texNames.map[rec_id], p.ofs);
      RMGR.cancelReading(p.texId.index());
      continue;
    }
    if (RMGR.getResMaxReqLev(p.texId) <= 1)
    {
      if ((pack.texHdr[rec_id].flags & (ddsx::Header::FLG_GENMIP_BOX | ddsx::Header::FLG_GENMIP_KAIZER)) &&
          (pack.texHdr[rec_id].flags & ddsx::Header::FLG_HOLD_SYSMEM_COPY) && RMGR.getRefCount(p.texId) > 0 &&
          !RMGR.hasTexBaseData(p.texId))
      {
        debug("restore ql=1 to get sysMemCopy: %s", pack.texNames.map[rec_id]);
        unsigned idx = p.texId.index();
        RMGR.updateResReqLev(p.texId, idx, RMGR.getLevDesc(idx, TQL_high));
      }
    }
    G_ASSERT(rec_id != last_recid);

    int tex_q = texProps[rec_id].curQID;
    RMGR_TRACE("loading tex %s (req=%d rd=%d) at 0x%x %dx%d", pack.texNames.map[rec_id], get_managed_res_maxreq_lev(p.texId),
      RMGR.resQS[p.texId.index()].getRdLev(), p.ofs, pack.texHdr[rec_id].w, pack.texHdr[rec_id].h);
    fastSeqCrd->seekto(p.ofs);
    interlocked_increment(ddsx_loaded_tex_cnt[prio]);
    if (DDSxDecodeCtx::dCtx && may_use_dctx)
    {
      DDSxDecodeCtx::TexCreateJob *j = DDSxDecodeCtx::dCtx->allocJob();
      while (!j)
      {
        sleep_msec(1);
        j = DDSxDecodeCtx::dCtx->allocJob();
      }

      if (j->initJob(pack.texHdr[rec_id], tex_q, pack.texNames.map[rec_id], pack.file->name, p.texId, *fastSeqCrd, p.packedDataSize,
            prio, processingTexData[prio]))
      {
        DDSxDecodeCtx::dCtx->submitJob(j);
      }
      else
      {
        G_ASSERT_LOG(0, "%s allocation of %dK failed, fallback to serial load", __FUNCTION__, p.packedDataSize >> 10);
        j->done = 1; // "free" it
        goto serial;
      }
    }
    else
    {
    serial:
      if (dbg_sleep)
        sleep_msec(dbg_sleep);

      on_tex_slice_loaded_cb_t on_tex_slice_loaded_cb = nullptr;
#if _TARGET_C1 | _TARGET_C2

#endif
      //== check here for cancelled reading
      TexLoadRes ldRet = RMGR.readDdsxTex(p.texId, pack.texHdr[rec_id], *fastSeqCrd, tex_q, on_tex_slice_loaded_cb);
      if (ldRet == TexLoadRes::ERR)
        if (!d3d::is_in_device_reset_now())
          logerr("failed loading tex %s", pack.texNames.map[rec_id].get());

      {
        texmgr_internal::TexMgrAutoLock tlock;
        RMGR.markUpdatedAfterLoad(p.texId.index());
      }
      G_ASSERT(RMGR.isValidID(p.texId));
      if (!is_managed_textures_streaming_load_on_demand())
        RMGR.scheduleReading(p.texId.index(), RMGR.getFactory(p.texId.index()));
    }
    data_sz += p.packedDataSize;
    mem_data_sz += pack.texHdr[rec_id].memSz;
    last_recid = rec_id;
  }

  // make sure, that no aio requests left (because at least on some platforms (i.e. windows)
  // other thread won't be able receive left aio callbacks)
  fastSeqCrd->reset();

  if (may_use_dctx)
    DDSxDecodeCtx::dCtx->waitAllDone(prio);

  int t0 = profile_time_usec(reft);
  debug("(%s).performDelayedLoad(%d): %d usec (%dK of %dK range in %d areas), %.2f Mb/s (unp. %dM)", pack.file->name, prio, t0,
    data_sz >> 10, (rangesBuf[num_ranges - 1].end - rangesBuf[0].start) >> 10, num_ranges, double(data_sz) / (t0 ? t0 : 1),
    mem_data_sz >> 20);
  G_UNUSED(t0);

  if (ALWAYS_REOPEN_FILES)
    pack.file->closeHandle();

  interlocked_decrement(ddsx_factory_delayed_load_entered_global);
  interlocked_decrement(entrance_cnt[prio]);
  if (may_use_dctx)
    interlocked_decrement(ddsx_factory_uses_dctx);
  return true;
}


bool ddsx::load_tex_pack2(const char *pack_path, int *ret_code, bool can_override_tex)
{
  if (texmgr_internal::auto_add_tex_on_get_id)
  {
    if (ret_code)
      *ret_code = TPRC_UNLOADED;
    return true;
  }
  char simplifiedPath[DAGOR_MAX_PATH];
  simplify_path(pack_path, simplifiedPath);
  const uint32_t simplifiedPathLen = strlen(simplifiedPath);
  tex_packs_lock.lockRead();
  const uint32_t simplifiedPathHash = packs_pathes.string_hash(simplifiedPath, simplifiedPathLen);
  int pathId = packs_pathes.getNameId(simplifiedPath, simplifiedPathLen, simplifiedPathHash);
  ;
  if (pathId != -1)
    for (int i = 0; i < tex_packs.size(); i++)
      if (pathId == tex_packs[i].pathId)
      {
        tex_packs[i].refCount++;
        if (ret_code)
          *ret_code = TPRC_ADDREF;
        tex_packs_lock.unlockRead();
        return true;
      }
  tex_packs_lock.unlockRead();

  TexPackALWr lockWr;
  DDSxTexturePack2 *p = DDSxTexturePack2::make(pack_path, can_override_tex, tex_packs.size());
  if (!p)
  {
    if (ret_code)
      *ret_code = TPRC_NOTFOUND;
    return false;
  }
  if (!p->file->packName)
  {
    // logwarn("skip empty pack %s", pack_path);
    p->termTex();
    p->deleteFile();
    memfree(p, midmem);
    if (ret_code)
      *ret_code = TPRC_UNLOADED;
    return true;
  }

  if (pathId == -1)
    pathId = packs_pathes.addNameId(simplifiedPath, simplifiedPathLen, simplifiedPathHash);
  ;
  TexPackRegRec &r = tex_packs.push_back();
  r.refCount = 1;
  r.pathId = pathId;
  r.pack = p;
  if (ret_code)
    *ret_code = TPRC_LOADED;
  return true;
}

bool ddsx::release_tex_pack2(const char *pack_path, int *ret_code)
{
  if (!pack_path || !*pack_path)
  {
    if (ret_code)
      *ret_code = TPRC_NOTFOUND;
    return false;
  }
  if (!tex_packs_lock.tryLockWrite())
  {
    bool intr = interrupt_texq_loading();
    tex_packs_lock.lockWrite();
    restore_texq_loading(intr);
  }
  int pathId = get_path_id(pack_path);
  if (pathId != -1)
    for (int i = 0; i < tex_packs.size(); i++)
      if (pathId == tex_packs[i].pathId)
      {
        tex_packs[i].refCount--;
        if (tex_packs[i].refCount > 0)
        {
          if (ret_code)
            *ret_code = TPRC_DELREF;
          tex_packs_lock.unlockWrite();
          return true;
        }

        tex_packs[i].pack->termTex();
        tex_packs[i].pack->deleteFile();

        memfree(tex_packs[i].pack, midmem);
        erase_items(tex_packs, i, 1);

        if (ret_code)
          *ret_code = TPRC_UNLOADED;
        tex_packs_lock.unlockWrite();
        return true;
      }

  if (ret_code)
    *ret_code = TPRC_NOTFOUND;
  tex_packs_lock.unlockWrite();
  return false;
}

bool ddsx::interrupt_texq_loading()
{
  if (texmgr_internal::reload_jobmgr_id < 0)
    return false;
  interlocked_release_store(texmgr_internal::texq_load_disabled, 1);
  bool mainThread = is_main_thread();
  cpujobs::remove_jobs_by_tag(texmgr_internal::reload_jobmgr_id, _MAKE4C('dlAT'), /*auto_release_jobs*/ mainThread);
  ddsx::cease_delayed_data_loading(0);
  if (ddsx::hq_tex_priority != 0)
    ddsx::cease_delayed_data_loading(ddsx::hq_tex_priority);
  if (mainThread)
  {
    for (int64_t reft = profile_ref_ticks(); !profile_usec_passed(reft, 30000);)
      if (cpujobs::is_job_manager_busy(texmgr_internal::reload_jobmgr_id))
        sleep_msec(1);
      else
        break;
    if (cpujobs::is_job_manager_busy(texmgr_internal::reload_jobmgr_id))
    {
      logwarn("failed to interrupt_texq_loading() from main thread");
      interlocked_release_store(texmgr_internal::texq_load_disabled, 0);
      return false;
    }
  }
  while (cpujobs::is_job_manager_busy(texmgr_internal::reload_jobmgr_id))
    sleep_msec(1);
  cpujobs::reset_job_queue(texmgr_internal::reload_jobmgr_id, mainThread);
  return true;
}
void ddsx::restore_texq_loading(bool was_interrupted)
{
  if (was_interrupted)
    interlocked_release_store(texmgr_internal::texq_load_disabled, 0);
}
bool ddsx::tex_pack2_perform_delayed_data_loading(int prio)
{
  int64_t reft = profile_ref_ticks();
  ddsx::process_ddsx_load_queue();
  process_arr_tex_load_queue();
  bool texq_interrupted = false;
  if (prio == 0 && DDSxDecodeCtx::dCtx && texmgr_internal::reload_jobmgr_id >= 0)
  {
    if (!interlocked_acquire_load(pendingTexCount[0]))
      return false;
    texq_interrupted = interrupt_texq_loading();
  }

  int full_prio = prio;
  prio &= 0xFF;
  G_ASSERT_RETURN(prio < DXP_PRIO_COUNT, false);

  if (interlocked_compare_exchange(processingTexData[prio], 1, 0))
  {
    reft = profile_ref_ticks();
    while (interlocked_acquire_load(processingTexData[prio]))
      sleep_msec(1);
    if (profile_usec_passed(reft, 1000))
      logwarn("simultaneous call to tex_pack2_perform_delayed_data_loading(%d) stalled thread for %d msec", prio,
        profile_time_usec(reft) / 1000);
    restore_texq_loading(texq_interrupted);
    return false;
  }
  if (!interlocked_acquire_load(pendingTexCount[prio]))
  {
    interlocked_release_store(processingTexData[prio], 0);
    if (texq_interrupted)
      interlocked_release_store(texmgr_internal::texq_load_disabled, 0);
    return false;
  }
  interlocked_release_store(ddsx_loaded_tex_cnt[prio], 0);
  DDSxDecodeCtx::clearWorkerUsedMask(prio);

  bool done = false;
  tex_packs_lock.lockRead();
  for (int pass = 0; pass < 8; pass++)
  {
    for (int i = 0; i < tex_packs.size() && interlocked_acquire_load(processingTexData[prio]); i++)
      done |= tex_packs[i].pack->file->performDelayedLoad(full_prio);
    if (!interlocked_acquire_load(pendingTexCount[prio]) || !interlocked_acquire_load(processingTexData[prio]))
      break;
#if DAGOR_DBGLEVEL > 0
    logwarn("tex_pack2_perform_delayed_data_loading(%d): do another pass (%d), pending=%d", prio, pass + 1,
      interlocked_relaxed_load(pendingTexCount[prio]));
#endif
  }
  tex_packs_lock.unlockRead();
  interlocked_release_store(processingTexData[prio], 0);
  int t0 = profile_time_usec(reft);
  if (t0 > 10)
  {
    const int loaded_tex_cnt = interlocked_acquire_load(ddsx_loaded_tex_cnt[prio]);
    if (DDSxDecodeCtx::getWorkerUsedMask(prio))
      debug("%s(%d): decoded %d tex (using %d threads of %d) for %d usec", __FUNCTION__, prio, loaded_tex_cnt,
        DDSxDecodeCtx::getWorkerUsedCount(prio), DDSxDecodeCtx::dCtx ? DDSxDecodeCtx::dCtx->numWorkers : 1, t0);
    else
      debug("%s(%d): decoded %d tex (serial mode) for %d usec", __FUNCTION__, prio, loaded_tex_cnt, t0);
    if (loaded_tex_cnt)
      RMGR.dumpMemStats();
  }
  restore_texq_loading(texq_interrupted);
  return done;
}

void ddsx::tex_pack2_perform_delayed_data_loading_async(int prio, int jobmgr_id, bool add_job_first, int max_job_in_flight)
{
  static constexpr int MAX_JOBMGRID = 64;
  static volatile int jobs_in_flight[MAX_JOBMGRID] = {0};
  G_ASSERT(prio >= 0 && (prio & 0xFF) < DXP_PRIO_COUNT);
  if (prio < 0 || (prio & 0xFF) >= DXP_PRIO_COUNT)
    return;
  G_ASSERT(jobmgr_id >= 0);
  if (jobmgr_id < 0)
    return;
  if (jobmgr_id < MAX_JOBMGRID && interlocked_relaxed_load(jobs_in_flight[jobmgr_id]) >= max_job_in_flight)
    return;

  struct AsyncLoadPendingTexJob : public cpujobs::IJob
  {
    int prio, jmId;
    AsyncLoadPendingTexJob(int p, int id) : prio(p), jmId(id) {}
    virtual void doJob()
    {
      ddsx::tex_pack2_perform_delayed_data_loading(prio);
      if (jmId < MAX_JOBMGRID)
      {
        interlocked_decrement(jobs_in_flight[jmId]);
        jmId = MAX_JOBMGRID;
      }
      interlocked_decrement(processingTexDataScheduled[prio & 0xFF]);
      prio = -1;
    }
    virtual void releaseJob()
    {
      if (jmId < MAX_JOBMGRID)
        interlocked_decrement(jobs_in_flight[jmId]);
      if (prio >= 0)
        interlocked_decrement(processingTexDataScheduled[prio & 0xFF]);
      delete this;
    }
    virtual unsigned getJobTag() { return _MAKE4C('dlAT'); };
  };
  if (jobmgr_id < MAX_JOBMGRID)
    interlocked_increment(jobs_in_flight[jobmgr_id]);
  interlocked_increment(processingTexDataScheduled[prio & 0xFF]);
  cpujobs::add_job(jobmgr_id, new AsyncLoadPendingTexJob(prio, jobmgr_id), add_job_first);
}

bool ddsx::cease_delayed_data_loading(int prio) { return interlocked_exchange(processingTexData[prio], 0) != 0; }

#ifdef _MSC_VER
#pragma warning(disable : 4316) // object allocated on the heap may not be aligned 128
#endif
int ddsx::set_decoder_workers_count(int wcnt)
{
  if (ddsx_factory_uses_dctx)
  {
    const int timeout_usec = is_main_thread() ? 100000 : 1000000;
    int64_t reft = profile_ref_ticks();
    while (ddsx_factory_uses_dctx && !profile_usec_passed(reft, timeout_usec))
      sleep_msec(1);
    if (profile_usec_passed(reft, 2000))
      logwarn("ddsx::set_decoder_workers_count(%d) waited %d usec for ddsx_factory_uses_dctx==0 (%d)", wcnt, profile_time_usec(reft),
        ddsx_factory_uses_dctx);
  }
  int prev = DDSxDecodeCtx::dCtx ? DDSxDecodeCtx::dCtx->numWorkers : 0;
  if (wcnt < 2)
    wcnt = 0;
  else if (wcnt > DDSxDecodeCtx::MAX_WORKERS)
    wcnt = DDSxDecodeCtx::MAX_WORKERS;

  if (prev != wcnt)
  {
    G_ASSERTF(!ddsx_factory_uses_dctx, "ddsx_factory_uses_dctx=%d", ddsx_factory_uses_dctx);
    del_it(DDSxDecodeCtx::dCtx);
    DDSxDecodeCtx::dCtx = wcnt >= 2 ? new DDSxDecodeCtx(wcnt) : NULL;
    debug("ddsx::set_decoder_workers_count(%d), reinit", wcnt);
  }
  return prev;
}
ddsx::StreamingMode ddsx::set_streaming_mode(ddsx::StreamingMode m)
{
  auto prev = (StreamingMode)interlocked_exchange(strmLoadMode, (int)m);
  if (prev != m)
  {
    // Wake up to speed-up threads exit (and hence free thread local data/segments)
    if (prev == MultiDecoders && DDSxDecodeCtx::dCtx)
      DDSxDecodeCtx::dCtx->wakeUpAll();

    debug("ddsx::set_streaming_mode() -> %s",
      strmLoadMode == MultiDecoders ? "MultiDecoders" : (strmLoadMode == BackgroundSerial ? "BackgroundSerial" : "?"));
  }
  return prev;
}
ddsx::StreamingMode ddsx::get_streaming_mode() { return (StreamingMode)interlocked_relaxed_load(strmLoadMode); }

void ddsx::dump_registered_tex_distribution()
{
  for (int i = 0; i < tex_packs.size(); i++)
    tex_packs[i].pack->updatePairedBaseTex(i);

  debug("--- DDSx distribution (memSz, diskSz) ---\n"
        "SPLIT   cnt=%5d lqSz=(%6dM, %6dM) hqSz=(%6dM, %6dM)\n"
        "unsplit cnt=%5d   sz=(%6dM, %6dM)\n"
        " --\n"
        "GENMIP  cnt=%5d   sz=(%6dM, %6dM)\n"
        "DIFFTEX cnt=%5d   sz=(%6dM, %6dM)\n"
        "MIP0    cnt=%5d   sz=(%6dM, %6dM)\n"
        "other   cnt=%5d   sz=(%6dM, %6dM)\n"
        "=Total: cnt=%5d   sz=(%6dM, %6dM)  in %d pack(s)\n",
    sum_lq_num, sum_lq_mem_sz >> 20, sum_lq_pack_sz >> 20, sum_hq_mem_sz >> 20, sum_hq_pack_sz >> 20, sum_ns_num, sum_ns_mem_sz >> 20,
    sum_ns_pack_sz >> 20, sum_gm_num, sum_gm_mem_sz >> 20, sum_gm_pack_sz >> 20, sum_dt_num, sum_dt_mem_sz >> 20, sum_dt_pack_sz >> 20,
    sum_sm_num, sum_sm_mem_sz >> 20, sum_sm_pack_sz >> 20, sum_ot_num, sum_ot_mem_sz >> 20, sum_ot_pack_sz >> 20,
    sum_lq_num + sum_ns_num, (sum_lq_mem_sz + sum_hq_mem_sz + sum_ns_mem_sz) >> 20,
    (sum_lq_pack_sz + sum_hq_pack_sz + sum_ns_pack_sz) >> 20, ::tex_packs.size());
}

namespace ddsx
{

void set_tex_pack_async_loading_allowed(bool on) { tex_packs_are_allowed_to_load.toggle(on); }

void shutdown_tex_pack2_data()
{
  for (int i = 0; i < DXP_PRIO_COUNT; i++)
    cease_delayed_data_loading(i);
  if (texmgr_internal::stop_bkg_tex_loading)
    texmgr_internal::stop_bkg_tex_loading(1);
  while (ddsx_factory_delayed_load_entered_global)
    sleep_msec(10);

  for (int i = 0; i < tex_packs.size(); i++)
    tex_packs[i].pack->termTex();
  for (int i = 0; i < tex_packs.size(); i++)
  {
    tex_packs[i].pack->deleteFile();
    memfree(tex_packs[i].pack, midmem);
  }
  clear_and_shrink(tex_packs);
  for (int i = 0; i < DXP_PRIO_COUNT; i++)
  {
    del_it(fastSeqCrdP[i]);
    clear_and_shrink(rangesBufP[i]);
  }
  del_it(DDSxDecodeCtx::dCtx);

  ddsx::term_ddsx_load_queue();
  term_arr_tex_load_queue();
}
} // namespace ddsx

bool ddsx::read_ddsx_contents(const char *tex_name, Tab<char> &out_data, ddsx::DDSxDataPublicHdr &out_desc)
{
  int idx = RMGR.toIndex(get_managed_texture_id(tex_name));
  out_data.clear();
  if (idx < 0 || !RMGR.getFactory(idx))
    return false;
  TexPackALRd lockRd;
  auto &bq_ref = RMGR.texDesc[idx].packRecIdx[TQL_base];
  if (bq_ref.pack >= 0)
  {
    int i = bq_ref.pack;
    int r = bq_ref.rec;

    auto &hq_ref =
      RMGR.texDesc[idx].packRecIdx[TQL_uhq].pack >= 0 ? RMGR.texDesc[idx].packRecIdx[TQL_uhq] : RMGR.texDesc[idx].packRecIdx[TQL_high];
    if (hq_ref.pack >= 0)
    {
      DDSxTexturePack2::Factory *hq_f = tex_packs[hq_ref.pack].pack->file;
      int hq_r = hq_ref.rec;
      ddsx::Header &hdr = hq_f->pack.texHdr[hq_r];

      memset(&out_desc, 0, sizeof(out_desc));
#define COPY_FIELD(X) out_desc.X = hdr.X
      COPY_FIELD(d3dFormat);
      COPY_FIELD(flags);
      COPY_FIELD(w);
      COPY_FIELD(h);
      COPY_FIELD(depth);
      COPY_FIELD(levels);
      COPY_FIELD(bitsPerPixel);
      COPY_FIELD(hqPartLevels);
      COPY_FIELD(dxtShift);
      COPY_FIELD(lQmip);
      COPY_FIELD(mQmip);
      COPY_FIELD(uQmip);
#undef COPY_FIELD

      out_data.resize(hdr.memSz);
      WinAutoLock lock(hq_f->pack.file->critSec);
      FullFileLoadCB fcrd(hq_f->pack.file->packName);
      G_ASSERT(fcrd.fileHandle);
      fcrd.seekto(hq_f->pack.texRec[hq_r].ofs);

      if (hdr.isCompressionZSTD())
      {
        ZstdLoadCB crd(fcrd, hdr.packedSz);
        crd.read(out_data.data(), data_size(out_data));
        crd.ceaseReading();
      }
      else if (hdr.isCompressionOODLE())
      {
        OodleLoadCB crd(fcrd, hdr.packedSz, hdr.memSz);
        crd.read(out_data.data(), data_size(out_data));
        crd.ceaseReading();
      }
      else if (hdr.isCompressionZLIB())
      {
        ZlibLoadCB crd(fcrd, hdr.packedSz);
        crd.read(out_data.data(), data_size(out_data));
        crd.ceaseReading();
      }
      else if (hdr.isCompression7ZIP())
      {
        LzmaLoadCB crd(fcrd, hdr.packedSz);
        crd.read(out_data.data(), data_size(out_data));
        crd.ceaseReading();
      }
      else
        fcrd.read(out_data.data(), data_size(out_data));

      ddsx::Header hdr_copy = hdr;
      ddsx_forward_mips_inplace(hdr_copy, out_data.data(), data_size(out_data));
      out_desc.flags = hdr_copy.flags;

      // HQ part read, proceed to basepart
      if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
        return true;
    }

    ddsx::Header &hdr = tex_packs[i].pack->texHdr[r];
    int data_ofs = out_data.size(), data_sz = hdr.memSz;
    if (data_ofs > 0)
    {
      append_items(out_data, data_sz);
      out_desc.levels += hdr.levels;
      goto read_data;
    }

    memset(&out_desc, 0, sizeof(out_desc));
#define COPY_FIELD(X) out_desc.X = hdr.X
    COPY_FIELD(d3dFormat);
    COPY_FIELD(flags);
    COPY_FIELD(w);
    COPY_FIELD(h);
    COPY_FIELD(depth);
    COPY_FIELD(levels);
    COPY_FIELD(bitsPerPixel);
    COPY_FIELD(hqPartLevels);
    COPY_FIELD(dxtShift);
    COPY_FIELD(lQmip);
    COPY_FIELD(mQmip);
    COPY_FIELD(uQmip);
#undef COPY_FIELD

    out_data.resize(hdr.memSz);

  read_data:
    WinAutoLock lock(tex_packs[i].pack->file->critSec);
    FullFileLoadCB fcrd(tex_packs[i].pack->file->packName);
    G_ASSERT(fcrd.fileHandle);
    fcrd.seekto(tex_packs[i].pack->texRec[r].ofs);

    if (hdr.isCompressionZSTD())
    {
      ZstdLoadCB crd(fcrd, hdr.packedSz);
      crd.read(&out_data[data_ofs], data_sz);
      crd.ceaseReading();
    }
    else if (hdr.isCompressionOODLE())
    {
      OodleLoadCB crd(fcrd, hdr.packedSz, hdr.memSz);
      crd.read(&out_data[data_ofs], data_sz);
      crd.ceaseReading();
    }
    else if (hdr.isCompressionZLIB())
    {
      ZlibLoadCB crd(fcrd, hdr.packedSz);
      crd.read(&out_data[data_ofs], data_sz);
      crd.ceaseReading();
    }
    else if (hdr.isCompression7ZIP())
    {
      LzmaLoadCB crd(fcrd, hdr.packedSz);
      crd.read(&out_data[data_ofs], data_sz);
      crd.ceaseReading();
    }
    else
      fcrd.read(&out_data[data_ofs], data_sz);

    ddsx::Header hdr_copy = hdr;
    ddsx_forward_mips_inplace(hdr_copy, &out_data[data_ofs], data_sz);
    out_desc.flags = hdr_copy.flags;
    return true;
  }
  G_ASSERTF(0, "%s idx=%d not found in packs", RMGR.getName(idx), idx);
  return false;
}
int ddsx::read_ddsx_header(const char *tex_name, DDSxDataPublicHdr &out_desc, bool full_quality)
{
  int idx = RMGR.toIndex(get_managed_texture_id(tex_name));
  if (idx < 0 || !RMGR.getFactory(idx))
    return -1;

  TexPackALRd lockRd;
  auto &bq_ref = RMGR.texDesc[idx].packRecIdx[TQL_base];
  if (bq_ref.pack >= 0)
  {
    int i = bq_ref.pack;
    int r = bq_ref.rec;

    const ddsx::Header &bq_hdr = tex_packs[i].pack->texHdr[r];
    ddsx::Header hdr;
    auto &hq_ref = RMGR.texDesc[idx].packRecIdx[TQL_high];
    if (hq_ref.pack >= 0 && full_quality)
    {
      DDSxTexturePack2::Factory *hq_f = tex_packs[hq_ref.pack].pack->file;
      hdr = hq_f->pack.texHdr[hq_ref.rec];
      hdr.levels += bq_hdr.levels;
      hdr.memSz += bq_hdr.memSz;
    }
    else
      hdr = bq_hdr;

    memset(&out_desc, 0, sizeof(out_desc));
#define COPY_FIELD(X) out_desc.X = hdr.X
    COPY_FIELD(d3dFormat);
    COPY_FIELD(flags);
    COPY_FIELD(w);
    COPY_FIELD(h);
    COPY_FIELD(depth);
    COPY_FIELD(levels);
    COPY_FIELD(bitsPerPixel);
    COPY_FIELD(hqPartLevels);
    COPY_FIELD(dxtShift);
    COPY_FIELD(lQmip);
    COPY_FIELD(mQmip);
    COPY_FIELD(uQmip);
#undef COPY_FIELD

    if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
    {
      // recompute full tex size for GENMIP since memSz was only mip0.size
      hdr.memSz = 0;
      for (int level = 0; level < hdr.levels; level++)
      {
        int d = max(hdr.depth >> level, 1);
        if (hdr.flags & hdr.FLG_VOLTEX)
          hdr.memSz += hdr.getSurfaceSz(level) * d;
        else if (hdr.flags & hdr.FLG_CUBTEX)
          hdr.memSz += hdr.getSurfaceSz(level) * 6;
        else
          hdr.memSz += hdr.getSurfaceSz(level);
      }
    }

    return hdr.memSz;
  }
  return -1;
}

void ddsx::reload_active_textures(int prio, bool interrupt)
{
  bool intr = interrupt ? interrupt_texq_loading() : false;
  TexPackALRd lockRd;
  for (int i = 0; i < tex_packs.size(); i++)
    tex_packs[i].pack->file->reloadActiveTextures(prio, i);
  restore_texq_loading(intr);
  reload_active_array_textures();
}

void ddsx::reset_lq_tex_list()
{
  OSSpinlockScopedLock autolock(forceQTexListSL);
  forcedLqTexList.reset();
}
void ddsx::reset_mq_tex_list()
{
  OSSpinlockScopedLock autolock(forceQTexListSL);
  forcedMqTexList.reset();
}
void ddsx::add_texname_to_lq_tex_list(TEXTUREID texId)
{
  OSSpinlockScopedLock autolock(forceQTexListSL);
  if (texId != BAD_TEXTUREID)
    forcedLqTexList.add(texId);
}
void ddsx::add_texname_to_mq_tex_list(TEXTUREID texId)
{
  OSSpinlockScopedLock autolock(forceQTexListSL);
  if (texId != BAD_TEXTUREID)
    forcedMqTexList.add(texId);
}

#if DAGOR_DBGLEVEL > 0
#define FATALERR DAG_FATAL
#else
#define FATALERR logerr
#endif

#if 0
#define TRACE_ARRTEX debug
#else
#define TRACE_ARRTEX(...) \
  do                      \
  {                       \
  } while (0)
#endif

static __forceinline void logAddManagedArrayTextureFailure(const char *texname, int slice, bool isHq, TexLoadRes ldRet,
  unsigned attempt, unsigned refFrame, int64_t refticks, unsigned refResUpdateFlushes)
{
  logerr("add_managed_array_texture(%s): d3d_load_ddsx_to_slice(%d.%s) failed with %s"
         "(on attempt %d for %d frames, %d usec and %d res.upd.flushes)",
    texname, slice, isHq ? "hq" : "bq", ldRet == TexLoadRes::ERR ? "error" : "RUB quota limit", attempt, dagor_frame_no() - refFrame,
    profile_time_usec(refticks), interlocked_acquire_load(texmgr_internal::drv_res_updates_flush_count) - refResUpdateFlushes);
}

struct DDSxArrayTextureFactory final : public TextureFactory
{
  static constexpr int MAX_REC_COUNT = 128;
  struct ArrayTexRecSpinlock
  {
    OSSpinlock spinlock;
    ArrayTexRecSpinlock() = default;
    ArrayTexRecSpinlock(const ArrayTexRecSpinlock &) {}
    ArrayTexRecSpinlock &operator=(const ArrayTexRecSpinlock &) { return *this; }
  };
  struct ArrayTexRec : public ArrayTexRecSpinlock
  {
    TEXTUREID tid = BAD_TEXTUREID;
    int16_t quality = -1;
    uint16_t gen = 0;
    struct SliceRec
    {
      SimpleString ddsxFn;
      DDSxTexturePack2 *bqPack = nullptr, *hqPack = nullptr;
      int bqIdx = -1, hqIdx = -1;
      bool viaFactory = false;
    };
    Tab<SliceRec> slice;
    ddsx::Header hdr = {0};
  };
  class ArrayTexRecList : public dag::Vector<ArrayTexRec>
  {
  public:
    int size() const { return interlocked_acquire_load(*(volatile int *)&mCount); }
    int appendOne()
    {
      int i = interlocked_increment(*(volatile int *)&mCount) - 1;
      if (DAGOR_LIKELY(i < capacity()))
      {
        new (data() + i, _NEW_INPLACE) ArrayTexRec();
        return i;
      }
      else
      {
        interlocked_decrement(*(volatile int *)&mCount);
        return -1;
      }
    }
  } rec;

  struct LoadQueueRec
  {
    int16_t recIdx = -1;
    uint16_t gen;
    TEXTUREID tid; // Note: intentionally not inited since it's not used in "operator bool()"

    LoadQueueRec() = default;
    LoadQueueRec(TEXTUREID tid_, int rec_idx, uint16_t gen_) : tid(tid_), recIdx(rec_idx), gen(gen_) { G_FAST_ASSERT(*this); }
    explicit operator bool() const { return recIdx >= 0; }
  };
  cpujobs::JobQueue<LoadQueueRec, true> loadQueue;

  struct ProcessQueueJob final : public cpujobs::IJob
  {
    void doJob() override { process_arr_tex_load_queue(); }
    void releaseJob() override { delete this; }
  };

  ~DDSxArrayTextureFactory()
  {
    // In theory this need to be called in dtor of any factory, but in practice this object exists
    // as static var ("arr_tex_factory") and attempt to call it is actually UB (because it relies on other static vars)
    /* onTexFactoryDeleted(this); */
  }

  BaseTexture *createTexture(TEXTUREID id) override
  {
    const char *texname = get_managed_texture_name(id);
    TRACE_ARRTEX("arrTex: create %s tid=0x%x", texname, id);
    for (int i = 0, n = rec.size(); i < n; i++)
    {
      ArrayTexRec &r = rec[i];
      if (r.tid != id)
        continue;
      int texq = get_texq(id, ::dgs_tex_quality);
      OSSpinlockScopedLock autolock(r.spinlock);
      if (!r.hdr.label)
        return NULL;
      if (r.quality < 0)
        r.quality = texq;
      BaseTexture *bt = d3d::alloc_ddsx_tex(r.hdr, TEXCF_RGB | TEXCF_ABEST | TEXCF_LOADONCE, r.quality, 0, texname, RES3D_ARRTEX);
      RMGR.setupQLev(id.index(), r.quality, r.hdr);
      TRACE_ARRTEX("arrTex: %s allocated bt=%p %dx%d, %d slices, is_main_thread()=%d reci=%d", texname, bt, r.hdr.w, r.hdr.h,
        r.hdr.depth, is_main_thread(), i);
      if (!bt)
      {
        FATALERR("add_managed_array_texture(%s): d3d::alloc_ddsx_tex() failed", texname);
        return NULL;
      }
      if (bt->restype() != RES3D_ARRTEX)
      {
        FATALERR("add_managed_array_texture(%s): d3d::alloc_ddsx_tex() returned restype=%d instead of %d", texname, bt->restype(),
          RES3D_ARRTEX);
        del_d3dres(bt);
        return NULL;
      }
      if (texmgr_internal::dbg_texq_only_stubs)
        return bt;

      if (!is_main_thread())
      {
        r.gen++;
        ArrayTexRec lr = r; // local copy
        r.spinlock.unlock();
        bool loaded = loadArrTexData(i, bt, lr);
        r.spinlock.lock();
        if (loaded)
          return bt;
        del_d3dres(bt);
        return NULL;
      }

      addLoadQueueRec(r.tid, i);

      return bt;
    }
#if DAGOR_DBGLEVEL > 0
    logerr("add_managed_array_texture createTex(0x%x), rec.size()=%d", id, rec.size());
    for (int i = 0; i < rec.size(); i++)
      logerr("  rec[%d] tid=0x%x slice=%d", i, rec[i].tid, rec[i].slice.size());
#endif
    FATALERR("add_managed_array_texture(%s): internal error 1", texname);
    return NULL;
  }

  bool loadArrTexData(int rec_idx, BaseTexture *bt)
  {
    rec[rec_idx].spinlock.lock();
    if (DAGOR_UNLIKELY(!bt || rec[rec_idx].tid == BAD_TEXTUREID))
    {
      rec[rec_idx].spinlock.unlock();
      return false;
    }
    ArrayTexRec lr = rec[rec_idx]; // local copy
    rec[rec_idx].spinlock.unlock();
    return loadArrTexData(rec_idx, bt, lr);
  }

  bool loadArrTexData(int rec_idx, BaseTexture *bt, const ArrayTexRec &r)
  {
    G_ASSERT(r.quality >= 0); // shall be already set by createTexture
    const char *texname = get_managed_texture_name(r.tid);
    G_UNUSED(texname);
    ArrayTexture *dest_tex = (ArrayTexture *)bt, *t = dest_tex;
    Tab<char> zeroes;

    // load BQ data
    TRACE_ARRTEX("arrTex: %s %p.load %d slices (rec[%d].tid=0x%x)", texname, dest_tex, r.slice.size(), rec_idx, r.tid);
    G_UNUSED(rec_idx);
    auto &resQS = RMGR.resQS[r.tid.index()];
    auto &texDesc = RMGR.texDesc[r.tid.index()];
    RMGR.startReading(r.tid.index(), RMGR.getLevDesc(r.tid.index(), TQL_base));
    unsigned target_lev = resQS.getMaxLev(), base_lev = RMGR.getLevDesc(r.tid.index(), TQL_base);
    int start_lev = target_lev > base_lev ? target_lev - base_lev : 0;

    // keep trying for ~25s to match watchdog timeout, this way if we have deadlock, we trigger watchdog+logerr
    // otherwise if application stalled for some time but in non related code (i.e. not deadlock in tex mgr),
    // we don't fail texture load and don't trigger logerr as everything is fine,
    // which happens if estimated trying time is less than watchdog timeout
    constexpr unsigned tries = 256u;

    {
      unsigned skip = (texDesc.dim.maxLev - target_lev);
      unsigned w = max(texDesc.dim.w >> skip, 1), h = max(texDesc.dim.h >> skip, 1);
      unsigned d = texDesc.dim.d, l = texDesc.dim.l - skip;
      t = dest_tex->makeTmpTexResCopy(w, h, d, l);
      if (!t)
      {
        logerr("%p->makeTmpTexResCopy(%d, %d, %d, %d) returns null (%s)", dest_tex, w, h, d, l, texname);
        return false;
      }
    }

    for (int s = 0; s < r.slice.size(); s++)
      if (r.slice[s].bqPack)
      {
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: BQ %s, ofs=0x%X", texname, t, s, r.slice[s].bqPack->file->packName,
          r.slice[s].bqPack->texRec[r.slice[s].bqIdx].ofs);
        FullFileLoadCB crd(r.slice[s].bqPack->file->packName);
        G_ASSERT(crd.fileHandle != NULL);
        unsigned fs = 0, rfs = 0;
        int64_t reft0 = 0;
        unsigned fe;
        unsigned rfe;
        int64_t reft;
        // no retry will happen if we fail here, ensure(with enough tries) we not fail on RUB quota
        for (unsigned attempt = 0; attempt < tries; attempt++)
        {
          crd.seekto(r.slice[s].bqPack->texRec[r.slice[s].bqIdx].ofs);
          TexLoadRes ldRet = d3d_load_ddsx_to_slice(t, s, r.slice[s].bqPack->texHdr[r.slice[s].bqIdx], crd, r.quality, start_lev, 0);

          if (ldRet != TexLoadRes::OK)
          {
            fe = dagor_frame_no() + 3;
            rfe = interlocked_acquire_load(texmgr_internal::drv_res_updates_flush_count) + 2;
            reft = profile_ref_ticks();
            if (attempt == 0)
              fs = fe - 3, rfs = rfe - 2, reft0 = reft;
          }
          else
            break;

          if (ldRet == TexLoadRes::ERR_RUB)
          {
            if (!is_main_thread())
            {
              while (dagor_frame_no() < fe && (unsigned)interlocked_acquire_load(texmgr_internal::drv_res_updates_flush_count) < rfe &&
                     profile_time_usec(reft) < 100000)
                sleep_msec(1);
            }
            else
              // on main thread we must process pending updates to free RUB memory
              d3d::driver_command(Drv3dCommand::PROCESS_PENDING_RESOURCE_UPDATED);

            if (attempt + 1 < tries)
              continue;
          }

          logAddManagedArrayTextureFailure(texname, s, false, ldRet, attempt, fs, reft0, rfs);

          return false;
        }
      }
      else if (r.slice[s].viaFactory)
      {
        TEXTUREID tid = get_managed_texture_id(r.slice[s].ddsxFn);
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: factory(%s)=%d", texname, t, s, r.slice[s].ddsxFn, tid);
        int id = RMGR.toIndex(tid);
        Tab<char> ddsx_data;

        if (RMGR.getFactory(id) && RMGR.getFactory(id)->getTextureDDSx(tid, ddsx_data))
        {
          InPlaceMemLoadCB crd(ddsx_data.data(), data_size(ddsx_data));
          ddsx::Header hdr;
          crd.read(&hdr, sizeof(hdr));

          TexLoadRes ldRet = d3d_load_ddsx_to_slice(t, s, hdr, crd, r.quality, start_lev, 0);
          if (ldRet != TexLoadRes::OK)
          {
            if (ldRet == TexLoadRes::ERR)
              FATALERR("add_managed_array_texture(%s): d3d_load_ddsx_to_slice(%d) failed", texname, s);
            return false;
          }
        }
        else
        {
          FATALERR("add_managed_array_texture(%s): getTextureDDSx(%d) failed", texname, s);
          return false;
        }
      }
      else if (!r.slice[s].ddsxFn.empty())
      {
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: %s", texname, t, s, r.slice[s].ddsxFn);
        ddsx::Header hdr;
        FullFileLoadCB crd(r.slice[s].ddsxFn);
        crd.read(&hdr, sizeof(hdr));
        TexLoadRes ldRet = d3d_load_ddsx_to_slice(t, s, hdr, crd, r.quality, start_lev, 0);
        if (ldRet != TexLoadRes::OK)
        {
          if (ldRet == TexLoadRes::ERR)
            FATALERR("add_managed_array_texture(%s): d3d_load_ddsx_to_slice(%d) failed", texname, s);
          return false;
        }
      }
      else
      {
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: empty slice", texname, t, s);
        // fill empty slice with zeroes
        ddsx::Header hdr = r.hdr;
        hdr.flags &= ~(hdr.FLG_ARRTEX | hdr.FLG_7ZIP | hdr.FLG_ZLIB | hdr.FLG_ZSTD | hdr.FLG_OODLE);

        if (zeroes.size())
          hdr.memSz = zeroes.size();
        else
        {
          for (int l = 0; l < hdr.levels; l++)
            hdr.memSz += hdr.getSurfaceSz(l);
          zeroes.resize(hdr.memSz);
          mem_set_0(zeroes);
        }
        InPlaceMemLoadCB crd(zeroes.data(), zeroes.size());
        TexLoadRes ldRet = d3d_load_ddsx_to_slice(t, s, hdr, crd, r.quality, start_lev, 0);
        if (ldRet != TexLoadRes::OK)
        {
          if (ldRet == TexLoadRes::ERR)
            FATALERR("add_managed_array_texture(%s): d3d_load_ddsx_to_slice(%d) failed", texname, s);
          return false;
        }
      }
    t->texmiplevel(start_lev, t->level_count() - 1);
    dest_tex->replaceTexResObject(t);
    RMGR.finishReading(r.tid.index());
    t = dest_tex;

    // load HQ data
    for (int s = 0; s < r.slice.size(); s++)
      if (r.slice[s].hqPack && target_lev > base_lev)
      {
        RMGR.startReading(r.tid.index(), RMGR.getLevDesc(r.tid.index(), TQL_high));
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: HQ %s, ofs=0x%X", texname, t, s, r.slice[s].hqPack->file->packName,
          r.slice[s].hqPack->texRec[r.slice[s].hqIdx].ofs);
        FullFileLoadCB crd(r.slice[s].hqPack->file->packName);
        G_ASSERT(crd.fileHandle != NULL);
        unsigned fs = 0, rfs = 0;
        int64_t reft0 = 0;
        unsigned fe;
        unsigned rfe;
        int64_t reft;
        // no retry will happen if we fail here, ensure(with enough tries) we not fail on RUB quota
        for (unsigned attempt = 0; attempt < tries; attempt++)
        {
          crd.seekto(r.slice[s].hqPack->texRec[r.slice[s].hqIdx].ofs);

          TexLoadRes ldRet =
            d3d_load_ddsx_to_slice(t, s, r.slice[s].hqPack->texHdr[r.slice[s].hqIdx], crd, r.quality, 0, resQS.getLdLev());

          if (ldRet != TexLoadRes::OK)
          {
            fe = dagor_frame_no() + 3;
            rfe = interlocked_acquire_load(texmgr_internal::drv_res_updates_flush_count) + 2;
            reft = profile_ref_ticks();
            if (attempt == 0)
              fs = fe - 3, rfs = rfe - 2, reft0 = reft;
          }
          else
            break;

          if (ldRet == TexLoadRes::ERR_RUB)
          {
            if (!is_main_thread())
            {
              while (dagor_frame_no() < fe && (unsigned)interlocked_acquire_load(texmgr_internal::drv_res_updates_flush_count) < rfe &&
                     profile_time_usec(reft) < 100000)
                sleep_msec(1);
            }
            else
              // on main thread we must process pending updates to free RUB memory
              d3d::driver_command(Drv3dCommand::PROCESS_PENDING_RESOURCE_UPDATED);

            if (attempt + 1 < tries)
              continue;
          }

          logAddManagedArrayTextureFailure(texname, s, true, ldRet, attempt, fs, reft0, rfs);

          return false;
        }
      }
      else if (r.slice[s].viaFactory && target_lev > base_lev)
      {
        TEXTUREID tid = get_managed_texture_id(r.slice[s].ddsxFn);
        TRACE_ARRTEX("arrTex: %s %p.load[%d]: HQ factory(%s)=%d", texname, t, s, r.slice[s].ddsxFn, tid);
        int id = RMGR.toIndex(tid);
        Tab<char> ddsx_data;

        if (RMGR.getFactory(id) && RMGR.getFactory(id)->getTextureDDSx(tid, ddsx_data))
        {
          InPlaceMemLoadCB crd(ddsx_data.data(), data_size(ddsx_data));
          ddsx::Header hdr;
          crd.read(&hdr, sizeof(hdr));

          TexLoadRes ldRet = d3d_load_ddsx_to_slice(t, s, hdr, crd, r.quality, 0, resQS.getLdLev());
          if (ldRet != TexLoadRes::OK)
          {
            if (ldRet == TexLoadRes::ERR)
              FATALERR("add_managed_array_texture(%s): d3d_load_ddsx_to_slice(%d) failed", texname, s);
            return false;
          }
        }
        else
        {
          FATALERR("add_managed_array_texture(%s): getTextureDDSx(%d) failed", texname, s);
          return false;
        }
      }

    if (resQS.isReading())
    {
      t->texmiplevel(0, t->level_count() - 1);
      RMGR.finishReading(r.tid.index());
    }

    return true;
  }

  void releaseTexture(BaseTexture *t, TEXTUREID id) override
  {
    TRACE_ARRTEX("arrTex: release %s tid=0x%x bt=%p", get_managed_texture_name(id), id, t);
    G_UNUSED(id);
    del_d3dres(t);
    RMGR.resQS[id.index()].setLdLev(0);
  }

  void onUnregisterTexture(TEXTUREID tid) override
  {
    for (int i = rec.size() - 1; i >= 0; i--)
      if (rec[i].tid == tid)
      {
        OSSpinlockScopedLock autolock(rec[i].spinlock);
        auto &r = rec[i];
        r.hdr.label = 0;
        r.tid = BAD_TEXTUREID;
        r.quality = -1;
        clear_and_shrink(r.slice);
        r.gen++;
        return;
      }
  }

  TEXTUREID addTex(const char *name, dag::ConstSpan<const char *> tex_slice_nm)
  {
    if (!loadQueue.queue)
    {
      loadQueue.allocQueue(MAX_REC_COUNT);
      rec.reserve(MAX_REC_COUNT);
    }

    const TEXTUREID tid = get_managed_texture_id(name);
    int r_idx = -1;
    for (int i = 0, n = rec.size(); i < n; i++)
      if (rec[i].tid == tid)
      {
        r_idx = i;
        break;
      }
    if (r_idx < 0 && rec.size() >= MAX_REC_COUNT)
    {
      FATALERR("add_managed_array_texture(%s): too many tex used %d", name, rec.size());
      return BAD_TEXTUREID;
    }

    TRACE_ARRTEX("arrTex: addTex(%s, %d) -> tid=0x%x r_idx=%d rc=%d (rec.count=%d total=%d)", name, tex_slice_nm.size(), tid, r_idx,
      get_managed_texture_refcount(tid), r_idx, rec.size(), rec.capacity());
    for (int i = 0; i < tex_slice_nm.size(); i++)
      if (tex_slice_nm[i] && *tex_slice_nm[i])
        TRACE_ARRTEX("  [%d] %s", i, tex_slice_nm[i]);
    if (tid != BAD_TEXTUREID)
    {
      if (r_idx < 0)
      {
        FATALERR("add_managed_array_texture(%s): trying to replace existing non-array-tex texID=0x%x", name, tid);
        return BAD_TEXTUREID;
      }
      OSSpinlockScopedLock autolock(rec[r_idx].spinlock);
      if (get_managed_texture_refcount(tid) > 0 && rec[r_idx].slice.size() != tex_slice_nm.size())
      {
        FATALERR("add_managed_array_texture(%s): trying to replace existing array texID with refcount=%d (%d slices -> %d slices)",
          name, get_managed_texture_refcount(tid), rec[r_idx].slice.size(), tex_slice_nm.size());
        return BAD_TEXTUREID;
      }
    }

    ddsx::Header hdr;
    ArrayTexRec r;
    memset(&r.hdr, 0, sizeof(r.hdr));
    memset(&hdr, 0, sizeof(hdr));
    String tex_stor_nm;
    bool arr_tex_hdr_inited = false;
    if (r_idx >= 0)
    {
      OSSpinlockScopedLock autolock(rec[r_idx].spinlock);
      arr_tex_hdr_inited = rec[r_idx].hdr.label && (get_managed_texture_refcount(tid) > 0);
      if (arr_tex_hdr_inited)
        r.hdr = rec[r_idx].hdr;
    }
    r.slice.resize(tex_slice_nm.size());
    for (int ti = 0; ti < tex_slice_nm.size(); ti++)
    {
      const char *tex_name = tex_slice_nm[ti];
      if (!tex_name || !*tex_name)
        continue;
      if (strchr(tex_name, '*'))
      {
      read_texpack:
        int idx = RMGR.toIndex(get_managed_texture_id(tex_name));
        if (idx < 0)
        {
          FATALERR("add_managed_array_texture(%s): failed to resolve %s", name, tex_name);
          return BAD_TEXTUREID;
        }
        tex_name = RMGR.getName(idx);
        tex_packs_lock.lockRead();
        auto &bq_ref = RMGR.texDesc[idx].packRecIdx[TQL_base];
        if (bq_ref.pack >= 0)
        {
          r.slice[ti].bqPack = tex_packs[bq_ref.pack].pack;
          r.slice[ti].bqIdx = bq_ref.rec;

          auto &hq_ref = RMGR.texDesc[idx].packRecIdx[TQL_high];
          if (hq_ref.pack >= 0)
          {
            r.slice[ti].hqPack = tex_packs[hq_ref.pack].pack;
            r.slice[ti].hqIdx = hq_ref.rec;
          }
        }
        tex_packs_lock.unlockRead();
        if (!r.slice[ti].bqPack)
        {
          Tab<char> ddsx_data;
          if (RMGR.getFactory(idx) && RMGR.getFactory(idx)->getTextureDDSx(get_managed_texture_id(tex_name), ddsx_data))
          {
            if (data_size(ddsx_data) < sizeof(hdr))
            {
              FATALERR("add_managed_array_texture(%s): factory->getTextureDDSx() returns bad data for %s", name, tex_name);
              return BAD_TEXTUREID;
            }
            r.slice[ti].viaFactory = true;
            r.slice[ti].ddsxFn = tex_name;
            hdr = *(ddsx::Header *)ddsx_data.data();
          }
          else
          {
            FATALERR("add_managed_array_texture(%s): failed to find %s in texpacks", name, tex_name);
            return BAD_TEXTUREID;
          }
        }
        if (r.slice[ti].hqPack)
        {
          hdr = r.slice[ti].hqPack->texHdr[r.slice[ti].hqIdx];
          if (hdr.lQmip < hdr.levels)
            hdr.lQmip = hdr.levels;
          hdr.levels += r.slice[ti].bqPack->texHdr[r.slice[ti].bqIdx].levels;
          hdr.flags &= ~hdr.FLG_HQ_PART;
        }
        else if (r.slice[ti].bqPack)
          hdr = r.slice[ti].bqPack->texHdr[r.slice[ti].bqIdx];
      }
      else if (strstr(tex_name, ".ddsx"))
      {
        FullFileLoadCB crd(tex_name);
        if (!crd.fileHandle)
        {
          FATALERR("add_managed_array_texture(%s): failed to open %s", name, tex_name);
          return BAD_TEXTUREID;
        }
        crd.read(&hdr, sizeof(hdr));
        if (!hdr.checkLabel())
        {
          FATALERR("add_managed_array_texture(%s): %s is not DDSx", name, tex_name);
          return BAD_TEXTUREID;
        }
        r.slice[ti].ddsxFn = tex_name;
      }
      else
      {
        tex_stor_nm.printf(0, "%s*", tex_name);
        tex_name = tex_stor_nm;
        goto read_texpack;
      }

      if (hdr.flags & (hdr.FLG_CUBTEX | hdr.FLG_VOLTEX | hdr.FLG_ARRTEX | hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER |
                        hdr.FLG_NEED_PAIRED_BASETEX | hdr.FLG_HOLD_SYSMEM_COPY))
      {
        FATALERR("add_managed_array_texture(%s): hdr(%s) bad flags=%08X", name, tex_slice_nm[ti], hdr.flags);
        return BAD_TEXTUREID;
      }
      if (!arr_tex_hdr_inited)
      {
        r.hdr = hdr;
        arr_tex_hdr_inited = true;
      }
      else if (r.hdr.d3dFormat != hdr.d3dFormat || r.hdr.w != hdr.w || r.hdr.h != hdr.h || r.hdr.levels > hdr.levels ||
               r.hdr.hqPartLevels != hdr.hqPartLevels || (r.hdr.flags & hdr.FLG_GAMMA_EQ_1) != (hdr.flags & hdr.FLG_GAMMA_EQ_1))
      {
        FATALERR("add_managed_array_texture(%s): r.hdr(%s) {f=%08X %dx%d l=%d,%d%s} != r.hdr(%s) {f=%08X %dx%d l=%d;%d%s}", name,
          tex_slice_nm[0], r.hdr.d3dFormat, r.hdr.w, r.hdr.h, r.hdr.levels, r.hdr.hqPartLevels,
          (r.hdr.flags & hdr.FLG_GAMMA_EQ_1) ? " G1" : "", tex_slice_nm[ti], hdr.d3dFormat, hdr.w, hdr.h, hdr.levels, hdr.hqPartLevels,
          (hdr.flags & hdr.FLG_GAMMA_EQ_1) ? " G1" : "");
        return BAD_TEXTUREID;
      }
    }

    r.hdr.flags |= hdr.FLG_ARRTEX;
    r.hdr.depth = r.slice.size();
    r.hdr.memSz = r.hdr.packedSz = 0;
    if (r_idx >= 0 && tid != BAD_TEXTUREID)
    {
      bool add_to_queue = false;
      r.tid = tid;
      rec[r_idx].spinlock.lock();
      r.quality = rec[r_idx].quality;
      r.gen = rec[r_idx].gen;
      if (memcmp(&rec[r_idx].hdr, &r.hdr, sizeof(r.hdr)) != 0)
      {
        G_ASSERTF(!get_managed_texture_refcount(tid), "discard_unused_managed_texture(%s tid=0x%x)", name, tid);
        debug("arrTex: discard unused ridx=%d tid=0x%x", r_idx, tid);
        rec[r_idx].spinlock.unlock();
        discard_unused_managed_texture(tid);
        rec[r_idx].spinlock.lock();
      }
      else if (!texmgr_internal::dbg_texq_only_stubs && r.hdr.label && RMGR.getD3dRes(tid.index()))
        add_to_queue = true;

      updateTexDesc(r);
      rec[r_idx] = eastl::move(r);

      if (add_to_queue)
        addLoadQueueRec(tid, r_idx);

      rec[r_idx].spinlock.unlock();

      if (texmgr_internal::hook_on_get_texture_id && RMGR.toIndex(tid) >= 0)
        texmgr_internal::hook_on_get_texture_id(tid);
      return tid;
    }
    r.tid = add_managed_texture(name, this);
    if (r.tid != BAD_TEXTUREID)
    {
      if (r_idx < 0)
      {
        r_idx = rec.appendOne();
        if (DAGOR_LIKELY(r_idx >= 0))
        {
          // no need to use spinlock here, interlocked within appendOne takes care of that
          updateTexDesc(r);
          rec[r_idx] = eastl::move(r);
        }
        else // very unlikely case of simultaneous addTex call from two threads in case of free last slot of rec array
        {
          FATALERR("add_managed_array_texture(%s): too many tex used %d", name, rec.size());
          return BAD_TEXTUREID;
        }
      }
      else
      {
        OSSpinlockScopedLock autolock(rec[r_idx].spinlock);
        r.gen = rec[r_idx].gen;
        updateTexDesc(r);
        rec[r_idx] = eastl::move(r);
      }
    }
    else
      FATALERR("add_managed_array_texture(%s): failed to add texture ID", name);
    if (texmgr_internal::hook_on_get_texture_id && RMGR.toIndex(r.tid) >= 0)
      texmgr_internal::hook_on_get_texture_id(r.tid);
    return r.tid;
  }
  void updateTexDesc(const ArrayTexRec &r)
  {
    int idx = r.tid.index();
    auto &desc = RMGR.texDesc[idx];
    desc.dim.w = r.hdr.w;
    desc.dim.h = r.hdr.h;
    desc.dim.d = r.hdr.depth;
    desc.dim.l = r.hdr.levels;
    desc.dim.maxLev = get_log2i(max(r.hdr.w, r.hdr.h));
    unsigned bq_mip = 0;
    RMGR.setLevDesc(idx, TQL_high, desc.dim.maxLev);
    for (const auto &s : r.slice)
      if (s.hqPack)
      {
        const auto &bhdr = s.bqPack->texHdr[s.bqIdx];
        if (!bq_mip)
        {
          RMGR.setLevDesc(idx, TQL_base, bq_mip = get_log2i(max(bhdr.w, bhdr.h)));
          RMGR.setLevDesc(idx, TQL_high, desc.dim.maxLev);
        }
        else
          G_ASSERTF(bq_mip == get_log2i(max(bhdr.w, bhdr.h)),
            "arrtex(%s) bqMip=%d (%dx%d,L%d) in slice[%d] differs from used bqMip=%d", RMGR.getName(idx),
            get_log2i(max(bhdr.w, bhdr.h)), bhdr.w, bhdr.h, bhdr.levels, &s - r.slice.data(), bq_mip);
      }
    if (!bq_mip)
      RMGR.setLevDesc(idx, TQL_base, desc.dim.maxLev);
    auto &resQS = RMGR.resQS[r.tid.index()];
    resQS.setQLev(desc.dim.maxLev);
    resQS.setMaxQL(RMGR.calcMaxQL(idx), RMGR.calcCurQL(idx, resQS.getLdLev()));
    // RMGR.dumpTexState(idx);
  }

  void term_load_queue() { loadQueue.freeQueue(); }

  void process_load_queue()
  {
    while (process_one_queue_item()) {}
  }

  bool process_one_queue_item()
  {
    LoadQueueRec lqr = loadQueue.pop();
    if (!lqr)
      return false; // queue exhausted

    int tex_idx = RMGR.toIndex(lqr.tid);

    {
      OSSpinlockScopedLock autolock(rec[lqr.recIdx].spinlock);
      if (lqr.gen != rec[lqr.recIdx].gen)
        return true;
      debug("arrTex: process_one_queue_item tid=0x%x lqr.gen=%d rec[%d].gen=%d slices.count=%d d3dRes=%p rc=%d", lqr.tid, lqr.gen,
        lqr.recIdx, rec[lqr.recIdx].gen, rec[lqr.recIdx].slice.size(), RMGR.getD3dRes(tex_idx), RMGR.getRefCount(tex_idx));
      G_ASSERT(lqr.tid == rec[lqr.recIdx].tid);
    }

    while (!RMGR.getD3dRes(tex_idx) && RMGR.getRefCount(tex_idx)) // createTexture() call not finished yet
      sleep_msec(0);

    if (!RMGR.getD3dRes(tex_idx) && !RMGR.getRefCount(tex_idx)) // already destroyed
    {
      debug("arrTex: skip loading discarded lqr.tid=0x%x", lqr.tid);
      return true;
    }

    bool ld = loadArrTexData(lqr.recIdx, acquire_managed_tex(lqr.tid));
    release_managed_tex(lqr.tid);
    if (!ld)
      logerr("loadArrTexData(%s tid=0x%x)", get_managed_texture_name(lqr.tid), lqr.tid);
    return true;
  }

  void addLoadQueueRec(TEXTUREID tid, int rec_idx)
  {
    debug("arrTex: addToQueue tid=0x%x rec=%d", tid, rec_idx);
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
    G_ASSERT(*reinterpret_cast<volatile int *>(&rec[rec_idx].spinlock)); // must be locked (for gen increment)
#endif
    loadQueue.push(
      LoadQueueRec(tid, rec_idx, ++rec[rec_idx].gen), /*on_queue_full_cb=*/
      [](void *self) {
        debug("%s: loadQueue.size()=%d", "addLoadQueueRec", reinterpret_cast<DDSxArrayTextureFactory *>(self)->loadQueue.size());
        static_cast<DDSxArrayTextureFactory *>(self)->process_one_queue_item();
      },
      this);
    if (texmgr_internal::reload_jobmgr_id >= 0)
      cpujobs::add_job(texmgr_internal::reload_jobmgr_id, new ProcessQueueJob);
  }
};
static DDSxArrayTextureFactory arr_tex_factory;
static void process_arr_tex_load_queue() { arr_tex_factory.process_load_queue(); }
static void term_arr_tex_load_queue() { arr_tex_factory.term_load_queue(); }
#undef FATALERR

TEXTUREID add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm)
{
  return arr_tex_factory.addTex(name, tex_slice_nm);
}

TEXTUREID update_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm) // just for readability
{
  return add_managed_array_texture(name, tex_slice_nm);
}

unsigned reload_managed_array_textures_for_changed_slice(const char *slice_tex_name)
{
  Tab<SimpleString> slicesNames;
  String tmp_stor1, tmp_stor2;
  unsigned changed_arrtex = 0;

  slice_tex_name = TextureMetaData::decodeFileName(slice_tex_name, &tmp_stor1);
  for (auto &texRec : arr_tex_factory.rec)
  {
    int tex_slice_idx = -1;
    slicesNames.clear();
    {
      OSSpinlockScopedLock autolock(texRec.spinlock);
      for (const auto &slice : texRec.slice)
      {
        slicesNames.push_back() = TextureMetaData::decodeFileName(
          slice.bqPack ? slice.bqPack->getTextureNameById(slice.bqIdx) : slice.ddsxFn.c_str(), &tmp_stor2);
        if (tex_slice_idx < 0 && slicesNames.back() == slice_tex_name)
          tex_slice_idx = &slice - texRec.slice.data();
      }
    }

    if (tex_slice_idx >= 0)
    {
      const char *arrtex_name = get_managed_texture_name(texRec.tid);
      debug("reloading arrtex: %s (slice[%d] %s changed)", arrtex_name, tex_slice_idx, slice_tex_name);
      // this make_span is legal since 'SimpleString' matches layout of 'char*'
      update_managed_array_texture(arrtex_name, make_span((const char **)slicesNames.data(), slicesNames.size()));
      changed_arrtex++;
    }
  }
  return changed_arrtex;
}

static void reload_active_array_textures()
{
  int i = 0;
  for (auto &r : arr_tex_factory.rec)
  {
    if (r.tid == BAD_TEXTUREID || get_managed_texture_refcount(r.tid) <= 0)
    {
      ++i;
      continue;
    }
    debug("reloading array tex: %s", get_managed_texture_name(r.tid));
    if (!arr_tex_factory.loadArrTexData(i++, acquire_managed_tex(r.tid)))
      logerr("failed to reload array tex: <%s>(0x%x)", get_managed_texture_name(r.tid), r.tid);
    release_managed_tex(r.tid);
  }
}

bool is_managed_texture_incomplete(TEXTUREID tid)
{
  int idx = RMGR.toIndex(tid);
  if (idx < 0)
    return false;

  if (texmgr_internal::dbg_texq_only_stubs)
    return false;

  texmgr_internal::acquire_texmgr_lock();
  bool incomplete = false;
  if (strchr(RMGR.getName(idx), '*'))
  {
    auto &bq_ref = RMGR.texDesc[idx].packRecIdx[TQL_base];
    if (bq_ref.pack >= 0 && (RMGR.texDesc[idx].packRecIdx[TQL_high].pack < 0 && RMGR.texDesc[idx].packRecIdx[TQL_uhq].pack < 0))
      if (tex_packs[bq_ref.pack].pack->file->texProps[bq_ref.rec].initHqPartLev > 0)
        incomplete = true;
  }
  texmgr_internal::release_texmgr_lock();
  return incomplete;
}

void ddsx::init_ddsx_load_queue(int qsz)
{
  ddsxLoadQueue.freeQueue();
  ddsxLoadQueue.allocQueue(qsz);
}
void ddsx::term_ddsx_load_queue() { ddsxLoadQueue.freeQueue(); }
void ddsx::enqueue_ddsx_load(BaseTexture *t, TEXTUREID tid, const char *fn, int quality_id, TEXTUREID bt_id)
{
  if (bt_id != BAD_TEXTUREID && RMGR.pairedBaseTexId[tid.index()] == BAD_TEXTUREID) // non-streamable texture case
  {
    RMGR.incBaseTexRc(bt_id);
    acquire_managed_tex(bt_id);
  }
  ddsxLoadQueue.push(DDSxLoadQueueRec(t, tid, fn, quality_id, bt_id), NULL, NULL);
}

void ddsx::process_ddsx_load_queue()
{
  while (ddsxLoadQueue.size())
  {
    DDSxLoadQueueRec rec = ddsxLoadQueue.pop();
    if (!rec.tex)
      continue;

    BaseTexture *cur_tex = get_managed_texture_refcount(rec.tid) > 0 ? acquire_managed_tex(rec.tid) : NULL;
    if (cur_tex != rec.tex)
    {
      // texture was destroyed before loading
      // debug("skip loading %s due to %p != %p", rec.fn, cur_tex, rec.tex);
      if (RMGR.texDesc[rec.tid.index()].dim.stubIdx >= 0)
        RMGR.cancelReading(rec.tid.index());
      else if (rec.btId != BAD_TEXTUREID)
      {
        RMGR.decBaseTexRc(rec.btId);
        release_managed_tex(rec.btId);
      }
      if (cur_tex)
        release_managed_tex(rec.tid);
      else
        discard_unused_managed_texture(rec.tid); // discard texture object to force loading on next acquire_managed_tex()
      continue;
    }

    // debug("loading %s  %p btId=0x%x (data %p)", rec.fn, rec.tex, rec.btId, RGMR.getTexBaseData(rec.btId));
    if (rec.btId != BAD_TEXTUREID && !RMGR.hasTexBaseData(rec.btId) && !d3d::is_stub_driver())
    {
      if (RMGR.pairedBaseTexId[rec.tid.index()] == BAD_TEXTUREID) // non-streamable texture case
      {
        release_managed_tex(rec.btId);
        RMGR.decBaseTexRc(rec.btId);
      }
      release_managed_tex(rec.tid);
      if (!ddsxLoadQueue.size())
        sleep_msec(10);
      ddsx::enqueue_ddsx_load(rec.tex, rec.tid, rec.fn, rec.qId, rec.btId);
      continue;
    }

    FullFileLoadCB crd(rec.fn, DF_READ | DF_IGNORE_MISSING);
    if (crd.fileHandle)
    {
      ddsx::Header hdr;
      crd.read(&hdr, sizeof(hdr));
      if (RMGR.texDesc[rec.tid.index()].dim.stubIdx >= 0)
      {
        TexLoadRes ldRet = RMGR.readDdsxTex(rec.tid, hdr, crd, 0);
        if (ldRet == TexLoadRes::ERR)
          if (!d3d::is_in_device_reset_now())
            logerr("failed to load DDSx contents: %s", rec.fn);
      }
      else
      {
        TexLoadRes ldRet = d3d_load_ddsx_tex_contents(rec.tex, rec.tid, rec.btId, hdr, crd, rec.qId);
        if (ldRet == TexLoadRes::ERR)
          if (!d3d::is_in_device_reset_now())
            logerr("failed to load DDSx contents: %s", rec.fn);
      }

      int idx = RMGR.toIndex(rec.tid);
      RMGR.resQS[idx].setCurQL(TQL_base);
      if (RMGR.resQS[idx].isReading())
        RMGR.markUpdatedAfterLoad(rec.tid.index());
    }

    if (rec.btId != BAD_TEXTUREID && RMGR.pairedBaseTexId[rec.tid.index()] == BAD_TEXTUREID) // non-streamable texture case
    {
      release_managed_tex(rec.btId);
      RMGR.decBaseTexRc(rec.btId);
    }
    release_managed_tex(rec.tid);
  }
}
