// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net.h"
#include <debug/dag_assert.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <math/random/dag_random.h>
#include "net/channel.h"
#include "netScope.h"
#include "netEvents.h"
#include "authEvents.h"
#include <daNet/getTime.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/tuple.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <string.h>
#include <stdlib.h>
#include <osApiWrappers/dag_messageBox.h>
#include <util/dag_watchdog.h>
#include <statsd/statsd.h>
#include <ioSys/dag_dataBlock.h>
#if _TARGET_XBOX
#include <gdk/user.h>
#endif
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/template.h>
#include <daECS/net/replay.h>
#include <daECS/net/connection.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/object.h>
#include <daECS/net/network.h>
#include <daECS/net/message.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/replayEvents.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>

#include <syncVroms/syncVroms.h>

#include "game/dasEvents.h"
#include "main/version.h"
#include "main/ecsUtils.h" // create entities
#include "main/level.h"    // is_level_loading()
#include "main/circuit.h"
#include "phys/netPhys.h"
#include "time.h"
#include "netMsg.h"
#include "phys/physUtils.h"
#include "userid.h"
#include "netStat.h"
#include "net/dedicated.h"
#include "net/netPropsRegistry.h"
#include "netConsts.h"
#include "netPrivate.h"

#include "game/riDestr.h"
#include "game/player.h"

#include "main/main.h" // set_window_title
#include "main/appProfile.h"

#include "protoVersion.h"
#include "main/gameLoad.h"
#include "main/vromfs.h"

ECS_REGISTER_EVENT(CmdAddInitialEntitiesInNetScope)
ECS_REGISTER_EVENT(CmdAddDefaultEntitiesInNetScope)
ECS_REGISTER_EVENT(OnNetControlClientCreated)
ECS_REGISTER_EVENT(OnNetControlClientDestroyed)
ECS_REGISTER_EVENT(OnNetControlClientInfoMsg)
ECS_REGISTER_EVENT(OnNetControlClientDisconnect)
ECS_REGISTER_EVENT(OnMsgSinkCreatedClient)
ECS_REGISTER_EVENT(OnNetUpdate)
ECS_REGISTER_EVENT(OnNetDestroy)

#define NET_AUTH_ECS_EVENT ECS_REGISTER_EVENT
NET_AUTH_ECS_EVENTS
#undef NET_AUTH_ECS_EVENT

ECS_NET_IMPL_MSG(ClientInfo,
  net::ROUTING_CLIENT_TO_SERVER,
  ECS_NET_NO_RCPTF,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  dedicated::get_clientinfo_msg_handler());

namespace sceneload
{
extern void on_apply_sync_vroms_diffs_msg(const net::IMessage *);
}

ECS_NET_IMPL_MSG(ApplySyncVromsDiffs,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_ORDERED,
  NC_EFFECT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &sceneload::on_apply_sync_vroms_diffs_msg);
ECS_NET_IMPL_MSG(SyncVromsDone,
  net::ROUTING_CLIENT_TO_SERVER,
  ECS_NET_NO_RCPTF,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  dedicated::get_sync_vroms_done_msg_handler());

extern void gather_replay_meta_info();
extern void clear_replay_meta_info();
extern Tab<uint8_t> gen_replay_meta_data();
extern bool try_create_replay_playback();
extern void server_create_replay_record();
net::CNetwork &net_ctx_init_client(net::INetDriver *drv);
extern void finalize_replay_record(const char *rpath);

typedef net::INetworkObserver *(*create_net_observer_cb_t)(void *, size_t);

struct NetCtx
{
  net::CNetwork &network;
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
  net::ServerFlags srvFlags = net::ServerFlags::None; // from client's POV
  SimpleString recordingReplayFileName;
  bool isRecordingReplay() const { return !recordingReplayFileName.empty(); }

  net::INetworkObserver *getNetObserver() { return reinterpret_cast<net::INetworkObserver *>(&netObserverStorage); }

