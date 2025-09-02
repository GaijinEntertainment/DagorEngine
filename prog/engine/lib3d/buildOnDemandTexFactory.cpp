// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_buildOnDemandTexFactory.h>
#include <drv/3d/dag_texture.h>
#include <3d/dag_createTex.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <3d/dag_texIdSet.h>
#include <3d/ddsxTex.h>
#include <3d/ddsxTexMipOrder.h>
#include <drv/3d/dag_lock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <osApiWrappers/dag_cpuJobsQueue.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_atomic.h>
#include <drv/3d/dag_info.h>

#include "texMgrData.h"

namespace build_on_demand_tex_factory
{
struct AsyncMassLoadJob;
struct DDSxPrebuildCtxBase;
struct DDSxPrebuildCtx;
struct DDSxBuildOnDemandTexFactory;

struct TexIdToLoad
{
  TEXTUREID id = BAD_TEXTUREID;
  unsigned nameId = ~0u;
};

struct SharedData
{
  Tab<ddsx::Header> ddsxHdr;
  DDSxPrebuildCtx *dCtx = nullptr;
  unsigned usageCount = 0;
  unsigned texLoadingAllowed = unsigned(true);
  int asyncMassLoadJobInProgress = 0;

  void addRef();
  void delRef();
};

static void update_on_each_frame(void *_f);
static void tql_on_frame_finished();

static void (*base_engine_on_frame_finished)() = nullptr;
static SharedData shared;
} // namespace build_on_demand_tex_factory

using texmgr_internal::RMGR;


//! threaded DDSx prebuild context (TQL-ordered queues are processed by thread workers, pull model)
struct build_on_demand_tex_factory::DDSxPrebuildCtxBase
{
  cpujobs::JobQueue<cpujobs::IJob *, false> q[TQL__COUNT]; //< queues for each TQL level (thumb, base, high, uhq)
  OSSpinlock jobsPoolSL;
  volatile int maybeIdle = 0;

  cpujobs::IJob *getNextJob()
  {
    for (auto &q1 : q)
      if (auto *j = q1.pop())
        return j;
    return nullptr;
  }
};
struct build_on_demand_tex_factory::DDSxPrebuildCtx : DDSxPrebuildCtxBase
{
  static constexpr int MAX_WORKERS = 32;

  //! job to prebuild (update cache) one DDSx texture by nameId; completition is marked as *status=1
  struct PrebuildJob : public cpujobs::IJob
  {
    volatile uint8_t *status = nullptr;
    ITexBuildOnDemandHelper *build_helper = nullptr;
    volatile uint32_t *updateCount = nullptr;
    TexIdToLoad toLoad;

    bool initJob(ITexBuildOnDemandHelper *h, const TexIdToLoad &tl, volatile uint8_t *s, volatile uint32_t *upd_cnt)
    {
      G_ASSERT(!interlocked_acquire_load(done));
      G_ASSERT(interlocked_acquire_load(*s) == 0);
      build_helper = h;
      toLoad = tl;
      status = s;
      updateCount = upd_cnt;
      return true;
    }

    bool initJob(ITexBuildOnDemandHelper *h, unsigned nameId, volatile uint8_t *s, volatile uint32_t *upd_cnt)
    {
      G_ASSERT(!interlocked_acquire_load(done));
      G_ASSERT(interlocked_acquire_load(*s) == 0);
      build_helper = h;
      toLoad.id = BAD_TEXTUREID;
      toLoad.nameId = nameId;
      status = s;
      updateCount = upd_cnt;
      return true;
    }

    bool initJob(ITexBuildOnDemandHelper *h, unsigned nameId)
    {
      build_helper = h;
      toLoad.id = BAD_TEXTUREID;
      toLoad.nameId = nameId;
      status = nullptr;
      updateCount = nullptr;
      return true;
    }

    const char *getJobName(bool &) const override { return "PrebuildJob"; }

    virtual void doJob()
    {
      bool cache_was_updated = false;
      if ((toLoad.id == BAD_TEXTUREID || get_managed_texture_refcount(toLoad.id) > 0 || RMGR.getBaseTexRc(toLoad.id) > 0) &&
          interlocked_acquire_load(shared.texLoadingAllowed))
      {
        if (!build_helper->prebuildTex(toLoad.nameId, cache_was_updated))
          debug("failed to update tex %s", build_helper->getTexName(toLoad.nameId));
      }
      if (updateCount && cache_was_updated)
        interlocked_increment(*updateCount);
      if (status)
        interlocked_release_store(*status, 1);
    }
  };

  //! worker thread that pulls and processes job from TQL-ordered queues
  struct PrebuildThread final : public DaThread
  {
    os_event_t event;
    DDSxPrebuildCtxBase *ctx = nullptr;
    unsigned wIdx = 0;

