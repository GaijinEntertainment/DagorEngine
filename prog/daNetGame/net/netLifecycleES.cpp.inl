// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netEvents.h"
#include "net/netPropsRegistry.h"
#include "phys/physUtils.h"
#include "main/main.h"
#include "replaySession.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/netEvent.h>
#include <daECS/net/netEvents.h>


ECS_TAG(gEntityMgr)
ECS_NO_ORDER
static void net_lifecycle_init_client_es(const OnNetInitClient &) { set_window_title("Client"); }


ECS_TAG(gEntityMgr)
ECS_NO_ORDER
static void net_lifecycle_init_server_es(const OnNetInitServer &)
{
  propsreg::init_net_registry_server();
  server_create_replay_record();
}


ECS_TAG(gEntityMgr)
ECS_NO_ORDER
static void net_lifecycle_pre_emgr_clear_es(const EventOnNetworkDestroyed &)
{
  if (is_replay_recording())
    gather_replay_meta_info();
}


ECS_TAG(gEntityMgr)
ECS_NO_ORDER
static void net_lifecycle_destroy_es(const OnNetDestroy &, ecs::EntityManager &manager)
{
  propsreg::term_net_registry();
  phys_set_tickrate();
  set_window_title(nullptr);
  if (ecs::EntityId msg_sink_eid = net::get_msg_sink())
    manager.destroyEntity(msg_sink_eid);
  net::set_msg_sink_created_cb({});
}


ECS_TAG(gEntityMgr)
ECS_NO_ORDER
static void net_lifecycle_torn_down_es(const EventNetTornDown &)
{
  finalize_replay_record();
  clear_replay_meta_info();
}