  NetCtx(net::INetDriver *drv, create_net_observer_cb_t create_obsrv_) :
    network(*new net::CNetwork(drv, getNetObserver(), NET_PROTO_VERSION, net::get_session_id())),
    zeroptr(nullptr),
    create_obsrv(create_obsrv_)
  {
    G_FAST_ASSERT(create_obsrv);
    G_FAST_ASSERT(!netObserverStorage[0]);
  }
  NetCtx(const NetCtx &) = delete;
  ~NetCtx()
  {
    // Explicitly shutdown observer before network as it might shutdown subsystems that depend on it (e.g. anti-cheat middleware)
    if (netObserverStorage[0])
      getNetObserver()->~INetworkObserver();

    delete &network;

    // rename tmp replay file to final one and write meta.json for it
    if (isRecordingReplay())
      finalize_replay_record(recordingReplayFileName.c_str());
  }

  void createObserver()
  {
    if (!netObserverStorage[0])
    {
      G_VERIFY(create_obsrv(netObserverStorage, sizeof(netObserverStorage)));
      G_FAST_ASSERT(netObserverStorage[0]);
    }
  }

  net::CNetwork &getNet() const { return network; }

  void update(int ms) { network.update(ms, NC_REPLICATION); }
};
static InitOnDemand<NetCtx, false> net_ctx;
net::CNetwork *get_net_internal()
{
  return net_ctx ? &net_ctx->getNet() : NULL;
} // Warning: this is *internal* function of network subsystem, don't call it from outside!
net::INetworkObserver *get_net_observer() { return net_ctx->getNetObserver(); }
static DummyTimeManager dummy_time;
struct ITimeMgrDeleter
{
  void operator()(ITimeManager *p)
  {
    if (p && p != &dummy_time)
      delete p;
  }
};
extern const uint8_t TIMESYNC_NET_CHANNEL = NC_TIME_SYNC;
static eastl::unique_ptr<ITimeManager, ITimeMgrDeleter> g_time(&dummy_time);
static DisconnectionCause last_client_dc = DC_CONNECTION_CLOSED;
static eastl::fixed_vector<eastl::string, 4> s_server_urls;
static int current_server_url_i = 0;
static int time_to_switch_server_addr = 0;
// in case of problems with current route, fallback shouldn't be too eager
static constexpr int DEFAULT_TIME_TO_SWITCH_SERVER_ADDR = 4000;
// in case of probing of alternative server route, fallback should be fast
static constexpr int DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST = DEFAULT_TIME_TO_SWITCH_SERVER_ADDR / 2;

static const char *select_next_server_url(bool rotate = true)
{
  if (++current_server_url_i >= s_server_urls.size() && rotate)
    current_server_url_i = 0;
  return (current_server_url_i < s_server_urls.size()) ? s_server_urls[current_server_url_i].c_str() : nullptr;
}

uint32_t get_current_server_route_id() { return current_server_url_i; }