    PrebuildThread() : DaThread("DDSx prebuild", 256 << 10, 0, WORKER_THREADS_AFFINITY_MASK)
    {
      stripStackInMinidump();
      os_event_create(&event, nullptr);
    }
    ~PrebuildThread() { os_event_destroy(&event); }

    void execute() override
    {
      while (!isThreadTerminating())
      {
        interlocked_or(ctx->maybeIdle, 1 << wIdx);
        int wres = os_event_wait(&event, cpujobs::DEFAULT_IDLE_TIME_SEC / 2 * 1000);
        interlocked_and(ctx->maybeIdle, ~(1 << wIdx));
        if (isThreadTerminating())
          break;

        cpujobs::IJob *job = ctx->getNextJob();
        if (!job)
        {
          if (DAGOR_UNLIKELY(wres == OS_WAIT_TIMEOUTED))
          {
            OSSpinlockScopedLock lock(ctx->jobsPoolSL);
            if (os_event_wait(&event, OS_WAIT_IGNORE) != OS_WAIT_OK)
              break;
          }
          continue;
        }

        while (job)
        {
          G_ASSERT(interlocked_acquire_load(job->done) == 0);
          job->doJob();
          G_ASSERT(interlocked_acquire_load(job->done) == 0);
          interlocked_release_store(job->done, 1);
          if (isThreadTerminating()) // check quit request
            return;
          job = ctx->getNextJob();
        }
      }
    }
  };

  PrebuildThread workers[MAX_WORKERS];
  PrebuildJob *jobs;
  int numWorkers;

  DDSxPrebuildCtx(int nworkers)
  {
    numWorkers = min(nworkers, MAX_WORKERS);
    for (auto &q1 : q)
      q1.allocQueue(get_bigger_pow2(numWorkers * 2));
    jobs = new PrebuildJob[numWorkers * 2];
    for (int i = 0; i < numWorkers; i++)
    {
      workers[i].ctx = this;
      workers[i].wIdx = i;
    }
  }
  ~DDSxPrebuildCtx()
  {
    for (int i = 0; i < numWorkers; i++)
      workers[i].terminate(true, -1, &workers[i].event);
    for (auto &q1 : q)
      q1.freeQueue();
    delete[] jobs;
  }

  bool wakeUpIdle()
  {
    for (int i = 0, tbit = 1; i < numWorkers; ++i, tbit <<= 1)
      if (tbit & interlocked_acquire_load(maybeIdle))
      {
        interlocked_and(maybeIdle, ~tbit); // Not idle anymore
        G_VERIFY(os_event_set(&workers[i].event) == 0);
        return true;
      }
    return false;
  }

  bool startOne()
  {
    for (int i = 0; i < numWorkers; ++i)
    {
      if (workers[i].isThreadRunnning())
        continue;
      if (workers[i].isThreadStarted())
        workers[i].terminate(true); // ZOMBIE -> DEAD
      G_VERIFY(os_event_set(&workers[i].event) == 0);
      G_VERIFY(workers[i].start());
      return true;
    }
    return false;
  }

  PrebuildJob *allocJob()
  {
    OSSpinlockScopedLock lock(jobsPoolSL);
    for (int i = 0; i < numWorkers * 2; i++)
      if (interlocked_acquire_load(jobs[i].done))
      {
        interlocked_release_store(jobs[i].done, 0);
        return &jobs[i];
      }
    return nullptr;
  }

  void submitJob(PrebuildJob *j, TexQL ql)
  {
    q[ql].push(j, nullptr, nullptr);
    OSSpinlockScopedLock lock(jobsPoolSL);
    if (!wakeUpIdle())
      if (!startOne())
        G_VERIFY(os_event_set(&workers[0].event) == 0); // All workers are started and busy - wake up first one just in case
  }
};


//! texture factory that implements async build and load of DDSx using ITexBuildOnDemandHelper interface
struct build_on_demand_tex_factory::DDSxBuildOnDemandTexFactory : public TextureFactory
{
  void init(ITexBuildOnDemandHelper *h)
  {
    defFactory = ::get_default_tex_factory();
    build_helper = h;

    shared.addRef();
  }
  void term()
  {
    if (defFactory)
      ::set_default_tex_factory(defFactory);
    shared.delRef();
    onTexFactoryDeleted(this);
    build_helper = nullptr;
    defFactory = nullptr;
  }

  static void mark_cur_ql(unsigned idx, TexQL ql)
  {
    if (RMGR.getLevDesc(idx, TQL_base) == 1)
    {
      RMGR.levDesc[idx] = 0;
      RMGR.setLevDesc(idx, ql, 2);
      RMGR.resQS[idx].setQLev(2);
    }
    RMGR.resQS[idx].setLdLev(ql == TQL_stub ? 1 : RMGR.getLevDesc(idx, ql));
    RMGR.resQS[idx].setMaxQL(ql, ql);
  }

