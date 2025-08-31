// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <streaming/dag_streamingMgr.h>
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_tab.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>

class BasicStreamingSceneManager;

#define WARNING_MULTIPLIER 1.5 // Notify on delays longer than WARNING_MULTIPLIER * usecAllowedPerFrame * usecAllowedPerFrameFactor.

//
// streaming render scene manager implemantation
//
class BasicStreamingSceneManager : public IStreamingSceneManager
{
  struct BindumpRec
  {
    SimpleString name;
    bool active; // may be active but not loaded yet (!bindump)
    BinaryDump *bindump;
  };

  Tab<BindumpRec> bdRec;
  Tab<int> toUnload;
  Tab<int> toLoad;

  IStreamingSceneStorage *client;
  struct LevelStreamJob : public cpujobs::IJob, public IBinaryDumpLoaderClient
  {
    LevelStreamJob(BasicStreamingSceneManager &_ssm) : ssm(_ssm), sceneId(-1), unloadRequested(false), done(false), loadJob(false) {}

    void startJob(int id, bool load)
    {
      startReft = ref_time_ticks_qpc();

      done = false;
      sceneId = id;
      loadJob = load;
      unloadRequested = false;
      name = ssm.bdRec[id].name;

      ldEnvi = ldScene = NULL;
      waitingPhase = false;

      if (loadJob)
        bd = NULL;
      else
      {
        debug("[STRM] unload: id=%i, name=\"%s\", sync part", sceneId, name.str());
        if (ssm.client)
          ssm.client->delBinDump(id);
        bd = ssm.bdRec[id].bindump;
        ssm.bdRec[id].active = false;
        ssm.bdRec[id].bindump = NULL;
      }

      ::enable_tex_mgr_mt(true, 0);
    }

    const char *getJobName(bool &) const override { return "LevelStreamJob"; }

    virtual void doJob()
    {
      struct FinishSyncAction : public DelayedAction
      {
        LevelStreamJob &job;
        int ti;

        FinishSyncAction(LevelStreamJob *j, int _ti) : job(*j), ti(_ti) {}
        virtual void performAction()
        {
          switch (ti)
          {
            case 0:
              if (job.ldEnvi)
                job.ssm.client->bdlEnviLoaded(job.sceneId, job.ldEnvi);
              add_delayed_action(new FinishSyncAction(&job, 1));
              break;
            case 1:
              if (job.ldScene)
                job.ssm.client->bdlSceneLoaded(job.sceneId, job.ldScene);
              add_delayed_action(new FinishSyncAction(&job, 10));
              break;

            case 10:
              job.ldEnvi = job.ldScene = NULL;
              job.ssm.client->bdlBinDumpLoaded(job.sceneId);
              add_delayed_action(new FinishSyncAction(&job, 11));
              break;

            case 11: job.waitingPhase = false; break;
          }
        }

      private:
        FinishSyncAction &operator=(const FinishSyncAction &) { return *this; }
      };

      if (loadJob)
      {
        debug("[STRM] load: id=%i, name=\"%s\"", sceneId, name.str());
        bd = load_binary_dump_async(name, *this, sceneId);

        add_delayed_action(new FinishSyncAction(this, 0));

        waitingPhase = true;
        while (waitingPhase)
          sleep_msec(30);
      }
      else
      {
        add_delayed_action(new FinishSyncAction(this, 11));
        waitingPhase = true;
        while (waitingPhase)
          sleep_msec(30);
        debug("[STRM] unload: id=%i, name=\"%s\"", sceneId, name.str());
        unload_binary_dump(bd, false);
      }
    }

    virtual void releaseJob()
    {
      ::enable_tex_mgr_mt(false, 0);

      if (loadJob)
      {
        ssm.bdRec[sceneId].bindump = bd;
        debug("[STRM] load time: %d ms", get_time_usec_qpc(startReft) / 1000);
      }
      else
      {
        debug("[STRM] unload time: %d ms", get_time_usec_qpc(startReft) / 1000);
      }

      sceneId = -1;
      done = true;
    }

    virtual bool bdlGetPhysicalWorldBbox(BBox3 &bbox) { return ssm.client->bdlGetPhysicalWorldBbox(bbox); }

    virtual bool bdlConfirmLoading(unsigned, int tag) { return ssm.client->bdlConfirmLoading(sceneId, tag); }
    virtual void bdlTextureMap(unsigned, dag::ConstSpan<TEXTUREID> texId) { ssm.client->bdlTextureMap(sceneId, texId); }

