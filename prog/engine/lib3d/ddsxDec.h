// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_lock.h>
#include <ioSys/dag_fastSeqRead.h>
#include <ioSys/dag_memIo.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_cpuJobsQueue.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_atomic.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/3d/dag_info.h>
#include <math/dag_bits.h>
#include <stdio.h>
#include "texMgrData.h"

static constexpr int DXP_PRIO_COUNT = 3;

#ifndef RMGR_TRACE
#define RMGR_TRACE(...) ((void)0)
#endif

struct DDSxDecodeCtxBase //-V730
{
  cpujobs::JobQueue<cpujobs::IJob *, false> q;
  OSSpinlock queueSL;
  volatile int maybeIdle = 0;
};

struct DDSxDecodeCtx : DDSxDecodeCtxBase
{
  static constexpr int MAX_WORKERS = 8;

  class NamedInPlaceMemLoadCB : public InPlaceMemLoadCB
  {
  public:
    const char *name;
    NamedInPlaceMemLoadCB(const void *ptr, int sz, const char *name_) : InPlaceMemLoadCB(ptr, sz), name(name_) {}
    virtual const char *getTargetName() { return name; };
  };

  struct TexCreateJob : public cpujobs::IJob
  {
    const ddsx::Header *hdr;
    char *data;
    const char *texName, *packName;
    int dataSize;
    int texQ;
    TEXTUREID tid;
    int prio = -1;
    volatile int *stillLoading;

    TexCreateJob() : hdr(NULL), texQ(-1), texName(NULL), packName(NULL) { tid = BAD_TEXTUREID; }
    bool initJob(const ddsx::Header &_hdr, int tq, const char *tex_name, const char *pack_name, TEXTUREID _tid, FastSeqReader &crd,
      int data_sz, int _prio, volatile int &still_loading)
    {
      G_ASSERT(!interlocked_acquire_load(done));
      stillLoading = &still_loading;
      prio = _prio;
      data = (char *)tmpmem->tryAlloc(data_sz);
      if (data)
      {
        crd.read(data, data_sz);
        dataSize = data_sz;
      }
      else
      {
        interlocked_release_store(done, 1);
        return false;
      }
      hdr = &_hdr;
      texName = tex_name;
      packName = pack_name;
      texQ = tq;
      tid = _tid;
      return true;
    }

    virtual void doJob()
    {
      using texmgr_internal::RMGR;

      if (hdr->flags & hdr->FLG_NEED_PAIRED_BASETEX)
      {
        TEXTUREID bt_tid = RMGR.pairedBaseTexId[tid.index()];
        G_ASSERTF(bt_tid != BAD_TEXTUREID && RMGR.getBaseTexRc(bt_tid) > 0, "bt_tid=0x%x getBaseTexRc()=%d", bt_tid,
          RMGR.getBaseTexRc(bt_tid));

        uint64_t reft = profile_ref_ticks();
        while (!RMGR.hasTexBaseData(bt_tid))
          if (!isTexReading(bt_tid, prio) || !interlocked_acquire_load(*stillLoading))
          {
            if (RMGR.hasTexBaseData(bt_tid))
              break;
            tmpmem->free(data);
            data = NULL;
            logwarn("'%s' wait for texBaseData '%s' interrupted, spent %d usec", texName, RMGR.getName(bt_tid.index()),
              profile_time_usec(reft));
            RMGR.cancelReading(RMGR.toIndex(tid));
            return;
          }
          else
            sleep_msec(1);
        if (profile_usec_passed(reft, 1000))
          logwarn("'%s' decoding stalled for %d usec to wait for basetex", texName, profile_time_usec(reft));
      }

      char metaName[DAGOR_MAX_PATH];
      SNPRINTF(metaName, sizeof(metaName), "%s/%s", packName, texName);
      NamedInPlaceMemLoadCB crd(data, dataSize, metaName);

      const auto completeReading = [](int idx) {
        {
          texmgr_internal::TexMgrAutoLock lock;
          RMGR.markUpdatedAfterLoad(idx);
        }
        if (!is_managed_textures_streaming_load_on_demand())
          RMGR.scheduleReading(idx, RMGR.getFactory(idx));
      };

      RMGR_TRACE("loading tex %s (req=%d rd=%d) %dx%d baseTid=0x%x(has_bd=%d)", RMGR.getName(tid.index()),
        RMGR.resQS[tid.index()].getMaxReqLev(), RMGR.resQS[tid.index()].getRdLev(), hdr->w, hdr->h, RMGR.pairedBaseTexId[tid.index()],
        RMGR.hasTexBaseData(RMGR.pairedBaseTexId[tid.index()]));
      {
        d3d::LoadingAutoLock loadingLock;

        TexLoadRes ldRet = RMGR.readDdsxTex(tid, *hdr, crd, texQ, completeReading);
        if (ldRet == TexLoadRes::ERR)
          if (!d3d::is_in_device_reset_now())
            logerr("failed loading tex '%s' from pack '%s'", texName, packName);

        if (ldRet != TexLoadRes::OK)
          completeReading(RMGR.toIndex(tid));
      }

      tmpmem->free(data);
      data = NULL;
      texName = packName = NULL;
    }
  };

