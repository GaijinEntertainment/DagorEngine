// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netPrivate.h"
#include "net.h"
#include "time.h"

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <debug/dag_assert.h>
#include <debug/dag_log.h>


thread_local NetStateSnapshot net_snap;
thread_local uint32_t net_snap_tls_version = 0;
thread_local bool net_snap_valid = false;


// Live impls defined in net.cpp (non-time) and time.cpp (dummy-time); used by
// publish_net_state_now to build the authoritative snapshot.
extern bool has_network_live();
extern bool is_server_live();
extern bool is_true_net_server_live();
extern net::ServerFlags get_server_flags_live();
extern bool is_dummy_time_live();


// Copy the authoritative snapshot into this thread's TLS and adopt its version. Shared by
// NetSnapshotScope (on scope entry) and net_snapshot_tls_resync (on a version-diverged read).
static void sync_net_snap_tls_from_authoritative()
{
  OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
  net_snap = g_net_globals.authoritativeSnapshot;
  net_snap_tls_version = g_net_globals.snapshotVersion.load(std::memory_order_relaxed);
}

// Slow path of read_snapshot_lazy_resync (netPrivate.h). Only reached from a scope-less thread
// whose cached version is stale (owner/scoped threads take the hot path), so resync unconditionally
// and warn once. A scope-less read is a bug in every mode -- online it can also race, offline it
// means the reader never primed its TLS. The read is correct either way, and this is a warn (not
// logerr, which is fatal in the test build).
void net_snapshot_tls_resync()
{
  sync_net_snap_tls_from_authoritative();
  LOGWARN_ONCE("READ_SNAPSHOT outside NetSnapshotScope on thread %lld; lazy-synced", (long long)get_current_thread_id());
}


net::NetSnapshotScope::NetSnapshotScope(bool assumeSingleUpdate, const char *label_) :
  label(label_ ? label_ : "anon"), wasValidAtCtor(net_snap_valid), assumeSingle(assumeSingleUpdate)
{
  if (wasValidAtCtor)
    return; // Owner thread (publish_net_em set net_snap_valid) or nested scope -- TLS is already
            // tracked by someone else. Don't touch it.

  // Non-owner thread, first scope on this thread for this session: sync TLS from authoritative.
  if (g_net_globals.snapshotVersion.load(std::memory_order_acquire) != net_snap_tls_version)
    sync_net_snap_tls_from_authoritative();
  capturedVersion = net_snap_tls_version;
  net_snap_valid = true;
}


net::NetSnapshotScope::~NetSnapshotScope()
{
  if (wasValidAtCtor)
    return; // We didn't set net_snap_valid; not ours to clear.

  if (assumeSingle)
  {
    const uint32_t now = g_net_globals.snapshotVersion.load(std::memory_order_acquire);
    if (now != capturedVersion)
      LOGERR_ONCE("NetSnapshotScope[%s]: snapshot changed during scope (%u -> %u); single-update contract broken", label,
        capturedVersion, now);
  }
  net_snap_valid = false;
}


void publish_net_state_now()
{
  G_ASSERTF(net::is_this_thread_net_em_owner(), "publish_net_state_now off the net owner thread (current=%lld)",
    (long long)get_current_thread_id());

  NetStateSnapshot s;
  s.has_network = has_network_live();
  s.is_server = is_server_live();
  s.is_true_net_server = is_true_net_server_live();
  s.get_server_flags = get_server_flags_live();
  s.is_dummy_time = is_dummy_time_live();
  s.get_sync_time = (float)g_net_globals.timeMgr->getSeconds();
  s.get_sync_time_d = g_net_globals.timeMgr->getSeconds();
  s.get_sync_millis = g_net_globals.timeMgr->getMillis();

  uint32_t next = 0;
  {
    OSSpinlockScopedLock lock(g_net_globals.snapshotLock);
    g_net_globals.authoritativeSnapshot = s;
    // Bump version inside the lock with release; bumping outside would let a fast-path reader
    // see the old version with new data and miss the update.
    next = g_net_globals.snapshotVersion.load(std::memory_order_relaxed) + 1;
    g_net_globals.snapshotVersion.store(next, std::memory_order_release);
  }
  // This call runs on the net-EM owner thread (asserted above). Update our own TLS in
  // lockstep with the global publish so subsequent READ_SNAPSHOT_NO_CURRENT reads on this
  // thread see what we just wrote, without ever needing to branch on owner identity.
  // Also mark the TLS valid: in offline / no-session-bound modes (active_matter / enlisted
  // launch tests with disableRemoteNetServices), publish_net_em never runs, so this is the
  // only place the publisher thread gets its valid flag set.
  net_snap = s;
  net_snap_tls_version = next;
  net_snap_valid = true;
}