const char *get_server_route_host(uint32_t route_id)
{
  static char tmpbuf[64]; // This is slightly ugly but done this way to avoid dealing with das block/temp string handling
  const char *surl = (route_id < s_server_urls.size()) ? s_server_urls[route_id].c_str() : nullptr;
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

uint32_t get_server_route_count() { return s_server_urls.size(); }

void switch_server_route(uint32_t route_id)
{
  G_VERIFY(has_network() && !is_server());
  G_ASSERT_RETURN(route_id < s_server_urls.size(), );
  G_ASSERT_RETURN(route_id != get_current_server_route_id(), );

  debug("%s route=%d -> %d, url=%s -> %s", __FUNCTION__, current_server_url_i, route_id, s_server_urls[current_server_url_i].c_str(),
    s_server_urls[route_id].c_str());

  net::CNetwork &network = *get_net_internal(); // dereference is safe after G_VERIFY(has_network()) above
  net::IConnection *sconn = network.getServerConnection();
  if (!sconn)
    return;

  const int timeToSwitchFast =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServerFast", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST);
  time_to_switch_server_addr = get_no_packets_time_ms() + timeToSwitchFast; // preparing to switch address faster if it's unresponsive

  current_server_url_i = route_id;
  sconn->changeSendAddress(s_server_urls[current_server_url_i].c_str());
}

void send_echo_msg(uint32_t route_id)
{
  G_ASSERT_RETURN(route_id < s_server_urls.size(), );
  G_ASSERT_RETURN(!is_server(), );

  net::CNetwork *netw = get_net_internal();
  if (!netw)
    return;
  net::IConnection *sconn = netw->getServerConnection();
  if (!sconn)
    return;

  sconn->sendEcho(s_server_urls[route_id].c_str(), route_id);
}

static inline NetCtx &get_net_ctx_ref()
{
  G_ASSERT(net_ctx);
  return *net_ctx.get();
}

static void on_client_disconnected(DisconnectionCause cause)
{
  last_client_dc = cause;
  g_entity_mgr->broadcastEvent(EventOnDisconnectedFromServer(cause));
}

net::ServerFlags net::get_server_flags() { return net_ctx ? net_ctx->srvFlags : net::ServerFlags::None; }

void net_ctx_set_recording_replay_filename(const char *path) { net_ctx->recordingReplayFileName = path; }

bool is_server()
{
  net::CNetwork *net = get_net_internal();
  return !net || net->isServer();
}
bool is_true_net_server()
{
  net::CNetwork *net = get_net_internal();
  return net && net->isServer() && net->getDriver()->getControlIface() != NULL;
}
bool has_network() { return get_net_internal() != NULL; }
ITimeManager &get_time_mgr() { return *g_time.get(); }
void set_time_internal(ITimeManager *tmgr) { g_time.reset(tmgr); }

net::IConnection *get_server_conn()
{
  net::CNetwork *net = get_net_internal();
  return net ? net->getServerConnection() : NULL;
}

net::IConnection *get_client_connection(int i)
{
  net::CNetwork *net = get_net_internal();
  if (!net)
    return NULL;
  auto &conns = net->getClientConnections();
  return (unsigned)i < conns.size() ? conns[i].get() : NULL;
}

dag::Span<net::IConnection *> get_client_connections()
{
  if (net::CNetwork *netw = get_net_internal())
  {
    auto &conns = netw->getClientConnections();
    G_STATIC_ASSERT(sizeof(conns[0]) == sizeof(net::IConnection *));
    return dag::Span<net::IConnection *>((net::IConnection **)conns.data(), conns.size());
  }
  else
    return {};
}

int send_net_msg(ecs::EntityManager &mgr, ecs::EntityId to_eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc)
{
  net::CNetwork *netw = get_net_internal();
  const net::MessageNetDesc &msgDesc = !msg_net_desc ? msg.getMsgClass() : *msg_net_desc;
  int numSends = 0;
  if (msgDesc.routing == net::ROUTING_SERVER_TO_CLIENT) // server -> client
  {
    if (!netw)
      return 0;
    net::recipient_filter_t rcptFilter = msgDesc.rcptFilter;
    G_FAST_ASSERT(rcptFilter != NULL); // verified on message registration
    int curTime = net_ctx->isRecordingReplay() ? g_time->getAsyncMillis() : 0;
    if (rcptFilter == &net::broadcast_rcptf)
    {
      // use iterator instead of 'getClientConnections' because former is filtering out pending connections
      for (net::ConnectionsIterator cit; cit; ++cit)
        numSends += netw->sendto(curTime, to_eid, msg, &*cit, msg_net_desc) ? 1 : 0;
    }
    else
    {
      Tab<net::IConnection *> conns(framemem_ptr());
      for (auto cn : rcptFilter(conns, to_eid, msg))
        if (cn)
          numSends += netw->sendto(curTime, to_eid, msg, cn, msg_net_desc) ? 1 : 0;
      if (net_ctx->isRecordingReplay() && rcptFilter != &net::direct_connection_rcptf)
      {
        auto &cliConns = netw->getClientConnections();
        if (net::Connection *conn = !cliConns.empty() ? cliConns.back().get() : nullptr)
          if (conn->isBlackHole())
            numSends += netw->sendto(curTime, to_eid, msg, conn, msg_net_desc) ? 1 : 0;
      }
    }
  }
  else // client -> server
  {
    if (netw)
      numSends = netw->sendto(/*time*/ 0, to_eid, msg, netw->getServerConnection(), msg_net_desc) ? 1 : 0;
    else // single (no network) mode -> execute client messages as if it was instantly routed to server
    {
      G_ASSERT(is_server());
      if (msgDesc.routing == net::ROUTING_CLIENT_TO_SERVER || msgDesc.routing == net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)
      {
        eastl::unique_ptr<net::IMessage, net::MessageDeleter> msgPtr(eastl::move(msg).moveHeap(), net::MessageDeleter{/*heap*/ true});
        msgPtr->connection = nullptr;
        if (net::event::try_receive(*msgPtr, mgr, to_eid))
          ;
        else
          g_entity_mgr->sendEventImmediate(to_eid, ecs::EventNetMessage(eastl::move(msgPtr)));
        numSends = 1;
      }
    }
  }
  return numSends;
}

int send_net_msg(ecs::EntityId to_eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc)
{
  return send_net_msg(*g_entity_mgr, to_eid, eastl::move(msg), msg_net_desc);
}

int get_no_packets_time_ms()
{
  auto net = get_net_internal();
  if (!net || net->isServer())
    return 0;
  DaNetTime last = net->getDriver()->getLastReceivedPacketTime(0);
  DaNetTime curTime = danet::GetTime();
  G_ASSERTF(curTime >= last, "current time (%u) < last recive time (%u)", curTime, last);
  return (last && curTime >= last) ? curTime - last : 0;
}

static void switch_unresponsive_server_addr(NetCtx &nctx)
{
  net::IConnection *sconn = (s_server_urls.size() >= 2) ? nctx.getNet().getServerConnection() : nullptr;
  if (!sconn)
    return;
  static const int timeToSwitch =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServer", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR);
  if (!timeToSwitch)
    return;
  if (!time_to_switch_server_addr)
    time_to_switch_server_addr = timeToSwitch;
  int nopktt = get_no_packets_time_ms();
  if (nopktt <= time_to_switch_server_addr)
  {
    if (nopktt < timeToSwitch)
      time_to_switch_server_addr = timeToSwitch;
    return;
  }

  G_ASSERT_RETURN(current_server_url_i < s_server_urls.size(), );

  logwarn("No network packets for %d ms from %s", nopktt, s_server_urls[current_server_url_i].c_str());
  g_entity_mgr->broadcastEvent(ChangeServerRoute{/*currentIsUnresponsive*/ true});
  eastl::string country_code;
  g_entity_mgr->broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country_code});
  statsd::counter("net.server_addr_change", 1,
    {{"addr", get_server_route_host(current_server_url_i)}, {"country", country_code.c_str()}});
  const int timeToSwitchFast =
    dgs_get_settings()->getBlockByNameEx("net")->getInt("timeToSwitchUnresponsiveServerFast", DEFAULT_TIME_TO_SWITCH_SERVER_ADDR_FAST);
  time_to_switch_server_addr = nopktt + timeToSwitchFast;
}


