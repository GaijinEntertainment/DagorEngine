// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <publicConfigs/publicConfigs.h>
#include <datacache/datacache.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <workCycle/dag_delayedAction.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_watchdog.h>
#include <util/dag_baseDef.h>
#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>

static datacache::Backend *public_configs_webcache = NULL;

namespace pubcfg
{

void init(const char *cache_dir, const char *default_public_configs_url)
{
  G_ASSERT(!public_configs_webcache);

  const char *url = dgs_get_settings()->getBlockByNameEx("network")->getStr("publicConfigsUrl", default_public_configs_url);

  DataBlock settings(framemem_ptr());
  DataBlock *urlsBlk = settings.addBlock("baseUrls");
  *urlsBlk = *dgs_get_settings()->getBlockByNameEx("network")->getBlockByNameEx("publicConfigsUrls");

  if (url && *url)
  {
    bool duplicate_url = false;
    dblk::iterate_params_by_name_and_type(*urlsBlk, "url", DataBlock::TYPE_STRING, [urlsBlk, &duplicate_url, url](int param_idx) {
      if (!duplicate_url && strcmp(urlsBlk->getStr(param_idx), url) == 0)
        duplicate_url = true;
    });
    if (!duplicate_url)
      urlsBlk->addStr("url", url);
  }
  if (urlsBlk->isEmpty()) // if empty urls list - no init
    return;

  settings.setBool("noIndex", true);
  settings.setBool("allowReturnStaleData", false);
  settings.setInt("traceLevel", dgs_get_settings()->getBlockByNameEx("debug")->getInt("webcacheTraceLevel", 0));
  settings.setStr("mountPath", cache_dir);
  settings.setInt("timeoutSec", dgs_get_settings()->getBlockByNameEx("network")->getInt("publicConfigsTimeoutSec", 5));
  settings.setInt("connectTimeoutSec", dgs_get_settings()->getBlockByNameEx("network")->getInt("publicConfigsConnTimeoutSec", 5));
  settings.setStr("jobMgrName", "pubcfgWebCache");

  public_configs_webcache = datacache::create(datacache::BT_WEB, settings);
}


void shutdown() { del_it(public_configs_webcache); }


struct GetFileFromWebcacheResult
{
  GetFileFromWebcacheResult() : err(datacache::ERR_OK), done(0) {}

  datacache::EntryHolder entryHolder;
  datacache::ErrorCode err;
  volatile int done;
};


class GetFileFromWebcacheAction final : public DelayedAction
{
public:
  GetFileFromWebcacheAction(const char *key_, datacache::Backend &cache_, GetFileFromWebcacheResult &result_) :
    key(key_), cache(cache_), result(result_)
  {
    G_ASSERT(key && *key);
  }

  static void completionCb(const char * /*key*/, datacache::ErrorCode error, datacache::Entry *entry, void *arg)
  {
    GetFileFromWebcacheResult *result = (GetFileFromWebcacheResult *)(arg);
    G_ASSERT(result);
    result->entryHolder.reset(entry);
    result->err = error;
    interlocked_release_store(result->done, 1);
  }

  virtual void performAction() override
  {
    datacache::ErrorCode err = datacache::ERR_UNKNOWN;
    datacache::Entry *entry = public_configs_webcache ? public_configs_webcache->get(key, &err, completionCb, &result) : NULL;
    if (datacache::ERR_PENDING != err)
    {
      completionCb(key, err, entry, &result);
      return;
    }
    G_ASSERT(!entry);
  }

  const char *key;
  datacache::Backend &cache;
  GetFileFromWebcacheResult &result;
};


struct WebcachePollingThread final : public DaThread
{
  WebcachePollingThread() : DaThread("WebcachePollingThread", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK) {}

  void execute() override
  {
    while (!isThreadTerminating())
    {
      public_configs_webcache->poll();
      sleep_msec(10);
    }
  }
};


static eastl::unique_ptr<WebcachePollingThread> g_webcache_poll_thread;


static void public_configs_webcache_polling()
{
#if _TARGET_ANDROID
  static const bool ALLOW_POLLING = !is_main_thread();
#else
  static const bool ALLOW_POLLING = true;
#endif

  if (ALLOW_POLLING)
    public_configs_webcache->poll();
  else if (!g_webcache_poll_thread)
  {
    g_webcache_poll_thread.reset(new WebcachePollingThread());
    g_webcache_poll_thread->start();
  }
}


static void stop_public_configs_webcache_polling()
{
  if (g_webcache_poll_thread)
  {
    g_webcache_poll_thread->terminate(true /*wait*/);
    g_webcache_poll_thread.reset();
  }
}


bool get(const char *filename, pubcfg_cb_t cb, void *arg)
{
  if (!public_configs_webcache)
    return false;
  G_ASSERT(filename && *filename);
  G_ASSERT(cb);

  GetFileFromWebcacheResult result;
  GetFileFromWebcacheAction act(filename, *public_configs_webcache, result);
  add_delayed_callback([](void *a) { ((GetFileFromWebcacheAction *)a)->performAction(); }, &act);

  while (0 == interlocked_acquire_load(result.done))
  {
    if (is_main_thread())
    {
#if _TARGET_ANDROID
      // Sync network request might be long enough to cause ANR on Android
      // Poll events loop while waiting for network request to complete
      dagor_idle_cycle();
#endif
      perform_delayed_actions();
      cpujobs::release_done_jobs();
      watchdog_kick();
    }
    sleep_msec(1);
    public_configs_webcache_polling();
  }

  stop_public_configs_webcache_polling();

  if (!result.err && result.entryHolder.get())
  {
    dag::ConstSpan<uint8_t> data(result.entryHolder->getData());
    debug("%s - got %s, %d bytes", __FUNCTION__, filename, data_size(data));
    cb(dag::ConstSpan<char>((const char *)data.data(), data_size(data)), arg);
    return true;
  }

  logerr("%s - failed to receive %s (error %d)", __FUNCTION__, filename, result.err);
  cb({}, arg);
  return false;
}


} // namespace pubcfg