  bool scheduleTexLoading(TEXTUREID id, TexQL ql) override
  {
    if (get_managed_texture_refcount(id) == 0 && RMGR.getBaseTexRc(id) == 0)
    {
      RMGR.cancelReading(id.index());
      return false;
    }

    unsigned nameId = getAssetNameIdFromPackRecIdx(id.index(), ql);
    G_ASSERTF_RETURN(nameId != ~0u, false, "id=0x%x (%s) ql=%d", id, get_managed_texture_name(id), ql);

    {
      OSSpinlockScopedLock lock(toLoadPendSL);
      toLoadPend[ql].push_back({id, nameId});
    }
    interlocked_increment(pendingTexCount[ql]);
    interlocked_increment(pendingTexCount[TQL__COUNT]);
    return true;
  }

  static unsigned getMaxQLev(unsigned idx, TexQL max_ql)
  {
    unsigned max_lev = 0;
    for (TexQL ql = max_ql;; ql = TexQL(ql - 1))
      if ((max_lev = RMGR.getLevDesc(idx, ql)) != 0 || ql == TQL__FIRST)
        break;
    return max_lev;
  }

  BaseTexture *createTexture(TEXTUREID id) override
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return nullptr;

    int idx = id.index();
    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));

    if (!build_helper->checkTexNameHandled(textureName))
    {
      mark_cur_ql(idx, TQL_base);
      return defFactory->createTexture(id);
    }
    else
      erase_items(fpath, fpath.size() - 2, 1);

    auto *a = build_helper->resolveTex(fpath, tmd);
    if (a)
    {
      ddsx::Header hdr;
      unsigned q_lev_desc = 0;
      unsigned name_ids[4];
      if (build_helper->getConvertibleTexHeader(a, hdr, q_lev_desc, name_ids))
      {
        updateDDSxHdr(idx, hdr, tmd, q_lev_desc, name_ids);

        BaseTexture *bt =
          d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, build_helper->getTexName(name_ids[TQL_base]), RMGR.texDesc[idx].dim.stubIdx);
        bt->setTID(id);
        if (RMGR.resQS[idx].getMaxReqLev() > 1)
          RMGR.scheduleReading(idx, this);
        return bt;
      }
    }

    float gamma = 1.0f;
    build_helper->getSimpleTexProps(a, fpath, gamma);

    int flags = TEXCF_RGB | TEXCF_ABEST;
    if (gamma > 1)
      flags |= TEXCF_SRGBREAD;

    if (BaseTexture *bt = ::create_texture(tmd.encode(fpath, &tmd_stor), flags, 0, false))
      return bt;
    logerr("failed to load <%s>", fpath);
    return nullptr;
  }

  void releaseTexture(BaseTexture *texture, TEXTUREID id) override
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return;

    if (!build_helper->checkTexNameHandled(textureName))
      return defFactory->releaseTexture(texture, id);

    unsigned idx = id.index();
    RMGR.setD3dRes(idx, nullptr);
    if (RMGR.resQS[idx].isReading())
      RMGR.cancelReading(idx);
    RMGR.markUpdated(idx, 0);

    if (texture)
      texture->destroy();
  }

  void unpack_ddsx(IGenLoad &crd, dag::Span<char> &b, ddsx::Header &hdr) const
  {
    auto unpacked_b = dag::Span<char>((char *)memalloc(sizeof(hdr) + hdr.memSz, tmpmem), sizeof(hdr) + hdr.memSz);
    hdr.flags &= ~hdr.FLG_COMPR_MASK;
    hdr.packedSz = 0;
    memcpy(unpacked_b.data(), &hdr, sizeof(hdr));
    crd.read(unpacked_b.data() + sizeof(hdr), hdr.memSz);
    build_helper->releaseDDSxTexData(b);
    crd.ceaseReading();
    b = unpacked_b;
  };
  void unpackDdsxBuf(dag::Span<char> &b) const
  {
    ddsx::Header &hdr = *reinterpret_cast<ddsx::Header *>(b.data());
    InPlaceMemLoadCB fcrd(b.data() + sizeof(hdr), hdr.packedSz);

    if (hdr.isCompressionZSTD())
    {
      ZstdLoadCB crd(fcrd, hdr.packedSz);
      unpack_ddsx(crd, b, hdr);
    }
    else if (hdr.isCompressionOODLE())
    {
      OodleLoadCB crd(fcrd, hdr.packedSz, hdr.memSz);
      unpack_ddsx(crd, b, hdr);
    }
    else if (hdr.isCompressionZLIB())
    {
      ZlibLoadCB crd(fcrd, hdr.packedSz);
      unpack_ddsx(crd, b, hdr);
    }
    else if (hdr.isCompression7ZIP())
    {
      LzmaLoadCB crd(fcrd, hdr.packedSz);
      unpack_ddsx(crd, b, hdr);
    }
  }

  bool getTextureDDSx(TEXTUREID id, Tab<char> &out_ddsx) override
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return false;

    if (!build_helper->checkTexNameHandled(textureName))
      return false;

    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));
    erase_items(fpath, fpath.size() - 2, 1);

    ddsx::Header hdr;
    unsigned q_lev_desc = 0, name_ids[4];
    if (build_helper->getConvertibleTexHeader(build_helper->resolveTex(fpath, tmd), hdr, q_lev_desc, name_ids))
    {
      const int hdr_sz = sizeof(ddsx::Header);
      G_ASSERTF_RETURN(((q_lev_desc >> 4) & 0xF) && name_ids[TQL_base] != ~0u, false, "q_lev_desc=0x%x bq.nameId=%d", q_lev_desc,
        name_ids[TQL_base]);
      dag::Span<char> b = build_helper->getDDSxTexData(name_ids[TQL_base]);
      if (b.empty())
        return false;

      unpackDdsxBuf(b);
      ddsx_forward_mips_inplace(*reinterpret_cast<ddsx::Header *>(b.data()), b.data() + hdr_sz, b.size() - hdr_sz);

      TexQL hql = ((q_lev_desc >> 12) & 0xF) ? TQL_uhq : ((q_lev_desc >> 8) & 0xF) ? TQL_high : TQL_stub;
      G_ASSERTF_RETURN(hql == TQL_stub || name_ids[hql] != ~0u, false, "q_lev_desc=0x%x hq.nameId=%d", q_lev_desc, name_ids[hql]);

      dag::Span<char> hq_b = (hql != TQL_stub) ? build_helper->getDDSxTexData(name_ids[hql]) : dag::Span<char>();
      if (!hq_b.empty())
      {
        unpackDdsxBuf(hq_b);
        ddsx::Header &hq_hdr = *reinterpret_cast<ddsx::Header *>(hq_b.data());
        ddsx_forward_mips_inplace(hq_hdr, hq_b.data() + hdr_sz, hq_b.size() - hdr_sz);

        if (hq_hdr.levels)
        {
          ddsx::Header hdr = hq_hdr;
          if (hdr.lQmip < hdr.levels)
            hdr.lQmip = hdr.levels;
          hdr.levels += reinterpret_cast<ddsx::Header *>(b.data())->levels;
          hdr.flags &= ~hdr.FLG_HQ_PART;

          if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
          {
            out_ddsx = hq_b;
            memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
          }
          else
          {
            out_ddsx.resize(hq_b.size() + b.size() - hdr_sz);
            memcpy(out_ddsx.data(), hq_b.data(), hq_b.size());
            memcpy(out_ddsx.data() + hq_b.size(), b.data() + hdr_sz, b.size() - hdr_sz);
            memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
          }
          build_helper->releaseDDSxTexData(b);
          build_helper->releaseDDSxTexData(hq_b);
          return true;
        }
        build_helper->releaseDDSxTexData(hq_b);
      }
      out_ddsx = b;
      build_helper->releaseDDSxTexData(b);
      return true;
    }
    return false;
  }

