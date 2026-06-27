// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPrivate.h"
#include "net.h"
#include "time.h"

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <debug/dag_assert.h>


thread_local NetStateSnapshot net_snap;
thread_local uint64_t net_snap_version = 0;
#if DAGOR_DBGLEVEL > 0
// Pinned mode (debug only): thread opted into snapshot semantics by calling
// sync_thread_net_snapshot() at least once. State is expected to stay fixed
// across the thread's cycle, so staleness against the authoritative version
// is a bug worth reporting. Default (unpinned) just lazy-refreshes on every
// read with no diagnostics -- right for ad-hoc readers (UI, loaders).
// In release the staleness diagnostic and the pinned/warned/erred flags go
// away entirely; ensure_tls_snapshot_fresh() just lazy-refreshes every time.
static thread_local bool net_snap_pinned = false;
static thread_local bool net_snap_warned_stale = false;
static thread_local bool net_snap_erred_stale = false;
static constexpr uint64_t NET_SNAPSHOT_STALE_WARN_THRESHOLD = 10;
static constexpr uint64_t NET_SNAPSHOT_STALE_ERR_THRESHOLD = 100;
#endif

static void refresh_tls_from_authoritative()
{
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  if (net_snap_version != g_net_globals.authoritativeVersion)
  {
    net_snap = g_net_globals.authoritativeSnapshot;
    net_snap_version = g_net_globals.authoritativeVersion;
  }
}

void net::sync_thread_net_snapshot_lazy()
{
  if (net::is_this_thread_net_em_owner())
    return;
  refresh_tls_from_authoritative();
}

void net::ensure_tls_snapshot_fresh()
{
  const uint64_t auth = interlocked_relaxed_load(g_net_globals.authoritativeVersion);
#if DAGOR_DBGLEVEL > 0
  if (DAGOR_LIKELY(!net_snap_pinned))
  {
    if (auth != net_snap_version)
      refresh_tls_from_authoritative();
    return;
  }
  const uint64_t delta = auth - net_snap_version;
  if (DAGOR_UNLIKELY(delta > NET_SNAPSHOT_STALE_ERR_THRESHOLD))
  {
    if (!net_snap_erred_stale)
    {
      net_snap_erred_stale = true;
      logerr("net snapshot TLS %llu publishes behind on pinned thread %lld -- missing periodic "
             "sync_thread_net_snapshot() at the cycle boundary; lazy-refreshing now (see following stack)",
        (unsigned long long)delta, (long long)get_current_thread_id());
      debug_dump_stack("net snapshot TLS hard-stale stack", /*skip_frames*/ 1);
    }
    refresh_tls_from_authoritative();
  }
  else if (DAGOR_UNLIKELY(delta > NET_SNAPSHOT_STALE_WARN_THRESHOLD))
  {
    if (!net_snap_warned_stale)
    {
      net_snap_warned_stale = true;
      logwarn("net snapshot TLS %llu publishes behind on pinned thread %lld -- sync hook may be running at a coarser "
              "cadence than the owner is publishing; lazy-refreshing now",
        (unsigned long long)delta, (long long)get_current_thread_id());
    }
    refresh_tls_from_authoritative();
  }
  // pinned within thresholds: keep stale value intentionally (frozen across cycle)
#else
  if (auth != net_snap_version)
    refresh_tls_from_authoritative();
#endif
}

void publish_net_state_now()
{
  G_ASSERTF(net::is_this_thread_net_em_owner(), "publish_net_state_now off the net-EM owner thread (current=%lld)",
    (long long)get_current_thread_id());
  NetStateSnapshot s;
  s.hasNetwork = has_network();
  s.isServer = is_server();
  s.isTrueNetServer = is_true_net_server();
  s.srvFlags = net::get_server_flags();
  s.isDummyTime = is_dummy_time();
  s.syncTimeSec = get_sync_time();
  s.syncTimeSecD = get_sync_time_d();
  s.syncMillis = get_sync_millis();
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  g_net_globals.authoritativeSnapshot = s;
  const uint64_t prevVer = g_net_globals.authoritativeVersion;
  g_net_globals.authoritativeVersion = prevVer + 1;
}
