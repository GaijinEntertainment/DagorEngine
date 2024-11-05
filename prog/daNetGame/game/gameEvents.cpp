// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/scripts/sqEntity.h>
#include "gameEvents.h"
#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/sqratDagor.h>
#include <ecs/scripts/sqBindEvent.h>
#include "phys/physEvents.h"
#include <daECS/net/netEvents.h>
#include "net/channel.h"
#include "main/level.h"
#include <daECS/net/netEvent.h>


#define DECL_GAME_EVENT ECS_REGISTER_EVENT
DEF_UNICAST_GAME_EVENTS
DEF_BROADCAST_GAME_EVENTS
#undef DECL_GAME_EVENT


SQ_DEF_AUTO_BINDING_MODULE(bind_game_events, "gameevents")
{
  return ecs::sq::EventsBind<
#define DECL_GAME_EVENT(x, ...) x,
    DEF_UNICAST_GAME_EVENTS DEF_BROADCAST_GAME_EVENTS
#undef DECL_GAME_EVENT
      CmdTeleportEntity,
    EventLevelLoaded,
    // TODO: remove these event types from here - it was moved to net module
    EventOnDisconnectedFromServer, EventOnClientConnected, EventOnClientDisconnected>::bindall(vm);
}