public:
  bool updateDDSxHdr(unsigned idx, const ddsx::Header &hdr, const TextureMetaData &tmd, unsigned q_lev_desc, unsigned *name_ids)
  {
    ddsx::Header &rmgr_hdr = shared.ddsxHdr[idx];
    if (!rmgr_hdr.label || memcmp(&rmgr_hdr, &hdr, sizeof(hdr)) != 0)
    {
      rmgr_hdr = hdr;
      auto &desc = RMGR.texDesc[idx];
      auto &resQS = RMGR.resQS[idx];
      desc.dim.w = rmgr_hdr.w;
      desc.dim.h = rmgr_hdr.h;
      desc.dim.d = rmgr_hdr.depth;
      desc.dim.l = rmgr_hdr.levels;
      desc.dim.maxLev = get_log2i(max(desc.dim.w, desc.dim.h));
      desc.dim.stubIdx = (tmd.stubTexIdx == 15) ? -1 : tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr));

      setupPackRecIdx(idx, TQL_thumb, q_lev_desc & 0xF, name_ids);
      setupPackRecIdx(idx, TQL_base, (q_lev_desc >> 4) & 0xF, name_ids);
      setupPackRecIdx(idx, TQL_high, (q_lev_desc >> 8) & 0xF, name_ids);
      setupPackRecIdx(idx, TQL_uhq, (q_lev_desc >> 12) & 0xF, name_ids);

      resQS.setLdLev(1);
      resQS.setMaxQL(RMGR.calcMaxQL(idx), TQL_stub);
      resQS.setQLev(desc.dim.maxLev);
      if (maxQL != TQL_uhq) // globally limit quality
      {
        uint8_t lev = (maxQL == TQL_stub) ? 1 : getMaxQLev(idx, maxQL);
        if (!lev)
          lev = 1;

        resQS.setQLev(0); // to remove influence on calcCurQL() call
        resQS.setMaxQL(RMGR.calcCurQL(idx, lev), TQL_stub);
        resQS.setQLev(lev);
      }
      if (!texmgr_internal::texq_load_on_demand)
        resQS.setMaxReqLev(15);
      RMGR.texSamplerInfo[idx] = get_sampler_info(tmd);
      RMGR.texSamplers[idx] = d3d::request_sampler(RMGR.texSamplerInfo[idx]);

      TEXTUREID bt_id = BAD_TEXTUREID;
      if (!tmd.baseTexName.empty())
      {
        bt_id = get_managed_texture_id(String::mk_str_cat(tmd.baseTexName, "*"));
        if (bt_id && !shared.ddsxHdr[bt_id.index()].label)
        {
          ddsx::Header bt_hdr;
          TextureMetaData bt_tmd;
          unsigned bt_q_lev_desc = 0;
          unsigned bt_name_ids[4];
          auto *bt_a = build_helper->resolveTex(String::mk_str_cat(tmd.baseTexName, "*"), bt_tmd);
          if (build_helper->getConvertibleTexHeader(bt_a, bt_hdr, bt_q_lev_desc, bt_name_ids))
            updateDDSxHdr(bt_id.index(), bt_hdr, bt_tmd, bt_q_lev_desc, bt_name_ids);
        }
      }
      RMGR.pairedBaseTexId[idx] = bt_id;
      return true;
    }
    return false;
  }
  bool forceMaxTexQL(TEXTUREID tid, TexQL ql)
  {
    G_ASSERT(!interlocked_acquire_load(shared.texLoadingAllowed) && !interlocked_acquire_load(shared.asyncMassLoadJobInProgress));
    unsigned idx = tid.index();
    ddsx::Header &rmgr_hdr = shared.ddsxHdr[idx];
    if (!rmgr_hdr.label)
      return false;
    auto &resQS = RMGR.resQS[idx];
    uint8_t lev = (ql == TQL_stub) ? 1 : getMaxQLev(idx, ql);
    if (!lev)
      lev = 1;

    resQS.setQLev(0); // to remove influence on calcCurQL() call
    resQS.setMaxQL(RMGR.calcCurQL(idx, lev), RMGR.calcCurQL(idx, min(lev, resQS.getLdLev())));
    resQS.setQLev(lev);
    if (resQS.getLdLev() > lev)
      if (!downgrade_managed_tex_to_lev(tid, lev))
        logerr("failed to downgrade tex (%s) from lev=%d to lev=%d (maxQL=%d)", RMGR.getName(idx), resQS.getLdLev(), lev, ql);
    return true;
  }
  bool loadTex(TEXTUREID tid, unsigned nameId, TexQL ql)
  {
    unsigned idx = tid.index();
    if (ql == RMGR.resQS[idx].getCurQL() && RMGR.resQS[idx].getRdLev() <= RMGR.resQS[idx].getLdLev())
    {
      RMGR.cancelReading(idx);
      return true;
    }

    BaseTexture *bt = RMGR.getRefCount(idx) > 0 ? RMGR.baseTexture(idx) : nullptr;
    if (ql != TQL_stub)
    {
      auto onCompleted = [](unsigned idx) {
        {
          texmgr_internal::TexMgrAutoLock tlock;
          RMGR.markUpdatedAfterLoad(idx);
        }
        if (!is_managed_textures_streaming_load_on_demand())
          RMGR.scheduleReading(idx, RMGR.getFactory(idx));
      };

      ddsx::Header &rmgr_hdr = shared.ddsxHdr[idx];
      dag::Span<char> tex_buf = build_helper->getDDSxTexData(nameId);
      if (tex_buf.empty())
      {
        logerr("cannot convert <%s> tex asset", build_helper->getTexName(nameId));
        RMGR.cancelReading(idx);
        return false;
      }

      ddsx::Header *hdr = reinterpret_cast<ddsx::Header *>(tex_buf.data());
      if (!hdr->d3dFormat && tex_buf.size() == sizeof(ddsx::Header))
      {
        RMGR.setLevDesc(idx, ql, 0);
        auto &desc = RMGR.texDesc[idx];
        desc.packRecIdx[ql].pack = -1;
        desc.packRecIdx[ql].rec = -1;
        RMGR.cancelReading(idx);
        if (!texmgr_internal::texq_load_on_demand)
        {
          RMGR.resQS[idx].setMaxReqLev(15);
          RMGR.scheduleReading(idx, RMGR.getFactory(idx));
        }
        return true;
      }
      unsigned actual_ql_lev = get_log2i(max(max(hdr->w, hdr->h), hdr->depth));
      if (actual_ql_lev != RMGR.getLevDesc(idx, ql))
      {
        RMGR.setLevDesc(idx, ql, actual_ql_lev);
        RMGR.cancelReading(idx);
        if (!texmgr_internal::texq_load_on_demand)
        {
          RMGR.resQS[idx].setMaxReqLev(15);
          RMGR.scheduleReading(idx, RMGR.getFactory(idx));
        }
        return true;
      }

      if (bt && RMGR.resQS[idx].getLdLev() <= 1)
        G_VERIFY(bt->updateTexResFormat(rmgr_hdr.d3dFormat = hdr->d3dFormat));
      rmgr_hdr.dxtShift = hdr->dxtShift;
      rmgr_hdr.bitsPerPixel = hdr->bitsPerPixel;

      InPlaceMemLoadCB crd(tex_buf.data(), tex_buf.size());
      crd.seekrel(sizeof(ddsx::Header));

      TexLoadRes ldRet = RMGR.readDdsxTex(tid, *hdr, crd, ql, onCompleted, nullptr);

      hdr = nullptr;
      build_helper->releaseDDSxTexData(tex_buf);
      if (ldRet == TexLoadRes::ERR)
        if (!d3d::is_in_device_reset_now())
          logerr("failed loading tex %s", build_helper->getTexName(nameId));

      if (ldRet != TexLoadRes::OK)
        onCompleted(idx);

      interlocked_add(totalLoadedSizeKb, crd.getTargetDataSize() >> 10);
      interlocked_increment(totalLoadedCount);
    }
    else
    {
      RMGR.cancelReading(idx);
      if (bt)
        RMGR.downgradeTexQuality(idx, *bt, max<int>(RMGR.getLevDesc(idx, TQL_thumb), 1));
    }
    return true;
  }

  void setupPackRecIdx(unsigned idx, TexQL ql, unsigned ql_lev, const unsigned *name_ids)
  {
    RMGR.setLevDesc(idx, ql, ql_lev);
    if (!ql_lev)
      return;

    unsigned nameId = name_ids[ql];
    G_ASSERTF(nameId <= 0xFFFFFF, "name=%s ql=%d nameId=%d", build_helper->getTexName(nameId), ql, nameId);
    auto &desc = RMGR.texDesc[idx];
    desc.packRecIdx[ql].pack = 0x7E00 | ((nameId & 0xFF0000) >> 16);
    desc.packRecIdx[ql].rec = nameId & 0xFFFF;
  }
  static unsigned getAssetNameIdFromPackRecIdx(unsigned idx, TexQL ql)
  {
    if (!RMGR.getLevDesc(idx, ql))
      return ~0u;
    auto &desc = RMGR.texDesc[idx];
    if ((desc.packRecIdx[ql].pack & 0xFF00) != 0x7E00)
      return ~0u;
    return (unsigned(desc.packRecIdx[ql].pack & 0xFF) << 16) | desc.packRecIdx[ql].rec;
  }

  void getLoadingState(AsyncTextureLoadingState &out_state)
  {
    OSSpinlockScopedLock lock(toLoadPendSL);
    out_state.pendingLdTotal = interlocked_acquire_load(pendingTexCount[TQL__COUNT]);
    for (unsigned i = 0; i < TQL__COUNT; i++)
      out_state.pendingLdCount[i] = interlocked_acquire_load(pendingTexCount[i]);
    out_state.totalLoadedCount = interlocked_acquire_load(totalLoadedCount);
    out_state.totalLoadedSizeKb = interlocked_acquire_load(totalLoadedSizeKb);
    out_state.totalCacheUpdated = interlocked_acquire_load(totalCacheUpdateCount);
  }

  TextureFactory *defFactory = nullptr;
  ITexBuildOnDemandHelper *build_helper = nullptr;

  Tab<TexIdToLoad> toLoadPend[TQL__COUNT];
  OSSpinlock toLoadPendSL;
  unsigned pendingTexCount[TQL__COUNT + 1] = {0};
  unsigned totalLoadedCount = 0, totalLoadedSizeKb = 0, totalCacheUpdateCount = 0;
  TexQL maxQL = TQL_uhq;
};


