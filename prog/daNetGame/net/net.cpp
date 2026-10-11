// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net.h"
#include "authCountryCode.h"
#include "game/dasEvents.h"
#include "netControlClient.h"
#include "netEvents.h"
#include "netListenServerObserver.h"
#include "netPrivate.h"
#include "replaySession.h"

#include "main/ecsUtils.h"
#include "main/main.h"
#include "net/channel.h"
#include "net/dedicated.h"
#include "netConsts.h"
#include "netStat.h"
#include "protoVersion.h"
#include "time.h"

#include <daECS/core/entityManager.h>
#include <daECS/net/connection.h>
#include <daECS/net/message.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/netEvent.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/network.h>

#include <daNet/daNetPeerInterface.h>
#include <daNet/getTime.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <statsd/statsd.h>
#include <util/dag_console.h>
#include <util/dag_string.h>

#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/vector.h>

#include <stdlib.h>
#include <string.h>

ECS_REGISTER_EVENT(OnNetUpdate)
ECS_REGISTER_EVENT(OnNetDestroy)
ECS_REGISTER_EVENT(EventNetTornDown)
ECS_REGISTER_EVENT(OnNetInitClient)
ECS_REGISTER_EVENT(OnNetInitServer)

extern void ucr_send_hello(const char *); // from daNet

namespace net
{
NetContext::NetContext(ecs::EntityManager &mgr, INetDriver *drv, create_net_observer_cb_t create_obsrv_) :
  entityMgr(mgr),
  network(mgr, drv, getNetObserver(), NET_PROTO_VERSION, get_session_id()),
  zeroptr(nullptr),
  create_obsrv(create_obsrv_)
{
  G_FAST_ASSERT(create_obsrv);
  G_FAST_ASSERT(!netObserverStorage[0]);
}

NetContext::~NetContext()
{
  if (netObserverStorage[0])
    getNetObserver()->~INetworkObserver();
}

void NetContext::createObserver()
{
  if (!netObserverStorage[0])
  {
    G_VERIFY(create_obsrv(netObserverStorage, sizeof(netObserverStorage)));
    G_FAST_ASSERT(netObserverStorage[0]);
  }
}

void NetContext::update(int ms) { network.update(ms, NC_REPLICATION); }

bool is_this_thread_net_em_owner() { return is_net_authority_thread_or_unset(); }

} // namespace net

InitOnDemand<net::NetContext, false> net_context;

NetGlobals g_net_globals;

struct OnNetCall
{
  int gen = 0;
  eastl::function<void(bool)> fn;
};
static OSSpinlock g_on_net_lock;
static eastl::vector<OnNetCall> g_on_net;

static void bump_net_gen()
{
  OSSpinlockScopedLock lock(g_on_net_lock);
  ++g_net_globals.connectGen;
}

void on_net_thread(eastl::function<void(bool)> fn)
{
  if (!fn)
    return;
  OSSpinlockScopedLock lock(g_on_net_lock);
  g_on_net.push_back({g_net_globals.connectGen, eastl::move(fn)});
}

static void drain_on_net_thread()
{
  eastl::vector<OnNetCall> fns;
  int gen = 0;
  {
    OSSpinlockScopedLock lock(g_on_net_lock);
    fns.swap(g_on_net);
    gen = g_net_globals.connectGen;
  }
  for (auto &call : fns)
    call.fn(call.gen == gen);
}

net::CNetwork *get_net_or_null_unchecked() { return net_context ? &net_context->getNet() : nullptr; }

net::INetworkObserver *get_net_observer()
{
  auto *ctx = GET_NET_CTX();
  return ctx ? ctx->getNetObserver() : nullptr;
}

bool send_msg_to_client(net::IMessage &&msg, int client_conn_id)
{
  net::CNetwork *netw = GET_NET();
  if (!netw)
    return false;
  auto &conns = netw->getClientConnections();
  net::IConnection *conn = ((unsigned)client_conn_id < conns.size()) ? conns[client_conn_id].get() : nullptr;
  if (!conn)
    return false;
  return netw->sendto(/*time*/ 0, net::get_msg_sink(), eastl::move(msg), conn);
}

