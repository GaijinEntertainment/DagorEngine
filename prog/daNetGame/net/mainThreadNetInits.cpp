// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mainThreadNetInits.h"
#include "net.h"
#include "netPrivate.h"
#include "netConsts.h"
#include "netControlClient.h"
#include "netEvents.h"
#include "netListenServerObserver.h"
#include "replaySession.h"
#include "time.h"

#include "net/dedicated.h"
#include "main/main.h"
#include "main/ecsUtils.h"

#include <daECS/core/entityManager.h>
#include <daECS/net/network.h>
#include <daECS/net/netEvents.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>


bool net_init_early(ecs::EntityManager &mgr)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(&mgr == g_entity_mgr);

  net::INetDriver *drv = dedicated::create_listen_net_driver();
  if (const char *listen = (!dedicated::is_dedicated() && DAGOR_DBGLEVEL > 0) ? dgs_get_argv("listen") : NULL)
  {
    uint16_t port = 0;
    drv = net::create_net_driver_listen(listen, NET_MAX_PLAYERS, &port);
    if (drv)
    {
      register_listen_server_msg_handler();
      debug("Inited listen net driver on port %d", port);
    }
  }
  if (drv)
    create_net_ctx(mgr, drv,
      dedicated::is_dedicated() ? &dedicated::create_server_net_observer : &net::create_listen_server_net_observer);

  if (dedicated::is_dedicated() && drv)
  {
    const char *relay_connection = dgs_get_argv("relay_connection");
    if (relay_connection)
    {
      debug("initiating relay connection to host/port '%s'", relay_connection);
      if (!establish_relay_connection(relay_connection))
        DAG_FATAL("failed to initiate connection with relay host/port '%s'", relay_connection);
      debug("initiated relay connection to host/port '%s'", relay_connection);
    }
  }

  return true;
}


bool net_init_late_client(net::ConnectParams &&connect_params, ecs::EntityManager &mgr)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(&mgr == g_entity_mgr);
#if DAGOR_DBGLEVEL == 0
  if (has_in_game_editor())
  {
    DAG_FATAL("daEditorE is not allowed for %s", __FUNCTION__);
    return false;
  }
#endif
  net::CNetwork *net = GET_NET();
  if (!net_context)
  {
    if (!connect_params.serverUrls.empty())
    {
      dag::Vector<eastl::string> urls = eastl::move(connect_params.serverUrls);
      eastl::string relayUrl = eastl::move(connect_params.relayStunRequestAddr);

      net_create_client_driver(mgr, urls.front(), eastl::move(connect_params));
      if (!net_context)
      {
        logwarn("net_init_late_client: net_create_client_driver failed");
        return true;
      }

      install_session_routes_and_connect(mgr, eastl::move(urls), eastl::move(relayUrl));
    }
    else
    {
      bool hasReplayPlayback = false;
      mgr.broadcastEventImmediate(EventQueryReplayPlayback{&hasReplayPlayback});
      if (hasReplayPlayback)
      {
        if (!try_create_replay_playback(mgr))
          DAG_FATAL("incorrect replay!");
      }
    }
    if (g_net_globals.isDummyTime())
      g_net_globals.resetTimeMgr(!net ? create_server_time() : create_client_time());
    return true;
  }
  return false;
}


void net_init_late_server(ecs::EntityManager &mgr)
{
  G_ASSERT(is_main_thread());
  G_ASSERT(&mgr == g_entity_mgr);
  if (net::get_msg_sink()) // already initialized for this session
    return;
  net::CNetwork *net = GET_NET();
  bool hasReplayPlayback = false;
  mgr.broadcastEventImmediate(EventQueryReplayPlayback{&hasReplayPlayback});
  if ((net && !net->isServer()) || hasReplayPlayback)
  {
    G_ASSERT(!g_net_globals.isDummyTime());
    return;
  }
  if (net_context)
  {
    G_UNUSED(net);
    G_ASSERT(net->isServer());
    net_context->createObserver();

    mgr.setEidsReservationMode(true);
    create_simple_entity(mgr, "msg_sink");

    g_net_globals.resetTimeMgr(create_server_time());

    if (dgs_get_settings()->getBlockByNameEx("net")->getBool("enableScopeQuery", false))
      net_context->getNet().setScopeQueryCb([&mgr](net::Connection *c) { mgr.broadcastEventImmediate(EventNetScopeQuery(c)); });

    mgr.broadcastEventImmediate(OnNetInitServer{});
  }
  else
  {
    // SP/offline session: no NetContext, but publish the EM binding so net_destroy's guard sees
    // an active session even after g_entity_mgr->clear() removes msg_sink. clear_net_em() runs
    // in destroy_net_ctx during net_destroy teardown.
    net::publish_net_em(mgr);
    create_simple_entity(mgr, "msg_sink");
    net::MessageClass::init(/*server*/ true, &mgr);
    g_net_globals.resetTimeMgr(create_accum_time());
    mgr.broadcastEventImmediate(OnNetInitServer{});
  }
  G_ASSERT(!g_net_globals.isDummyTime());
}