//! special job for mass load of pending textures using TQL-ordered queues
struct build_on_demand_tex_factory::AsyncMassLoadJob final : public cpujobs::IJob
{
  static constexpr int TAG = _MAKE4C('AMTL');
  DDSxBuildOnDemandTexFactory &factory;
  AsyncMassLoadJob(DDSxBuildOnDemandTexFactory &f) : factory(f) {}

  const char *getJobName(bool &) const override { return "AsyncMassLoadJob"; }

  void doJob() override
  {
    Tab<TexIdToLoad> toLoad;
    Tab<uint8_t> toLoadState;
    bool need_cycle_again = false;
    interlocked_increment(shared.asyncMassLoadJobInProgress);
    for (TexQL ql = TQL_thumb; ql < TQL__COUNT; ql = need_cycle_again ? TQL_thumb : TexQL(ql + 1))
    {
      {
        OSSpinlockScopedLock lock(factory.toLoadPendSL);
        toLoad = eastl::move(factory.toLoadPend[ql]);
      }
      need_cycle_again = !toLoad.empty();
      toLoadState.resize(toLoad.size());
      mem_set_0(toLoadState);

      auto load_prebuilt_textures = [this, &toLoad, &toLoadState](TexQL ql, unsigned i_limit, unsigned &loadedCnt) {
        unsigned cnt = 0;
        for (unsigned i = 0; i < i_limit; i++)
          if (interlocked_acquire_load(toLoadState[i]) == 1)
          {
            interlocked_release_store(toLoadState[i], 2);
            if (interlocked_acquire_load(shared.texLoadingAllowed))
              factory.loadTex(toLoad[i].id, toLoad[i].nameId, ql);
            else
              RMGR.cancelReading(toLoad[i].id.index());
            interlocked_decrement(factory.pendingTexCount[ql]);
            interlocked_decrement(factory.pendingTexCount[TQL__COUNT]);
            cnt++;
            if (++loadedCnt >= toLoad.size())
              break;
          }
        return cnt > 0;
      };

      unsigned loadedCnt = 0;
      for (unsigned i = 0; i < toLoad.size();)
      {
        if (auto *j = shared.dCtx->allocJob())
        {
          j->initJob(factory.build_helper, toLoad[i], &toLoadState[i], &factory.totalCacheUpdateCount);
          shared.dCtx->submitJob(j, ql);
          i++;
        }

        if (!load_prebuilt_textures(ql, i, loadedCnt))
          sleep_msec(1);
      }
      while (loadedCnt < toLoad.size())
        if (!load_prebuilt_textures(ql, toLoad.size(), loadedCnt))
          sleep_msec(1);

      toLoad.clear();
      if (interlocked_acquire_load(factory.pendingTexCount[TQL__COUNT]) == 0 || !interlocked_acquire_load(shared.texLoadingAllowed))
        break;
    }
    if (!interlocked_acquire_load(shared.texLoadingAllowed))
      debug("%s: interrupted due to texLoadingAllowed=false", __FUNCTION__);
    interlocked_decrement(shared.asyncMassLoadJobInProgress);
  }
  void releaseJob() override { delete this; }
  unsigned getJobTag() override { return TAG; };
};