    virtual void bdlEnviLoaded(unsigned, RenderScene *envi) { ldEnvi = envi; }
    virtual void bdlSceneLoaded(unsigned, RenderScene *scene) { ldScene = scene; }
    virtual void bdlBinDumpLoaded(unsigned) {}
    virtual bool bdlCustomLoad(unsigned, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texId)
    {
      return ssm.client->bdlCustomLoad(sceneId, tag, crd, texId);
    }
    virtual const char *bdlGetBasePath() { return ssm.client->bdlGetBasePath(); }

    BasicStreamingSceneManager &ssm;
    SimpleString name;
    BinaryDump *bd;
    int sceneId;
    bool unloadRequested;
    bool done, loadJob;
    int64_t startReft;

    RenderScene *ldEnvi, *ldScene;
    volatile bool waitingPhase;

  private:
    LevelStreamJob &operator=(const LevelStreamJob &);
  };

  LevelStreamJob *curLoading;
  mutable int jobCoreId = -1;
  static const int usecAllowedPerFrame = 5000;
  float usecAllowedPerFrameFactor;

  bool curLoadingIsUnloading;

public:
  int getJobCoreId() const
  {
    if (jobCoreId < 0)
    {
      if (!cpujobs::is_inited())
        cpujobs::init();
      const int priority = cpujobs::DEFAULT_THREAD_PRIORITY + cpujobs::THREAD_PRIORITY_LOWER_STEP * 2;
      jobCoreId = cpujobs::create_virtual_job_manager(128 << 10, WORKER_THREADS_AFFINITY_MASK, "streaming", priority);
      G_ASSERT(jobCoreId >= 0);
    }
    return jobCoreId;
  }


  BasicStreamingSceneManager() : client(NULL), bdRec(midmem_ptr()), toUnload(tmpmem_ptr()), toLoad(tmpmem_ptr())
  {
    curLoading = NULL;
    curLoadingIsUnloading = false;
    usecAllowedPerFrameFactor = 1;
  }
  ~BasicStreamingSceneManager()
  {
    unloadAllBinDumps();
    if (jobCoreId >= 0)
      cpujobs::destroy_virtual_job_manager(jobCoreId, false);
  }

  virtual void destroy() { delete this; }

  virtual void setClient(IStreamingSceneStorage *cli) { client = cli; }

  virtual void setAllowedTimeFactor(float factor) { usecAllowedPerFrameFactor = factor; }

  virtual void act()
  {
    if (!toUnload.size() && !toLoad.size())
      return;

    if (!curLoading)
    {
      if (toUnload.size())
      {
        int id = toUnload[0];

        curLoading = new LevelStreamJob(*this);
        curLoading->startJob(id, false);
        curLoadingIsUnloading = true;
        DEBUG_CTX("started scene unloading: %s, id=#%d, queue.depth=%d", (char *)bdRec[id].name, id, toUnload.size());
        cpujobs::add_job(getJobCoreId(), curLoading);
      }
      else
      {
        int id = 0;

        if (toLoad.size() > 1 && client)
        {
          // select most wanted binary
          real optima = client->getBinDumpOptima(toLoad[id]);
          for (int i = 1; i < toLoad.size(); i++)
          {
            real opt2 = client->getBinDumpOptima(toLoad[i]);
            if (opt2 < optima)
            {
              optima = opt2;
              id = i;
            }
          }
          if (id != 0)
          {
            // swap to make first
            DEBUG_CTX("chosen item #%d in queue", id);
            int tmp = toLoad[id];
            toLoad[id] = toLoad[0];
            toLoad[0] = tmp;
          }
        }

        G_ASSERTF(toLoad.size() > 0, "BasicStreamingSceneManager::act() : id = toLoad[0]");
        id = toLoad[0];

        curLoading = new LevelStreamJob(*this);
        curLoading->startJob(id, true);
        curLoadingIsUnloading = false;
        DEBUG_CTX("started scene loading: %s, id=#%d, queue.depth=%d", (char *)bdRec[id].name, id, toLoad.size());
        cpujobs::add_job(getJobCoreId(), curLoading);
      }
    }

    int64_t reft = ref_time_ticks_qpc();
    int max_allowed_usec = int(usecAllowedPerFrame * usecAllowedPerFrameFactor);
    while (!curLoading->done)
    {
      sleep_msec(1);
      cpujobs::release_done_jobs();
      if (get_time_usec_qpc(reft) > max_allowed_usec || curLoading->waitingPhase)
        break;
    }

    if (curLoading->done)
    {
      int idToUnload = curLoading->unloadRequested ? toLoad[0] : -1000;
      if (curLoadingIsUnloading)
      {
        G_ASSERTF(toUnload.size() > 0, "BasicStreamingSceneManager::act() : toUnload.erase(0,1) [done]");

        int id = toUnload[0];
        DEBUG_CTX("scene unloading done: %s, id=#%d", (char *)bdRec[id].name, id);
        (void)id;
        erase_items(toUnload, 0, 1);
      }
      else
      {
        G_ASSERTF(toLoad.size() > 0, "BasicStreamingSceneManager::act() : toLoad.erase(0,1) [done]");

        int id = toLoad[0];
        DEBUG_CTX("scene loading done: %s, id=#%d", (char *)bdRec[id].name, id);
        (void)id;
        erase_items(toLoad, 0, 1);
      }

      delete curLoading;
      curLoading = NULL;

      if (idToUnload != -1000)
        unloadBinDumpScheduled(idToUnload);
    }

    if (get_time_usec_qpc(reft) >= WARNING_MULTIPLIER * max_allowed_usec)
      debug("streaming job - elapsed %.3f ms (frame #%u)", get_time_usec_qpc(reft) * 0.001f, ::dagor_frame_no());
  }

