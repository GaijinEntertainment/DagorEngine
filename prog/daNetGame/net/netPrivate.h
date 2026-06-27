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


struct NetStateSnapshot
{
  bool hasNetwork = false;
  bool isServer = true;
  bool isTrueNetServer = false;
  bool isDummyTime = true;
  net::ServerFlags srvFlags = net::ServerFlags::None;
  float syncTimeSec = 0.f;
  double syncTimeSecD = 0.0;
  int syncMillis = 0;
};

struct NetGlobals
{
  int connectGen = 0;

  DummyTimeManager dummyTime;
  ITimeManager *timeMgr = &dummyTime;

  OSSpinlock snapshotLock;
  NetStateSnapshot authoritativeSnapshot DAG_TS_GUARDED_BY(snapshotLock);
  volatile uint64_t authoritativeVersion = 0;

  ~NetGlobals();
};

extern NetGlobals g_net_globals;

// daNetGame-private NetContext accessor. Same lenient check as GET_NET() in net.h.
#define GET_NET_CTX()                                                                                                      \
  ([]() -> net::NetContext * {                                                                                             \
    G_ASSERTF(net::is_this_thread_net_em_owner() || net::topology_read_pin_active(),                                       \
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

extern thread_local NetStateSnapshot net_snap;
extern thread_local uint64_t net_snap_version;

namespace net
{
// Refresh TLS snapshot from authoritative without touching the pinned flag.
// Used by sync_thread_net_snapshot() and by ensure_tls_snapshot_fresh() to catch up
// lazy/unpinned readers and to refresh stale pinned readers after diagnostic.
void sync_thread_net_snapshot_lazy();

// Bring the TLS snapshot up to date for a read about to happen on this thread.
// In debug, also emits warn/err diagnostics for pinned threads that fall behind.
// Caller must already have asserted off-EM-owner; this only touches TLS.
void ensure_tls_snapshot_fresh();
} // namespace net

#define READ_NET_SNAPSHOT_FIELD(field)                                                                                            \
  ([]() -> auto {                                                                                                                 \
    G_ASSERTF(!net::is_this_thread_net_em_owner(),                                                                                \
      "net snapshot TLS read from net-EM owner thread (%lld) -- owner reads truth, not TLS", (long long)get_current_thread_id()); \
    net::ensure_tls_snapshot_fresh();                                                                                             \
    return net_snap.field;                                                                                                        \
  }())

void publish_net_state_now();

void flush_new_connection(net::IConnection &conn);

void init_remote_recreate_entity_from_client();

const char *select_next_server_url(bool rotate = true);

void on_client_disconnected(ecs::EntityManager &manager, DisconnectionCause cause);

bool create_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool bootstrap_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool destroy_net_ctx();

void install_session_routes_and_connect(ecs::EntityManager &mgr, dag::Vector<eastl::string> urls, eastl::string relayUrl);