void flush_new_connection(net::IConnection &conn)
{
  propsreg::flush_net_registry(conn);
  ridestr::send_initial_ridestr(conn);
  if (!conn.isBlackHole())
    g_entity_mgr->broadcastEventImmediate(CmdAddInitialEntitiesInNetScope{conn.getId()});
}

struct NetControlClient : public net::INetworkObserver
{
  NetControlClient() { g_entity_mgr->broadcastEventImmediate(OnNetControlClientCreated{}); }
  ~NetControlClient() { g_entity_mgr->broadcastEventImmediate(OnNetControlClientDestroyed{}); }

  static void onServerInfoMsg(const net::IMessage *msgraw)
  {
    using namespace net;
    auto msg = msgraw->cast<ServerInfo>();
    G_ASSERT(msg);
    auto serverFlags = net_ctx->srvFlags = (ServerFlags)msg->get<0>();
    if ((serverFlags & ServerFlags::Encryption) != ServerFlags::None)
      msgraw->connection->setEncryptionKey(net_ctx->encryptionKey, EncryptionKeyBits::Encryption | EncryptionKeyBits::Decryption);
    uint32_t ver = msg->get<3>();
    auto srvFlagChar = [serverFlags](ServerFlags f, char c) -> char { return ((serverFlags & f) != ServerFlags::None) ? c : ' '; };
    debug("Connected to server: flags:%c%c%c, tickrate:%d/%d, ver:%d.%d.%d.%d, scene:...%s, level:...%s",
      srvFlagChar(ServerFlags::Encryption, 'E'), srvFlagChar(ServerFlags::Dev, 'D'), srvFlagChar(ServerFlags::DynTick, 'T'),
      msg->get<1>(), msg->get<2>(), ver >> 24, (ver >> 16) & UCHAR_MAX, (ver >> 8) & UCHAR_MAX, ver & UCHAR_MAX,
      strrchr(msg->get<4>().c_str(), '/'), strrchr(msg->get<5>().c_str(), '/'));

    g_entity_mgr->broadcastEventImmediate(OnNetControlClientInfoMsg{static_cast<const ServerInfo *>(msg)});

    phys_set_tickrate(msg->get<1>(), msg->get<2>());
    sceneload::apply_scene_level(eastl::move(const_cast<eastl::string &>(msg->get<4>())),
      eastl::move(const_cast<eastl::string &>(msg->get<5>())));

    g_entity_mgr->broadcastEvent(EventOnConnectedToServer());
  }