void debug_verify_net_connection_ptr(net::IConnection *conn)
{
#if DAGOR_DBGLEVEL > 0
  if (net::CNetwork *netw = GET_NET())
    G_ASSERT(netw->debugVerifyNetConnectionPtr(conn));
#else
  G_UNUSED(conn);
#endif
}

bool create_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb)
{
  G_ASSERT(drv);
  G_ASSERT(obs_cb);
  if (net_context)
  {
    drv->destroy();
    return false;
  }
  net::set_net_authority_thread();
  net_context.demandInit(mgr, drv, obs_cb);
  return true;
}

bool destroy_net_ctx()
{
  if (!net_context)
  {
    net::clear_net_authority_thread();
    return false;
  }
  net_context.demandDestroy();
  net::clear_net_authority_thread();
  return true;
}

bool bootstrap_net_ctx(ecs::EntityManager &mgr, net::INetDriver *drv, net::create_net_observer_cb_t obs_cb)
{
  bump_net_gen();
  if (!create_net_ctx(mgr, drv, obs_cb))
    return false;
  net_context->createObserver();
  return true;
}
extern const uint8_t TIMESYNC_NET_CHANNEL = NC_TIME_SYNC;
void timesync_push_rtt(uint32_t rtt) { netstat::set(netstat::CT_RTT, rtt); }

// in case of problems with current route, fallback shouldn't be too eager
static constexpr int DEFAULT_TIME_TO_SWITCH_SERVER_ADDR = 4000;
// in case of probing of alternative server route, fallback should be fast
static constexpr int DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST = DEFAULT_TIME_TO_SWITCH_SERVER_ADDR / 2;

const char *select_next_server_url(bool rotate)
{
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return nullptr;
  if (++ctx->currentServerUrlIdx >= (int)ctx->serverUrls.size() && rotate)
    ctx->currentServerUrlIdx = 0;
  return (ctx->currentServerUrlIdx < (int)ctx->serverUrls.size()) ? ctx->serverUrls[ctx->currentServerUrlIdx].c_str() : nullptr;
}

uint32_t get_current_server_route_id()
{
  auto *ctx = GET_NET_CTX();
  return ctx ? (uint32_t)ctx->currentServerUrlIdx : 0u;
}

const char *get_server_route_host(uint32_t route_id)
{
  static char tmpbuf[64]; // This is slightly ugly but done this way to avoid
                          // dealing with das block/temp string handling
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return nullptr;
  const char *surl = (route_id < ctx->serverUrls.size()) ? ctx->serverUrls[route_id].c_str() : nullptr;
  if (const char *pcol = surl ? strchr(surl, ':') : nullptr)
  {
    size_t l = eastl::min<size_t>(pcol - surl, countof(tmpbuf) - 1); //-V1004
    memcpy(tmpbuf, surl, l);                                         //-V1004
    tmpbuf[l] = 0;
    return tmpbuf;
  }
  else
    return surl;
}

uint32_t get_server_route_count()
{
  auto *ctx = GET_NET_CTX();
  return ctx ? (uint32_t)ctx->serverUrls.size() : 0u;
}

void switch_server_route(uint32_t route_id)
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->getNet().isServer())
    return;
  G_ASSERT_RETURN(route_id < ctx->serverUrls.size(), );
  G_ASSERT_RETURN(route_id != (uint32_t)ctx->currentServerUrlIdx, );

  debug("%s route=%d -> %d, url=%s -> %s", __FUNCTION__, ctx->currentServerUrlIdx, route_id,
    ctx->serverUrls[ctx->currentServerUrlIdx].c_str(), ctx->serverUrls[route_id].c_str());

  net::IConnection *sconn = ctx->network.getServerConnection();
  if (!sconn)
    return;

  const int timeToSwitchFast =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServerFast", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST);
  ctx->timeToSwitchServerAddr = get_no_packets_time_ms() + timeToSwitchFast;

  ctx->currentServerUrlIdx = (int)route_id;
  sconn->changeSendAddress(ctx->serverUrls[ctx->currentServerUrlIdx].c_str());
}