  void removeFromQueueArray(Tab<int> &to_load_or_unload, int sceneId)
  {
    // remove scene from queue (except of elem 0 - it may be loading now)
    for (int i = to_load_or_unload.size() - 1; i > 0; i--)
      if (to_load_or_unload[i] == sceneId)
        erase_items(to_load_or_unload, i, 1);

    // if elem 0 in the queue is not loading now and is the very scene,
    // remove it from queue
    if (!curLoading && to_load_or_unload.size() > 0 && to_load_or_unload[0] == sceneId)
      erase_items(to_load_or_unload, 0, 1);
  }

  virtual int loadBinDump(const char *bindump)
  {
    int id = findSceneRec(bindump);
    if (id != -1 && bdRec[id].active && bdRec[id].bindump)
    {
      debug("[STRM:ERR] loadBinDump: rec for <%s> already exists (%d)!", bindump, id);
      return id;
    }

    id = addSceneRec(bindump);
    if (curLoading && curLoading->sceneId == id)
    {
      G_ASSERT(curLoading->unloadRequested);
      curLoading->unloadRequested = false;
    }
    else
      readScene(id);
    return id;
  }
  virtual int loadBinDumpAsync(const char *bindump)
  {
    int id = findSceneRec(bindump);

    removeFromQueueArray(toUnload, id);
    if (curLoading && curLoading->sceneId == id && curLoadingIsUnloading)
    {
      tabutils::addUnique(toLoad, id);
      return id;
    }

    if (id != -1 && bdRec[id].active && bdRec[id].bindump)
    {
      debug("[STRM:ERR] loadBinDumpAsync: rec for <%s> already exists (%d)!", bindump, id);
      return id;
    }

    id = addSceneRec(bindump);
    if (curLoading && curLoading->sceneId == id && !curLoadingIsUnloading)
    {
      if (!curLoading->unloadRequested)
        debug("[STRM:ERR] loadBinDumpAsync: curLoading for <%s> already exists (%d)!", bindump, id);
      curLoading->unloadRequested = false;
    }
    else
    {
      if (tabutils::find(toLoad, id))
      {
        debug("[STRM:ERR] loadBinDumpAsync: toLoad rec for <%s> already exists (%d)!", bindump, id);
        return id;
      }
      toLoad.push_back(id);
    }

    debug("[STRM] scheduled backgnd scene loading: %s, id=#%d, queue.depth=%d", bindump, id, toLoad.size());
    return id;
  }

  void unloadBinDumpScheduled(int id) { tabutils::addUnique(toUnload, id); }

  void unloadBinDumpInternal(int id)
  {
    int64_t unloadStart = ref_time_ticks();
    if (client)
      client->delBinDump(id);

    execute_delayed_action_on_main_thread(NULL, true, 10);
    unload_binary_dump(bdRec[id].bindump, false);
    debug("unloadtime = %d ms", get_time_usec(unloadStart) / 1000);
    (void)unloadStart;

    // bdRec[id].name = "";
    bdRec[id].active = false;
    bdRec[id].bindump = NULL;
  }