  virtual void onConnect(net::Connection &conn) override
  {
    if (!net_ctx->encryptionKey.empty())
      // this is no-op if server's encryption disabled
      conn.setEncryptionKey(net_ctx->encryptionKey, net::EncryptionKeyBits::Decryption);
    debug("Connected to server, wait for msg_sink creation");
  }
  virtual void onDisconnect(net::Connection *, DisconnectionCause cause) override
  {
    if (cause == DC_CONNECTTION_ATTEMPT_FAILED)
    {
      eastl::string country_code;
      g_entity_mgr->broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country_code});
      statsd::counter("net.connect_failed", 1,
        {{"addr", get_server_route_host(current_server_url_i)}, {"country", country_code.c_str()}});
      if (const char *nextSurl = select_next_server_url(/*rotate*/ false))
      {
        logwarn("Connect to %s failed, try connect to next server url: %s...", s_server_urls[current_server_url_i - 1].c_str(),
          nextSurl);
        net_ctx->getNet().getDriver()->connect(nextSurl, NET_PROTO_VERSION);
        return;
      }
      else // no more server urls to connect to
        statsd::counter("net.host_connect_failed", 1, {{"addr", get_server_route_host(0)}, {"country", country_code.c_str()}});
    }
    g_entity_mgr->broadcastEventImmediate(OnNetControlClientDisconnect{});
    on_client_disconnected(cause);
    if (cause == DC_CONNECTTION_ATTEMPT_FAILED && dgs_get_argv("fatal_on_failed_conn"))
      DAG_FATAL("Unable to connect to server");
  }

  static net::INetworkObserver *create(void *buf, size_t bufsz)
  {
    G_ASSERT(sizeof(NetControlClient) <= bufsz);
    G_UNUSED(bufsz);
    return new (buf, _NEW_INPLACE) NetControlClient;
  }
};
ECS_NET_IMPL_MSG(ServerInfo,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DONT_COMPRESS,
  ECS_NET_NO_DUP,
  &NetControlClient::onServerInfoMsg);

net::CNetwork &net_ctx_init_client(net::INetDriver *drv)
{
  auto &cnet = net_ctx.demandInit(drv, &NetControlClient::create)->network;
  net_ctx->createObserver();
  return cnet;
}

void net_disconnect(net::IConnection &conn, DisconnectionCause cause)
{
  conn.disconnect(cause);
  conn.getConnFlagsRW() |= net::CF_PENDING; // set pending to avoid send traffic to player that will be disconnected in the near future
}

namespace net
{

void ConnectionsIterator::advance()
{
  NetCtx *nctx = net_ctx.get();
  auto net = nctx ? &nctx->getNet() : NULL;
  if (!net)
    ;
  else if (net->isServer())
  {
    auto &conns = net->getClientConnections();
    for (; i < conns.size(); ++i)
      if (conns[i] && !(conns[i]->getConnFlags() & net::CF_PENDING))
        return;
  }
  else if (i == 0 && net->getServerConnection())
    return;
  i = -1;
}

IConnection &ConnectionsIterator::operator*() const
{
  G_ASSERT(*this);
  auto net = get_net_internal();
  G_ASSERT(net);
  Connection *conn = net->isServer() ? net->getClientConnections()[i].get() : net->getServerConnection();
  G_ASSERT(conn != NULL);
  return *conn;
}

}; // namespace net