void build_on_demand_tex_factory::SharedData::addRef()
{
  interlocked_increment(usageCount);
  if (ddsxHdr.empty())
  {
    struct OpenD3dResManagerData : public D3dResManagerData
    {
      static unsigned getMaxTotalIndexCount() { return D3dResManagerData::maxTotalIndexCount; }
    };
    ddsxHdr.resize(OpenD3dResManagerData::getMaxTotalIndexCount());
    mem_set_0(ddsxHdr);
  }
  if (!dCtx)
    dCtx = new DDSxPrebuildCtx(cpujobs::get_logical_core_count());
}
void build_on_demand_tex_factory::SharedData::delRef()
{
  if (interlocked_decrement(usageCount) == 0)
  {
    clear_and_shrink(ddsxHdr);
    del_it(dCtx);
  }
}

//! single instance of DDSxBuildOnDemandTexFactory (but may be refactored to provide several instances)
static build_on_demand_tex_factory::DDSxBuildOnDemandTexFactory ddsx_bod_tf;

//! regular action to be called on each update (via idlecycle)
static void build_on_demand_tex_factory::update_on_each_frame(void *_f)
{
  auto &f = *reinterpret_cast<DDSxBuildOnDemandTexFactory *>(_f);
  if (interlocked_acquire_load(f.pendingTexCount[TQL__COUNT]) && interlocked_acquire_load(shared.texLoadingAllowed))
    cpujobs::add_job(texmgr_internal::reload_jobmgr_id, new AsyncMassLoadJob(f));
}
//! regular action to be called on end of frame (via tql::on_frame_finished)
static void build_on_demand_tex_factory::tql_on_frame_finished()
{
  if (base_engine_on_frame_finished)
    base_engine_on_frame_finished();
  update_on_each_frame(&ddsx_bod_tf);
}
//! helper to complete texture loading
static void perform_async_tex_repl()
{
  d3d::GpuAutoLock lock;
  RMGR.performAsyncTextureReplacementCompletions();
}

