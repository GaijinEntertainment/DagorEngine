// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "speechEvents.h"
#include <daECS/net/netEvent.h>
#include <ecs/scripts/sqEntity.h>
#include <ecs/scripts/sqBindEvent.h>
#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/sqratDagor.h>
#include <daECS/net/netEvents.h>


#define DECL_GAME_EVENT ECS_REGISTER_EVENT
DEF_SPEECH_EVENTS
#undef DECL_GAME_EVENT


SQ_DEF_AUTO_BINDING_MODULE(bind_speech_events, "speechevents")
{
  return ecs::sq::EventsBind<
#define DECL_GAME_EVENT(x, ...) x,
    DEF_SPEECH_EVENTS
#undef DECL_GAME_EVENT
      ecs::sq::term_event_t>::bindall(vm);
}