  struct DecThread final : public DaThread
  {
    os_event_t event;
    DDSxDecodeCtxBase *ctx;
    unsigned wIdx = 0;

    DecThread() : DaThread("DDSX decoder", 128 << 10, 0, WORKER_THREADS_AFFINITY_MASK), ctx(NULL)
    {
      stripStackInMinidump();
      os_event_create(&event, NULL);
    }
    ~DecThread() { os_event_destroy(&event); }

    void execute() override
    {
      while (!isThreadTerminating())
      {
        interlocked_or(ctx->maybeIdle, 1 << wIdx);
        int wres = os_event_wait(&event, cpujobs::DEFAULT_IDLE_TIME_SEC / 2 * 1000);
        interlocked_and(ctx->maybeIdle, ~(1 << wIdx));
        if (isThreadTerminating())
          break;

        cpujobs::IJob *job = ctx->q.pop();
        if (!job)
        {
          if (DAGOR_UNLIKELY(wres == OS_WAIT_TIMEOUTED || ddsx::get_streaming_mode() == ddsx::BackgroundSerial))
          {
            OSSpinlockScopedLock lock(ctx->queueSL);
            if (os_event_wait(&event, OS_WAIT_IGNORE) != OS_WAIT_OK)
              break;
          }
          continue;
        }

        while (job)
        {
          G_ASSERT(interlocked_acquire_load(job->done) == 0);
          job->doJob();
          DDSxDecodeCtx::updateWorkerUsedMask(static_cast<TexCreateJob *>(job)->prio, wIdx);
          G_ASSERT(interlocked_acquire_load(job->done) == 0);
          interlocked_release_store(job->done, 1);
          if (isThreadTerminating()) // check quit request
            return;
          job = ctx->q.pop();
        }
      }
    }
  };

  DecThread workers[MAX_WORKERS];
  TexCreateJob *jobs;
  int numWorkers;

  DDSxDecodeCtx(int nworkers)
  {
    numWorkers = min(nworkers, MAX_WORKERS);
    q.allocQueue(get_bigger_pow2(numWorkers * 2));
    jobs = new TexCreateJob[numWorkers * 2];
    for (int i = 0; i < numWorkers; i++)
    {
      workers[i].ctx = this;
      workers[i].wIdx = i;
    }
  }
  ~DDSxDecodeCtx()
  {
    for (int i = 0; i < numWorkers; i++)
      workers[i].terminate(true, -1, &workers[i].event);
    q.freeQueue();
    delete[] jobs;
  }

  void wakeUpAll()
  {
    for (int i = 0; i < numWorkers; ++i)
      if (workers[i].isThreadRunnning())
        G_VERIFY(os_event_set(&workers[i].event) == 0);
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

  TexCreateJob *allocJob()
  {
    OSSpinlockScopedLock lock(queueSL);
    for (int i = 0; i < numWorkers * 2; i++)
      if (interlocked_acquire_load(jobs[i].done))
      {
        interlocked_release_store(jobs[i].done, 0);
        return &jobs[i];
      }
    return NULL;
  }

  void submitJob(TexCreateJob *j)
  {
    q.push(j, NULL, NULL);
    OSSpinlockScopedLock lock(queueSL);
    if (!wakeUpIdle())
      if (!startOne())
        // All workers are started and busy - wake up first one just in case
        G_VERIFY(os_event_set(&workers[0].event) == 0);
  }

  void waitAllDone(int prio)
  {
    if (prio < 0)
      while (q.size() > 0)
        sleep_msec(1);

  check_done:
    for (int i = 0; i < numWorkers * 2; i++)
      if (!interlocked_acquire_load(jobs[i].done))
      {
        if (jobs[i].prio != prio)
          continue;
        sleep_msec(1);
        goto check_done;
      }
  }
  static bool isTexReading(TEXTUREID tid, int prio)
  {
    if (!texmgr_internal::RMGR.resQS[tid.index()].isReading() || !dCtx)
      return false;
    TexCreateJob *jobs = dCtx->jobs;
    for (int i = 0, ie = dCtx->numWorkers * 2; i < ie; i++)
      if (!interlocked_acquire_load(jobs[i].done) && jobs[i].prio == prio)
        if (jobs[i].tid == tid)
          return true;
    return false;
  }

  // single decding context
  static DDSxDecodeCtx *dCtx;

  // helpers to gather statistics of workers usage
  static uint32_t prioWorkerUsedMask[DXP_PRIO_COUNT];

  static void clearWorkerUsedMask(int prio) { interlocked_release_store(prioWorkerUsedMask[prio], 0); }
  static void updateWorkerUsedMask(int prio, unsigned t) { interlocked_or(prioWorkerUsedMask[prio], 1 << t); }
  static uint32_t getWorkerUsedMask(int prio) { return interlocked_acquire_load(prioWorkerUsedMask[prio]); }
  static uint32_t getWorkerUsedCount(int prio) { return __popcount(interlocked_acquire_load(prioWorkerUsedMask[prio])); }
};