void send_echo_msg(uint32_t route_id)
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->getNet().isServer())
    return;
  G_ASSERT_RETURN(route_id < ctx->serverUrls.size(), );

  net::IConnection *sconn = ctx->network.getServerConnection();
  if (!sconn)
    return;

  sconn->sendEcho(ctx->serverUrls[route_id].c_str(), route_id);
}

void on_client_disconnected(ecs::EntityManager &manager, DisconnectionCause cause)
{
  bump_net_gen();
  if (net_context)
    net_context->lastClientDc = cause;
  // Immediate so the netClient,userEM ES fires before sweep_pending_destroy
  // switches the role tag back to server.
  eastl::string details;
  if (net_context)
    details.swap(net_context->disconnectDetails);
  manager.broadcastEventImmediate(EventOnDisconnectedFromServer(cause, eastl::move(details)));
}

bool has_network() { return bool(net_context); }
bool is_server() { return !net_context || net_context->getNet().isServer(); }
bool is_true_net_server()
{
  return net_context && net_context->getNet().isServer() && net_context->getNet().getDriver()->getControlIface() != nullptr;
}
net::ServerFlags net::get_server_flags() { return net_context ? net_context->srvFlags : ServerFlags::None; }

// Connection getters: net-owner thread (GET_NET_CTX).
net::IConnection *get_server_conn()
{
  auto *ctx = GET_NET_CTX();
  return ctx ? ctx->getNet().getServerConnection() : nullptr;
}

net::IConnection *get_client_connection(int i)
{
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return nullptr;
  auto &conns = ctx->getNet().getClientConnections();
  return (unsigned)i < conns.size() ? conns[i].get() : nullptr;
}

dag::Span<net::IConnection *> get_client_connections()
{
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return {};
  auto &conns = ctx->getNet().getClientConnections();
  G_STATIC_ASSERT(sizeof(conns[0]) == sizeof(net::IConnection *));
  return dag::Span<net::IConnection *>((net::IConnection **)conns.data(), //-V1032
    conns.size());
}

int get_no_packets_time_ms()
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->getNet().isServer())
    return 0;
  DaNetTime last = ctx->getNet().getDriver()->getLastReceivedPacketTime(0);
  DaNetTime curTime = danet::GetTime();
  G_ASSERTF(curTime >= last, "current time (%u) < last recive time (%u)", curTime, last);
  return (last && curTime >= last) ? curTime - last : 0;
}

static void switch_unresponsive_server_addr(ecs::EntityManager &manager, net::NetContext &nctx)
{
  net::IConnection *sconn = (nctx.serverUrls.size() >= 2) ? nctx.getNet().getServerConnection() : nullptr;
  if (!sconn)
    return;
  static const int timeToSwitch =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServer", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR);
  if (!timeToSwitch)
    return;
  if (!nctx.timeToSwitchServerAddr)
    nctx.timeToSwitchServerAddr = timeToSwitch;
  int nopktt = get_no_packets_time_ms();
  if (nopktt <= nctx.timeToSwitchServerAddr)
  {
    if (nopktt < timeToSwitch)
      nctx.timeToSwitchServerAddr = timeToSwitch;
    return;
  }

  G_ASSERT_RETURN(nctx.currentServerUrlIdx < (int)nctx.serverUrls.size(), );

  logwarn("No network packets for %d ms from %s", nopktt, nctx.serverUrls[nctx.currentServerUrlIdx].c_str());
  manager.broadcastEvent(ChangeServerRoute{/*currentIsUnresponsive*/ true});
  eastl::string country_code;
  auth_get_country_code(manager, country_code);
  statsd::counter("net.server_addr_change", 1,
    {{"addr", get_server_route_host((uint32_t)nctx.currentServerUrlIdx)}, {"country", country_code.c_str()}});
  const int timeToSwitchFast =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServerFast", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST);
  nctx.timeToSwitchServerAddr = nopktt + timeToSwitchFast;
}

void net_disconnect(net::IConnection &conn, DisconnectionCause cause)
{
  // conn ref validity is the caller's; conn.disconnect() is driver-internal.
  // Anticheat fires off-thread.
  if (!has_network())
    return;
  conn.disconnect(cause);
}

void disconnect_from_relay()
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->network.getDriver() == nullptr)
    return;
  ctx->network.getDriver()->disconnect_relay();
}

