// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net.h"
#include "netPrivate.h"
#include "netEvents.h"
#include "netScope.h"
#include "replaySession.h"
#include "authEvents.h"
#include "netConsts.h"
#include "net/channel.h"
#include "time.h"

#include <osApiWrappers/dag_miscApi.h>

#include <daECS/core/coreEvents.h>

ECS_REGISTER_EVENT(CmdAddInitialEntitiesInNetScope)
#define NET_AUTH_ECS_EVENT ECS_REGISTER_EVENT
NET_AUTH_ECS_EVENTS
#undef NET_AUTH_ECS_EVENT

#include <daECS/core/entityManager.h>
#include <daECS/net/network.h>
#include <daECS/net/connection.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/message.h>
#include <daECS/net/netEvents.h>
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <EASTL/unique_ptr.h>
#include <debug/dag_assert.h>

#include "net/netPropsRegistry.h"
#include <rendInst/riexSync.h>
#include "game/riDestr.h"

#include "netMsg.h"
#include "net/dedicated.h"

namespace sceneload
{
extern void on_apply_sync_vroms_diffs_msg(const net::IMessage *);
}

ECS_NET_IMPL_MSG(ClientInfo,
  net::ROUTING_CLIENT_TO_SERVER,
  ECS_NET_NO_RCPTF,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  dedicated::get_clientinfo_msg_handler());

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


int send_net_msg(ecs::EntityManager &mgr, ecs::EntityId to_eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc)
{
  const net::MessageNetDesc &msgDesc = !msg_net_desc ? msg.getMsgClass() : *msg_net_desc;

  if (!has_network())
  {
    // SP fallback: mgr is used to dispatch the event in this EM's world; verify the binding
    // matches so we don't deliver into the wrong EM. mgr.sendEventImmediate is MT-safe via its
    // own ScopedMTMutex.
    ASSERT_NET_EM_IS(mgr);
    G_ASSERT(is_server());
    if (msgDesc.routing == net::ROUTING_CLIENT_TO_SERVER || msgDesc.routing == net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)
    {
      eastl::unique_ptr<net::IMessage, net::MessageDeleter> msgPtr(eastl::move(msg).moveHeap(), net::MessageDeleter{/*heap*/ true});
      msgPtr->connection = nullptr;
      if (!net::event::try_receive(*msgPtr, mgr, to_eid))
        mgr.sendEventImmediate(to_eid, ecs::EventNetMessage(eastl::move(msgPtr)));
      return 1;
    }
    return 0;
  }

  // MP path: mgr is not used. Pin NetContext lifetime only.
  net::TopologyLock::ReadScope topoPin;
  auto *ctx = GET_NET_CTX();
  if (!ctx)
    return 0;

  net::CNetwork *netw = &ctx->getNet();
  int numSends = 0;
  if (msgDesc.routing == net::ROUTING_SERVER_TO_CLIENT)
  {
    net::recipient_filter_t rcptFilter = msgDesc.rcptFilter;
    G_FAST_ASSERT(rcptFilter != NULL);
    int curTime = is_replay_recording() ? get_time_mgr().getAsyncMillis() : 0;
    if (rcptFilter == &net::broadcast_rcptf)
    {
      for (net::ConnectionsIterator cit; cit; ++cit)
        numSends += netw->sendto(curTime, to_eid, msg, &*cit, msg_net_desc) ? 1 : 0;
    }
    else
    {
      Tab<net::IConnection *> conns(framemem_ptr());
      for (auto cn : rcptFilter(conns, to_eid, msg))
        if (cn)
          numSends += netw->sendto(curTime, to_eid, msg, cn, msg_net_desc) ? 1 : 0;
      if (is_replay_recording() && rcptFilter != &net::direct_connection_rcptf)
      {
        auto &cliConns = netw->getClientConnections();
        if (net::Connection *conn = !cliConns.empty() ? cliConns.back().get() : nullptr)
          if (conn->isBlackHole())
            numSends += netw->sendto(curTime, to_eid, msg, conn, msg_net_desc) ? 1 : 0;
      }
    }
  }
  else
  {
    numSends = netw->sendto(/*time*/ 0, to_eid, msg, netw->getServerConnection(), msg_net_desc) ? 1 : 0;
  }
  return numSends;
}

int send_net_msg(ecs::EntityId to_eid, net::IMessage &&msg, const net::MessageNetDesc *msg_net_desc)
{
  // getRaw() bypasses g_entity_mgr's validateThread so off-thread callers (anticheat msg_sink
  // sends) get through; the MP branch in the EM-taking overload doesn't touch mgr, and the SP
  // branch has its own EM-binding check + MT-safe sendEventImmediate.
  return send_net_msg(*g_entity_mgr.getRaw(), to_eid, eastl::move(msg), msg_net_desc);
}


void flush_new_connection(net::IConnection &conn)
{
  propsreg::flush_net_registry(conn);
  riexsync::send_initial_snapshot_to(conn);
  ridestr::send_initial_ridestr(conn);
  if (!conn.isBlackHole())
    g_entity_mgr->broadcastEventImmediate(CmdAddInitialEntitiesInNetScope{conn.getId()});
}


void net_enable_component_filtering(net::ConnectionId id, bool on)
{
  if (auto *ctx = GET_NET_CTX())
    ctx->getNet().enableComponentFiltering(id, on);
}

bool net_is_component_filtering_enabled(net::ConnectionId id)
{
  if (auto *ctx = GET_NET_CTX())
    return ctx->getNet().isComponentFilteringEnabled(id);
  return false;
}
