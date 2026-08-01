// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <generic/dag_initOnDemand.h>
#include <daNet/disconnectionCause.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/fixed_vector.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <atomic>
#include <daECS/core/internal/typesAndLimits.h>
#include <daECS/core/internal/asserts.h>
#include <daECS/net/time.h>
#include <daECS/net/network.h>
#include "net.h"
#include <daECS/net/topologyLock.h> // for TopologyLock::ReadScope (used by GET_NET / GET_NET_CTX lenient check)

namespace ecs
{
class EntityManager;
}

namespace net
{
class IConnection;
class INetworkObserver;
class INetDriver;

typedef INetworkObserver *(*create_net_observer_cb_t)(void *buf, size_t bufsz);

struct NetContext
{
  ecs::EntityManager &entityMgr;

  CNetwork network;
  union
  {
    void *netObserverStorage[2];
    struct
    {
      void *zeroptr;
      create_net_observer_cb_t create_obsrv;
    };
  };

  dag::Vector<uint8_t> encryptionKey;
  ServerFlags srvFlags = ServerFlags::None;

  DisconnectionCause lastClientDc = DC_CONNECTION_CLOSED;
  int timeToSwitchServerAddr = 0;

  eastl::fixed_vector<eastl::string, 16> serverUrls;
  int currentServerUrlIdx = 0;

  struct PendingConnect
  {
    eastl::string url;
    int connectGen = 0;
    uint32_t fireAtMs = 0;
  };
  PendingConnect pendingConnect;

  NetContext(ecs::EntityManager &mgr, INetDriver *drv, create_net_observer_cb_t create_obsrv_);
  NetContext(const NetContext &) = delete;
  ~NetContext();
  void createObserver();
  void update(int ms);

  INetworkObserver *getNetObserver() { return reinterpret_cast<INetworkObserver *>(&netObserverStorage); }
  CNetwork &getNet() { return network; }
};

// daNetGame net binding (g_net_em). Defined file-private in net.cpp behind a publish spinlock.
bool is_this_thread_net_em_owner();                    // true on owner thread OR when no session is open
bool net_em_matches_or_unset(ecs::EntityManager &mgr); // false only if bound to a different EM
bool is_net_em_active();                               // true iff a session is open
void publish_net_em(ecs::EntityManager &mgr);
void clear_net_em();

} // namespace net

extern InitOnDemand<net::NetContext, false> net_context;


// Field names mirror the live function names so READ_CURRENT_OR_SNAPSHOT(name) works one-arg.
struct NetStateSnapshot
{
  bool has_network = false;
  bool is_server = true;
  bool is_true_net_server = false;
  bool is_dummy_time = true;
  net::ServerFlags get_server_flags = net::ServerFlags::None;
  float get_sync_time = 0.f;
  double get_sync_time_d = 0.0;
  int get_sync_millis = 0;
};

struct NetGlobals
{
  int connectGen = 0;

  DummyTimeManager dummyTime;
  ITimeManager *timeMgr = &dummyTime;

  OSSpinlock snapshotLock;
  NetStateSnapshot authoritativeSnapshot DAG_TS_GUARDED_BY(snapshotLock);
  std::atomic<uint32_t> snapshotVersion{0}; // bumped at most once per owner tick (release, under lock)

  ~NetGlobals();
};

extern NetGlobals g_net_globals;

// daNetGame-private NetContext accessor. Same lenient check as GET_NET() in net.h.
#define GET_NET_CTX()                                                                                                      \
  ([]() -> net::NetContext * {                                                                                             \
    DAECS_EXT_ASSERTF(net::is_this_thread_net_em_owner() || net::topology_read_pin_active(),                               \
      "GET_NET_CTX off the net-owner thread without a topology ReadScope (cur=%lld)", (long long)get_current_thread_id()); \
    return net_context.get();                                                                                              \
  }())

#define GET_NET_CTX_FOR(mgr_)                                                                                    \
  ([&]() -> net::NetContext * {                                                                                  \
    net::NetContext *ctx_ = net_context.get();                                                                   \
    if (!ctx_)                                                                                                   \
      return nullptr;                                                                                            \
    const bool emOK_ = &ctx_->entityMgr == &(mgr_);                                                              \
    const bool threadOK_ = net::is_this_thread_net_em_owner();                                                   \
    if (!emOK_ && !threadOK_)                                                                                    \
      return nullptr;                                                                                            \
    G_ASSERTF_RETURN(emOK_ &&threadOK_, nullptr, "net: misrouted call (emOK=%d threadOK=%d)", emOK_, threadOK_); \
    return ctx_;                                                                                                 \
  }())