bool establish_relay_connection(const char *relay_url)
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->network.getDriver() == nullptr)
  {
    logerr("failed to initiate connection with relay host/port '%s' - no "
           "network initialized",
      relay_url);
    return false;
  }

  auto *daif = static_cast<DaNetPeerInterface *>(ctx->network.getDriver()->getControlIface());
  if (!daif || !daif->GetHostRaw())
  {
    logerr("failed to initiate connection with relay host/port '%s' - no "
           "enet host",
      relay_url);
    return false;
  }

  if (!ctx->network.getDriver()->connect(relay_url, 0, true))
  {
    logerr("failed to initiate connection with relay host/port '%s' - "
           "connection failed",
      relay_url);
    return false;
  }
  return true;
}

eastl::string get_received_stun_system_address_str()
{
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return {};
  SystemAddress addr = ctx->network.getStunSystemAddress();
  if (addr.port == 0)
    return {};
  const char *addrStr = addr.ToString();
  return addrStr ? eastl::string(addrStr) : eastl::string();
}

void request_udp_punch_via_relay(const char *relay_addr)
{
  debug("request_udp_punch_via_relay: '%s'", relay_addr ? relay_addr : "<null>");
  auto *ctx = GET_NET_CTX();
  auto *daif = ctx ? static_cast<DaNetPeerInterface *>(ctx->network.getDriver()->getControlIface()) : nullptr;
  if (!daif)
  {
    debug("request_udp_punch_via_relay: no active network peer, ignoring");
    return;
  }
  const char *colon = relay_addr ? strrchr(relay_addr, ':') : nullptr;
  if (!colon)
  {
    logerr("request_udp_punch_via_relay: bad address '%s'", relay_addr);
    return;
  }
  char hostBuf[128];
  size_t hostLen = eastl::min(size_t(colon - relay_addr), sizeof(hostBuf) - 1);
  memcpy(hostBuf, relay_addr, hostLen);
  hostBuf[hostLen] = '\0';
  char *endPtr = nullptr;
  long port = strtol(colon + 1, &endPtr, 10);
  if (endPtr == colon + 1 || port <= 0 || port > 65535)
  {
    logerr("request_udp_punch_via_relay: bad port in '%s'", relay_addr);
    return;
  }
  SystemAddress addr;
  addr.SetBinaryAddress(hostBuf);
  addr.port = (uint16_t)port;
  if (!addr.host)
  {
    logerr("request_udp_punch_via_relay: failed to resolve host in '%s'", relay_addr);
    return;
  }
  daif->SendUnconnected(addr);
  debug("request_udp_punch_via_relay: queued punch to %s", relay_addr);
}

bool set_relay_connection_handler(void (*relayConnectionHandler)(bool))
{
  auto *ctx = GET_NET_CTX();
  if (!ctx || ctx->network.getDriver() == nullptr)
  {
    logerr("failed to set relay connection handler - no network initialized");
    return false;
  }

  ctx->network.getDriver()->setRelayConnectionHandler(relayConnectionHandler);
  return true;
}

static void net_do_connect(const eastl::string &url, int gen)
{
  if (!net_context || g_net_globals.connectGen != gen)
  {
    debug("net_do_connect: skipped (net_context=%s, gen %d vs current %d) for "
          "'%s'",
      net_context ? "alive" : "gone", gen, g_net_globals.connectGen, url.c_str());
    return;
  }
  debug("net_do_connect: connecting to '%s'", url.c_str());
  net_context->network.getDriver()->connect(url.c_str(), NET_PROTO_VERSION);
}