//! factory init
TextureFactory *build_on_demand_tex_factory::init(ITexBuildOnDemandHelper *h)
{
  if (::get_default_tex_factory() == &ddsx_bod_tf)
    return &ddsx_bod_tf;

  texmgr_internal::disable_add_warning = true;
  ddsx_bod_tf.init(h);

  base_engine_on_frame_finished = tql::on_frame_finished;
  tql::on_frame_finished = &tql_on_frame_finished;
  register_regular_action_to_idle_cycle(&update_on_each_frame, &ddsx_bod_tf);
  return &ddsx_bod_tf;
}
//! factory shutdown
void build_on_demand_tex_factory::term(TextureFactory *f)
{
  if (f != &ddsx_bod_tf)
    return;
  build_on_demand_tex_factory::cease_loading(f, false);
  unregister_regular_action_to_idle_cycle(&update_on_each_frame, &ddsx_bod_tf);
  tql::on_frame_finished = base_engine_on_frame_finished;
  ddsx_bod_tf.term();
}
bool build_on_demand_tex_factory::cease_loading(TextureFactory *f, bool allow_further_tex_loading)
{
  if (f != &ddsx_bod_tf)
    return false;
  unsigned prev_allow_loading = interlocked_exchange(shared.texLoadingAllowed, unsigned(false));
  cpujobs::remove_jobs_by_tag(texmgr_internal::reload_jobmgr_id, AsyncMassLoadJob::TAG);
  while (interlocked_acquire_load(shared.asyncMassLoadJobInProgress) > 0)
    sleep_msec(1);
  perform_async_tex_repl();
  interlocked_release_store(shared.texLoadingAllowed, unsigned(allow_further_tex_loading));
  return bool(prev_allow_loading);
}
//! schedule tex prebuilding job with specified quality
void build_on_demand_tex_factory::schedule_prebuild_tex(TextureFactory *f, unsigned tex_nameId, TexQL ql)
{
  auto &_f = *static_cast<DDSxBuildOnDemandTexFactory *>(f);
  if (auto *j = shared.dCtx->allocJob())
  {
    j->initJob(_f.build_helper, tex_nameId);
    shared.dCtx->submitJob(j, ql);
  }
}