#if DAGOR_DBGLEVEL > 0
struct ListenServerNetObserver final : public net::INetworkObserver // Warn: no auth verify, nor encryption
{
  static void onClientInfoMsg(const net::IMessage *msgraw)
  {
    G_FAST_ASSERT(msgraw->connection);
    net::IConnection &conn = *msgraw->connection;
    auto msg = msgraw->cast<ClientInfo>();
    G_ASSERT(msg);
    uint32_t &connFlags = conn.getConnFlagsRW();
    if (connFlags & net::CF_PENDING)
    {
      if (msg->get<0>() == NET_PROTO_VERSION) // connect successfull
      {
        matching::UserId userId = msg->get<1>(), groupId = userId;
        int mteam = TEAM_UNASSIGNED;
        const eastl::string &userName = msg->get<2>();
        auto serverFlags = net::ServerFlags::None;
#if _DEBUG_TAB_ || EA_ASAN_ENABLED || defined(__SANITIZE_ADDRESS__)
        serverFlags |= net::ServerFlags::Dev;
#endif
        if (dedicated::is_dynamic_tickrate_enabled())
          serverFlags |= net::ServerFlags::DynTick;
        const eastl::string &pltf = msg->get<5>();
        const eastl::string &platformUid = msg->get<6>();
        uint32_t ver = msg->get<7>();

        debug("Client #%d connected with user{name,id}=<%s>/%lld, groupId=%lld, mteam=%d, flags=0x%x, ver=%d.%d.%d.%d, pltf=%s",
          (int)conn.getId(), userName.c_str(), (long long)userId, (long long)groupId, mteam, (int)msg->get<3>(), ver >> 24,
          (ver >> 16) & UCHAR_MAX, (ver >> 8) & UCHAR_MAX, ver & UCHAR_MAX, pltf.c_str());

        connFlags &= ~net::CF_PENDING; // not pending anymore

        {
          ServerInfo srvInfoMsg((uint16_t)serverFlags, (uint8_t)phys_get_tickrate(), (uint8_t)phys_get_bot_tickrate(),
            get_exe_version32(), sceneload::get_current_game().sceneName, sceneload::get_current_game().levelBlkPath, Tab<uint8_t>{});
          srvInfoMsg.connection = &conn;
          send_net_msg(*g_entity_mgr, net::get_msg_sink(), eastl::move(srvInfoMsg), nullptr);
        }

        {
          EventOnClientConnected evt(conn.getId(), userId, eastl::move(userName), groupId, msg->get<3>(), platformUid,
            eastl::string(pltf), mteam, app_profile::get().appId);
          g_entity_mgr->broadcastEventImmediate(eastl::move(evt));
        }

        if (!(connFlags & net::CF_PENDING)) // i.e. not disconnected in event handlers
          flush_new_connection(conn);
      }
      else
      {
        logwarn("Invalid client #%d net proto 0x%x (vs 0x%x)", (int)conn.getId(), msg->get<0>(), NET_PROTO_VERSION);
        conn.disconnect();
      }
    }
    else
      logwarn("ClientInfo for not pending connection %d", (int)conn.getId());
  }

  void onConnect(net::Connection &conn) override
  {
    G_VERIFY(conn.setEntityInScopeAlways(net::get_msg_sink()));
    if (!conn.isBlackHole()) // network connection
    {
      debug("Client #%d connected, wait for identity message", (int)conn.getId());
      conn.getConnFlagsRW() = net::CF_PENDING;
    }
    else // replay connection
    {
      G_ASSERT(get_time_mgr().getMillis() == 0); // make sure that all replay connections has same timeline
      flush_new_connection(conn);
    }
  }

  void onDisconnect(net::Connection *conn, DisconnectionCause cause) override
  {
    G_FAST_ASSERT(conn != nullptr); // on server conn can't be null here
    debug("Client #%d disconnected with %s", (int)conn->getId(), describe_disconnection_cause(cause));
    // send immediately because connection might get destroyed after this
    g_entity_mgr->broadcastEventImmediate(EventOnClientDisconnected(conn->getId(), cause));
  }
};
static net::INetworkObserver *create_listen_server_net_observer(void *buf, size_t bufsz)
{
  G_ASSERT(bufsz >= sizeof(ListenServerNetObserver));
  G_UNUSED(bufsz);
  return new (buf, _NEW_INPLACE) ListenServerNetObserver;
}
#else
static net::INetworkObserver *create_listen_server_net_observer(void *, size_t) { return nullptr; }
#endif

