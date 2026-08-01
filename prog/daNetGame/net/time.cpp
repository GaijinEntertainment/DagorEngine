// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "time.h"
#include "netPrivate.h"

#include <daECS/net/time.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_assert.h>


// Live impl for is_dummy_time; consumed by publish_net_state_now via extern decl.
bool is_dummy_time_live() { return g_net_globals.timeMgr == &g_net_globals.dummyTime; }

// Time getters: pure TLS read on every thread. Owner TLS is updated by publish_net_state_now
// in lockstep with the global; non-owner TLS is synced by NetSnapshotScope. Read speed wins
// over freshness here -- owner sees the previous publish's value within a frame.
float get_sync_time() { return READ_SNAPSHOT_NO_CURRENT(get_sync_time); }
double get_sync_time_d() { return READ_SNAPSHOT_NO_CURRENT(get_sync_time_d); }
int get_sync_millis() { return READ_SNAPSHOT_NO_CURRENT(get_sync_millis); }
bool is_dummy_time() { return READ_SNAPSHOT_NO_CURRENT(is_dummy_time); }

int get_async_millis()
{
  // Advances continuously within a frame; can't snapshot. Off-owner locks against reset/advance.
  if (net::is_this_thread_net_em_owner())
    return g_net_globals.timeMgr->getAsyncMillis();
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  return g_net_globals.timeMgr->getAsyncMillis();
}


double advance_time(float dt, float &out_rt_dt)
{
  if (!net::is_this_thread_net_em_owner())
    return get_sync_time_d();
  // No publish here; the per-frame net_update publish picks up the new time (chaining would double-bump).
  // Lock pairs with off-owner get_async_millis so accum-driven mgrs aren't observed mid-mutation.
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  return g_net_globals.timeMgr->advance(dt, out_rt_dt);
}

void reset_time_mgr(ITimeManager *new_mgr)
{
  G_ASSERTF(net::is_this_thread_net_em_owner(), "reset_time_mgr off the net owner thread (current=%lld)",
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
    "time-mgr dispatch trampoline ran off the mgr-owning thread (current=%lld); "
    "net message dispatch is supposed to be on the owner thread by construction",
    (long long)get_current_thread_id());
  return *g_net_globals.timeMgr;
}


NetGlobals::~NetGlobals() { reset_time_mgr(nullptr); }
