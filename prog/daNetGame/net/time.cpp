// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "time.h"
#include "netPrivate.h"

#include <daECS/net/time.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_log.h>

#if THREAD_SAFE_NET_TIME

static dag::AtomicFloat<double> g_net_sync_time = 0.0;

static OSSpinlock g_time_mgr_mutex;
struct GlobalTimeManagerLock : OSSpinlockScopedLock
{
  GlobalTimeManagerLock() : OSSpinlockScopedLock(g_time_mgr_mutex) {}
};

static void publish_sync_time() { g_net_sync_time.store(g_net_globals.timeMgr->getSeconds(), dag::memory_order_relaxed); }

double get_sync_time_d() { return g_net_sync_time.load(dag::memory_order_relaxed); }

#else

static double g_net_sync_time = 0.0;

struct GlobalTimeManagerLock
{
  GlobalTimeManagerLock() {} // Define a default constructor to avoid 'unused variable' warnings.
};

static void publish_sync_time() { g_net_sync_time = g_net_globals.timeMgr->getSeconds(); }

double get_sync_time_d() { return g_net_sync_time; }

#endif

float get_sync_time() { return float(get_sync_time_d()); }
int get_sync_millis() { return int(get_sync_time_d() * 1000.0 + 0.5); }

bool is_default_time()
{
  GlobalTimeManagerLock lock;
  return g_net_globals.isDefaultTimeMgr;
}

int get_async_millis()
{
  GlobalTimeManagerLock lock;
  return g_net_globals.timeMgr->getAsyncMillis();
}

void on_time_sync_response_received(const net::IMessage *msg)
{
  auto respMsg = msg->cast<TimeSyncResponse>();
  G_ASSERT(respMsg);
  GlobalTimeManagerLock lock;
  g_net_globals.timeMgr->onReceiveTimeSyncResponse(respMsg);
  publish_sync_time();
}

double advance_time(float dt, float &out_rt_dt)
{
  if (!net::is_net_lifecycle_thread())
    return get_sync_time_d();

  GlobalTimeManagerLock lock;
  const double advancedTime = g_net_globals.timeMgr->advance(dt, out_rt_dt);
  publish_sync_time();
  return advancedTime;
}

void reset_time_mgr(ITimeManager *new_mgr)
{
  if (!net::is_net_lifecycle_thread())
  {
    if (new_mgr)
    {
      logerr("reset_time_mgr refused off the net lifecycle thread (current=%lld); dropping new mgr",
        (long long)get_current_thread_id());
      delete new_mgr;
    }
    return;
  }
  ITimeManager *otherMgr = new_mgr ? new_mgr : create_accum_time();
  {
    GlobalTimeManagerLock lock;
    g_net_globals.isDefaultTimeMgr = new_mgr == nullptr;
    eastl::swap(g_net_globals.timeMgr, otherMgr);
    publish_sync_time();
  }
  delete otherMgr;
}

NetGlobals::~NetGlobals()
{
  GlobalTimeManagerLock lock;
  delete g_net_globals.timeMgr;
}