bool net_init_early()
{
  net::INetDriver *drv = dedicated::create_listen_net_driver();
  if (const char *listen = (!dedicated::is_dedicated() && DAGOR_DBGLEVEL > 0) ? dgs_get_argv("listen") : NULL) //-V560
  {
    uint16_t port = 0;
    drv = net::create_net_driver_listen(listen, NET_MAX_PLAYERS, &port);
    if (drv)
    {
#if DAGOR_DBGLEVEL > 0
      ClientInfo::messageClass.msgSinkHandler = &ListenServerNetObserver::onClientInfoMsg;
#endif
      debug("Inited listen net driver on port %d", port);
    }
  }
  if (drv)
    net_ctx.demandInit(drv, dedicated::is_dedicated() ? &dedicated::create_server_net_observer : &create_listen_server_net_observer);
  return true;
}

static eastl::string get_platform_uid()
{
#if _TARGET_XBOX
  return gdk::active_user::get_xuid_str();
#else
  // To consider: what about ps4/5, nswitch?
  return eastl::string{};
#endif
}

static void on_msg_sink_created_client(ecs::EntityId msg_sink_eid, const dag::Vector<uint8_t> &auth_key)
{
  uint16_t clientFlags = app_profile::get().replay.record ? CNF_REPLAY_RECORDING : 0;
  if (DAGOR_DBGLEVEL > 0)
    clientFlags |= CNF_DEVELOPER;

  matching::UserId userId = net::get_user_id();
  g_entity_mgr->broadcastEventImmediate(OnMsgSinkCreatedClient{&clientFlags});

  debug("msg_sink created, send client info to server, client flags = 0x%x", (int)clientFlags);

  danet::BitStream syncVromsStream;

  if (!circuit::get_conf()->getBool("syncVromsDisabled", false))
    syncvroms::write_sync_vroms(syncVromsStream, syncvroms::get_mounted_sync_vroms_list());

  ClientInfo cimsg(NET_PROTO_VERSION, userId, eastl::string(net::get_user_name()), clientFlags, auth_key,
    eastl::string(get_platform_string_id()), get_platform_uid(), get_exe_version32(), syncVromsStream);
  send_net_msg(msg_sink_eid, eastl::move(cimsg));
}

static void net_connect(const eastl::string &server_url, net::ConnectParams &&connect_params)
{
  G_ASSERT(!net_ctx);
  net::INetDriver *netDrv = net::create_net_driver_connect(server_url.c_str(), NET_PROTO_VERSION), *drv = netDrv;
  if (!netDrv)
  {
    logwarn("failed connect to '%s'", server_url);
    on_client_disconnected(DC_CONNECTTION_ATTEMPT_FAILED);
    return;
  }
  eastl::optional<eastl::string> &recordOpt = app_profile::getRW().replay.record;
  const char *record = recordOpt ? recordOpt->c_str() : nullptr;
  if (record && *record)
  {
    drv = create_replay_client_net_driver(netDrv, record, NET_PROTO_VERSION, &gen_replay_meta_data);
    if (drv)
      debug("Started replay record to '%s'", record);
    else
    {
      logerr("Failed to record replay to '%s'", record);
      recordOpt.reset();
      drv = netDrv;
    }
  }
  if (drv)
  {
    net_ctx_init_client(drv);
    net_ctx->encryptionKey = eastl::move(connect_params.encryptKey);
    net::set_msg_sink_created_cb(
      [authKey = eastl::move(connect_params.authKey)](ecs::EntityId eid) { on_msg_sink_created_client(eid, authKey); });
    g_time.reset(create_client_time());
    net::event::init_client();
    netstat::init();
    set_window_title("Client");
  }
}

bool net_init_late_client(net::ConnectParams &&connect_params)
{
#if DAGOR_DBGLEVEL == 0
  if (has_in_game_editor())
  {
    DAG_FATAL("daEditorE is not allowed for %s", __FUNCTION__);
    return false;
  }
#endif
  net::CNetwork *net = get_net_internal();
  if (!net_ctx)
  {
    if (!connect_params.serverUrls.empty())
    {
      s_server_urls.clear();
      eastl::string country;
      g_entity_mgr->broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country});
      for (int i = 0; i < connect_params.serverUrls.size(); ++i)
      {
        s_server_urls.emplace_back(eastl::move(connect_params.serverUrls[i]));
        if (!s_server_urls.back().empty())
        {
          statsd::counter("net.server_addr_init", 1, {{"addr", get_server_route_host(i)}, {"country", country.c_str()}});
        }
      }
      current_server_url_i = 0;
      if (!s_server_urls.empty())
        net_connect(s_server_urls.front(), eastl::move(connect_params));
      else
        logwarn("net_init_late_client failed due to empty server urls list");
    }
    else if (!app_profile::get().replay.playFile.empty())
    {
      if (!try_create_replay_playback())
        DAG_FATAL("incorrect replay!");
    }
    if (g_time.get() == &dummy_time)
      g_time.reset(!net ? create_server_time() : create_client_time()); // single mode
    return true;
  }
  return false;
}