#define ASSERT_NET_EM_IS(mgr_)                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    G_UNUSED(mgr_);                                                                                                    \
    G_ASSERTF(::net::net_em_matches_or_unset(mgr_), "send through EM=%p but net is bound to a different EM", &(mgr_)); \
  } while (0)

// Wrong-thread early-return; pass the value to return for non-void callers.
#define RETURN_IF_NOT_NET_CTX_OWNER_THREAD(...) \
  do                                            \
  {                                             \
    if (!net::is_this_thread_net_em_owner())    \
      return __VA_ARGS__;                       \
  } while (0)

#define ASSERT_MAIN_THREAD_GLOBAL_NET_FOR(mgr_)                                                                                \
  do                                                                                                                           \
  {                                                                                                                            \
    G_ASSERTF(is_main_thread_network(), "needs main-thread-net (net=%d)", (int)is_main_thread_network());                      \
    G_ASSERTF(&(mgr_) == g_entity_mgr.getRaw(), "targeted send needs g_entity_mgr=%p got %p", g_entity_mgr.getRaw(), &(mgr_)); \
    G_ASSERTF((mgr_).getOwnerThreadId() == get_current_thread_id(),                                                            \
      "targeted send needs current thread to own the EM (owner=%lld cur=%lld)", (long long)(mgr_).getOwnerThreadId(),          \
      (long long)get_current_thread_id());                                                                                     \
  } while (0)

// TLS cache of net state, versioned against the authoritative snapshot. NetSnapshotScope primes
// it on entry; publish_net_state_now bumps the version on the owner tick. Reads compare versions
// and lazy-resync on divergence (see read_snapshot_lazy_resync), so a read from any thread returns
// a coherent value even with no active scope.
extern thread_local NetStateSnapshot net_snap;
extern thread_local uint32_t net_snap_tls_version; // 0 = never synced
extern thread_local bool net_snap_valid;

// Out-of-line slow path (netSnapshot.cpp): resync a scope-less thread's TLS from the authoritative
// snapshot and warn once (logwarn, not logerr -- logerr fails the test build). Kept off the hot
// path.
void net_snapshot_tls_resync();

// Hot path. Owner and active-scope threads keep their own TLS coherent (publish primes the owner
// each tick; NetSnapshotScope primes on entry and holds a stable snapshot for its whole duration),
// so we read it as-is -- resyncing them mid-scope would reintroduce the cross-field TOCTOU the scope
// exists to prevent. Only a scope-less thread lazy-resyncs, and only when its cached version is
// stale so repeated reads stay a single TLS load.
inline void read_snapshot_lazy_resync()
{
  if (DAGOR_UNLIKELY(!net_snap_valid && g_net_globals.snapshotVersion.load(std::memory_order_acquire) != net_snap_tls_version))
    net_snapshot_tls_resync();
}

inline void assert_non_owner_snapshot_read()
{
  G_FAST_ASSERT(!net::is_this_thread_net_em_owner() && "owner reading non-time snapshot field; call the live function");
}

// Non-time field: owner -> name_live() (file-static); non-owner -> scope-guarded TLS.
#define READ_CURRENT_OR_SNAPSHOT(name) (net::is_this_thread_net_em_owner() ? (name##_live()) : READ_SNAPSHOT(name))

// Strict snapshot read; asserts non-owner (owner must call the live function for non-time fields).
#define READ_SNAPSHOT(field) ((read_snapshot_lazy_resync(), assert_non_owner_snapshot_read(), net_snap.field))

// "No current": single TLS load on every thread, no branch on owner identity. The owner
// thread sees the previous publish's value within one frame (publish_net_state_now updates
// owner TLS atomically with the global snapshot, so the gap is bounded to one publish);
// callers that need this-tick precision must go through the live timeMgr instead. Read speed
// matters more here than freshness -- daRg/UI builds call these getters in tight loops.
// read_snapshot_lazy_resync keeps the TLS coherent on every thread (owner primes it via publish,
// others via NetSnapshotScope or the lazy resync), so this stays a single TLS load in the common
// case with only a version compare guarding it.
#define READ_SNAPSHOT_NO_CURRENT(field) (read_snapshot_lazy_resync(), net_snap.field)

void publish_net_state_now();

void flush_new_connection(net::IConnection &conn);

void init_remote_recreate_entity_from_client();

const char *select_next_server_url(bool rotate = true);

void on_client_disconnected(ecs::EntityManager &manager, DisconnectionCause cause);

bool create_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool bootstrap_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool destroy_net_ctx();

void install_session_routes_and_connect(ecs::EntityManager &mgr, dag::Vector<eastl::string> urls, eastl::string relayUrl);
