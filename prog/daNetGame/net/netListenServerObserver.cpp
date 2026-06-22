// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netListenServerObserver.h"
#include "netPrivate.h"
#include "net.h"
#include "netEvents.h"
#include "netConsts.h"
#include "netMsg.h"
#include "protoVersion.h"
#include "net/dedicated.h"
#include "phys/netPhys.h"
#include "phys/physUtils.h"
#include "main/version.h"
#include "main/gameLoad.h"
#include "main/appProfile.h"
#include "userid.h"
#include "time.h"
#include <ecs/game/generic/team.h>

#include <daECS/core/entityManager.h>
#include <daECS/net/network.h>
#include <daECS/net/connection.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/message.h>
#include <daECS/net/netEvents.h>
#include <daNet/disconnectionCause.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <climits>


#if DAGOR_DBGLEVEL > 0

namespace
{
struct ListenServerNetObserver final : public net::INetworkObserver
{
  ListenServerNetObserver()
  {
    G_ASSERTF(net_context, "ListenServerNetObserver created without a live NetContext");
    G_ASSERTF(&net_context->entityMgr == g_entity_mgr, "ListenServerNetObserver is hardwired to g_entity_mgr");
    G_ASSERTF(is_main_thread(), "ListenServerNetObserver assumes main-thread execution");
  }

  static void onClientInfoMsg(const net::IMessage *msgraw)
  {
    G_FAST_ASSERT(msgraw->connection);
    net::IConnection &conn = *msgraw->connection;
    auto msg = msgraw->cast<ClientInfo>();
    G_ASSERT(msg);
    uint32_t &connFlags = conn.getConnFlagsRW();
    if (connFlags & net::CF_PENDING)
    {
      if (msg->get<0>() == NET_PROTO_VERSION)
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

        connFlags &= ~net::CF_PENDING;

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

        if (!(connFlags & net::CF_PENDING))
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
    if (!conn.isBlackHole())
    {
      debug("Client #%d connected, wait for identity message", (int)conn.getId());
      conn.getConnFlagsRW() = net::CF_PENDING;
    }
    else
    {
      G_ASSERT(get_time_mgr().getMillis() == 0);
      flush_new_connection(conn);
    }
  }

  void onDisconnect(net::Connection *conn, DisconnectionCause cause, ecs::EntityManager &mgr) override
  {
    G_FAST_ASSERT(conn != nullptr);
    debug("Client #%d disconnected with %s", (int)conn->getId(), describe_disconnection_cause(cause));
    mgr.broadcastEventImmediate(EventOnClientDisconnected(conn->getId(), cause));
  }
};
} // namespace

namespace net
{
INetworkObserver *create_listen_server_net_observer(void *buf, size_t bufsz)
{
  G_ASSERT(bufsz >= sizeof(ListenServerNetObserver));
  G_UNUSED(bufsz);
  return new (buf, _NEW_INPLACE) ListenServerNetObserver;
}
} // namespace net

void register_listen_server_msg_handler() { ClientInfo::messageClass.handler = &ListenServerNetObserver::onClientInfoMsg; }

#else

namespace net
{
INetworkObserver *create_listen_server_net_observer(void *, size_t) { return nullptr; }
} // namespace net

void register_listen_server_msg_handler() {}

#endif
