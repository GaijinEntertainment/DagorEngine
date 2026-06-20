// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netControlClient.h"
#include "netPrivate.h"
#include "net.h"
#include "netEvents.h"
#include "authCountryCode.h"

#include <daECS/core/coreEvents.h>

ECS_REGISTER_EVENT(OnNetControlClientCreated)
ECS_REGISTER_EVENT(OnNetControlClientDestroyed)
ECS_REGISTER_EVENT(OnNetControlClientInfoMsg)
ECS_REGISTER_EVENT(OnNetControlClientDisconnect)
ECS_REGISTER_EVENT(OnMsgSinkCreatedClient)
#include "netConsts.h"
#include "netMsg.h"
#include "netStat.h"
#include "net/channel.h"
#include "replaySession.h"
#include "protoVersion.h"
#include "phys/netPhys.h"
#include "phys/physUtils.h"
#include "game/player.h"
#include "main/gameLoad.h"
#include "main/appProfile.h"
#include "main/circuit.h"
#include "main/version.h"
#include "main/platformUid.h"
#include "userid.h"
#include "time.h"
#include <daECS/net/replay.h>

#include <daECS/core/entityManager.h>
#include <daECS/net/network.h>
#include <daECS/net/connection.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/message.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/replay.h>
#include <daNet/disconnectionCause.h>
#include <syncVroms/syncVroms.h>
#include <osApiWrappers/dag_miscApi.h>
#include <statsd/statsd.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <climits>


namespace
{
struct NetControlClient final : public net::INetworkObserver
{
  NetControlClient()
  {
    G_ASSERTF(net_context, "NetControlClient created without a live NetContext");
    G_ASSERTF(&net_context->entityMgr == g_entity_mgr,
      "NetControlClient is hardwired to g_entity_mgr; for other EntityManagers write a different INetworkObserver");
    G_ASSERTF(is_main_thread(), "NetControlClient calls into main-thread modules; constructor must run on main thread");
    g_entity_mgr->broadcastEventImmediate(OnNetControlClientCreated{});
  }
  ~NetControlClient()
  {
    netstat::shutdown();
    g_entity_mgr->broadcastEventImmediate(OnNetControlClientDestroyed{});
  }

  static void onServerInfoMsg(const net::IMessage *msgraw)
  {
    using namespace net;
    auto msg = msgraw->cast<ServerInfo>();
    G_ASSERT(msg);
    auto serverFlags = net_context->srvFlags = (ServerFlags)msg->get<0>();
    if ((serverFlags & ServerFlags::Encryption) != ServerFlags::None)
      msgraw->connection->setEncryptionKey(net_context->encryptionKey, EncryptionKeyBits::Encryption | EncryptionKeyBits::Decryption);
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

  void onConnect(net::Connection &conn) override
  {
    if (!net_context->encryptionKey.empty())
      conn.setEncryptionKey(net_context->encryptionKey, net::EncryptionKeyBits::Decryption);
    debug("Connected to server, wait for msg_sink creation");
  }

  void onDisconnect(net::Connection *, DisconnectionCause cause, ecs::EntityManager &mgr) override
  {
    if (cause == DC_CONNECTION_ATTEMPT_FAILED)
    {
      eastl::string country_code;
      auth_get_country_code(mgr, country_code);
      statsd::counter("net.connect_failed", 1,
        {{"addr", get_server_route_host((uint32_t)net_context->currentServerUrlIdx)}, {"country", country_code.c_str()}});
      if (const char *nextSurl = select_next_server_url(/*rotate*/ false))
      {
        logwarn("Connect to %s failed, try connect to next server url: %s...",
          net_context->serverUrls[net_context->currentServerUrlIdx - 1].c_str(), nextSurl);
        net_context->getNet().getDriver()->connect(nextSurl, NET_PROTO_VERSION);
        return;
      }
      else
        statsd::counter("net.host_connect_failed", 1, {{"addr", get_server_route_host(0)}, {"country", country_code.c_str()}});
    }
    mgr.broadcastEventImmediate(OnNetControlClientDisconnect{});
    on_client_disconnected(mgr, cause);
    if (cause == DC_CONNECTION_ATTEMPT_FAILED && dgs_get_argv("fatal_on_failed_conn"))
      DAG_FATAL("Unable to connect to server");
  }
};
} // namespace

namespace net
{
INetworkObserver *create_net_control_client(void *buf, size_t bufsz)
{
  G_ASSERT(sizeof(NetControlClient) <= bufsz);
  G_UNUSED(bufsz);
  return new (buf, _NEW_INPLACE) NetControlClient;
}
} // namespace net


ECS_NET_IMPL_MSG(ServerInfo,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DONT_COMPRESS,
  ECS_NET_NO_DUP,
  &NetControlClient::onServerInfoMsg);


net::CNetwork &net_ctx_init_client(ecs::EntityManager &mgr, net::INetDriver *drv)
{
  G_VERIFY(bootstrap_net_ctx(mgr, drv, &net::create_net_control_client));
  return net_context->network;
}


static void on_msg_sink_created_client(ecs::EntityManager &manager, ecs::EntityId msg_sink_eid, const dag::Vector<uint8_t> &auth_key)
{
  uint16_t clientFlags = app_profile::get().replay.record ? CNF_REPLAY_RECORDING : 0;
  if (DAGOR_DBGLEVEL > 0)
    clientFlags |= CNF_DEVELOPER;

  matching::UserId userId = net::get_user_id();
  manager.broadcastEventImmediate(OnMsgSinkCreatedClient{&clientFlags});

  debug("msg_sink created, send client info to server, client flags = 0x%x", (int)clientFlags);

  danet::BitStream syncVromsStream;

  if (!circuit::get_conf()->getBool("syncVromsDisabled", false))
    syncvroms::write_sync_vroms(syncVromsStream, syncvroms::get_mounted_sync_vroms_list());

  ClientInfo cimsg(NET_PROTO_VERSION, userId, eastl::string(net::get_user_name()), clientFlags, auth_key,
    eastl::string(get_platform_string_id()), get_platform_uid(), get_exe_version32(), syncVromsStream);
  send_net_msg(manager, msg_sink_eid, eastl::move(cimsg));
}


void net_create_client_driver(ecs::EntityManager &mgr, const eastl::string &server_url, net::ConnectParams &&connect_params)
{
  G_ASSERT(!net_context);
  net::INetDriver *netDrv = net::create_net_driver_startup();
  net::INetDriver *drv = netDrv;
  if (!netDrv)
  {
    logwarn("failed to startup net driver for '%s'", server_url);
    on_client_disconnected(mgr, DC_CONNECTION_ATTEMPT_FAILED);
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
    net_ctx_init_client(mgr, drv);
    net_context->encryptionKey = eastl::move(connect_params.encryptKey);
    net::set_msg_sink_created_cb(
      [authKey = eastl::move(connect_params.authKey), &mgr](ecs::EntityId eid) { on_msg_sink_created_client(mgr, eid, authKey); });
    g_net_globals.resetTimeMgr(create_client_time());
    netstat::init();
    mgr.broadcastEventImmediate(OnNetInitClient{});
  }
}