  virtual void unloadBinDump(const char *bindump, bool sync)
  {
    int id = findSceneRec(bindump);
    if (id != -1)
    {
      debug("[STRM] unloading scene : %s, id=#%d, queue.depth=%d", bindump, id, toLoad.size());

      removeFromQueueArray(toLoad, id);

      if (curLoading && curLoading->sceneId == id && !curLoadingIsUnloading)
      {
        // G_ASSERT(!curLoading->unloadRequested);
        curLoading->unloadRequested = true;
      }
      else if (bdRec[id].active)
      {
        if (sync)
          unloadBinDumpInternal(id);
        else
          unloadBinDumpScheduled(id);
      }

      return;
    }
    // DEBUG_CTX("not found bindump: %s", bindump);
  }

  virtual void unloadAllBinDumps()
  {
    debug("[STRM] unloadAllBinDumps");
    toLoad.clear();
    int bd_to_unload = 0;
    for (int i = bdRec.size() - 1; i >= 0; i--)
      if (bdRec[i].bindump)
      {
        bd_to_unload++;
        if (client)
          client->delBinDump(i);
      }

    if (curLoading || bd_to_unload)
    {
      if (curLoading)
        curLoading->unloadRequested = true;

      execute_delayed_action_on_main_thread(NULL, true, 10);

      for (int i = bdRec.size() - 1; i >= 0; i--)
        if (bdRec[i].bindump)
          unload_binary_dump(bdRec[i].bindump, false);
    }
    bdRec.clear();
  }

  virtual bool isBinDumpValid(int id)
  {
    if (id < 0 || id >= bdRec.size())
      return false;
    if (!bdRec[id].bindump && bdRec[id].name.length() == 0)
      return false;
    return true;
  }
  virtual bool isBinDumpLoaded(int id)
  {
    if (id < 0 || id >= bdRec.size())
      return false;
    return bdRec[id].bindump;
  }

  virtual bool isLoading() { return isLoadingNow() || isLoadingScheduled(); }
  virtual bool isLoadingNow() { return curLoading != NULL; }
  virtual bool isLoadingScheduled() { return toLoad.size() > 0 || toUnload.size() > 0; }
  virtual void clearLoadingSchedule()
  {
    debug("[STRM] clearLoadingSchedule");
    if (!curLoading)
    {
      clear_and_shrink(toLoad);
      clear_and_shrink(toUnload);
    }
    else
    {
      if (curLoadingIsUnloading)
      {
        if (toUnload.size() > 1)
          erase_items(toUnload, 1, toUnload.size() - 1);
        clear_and_shrink(toLoad);
      }
      else
      {
        if (toLoad.size() > 1)
          erase_items(toLoad, 1, toLoad.size() - 1);
        clear_and_shrink(toUnload);
      }
    }
  }

  virtual int findSceneRec(const char *name)
  {
    for (int i = 0; i < bdRec.size(); i++)
      if (dd_stricmp(bdRec[i].name, name) == 0)
        return i;
    return -1;
  }

protected:
  int addSceneRec(const char *lmdir)
  {
    int id = -1;
    for (int i = 0; i < bdRec.size(); i++)
      if (!bdRec[i].bindump && (bdRec[i].name.length() == 0 || dd_stricmp(bdRec[i].name, lmdir) == 0))
      {
        id = i;
        break;
      }

    if (id == -1)
      id = append_items(bdRec, 1);

    bdRec[id].name = lmdir;
    bdRec[id].active = true;
    bdRec[id].bindump = NULL;
    return id;
  }

  void readScene(int id)
  {
    if (id < 0 || id >= bdRec.size())
    {
      DEBUG_CTX("invalid id=%d, bdRec.size()=%d", id, bdRec.size());
      return;
    }
    if (bdRec[id].bindump)
    {
      DEBUG_CTX("scene id=%d already loaded", id);
      return;
    }
    debug("[STRM] started foregnd scene loading: %s, id=#%d", (char *)bdRec[id].name, id);
    BinaryDump *bd = load_binary_dump_async(bdRec[id].name, *client, id);

    execute_delayed_action_on_main_thread(NULL, true, 10);

    bdRec[id].bindump = bd;
    debug("[STRM] foregnd scene loaded");
  }
};

IStreamingSceneManager *IStreamingSceneManager::create()
{
  measure_cpu_freq();
  return new (inimem) BasicStreamingSceneManager;
}