void install_session_routes_and_connect(ecs::EntityManager &mgr, dag::Vector<eastl::string> urls, eastl::string relayUrl)
{
  G_ASSERT_RETURN(net_context, );
  G_ASSERT_RETURN(!urls.empty(), );
  auto &ctx = *net_context;

  eastl::string country;
  auth_get_country_code(mgr, country);
  for (int i = 0; i < (int)urls.size(); ++i)
  {
    ctx.serverUrls.emplace_back(eastl::move(urls[i]));
    if (!ctx.serverUrls.back().empty())
    {
      statsd::counter("net.server_addr_init", 1, {{"addr", get_server_route_host((uint32_t)i)}, {"country", country.c_str()}});
    }
  }
  ctx.currentServerUrlIdx = 0;

  debug("install_session_routes: relayStunRequestAddr='%s'", relayUrl.c_str());
  if (!relayUrl.empty())
  {
    request_udp_punch_via_relay(relayUrl.c_str());
    ctx.pendingConnect.url = ctx.serverUrls.front();
    ctx.pendingConnect.connectGen = g_net_globals.connectGen;
    ctx.pendingConnect.fireAtMs = danet::GetTime() + 200;
  }
  else
  {
    debug("install_session_routes: no relayStunRequestAddr, connecting "
          "immediately");
    net_do_connect(ctx.serverUrls.front(), g_net_globals.connectGen);
  }
  for (int i = 1; i < (int)ctx.serverUrls.size(); ++i)
    ucr_send_hello(ctx.serverUrls[i].c_str());
}

void net_update(ecs::EntityManager &mgr)
{
  G_UNUSED(mgr);
  RETURN_IF_NOT_NET_CTX_OWNER_THREAD();
  drain_on_net_thread();
  auto *nctx = GET_NET_CTX();
  if (nctx)
  {
    TIME_PROFILE(net_update);
    int curMs = get_async_millis();
    nctx->update(curMs);
    netstat::update(curMs, static_cast<DaNetPeerInterface *>(nctx->network.getDriver()->getControlIface()));

    if (nctx->pendingConnect.fireAtMs != 0 && danet::GetTime() >= nctx->pendingConnect.fireAtMs)
    {
      eastl::string url = eastl::move(nctx->pendingConnect.url);
      int gen = nctx->pendingConnect.connectGen;
      nctx->pendingConnect.fireAtMs = 0;
      nctx->pendingConnect.url.clear();
      net_do_connect(url, gen);
    }

    nctx->network.getEntityManager().broadcastEventImmediate(OnNetUpdate{});
    switch_unresponsive_server_addr(nctx->network.getEntityManager(), *nctx);
  }
}

static void net_dump_stats()
{
  if (auto *ctx = GET_NET_CTX())
    ctx->getNet().dumpStats();
}

static bool net_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("net", "change_server_addr", 1, 2)
  {
    if (net::IConnection *conn = get_server_conn())
    {
      if (const char *newAddr = (argc < 2) ? select_next_server_url() : argv[1])
        console::print_d("Change server addr to %s - %s", newAddr, conn->changeSendAddress(newAddr) ? "OK" : "Failed!");
      else
        console::print_d("No server addresses to choose from");
    }
    else
      console::print_d("No active server connection");
  }
  CONSOLE_CHECK_NAME("net", "dump_stats", 1, 1) { net_dump_stats(); }
  return found;
}
REGISTER_CONSOLE_HANDLER(net_console_handler);

bool net_on_about_to_clear_all_entities(ecs::EntityManager &mgr)
{
  G_UNUSED(mgr);
  RETURN_IF_NOT_NET_CTX_OWNER_THREAD(false);
  if (auto *ctx = GET_NET_CTX())
  {
    ctx->network.getEntityManager().broadcastEventImmediate(EventOnNetworkDestroyed(ctx->lastClientDc));
    return true;
  }
  return false;
}

bool net_destroy(ecs::EntityManager &mgr, bool final)
{
  RETURN_IF_NOT_NET_CTX_OWNER_THREAD(false);
  // SP/offline opens a session without NetContext (net_init_late_server SP branch);
  // authority tid tracks presence the way the old EM binding did.
  if (!net::is_net_session_active())
    return false;
  net_dump_stats();
  mgr.broadcastEventImmediate(OnNetDestroy{final});
  bump_net_gen();
  // destroy_net_ctx clears authority tid; user-thread reset would then fail
  // is_net_lifecycle_thread (tid 0 means main).
  reset_time_mgr();
  destroy_net_ctx();
  drain_on_net_thread();
  net::event::release_claim(&mgr);
  mgr.broadcastEventImmediate(EventNetTornDown{});
  return true;
}

void net_stop()
{
  if (auto *ctx = GET_NET_CTX())
    ctx->getNet().stopAll(DC_CONNECTION_STOPPED);
}