void net_init_late_server()
{
  net::CNetwork *net = get_net_internal();
  if ((net && !net->isServer()) || !app_profile::get().replay.playFile.empty()) // client
  {
    G_ASSERT(g_time.get() != &dummy_time);
    return;
  }
  if (net_ctx) // server
  {
    G_UNUSED(net);
    G_ASSERT(net->isServer());
    net_ctx->createObserver();

    g_entity_mgr->setEidsReservationMode(true);
    create_simple_entity("msg_sink"); // create dummy entity for sending (only for server though, on client it's created from server)

    g_time.reset(create_server_time()); // before adding connection because it's used on connect

    if (dgs_get_settings()->getBlockByNameEx("net")->getBool("enableScopeQuery", false))
      net_ctx->getNet().setScopeQueryCb([](net::Connection *c) { g_entity_mgr->broadcastEventImmediate(EventNetScopeQuery(c)); });

    server_create_replay_record(); // right after timer creation (with 0 async time)

    net::event::init_server();
    propsreg::init_net_registry_server();
  }
  else // single player
  {
    create_simple_entity("msg_sink");
    net::MessageClass::init(/*server*/ true);
    net::event::init_server();
    g_time.reset(create_accum_time());
  }
  G_ASSERT(g_time.get() != &dummy_time);
}

void net_update()
{
  NetCtx *nctx = net_ctx.get();
  if (!nctx)
    return;
  TIME_PROFILE(net_update);
  int curMs = get_time_mgr().getAsyncMillis();
  nctx->update(curMs);
  netstat::update(curMs);
  g_entity_mgr->broadcastEventImmediate(OnNetUpdate{});
  switch_unresponsive_server_addr(*nctx);
}

static void net_dump_stats()
{
  if (net::CNetwork *netw = get_net_internal())
    netw->dumpStats();
}

static bool net_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = false;
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

void net_on_before_emgr_clear()
{
  net_dump_stats();

  if (net_ctx && net_ctx->isRecordingReplay()) // stash replay meta json for consuming in gen_replay_meta_data later
    gather_replay_meta_info();

  bool wasNet = (bool)net_ctx;
  if (wasNet) // immediate send otherwise it might be lost while we unload game or close app
    g_entity_mgr->broadcastEventImmediate(EventOnNetworkDestroyed(last_client_dc));
}

void net_destroy(bool final)
{
  net_dump_stats();
  net_ctx.demandDestroy();
  g_time.reset(&dummy_time);
  if (ecs::EntityId msg_sink_eid = net::get_msg_sink())
    g_entity_mgr->destroyEntity(msg_sink_eid);
  netstat::term();
  propsreg::term_net_registry();
  net::event::shutdown();
  net::set_msg_sink_created_cb({});
  s_server_urls.clear();
  current_server_url_i = 0;
  time_to_switch_server_addr = 0;
  phys_set_tickrate(); // restore default tickrates
  set_window_title(nullptr);
  clear_replay_meta_info();
  last_client_dc = DC_CONNECTION_CLOSED; // reset to default
  g_entity_mgr->broadcastEventImmediate(OnNetDestroy{final});
}

void net_stop()
{
  if (net::CNetwork *netw = get_net_internal())
    netw->stopAll(DC_CONNECTION_STOPPED);
}

void net_enable_component_filtering(net::ConnectionId id, bool on)
{
  if (net::CNetwork *netw = get_net_internal())
    netw->enableComponentFiltering(id, on);
}

bool net_is_component_filtering_enabled(net::ConnectionId id)
{
  if (net::CNetwork *netw = get_net_internal())
    return netw->isComponentFilteringEnabled(id);
  return false;
}