//! limit texture quality to max specified level for textures in list
void build_on_demand_tex_factory::limit_tql(TextureFactory *f, TexQL max_tql, const TextureIdSet &tid)
{
  if (f != &ddsx_bod_tf)
    return;
  bool ld = cease_loading(f, false);
  for (TEXTUREID id : tid)
    if (RMGR.getFactory(id.index()) == f)
    {
      acquire_managed_tex(id);
      ddsx_bod_tf.forceMaxTexQL(id, max_tql);
      release_managed_tex(id);
    }
  if (ld)
    cease_loading(f, ld);
}
//! limit texture quality to max specified level for all registered textures)
void build_on_demand_tex_factory::limit_tql(TextureFactory *f, TexQL max_tql)
{
  if (f != &ddsx_bod_tf)
    return;
  bool ld = cease_loading(f, false);
  ddsx_bod_tf.maxQL = max_tql;
  for (TEXTUREID id = first_managed_texture(0); id != BAD_TEXTUREID; id = next_managed_texture(id, 0))
    if (RMGR.getFactory(id.index()) == f)
      ddsx_bod_tf.forceMaxTexQL(id, max_tql);
  if (ld)
    cease_loading(f, ld);
}

//! provides current loading state
bool build_on_demand_tex_factory::get_current_loading_state(const TextureFactory *f, AsyncTextureLoadingState &out_state)
{
  if (f != &ddsx_bod_tf)
    return false;
  ddsx_bod_tf.getLoadingState(out_state);
  return true;
}

//! provides description of managed texture entry (dimensions, QL, etc.)
bool get_managed_tex_entry_desc(TEXTUREID tid, ManagedTexEntryDesc &out_desc)
{
  if (!RMGR.isValidID(tid, nullptr))
    return false;

  const auto &ddsxHdr = build_on_demand_tex_factory::shared.ddsxHdr;
  unsigned idx = tid.index();
  auto &desc = RMGR.texDesc[idx];
  auto &resQS = RMGR.resQS[idx];
  out_desc.width = desc.dim.w;
  out_desc.height = desc.dim.h;
  out_desc.depth = desc.dim.d;
  out_desc.mipLevels = desc.dim.l;
  out_desc.maxLev = desc.dim.maxLev;
  out_desc.ldLev = resQS.getLdLev();
  out_desc.maxQL = resQS.getMaxQL();
  out_desc.curQL = resQS.getCurQL();
  for (TexQL ql = TQL_thumb; ql <= TQL_uhq; ql = TexQL(ql + 1))
  {
    out_desc.qLev[ql] = RMGR.getLevDesc(idx, ql);
    if (out_desc.qLev[ql] && idx < ddsxHdr.size())
      out_desc.qResSz[ql] = tql::sizeInKb(RMGR.calcTexMemSize(idx, out_desc.qLev[ql], ddsxHdr[idx])) << 10;
    else
      out_desc.qResSz[ql] = 0;
  }
  return true;
}

//! downgrades/discards texture to specified level (1 means stub)
bool downgrade_managed_tex_to_lev(TEXTUREID tid, int req_lev)
{
  if (tid)
    if (BaseTexture *bt = RMGR.getBaseTex(tid))
      return RMGR.downgradeTexQuality(tid.index(), *bt, req_lev);
  return false;
}

void dump_managed_tex_state(TEXTUREID tid)
{
  if (RMGR.isValidID(tid, nullptr))
    RMGR.dumpTexState(tid.index());
  else
    debug("invalid/stale tid=0x%x passed to %s", tid, __FUNCTION__);
}
