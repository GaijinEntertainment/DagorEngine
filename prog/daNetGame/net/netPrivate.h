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
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <daECS/core/internal/asserts.h>
#include <daECS/net/authority.h>
#include <daECS/net/time.h>
#include <daECS/net/network.h>
#include "net.h"

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
  eastl::string disconnectDetails; // from DisconnectDetails msg, consumed by on_client_disconnected
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
  void setSrvFlags(ServerFlags f) { srvFlags = f; }

  INetworkObserver *getNetObserver() { return reinterpret_cast<INetworkObserver *>(&netObserverStorage); }
  CNetwork &getNet() { return network; }
};

// Compat name; maps to net authority thread (or unset).
bool is_this_thread_net_em_owner();

} // namespace net

extern InitOnDemand<net::NetContext, false> net_context;

struct NetGlobals
{
  int connectGen = 0;

  bool isDefaultTimeMgr = true;
  ITimeManager *timeMgr = create_accum_time();

  ~NetGlobals();
};

extern NetGlobals g_net_globals;

// daNetGame-private NetContext accessor. No owner-thread assert (same as GET_NET):
// off-owner callers (relay / messaging / eden main-thread teardown) may read the seat.
#define GET_NET_CTX() (net_context.get())

// Mutating net: authority_tid (or main thread when unset). Reads use GET_NET_CTX.
#define RETURN_IF_NOT_NET_CTX_OWNER_THREAD(...) \
  do                                            \
  {                                             \
    if (!net::is_net_lifecycle_thread())        \
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

void flush_new_connection(net::IConnection &conn);

void init_remote_recreate_entity_from_client();

const char *select_next_server_url(bool rotate = true);

void on_client_disconnected(ecs::EntityManager &manager, DisconnectionCause cause);

bool create_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool bootstrap_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb);

bool destroy_net_ctx();

void install_session_routes_and_connect(ecs::EntityManager &mgr, dag::Vector<eastl::string> urls, eastl::string relayUrl);
