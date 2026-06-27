// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "time.h"
#include "netPrivate.h"

#include <daECS/net/time.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_assert.h>


float get_sync_time()
{
  if (net::is_this_thread_net_em_owner())
    return (float)g_net_globals.timeMgr->getSeconds();
  return READ_NET_SNAPSHOT_FIELD(syncTimeSec);
}
double get_sync_time_d()
{
  if (net::is_this_thread_net_em_owner())
    return g_net_globals.timeMgr->getSeconds();
  return READ_NET_SNAPSHOT_FIELD(syncTimeSecD);
}
int get_sync_millis()
{
  if (net::is_this_thread_net_em_owner())
    return g_net_globals.timeMgr->getMillis();
  return READ_NET_SNAPSHOT_FIELD(syncMillis);
}
int get_async_millis()
{
  if (net::is_this_thread_net_em_owner())
    return g_net_globals.timeMgr->getAsyncMillis();
  // Off-owner readers can't use the snapshot here: get_async_millis() advances continuously
  // within a frame, snapshot is only refreshed at publish cadence. snapshotLock serialises both
  // the timeMgr pointer swap in reset_time_mgr AND the owner-side advance() in advance_time,
  // so accum-driven managers (AccumTime, ReplayTime) are race-free across threads too.
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  return g_net_globals.timeMgr->getAsyncMillis();
}

bool is_dummy_time()
{
  if (net::is_this_thread_net_em_owner())
    return g_net_globals.timeMgr == &g_net_globals.dummyTime;
  return READ_NET_SNAPSHOT_FIELD(isDummyTime);
}


double advance_time(float dt, float &out_rt_dt)
{
  if (!net::is_this_thread_net_em_owner())
  {
    return get_sync_time_d();
  }
  // Hold snapshotLock around advance() so off-owner get_async_millis() (which also takes the
  // lock) can't catch accum-driven managers (AccumTime, ReplayTime) mid-mutation. Cheap: lock
  // is uncontended in the prod single-owner case.
  double t;
  {
    OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
    t = g_net_globals.timeMgr->advance(dt, out_rt_dt);
  }
  publish_net_state_now();
  return t;
}

void reset_time_mgr(ITimeManager *new_mgr)
{
  G_ASSERTF(net::is_this_thread_net_em_owner(), "reset_time_mgr off the net-EM owner thread (current=%lld)",
    (long long)get_current_thread_id());
  ITimeManager *prev = nullptr;
  {
    OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
    prev = g_net_globals.timeMgr;
    g_net_globals.timeMgr = new_mgr ? new_mgr : &g_net_globals.dummyTime;
  }
  if (prev && prev != &g_net_globals.dummyTime)
    delete prev;
  publish_net_state_now();
}


ITimeManager &get_time_mgr()
{
  G_ASSERTF(net::is_this_thread_net_em_owner(),
    "time-mgr dispatch trampoline ran off the net-EM owner thread (current=%lld); "
    "net message dispatch is supposed to be on the owner thread by construction",
    (long long)get_current_thread_id());
  return *g_net_globals.timeMgr;
}


NetGlobals::~NetGlobals() { reset_time_mgr(nullptr); }
